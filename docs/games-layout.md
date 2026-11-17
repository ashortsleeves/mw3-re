# How the rebuild is organised: MW3, Pirate's Moon and the launcher

Decided with Owen, 2026-10-31. (The original exe's own structure is
`architecture.md`; this page is about the rebuild.)

## Three programs, one package

- **The launcher** (ours, not original code): the main icon. It finds the
  installs, runs SETUP (install from disc, the music, the movies) and
  starts the game the player picks. It is its own program, so either game
  can also be started without it.
- **`mw3`**: MechWarrior 3. Started directly, it goes straight into the
  game.
- **`mw3pm`**: Pirate's Moon (after MW3 `v1.0`). Started directly, it goes
  straight into PM.

A release has one download per platform (the Windows installer, the Mac
DMG, the Linux AppImage) holding all three. The installer's main shortcut
is the launcher; the two games get their own Start-menu shortcuts. Each
game runs without the other: removing `mw3pm` leaves MW3 working and the
other way round. Each game mounts its own install's data (the player's
own MW3 or PM files) and keeps its own settings and saves.

## The code

| Folder | What | Address comments |
|---|---|---|
| `src/gamez/` | the shared GameZ engine (render, zTwoD, zSound, readers, ...) | `// Mech3.exe 1.2 @ 0x...` |
| `src/mech3/` | MechWarrior 3's game code | `// Mech3.exe 1.2 @ 0x...` |
| `src/pm/` | only what Pirate's Moon adds or changes | `// Pirate's Moon 1.0 @ 0x...` |
| `src/frontend/sdl/` | the SDL3 platform layer and the launcher | ours |

- MW3's code never includes anything from `src/pm/`.
- Where PM changed a function MW3 also has, PM's version lives in
  `src/pm/` under the same name, with a one-line note on what changed
  (`docs/pm-map.md` lists every pair). An engine function PM changed is
  chosen per game at run time through that game's table, never by `#ifdef`
  scattered in shared code.
- Build targets: `mw3`, `mw3pm`, the launcher, and each game's tests. A
  switch (`-DMW3_BUILD_PM=OFF`) builds MW3 alone.
- Tests: each game has its own test executables and differential tests
  (`MW3_EXE=pm` runs a test against PM's exe), so a PM change cannot break
  MW3's suite unseen.

## Which branch builds what (Owen, 2026-11-04, replacing the shared-engine plan)

PM lives on the branch `pm`, which never merges into `main`: a complete copy
of the game with PM's changes in place, as the original's `branchep1` was.

| Branch | Builds | Released from |
|---|---|---|
| `main` | `mw3`, the launcher (`mw3launcher`), MW3's tests | a tag on `main` (`v1.x.y`) |
| `pm` | `mw3pm`, PM's tests (`mw3pmtests`, `mw3pmsdltests`, `pmprobe`), `zipkit` | a tag on `pm` (`pm-v1.x.y`) |

- The combined package (launcher, `mw3`, `mw3pm`) takes the launcher and
  `mw3` from a tag on `main` and `mw3pm` from a tag on `pm`. The launcher
  knows every game and starts each by its path; the games know nothing of
  each other.
- Fixes cross by git. MW3's fixes merge from `main` into `pm`. A bug found on
  `pm` that MW3 also has is cherry-picked or redone on `main`. PM's
  improvements never reach `main`.
- MW3's targets stay in `pm`'s CMakeLists behind `-DMW3_BUILD_MW3` (OFF), so
  `main`'s merges into `pm` stay clean and a shared-code change on `pm` can
  be checked against MW3's tests (`pm-tests.md`, MW3's tests built from
  `pm`). Nothing is built or released from them.
- Another game on PM's engine (MW1: Critical Hit, its own repo) takes the
  engine from `pm` by merging it, and its launcher entry from `main`'s
  launcher (`launcher.md`, MechWarrior 1).
