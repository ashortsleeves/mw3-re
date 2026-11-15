"""The turn limits' clamp (vt+0x04) against the original x86 (docs/vehicles.md, The turn limits' clamp): Fixed
0x4a43c0, Narrow 0x4a4660, Wide 0x4a48e0 (with 0x4a3fb0 and 0x4a3fe0) and Full (0x4a4aa0 -> 0x4a41b0), called
through each kind's vtable (0x5c8310/20/30/40) on records made the way 0x4a4b30 makes them, or of random floats.
Nothing in the release calls them; the port keeps them unwired (mech3/turret).

    cmake --build build --target mw3probe && python3 tests/differential/test_turn_clamp.py [runs]

Directions inside and past the pitch and the headings, level, straight up, zero and NaN now and then; `out` starts
with leftovers (the exe's hypot reads its y). The x87 at 24 bits, the CRT's _hypot as x87 code. Compared: the
answer, and the output floats within 4 ulps or 1e-6 (the port's hypot is a double one). The probe library has the
fixes off, so this is the exe's path; the fix's own check (hypot(x, z)) is that a clamped direction sits on the
limit's heading.
"""
import ctypes
import math
import random
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from calltrace import load_probe  # noqa: E402
from original import Original  # noqa: E402
from unicorn.x86_const import UC_X86_REG_EAX  # noqa: E402

KINDS = {'fixed': 0x5c8310, 'narrow': 0x5c8320, 'wide': 0x5c8330, 'full': 0x5c8340}
HYPOT = bytes.fromhex('dd442404 d8c8 dd44240c d8c8 dec1 d9fa c3')  # MSVCRT _hypot: sqrt(x x + y y) on the x87
F3 = ctypes.c_float * 3


def f32(x):
    return struct.unpack('<f', struct.pack('<f', x))[0]


def near(a, b):
    if a == b or (math.isnan(a) and math.isnan(b)):
        return True
    if math.isnan(a) or math.isnan(b) or math.isinf(a) or math.isinf(b):
        return False
    return abs(a - b) <= max(1e-6, 4 * abs(a) * 2 ** -23)


def record(r):
    """0x4a4b30's record by the yaw range (turret.cpp TurnLimits::make), or random floats."""
    lo, hi = sorted([r.uniform(-1.5, 1.5) for _ in range(2)])
    ylo = r.uniform(-math.pi, math.pi)
    rng = r.choice([0.0, 0.0005, r.uniform(0.01, 3.1), r.uniform(3.2, 6.2), 7.0])
    yhi = ylo + rng
    s = lambda a: f32(math.sin(f32(a)))
    c = lambda a: f32(math.cos(f32(a)))
    f = [s(lo), c(lo), s(hi), c(hi)] + [r.uniform(-1, 1) for _ in range(6)]  # stale edges stay
    if rng <= 0.001:
        kind = 'fixed'
        f[4:7] = [c(ylo), 0.0, -s(ylo)]
    elif rng >= 2 * math.pi:
        kind = 'full'
    else:
        kind = 'wide' if rng >= math.pi else 'narrow'
        f[4:10] = [-c(ylo), 0.0, s(ylo), c(yhi), 0.0, -s(yhi)]
    if r.random() < 0.15:
        f = [r.uniform(-1.2, 1.2) for _ in range(10)]
        kind = r.choice(list(KINDS))
    if r.random() < 0.03:
        f[r.randrange(10)] = float('nan')
    return kind, [f32(x) for x in f]


def direction(r, rec):
    c = r.random()
    if c < 0.03:
        return [0.0, 0.0, 0.0]
    if c < 0.05:
        return [float('nan'), 0.5, -0.5]
    if c < 0.065:
        return [0.3, float('nan'), -0.5]
    if c < 0.08:
        return [0.0, r.choice([1.0, -1.0]), 0.0]
    if c < 0.12:  # on a limit's sine, or along an edge
        d = [r.uniform(-1, 1), r.choice([rec[0], rec[2]]), r.uniform(-1, 1)]
        if r.random() < 0.5:
            d[0], d[2] = -rec[6], rec[4]
        return [f32(x) for x in d]
    e = r.uniform(-1.4, 1.4)
    h = r.uniform(-math.pi, math.pi)
    s = r.uniform(0.2, 3) if r.random() < 0.3 else 1.0
    return [f32(x) for x in (s * math.cos(e) * math.sin(h), s * math.sin(e), -s * math.cos(e) * math.cos(h))]


def main():
    runs = int(sys.argv[1]) if len(sys.argv) > 1 else 20000
    r = random.Random(0x5c8310)
    cpu = Original()
    cpu.import_code(0x5bb5b0, HYPOT)
    rec_a, d_a, out_a = cpu.alloc(0x2c), cpu.alloc(12), cpu.alloc(12)
    probe = load_probe()
    probe.probe_turn_clamp.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p]
    probe.probe_turn_clamp.restype = ctypes.c_int
    probe.probe_set_bugs.argtypes = [ctypes.c_int]
    seen = {}
    for n in range(runs):
        kind, f = record(r)
        d = direction(r, f)
        leftover = [r.choice([0.0, 1.0, r.uniform(-2, 2), 1e-30]) for _ in range(3)]
        raw = struct.pack('<I10f', KINDS[kind], *f)
        fn = struct.unpack('<I', cpu.read(KINDS[kind] + 4, 4))[0]
        cpu.write(rec_a, raw)
        cpu.write(d_a, struct.pack('<3f', *d))
        cpu.write(out_a, struct.pack('<3f', *leftover))
        cpu.call(fn, ecx=rec_a, stack_args=(d_a, out_a))
        want_r = cpu.uc.reg_read(UC_X86_REG_EAX) & 0xff
        want = struct.unpack('<3f', cpu.read(out_a, 12))
        out = F3(*leftover)
        got_r = probe.probe_turn_clamp(ctypes.create_string_buffer(raw, 0x2c), F3(*d), out)
        got = list(out)
        if want_r != got_r or not all(near(a, b) for a, b in zip(want, got)):
            raise SystemExit(f'run {n} {kind} at 0x{fn:x}: limits {f} d {d} out {leftover}: '
                             f'original {want_r} {want}, rebuilt {got_r} {got}')
        k = (kind, want_r)
        seen[k] = seen.get(k, 0) + 1
    print(f'{runs} clamps identical: ' + ', '.join(f'{k} -> {a}: {v}' for (k, a), v in sorted(seen.items())))
    # The fix (zfix on): a Fixed record's clamp past the pitch lands on the limit, at the one heading.
    probe.probe_set_bugs(1)
    fixed = 0
    for n in range(2000):
        lo, hi, yaw = r.uniform(-1.2, 0), r.uniform(0, 1.2), r.uniform(-math.pi, math.pi)
        f = [f32(x) for x in (math.sin(lo), math.cos(lo), math.sin(hi), math.cos(hi), math.cos(yaw), 0.0,
                              -math.sin(yaw), 0, 0, 0)]
        d = F3(r.uniform(-1, 1), r.choice([-1.0, 1.0]) * r.uniform(0.95, 1.0), r.uniform(-1, 1))
        out = F3(r.uniform(-2, 2), r.uniform(-2, 2), r.uniform(-2, 2))
        raw = struct.pack('<I10f', KINDS['fixed'], *f)
        probe.probe_turn_clamp(ctypes.create_string_buffer(raw, 0x2c), d, out)
        s, c = (f[0], f[1]) if d[1] < f[0] else (f[2], f[3])
        if not (abs(out[1] - s) < 1e-6 and abs(math.hypot(out[0], out[2]) - c) < 1e-5
                and abs(out[0] * f[4] + out[2] * f[6]) < 1e-5):
            raise SystemExit(f'fix: {list(d)} -> {list(out)}, limit {s} {c}')
        fixed += 1
    probe.probe_set_bugs(0)
    print(f'{fixed} fixed clamps past the pitch (zfix on) on the limit at the heading')
    print('ok: the turn limits\' clamp, all four kinds, matches the original (24-bit FPU)')


if __name__ == '__main__':
    main()
