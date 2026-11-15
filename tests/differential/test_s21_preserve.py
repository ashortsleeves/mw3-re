"""S21's preservation ports, the last of MW3's audit, against the original x86 (docs/render.md, The unused zModel
helpers; zreader.md; editor.md): the material flag-2 setter 0x5374b0 and unload 0x5374d0 with their model loops
0x53bb90 / 0x53bbd0 and the texture chain's unload 0x5190e0 (0x5185f0, the image's free, logged on both sides);
zReader's pair of blocks freed 0x557450 (MSVCRT's free logged); the mech's view pick 0x41cee0 (0x430aa0, the pilot
HUD, answered and logged on both sides).

    cmake --build build --target mw3probe && .venv/bin/python tests/differential/test_s21_preserve.py [runs]
"""
import ctypes
import os
import random
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from calltrace import load_probe  # noqa: E402
from original import Original  # noqa: E402
from unicorn.x86_const import UC_X86_REG_EAX  # noqa: E402

SET_FLAG2, UNLOAD, MODEL_FLAG2, MODEL_UNLOAD, CHAIN, PAIR, PICK = (0x5374b0, 0x5374d0, 0x53bb90, 0x53bbd0, 0x5190e0,
                                                                  0x557450, 0x41cee0)
IMAGE_FREE, IAT_FREE, PILOT_HUD, VIEW = 0x5185f0, 0x5bb65c, 0x430aa0, 0x5f4c8c


class Emulated:
    def __init__(self):
        self.cpu = cpu = Original()
        self.freed = []
        cpu.stub_function(IMAGE_FREE, lambda c: self.freed.append(c.ecx) or 0)
        cpu.stub_import(IAT_FREE, lambda c: self.freed.append(c.arg(0)) or 0)
        cpu.stub_function(PILOT_HUD, self.pilot_hud)
        self.base = cpu.alloc(0x60000)  # the view pick's mechs, data, HUDs and out (mapped, kept)
        self.heap = cpu.heap_next

    def pilot_hud(self, c):
        self.asked = c.ecx
        return self.hud

    def alloc(self, n):
        a = self.cpu.alloc(n)
        self.cpu.write(a, bytes((n + 0xf) & ~0xf))
        return a

    def materials(self, entries, mats, polys, mode, on):
        cpu = self.cpu
        cpu.heap_next, self.freed = self.heap, []
        ea = [self.alloc(0x28) for _ in entries]
        for a, (state, image, mip) in zip(ea, entries):
            cpu.u32(a, image), cpu.u32(a + 0x1c, state), cpu.u32(a + 0x24, ea[mip] if mip >= 0 else 0)
        ma = []
        for word, tex, frames in mats:
            a = self.alloc(0x2c)
            cpu.write(a, struct.pack('<H', word))
            cpu.u32(a + 0x10, ea[tex] if tex >= 0 else 0)
            if frames is not None:
                cyc, arr = self.alloc(0x1c), self.alloc(4 * max(len(frames), 1))
                for k, f in enumerate(frames):
                    cpu.u32(arr + 4 * k, ea[f])
                cpu.u32(cyc + 0x10, len(frames)), cpu.u32(cyc + 0x18, arr), cpu.u32(a + 0x24, cyc)
            ma.append(a)
        model, parr = self.alloc(0x5c), self.alloc(0x24 * max(len(polys), 1))
        for k, mi in enumerate(polys):
            cpu.u32(parr + 0x24 * k + 0x1c, ma[mi])
        cpu.u32(model + 0x10, len(polys)), cpu.u32(model + 0x34, parr)
        first = ma[0] if ma else 0
        if mode == 0:
            cpu.call(SET_FLAG2, ecx=first, edx=on)
        elif mode == 1:
            cpu.call(UNLOAD, ecx=first)
        elif mode == 2:
            cpu.call(MODEL_FLAG2, ecx=model, edx=on)
        elif mode == 3:
            cpu.call(MODEL_UNLOAD, ecx=model)
        else:
            cpu.call(CHAIN, ecx=ea[0] if ea else 0)
        answer = cpu.uc.reg_read(UC_X86_REG_EAX) if mode == 0 else 0
        out = [answer]
        out += [struct.unpack('<H', cpu.read(a, 2))[0] for a in ma]
        for a in ea:
            out += [struct.unpack('<i', cpu.read(a + 0x1c, 4))[0], struct.unpack('<i', cpu.read(a, 4))[0]]
        return out + [len(self.freed)] + self.freed

    def pair(self, a, b):
        cpu = self.cpu
        cpu.heap_next, self.freed = self.heap, []
        p = self.alloc(8)
        cpu.write(p, struct.pack('<II', a, b))
        cpu.call(PAIR, ecx=p)
        return self.freed

    def pick(self, mech, viewed, data, hud, words, dst):
        cpu = self.cpu
        cpu.heap_next = self.heap
        view = self.alloc(0x80)
        cpu.u32(VIEW, view), cpu.u32(view + 0x64, viewed)
        cpu.u32(mech + 0x684, data)
        for k, w in enumerate(words if hud else ()):
            cpu.u32(hud + 0x1d48 + 4 * k, w)
        if dst:
            cpu.write(dst, bytes(12))
        self.hud, self.asked = hud, 0xffffffff
        cpu.call(PICK, ecx=mech, stack_args=[dst])
        got = cpu.uc.reg_read(UC_X86_REG_EAX)
        return [got, self.asked] + (list(struct.unpack('<3I', cpu.read(dst, 12))) if dst else [0, 0, 0])


def material_case(rng):
    n = rng.randint(0, 6)
    entries = [(rng.choice((1, 1, 1, 2, 3, 0)), 0x7000 + 0x10 * i, -1) for i in range(n)]
    for i in range(n - 1):  # mip chains forward, now and then
        if rng.random() < 0.4:
            s, im, _ = entries[i]
            entries[i] = (s, im, rng.randrange(i + 1, n))
    mats = []
    for _ in range(rng.randint(0, 4)):
        word = rng.randrange(256) | rng.choice((0, 1, 2, 3, 3, 3, 7, 0x13)) << 8
        tex = rng.randrange(n) if n and rng.random() < 0.8 else -1
        frames = [rng.randrange(n) for _ in range(rng.randint(0, 3))] if n and rng.random() < 0.4 else None
        mats.append((word, tex, frames))
    polys = [rng.randrange(len(mats)) for _ in range(rng.randint(0, 6))] if mats else []
    return entries, mats, polys


def main():
    p = load_probe()
    I, U = ctypes.POINTER(ctypes.c_int32), ctypes.POINTER(ctypes.c_uint32)
    p.probe_s21_materials.argtypes = [I, ctypes.c_int, I, ctypes.c_int, I, ctypes.c_int, ctypes.c_int, ctypes.c_int, I]
    p.probe_s21_pair.argtypes = [ctypes.c_int32, ctypes.c_int32, I]
    p.probe_s21_pick.argtypes = [ctypes.c_uint32] * 4 + [U, ctypes.c_uint32, U]
    emu = Emulated()
    runs = int(sys.argv[1]) if len(sys.argv) > 1 else 600
    rng = random.Random(0x5374b0)
    out = (ctypes.c_int32 * 4096)()
    stats = dict.fromkeys(('materials', 'freed', 'flags changed', 'pairs', 'picks', 'copied'), 0)

    def same(what, want, got):
        if want != got:
            print(f'{what}:\n  exe  {want}\n  port {got}')
            raise SystemExit(1)

    for run in range(runs):
        entries, mats, polys = material_case(rng)
        mode, on = rng.randrange(5), rng.choice((0, 1, 2, 3, -1))
        if mode in (2, 3) and not polys:
            mode = 4
        want = emu.materials(entries, mats, polys, mode, on)
        fe = [v for e in entries for v in e] or [0]
        fm = []
        for word, tex, frames in mats:
            fm += [word, tex, -1 if frames is None else len(frames)] + (frames or [])
        fm = fm or [0]
        fp = polys or [0]
        k = p.probe_s21_materials((ctypes.c_int32 * len(fe))(*fe), len(entries), (ctypes.c_int32 * len(fm))(*fm),
                                  len(mats), (ctypes.c_int32 * len(fp))(*fp), len(polys), mode, on, out)
        got = list(out[:k])
        want = [v - (1 << 32) if v >= 1 << 31 else v for v in want]
        same(f'run {run} mode {mode} on {on} entries {entries} mats {mats} polys {polys}', want, got)
        stats['materials'] += 1
        stats['freed'] += want[1 + len(mats) + 2 * len(entries)]
        stats['flags changed'] += sum(1 for (w, _, _), v in zip(mats, want[1:1 + len(mats)]) if w != v)

        a, b = rng.choice((0, 0x6000 + 0x10 * run)), rng.choice((0, 0x6800 + 0x10 * run))
        k = p.probe_s21_pair(a, b, out)
        same(f'pair {a:#x} {b:#x}', emu.pair(a, b), list(out[:k]))
        stats['pairs'] += 1

        b = emu.base
        mech = b + 0x1000 * rng.randrange(4)
        viewed = rng.choice((mech, mech, b, 0))
        data, hud = b + 0x10000 + 0x100 * rng.randrange(8), rng.choice((b + 0x20000, b + 0x30000, 0))
        words = [rng.getrandbits(32) for _ in range(3)]
        dst = rng.choice((0, b + 0x50000))
        want = emu.pick(mech, viewed, data, hud, words, dst)
        k = p.probe_s21_pick(mech, viewed, data, hud, (ctypes.c_uint32 * 3)(*words), dst,
                             ctypes.cast(out, U))
        got = [v & 0xffffffff for v in out[:k]]
        same(f'pick mech {mech:#x} viewed {viewed:#x} hud {hud:#x} dst {dst:#x}', want, got)
        stats['picks'] += 1
        stats['copied'] += bool(dst and want[0])
    print(f'S21 preserve: {runs} runs identical: {stats}')
    sys.stdout.flush()
    os._exit(0)


if __name__ == '__main__':
    main()
