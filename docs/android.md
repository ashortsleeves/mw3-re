# Android

The rebuild on Android: the same program as on the desktop (SDL3, its GPU API
through Vulkan), built as `libmain.so` for SDL's `SDLActivity` and packaged
as an APK. Aimed at handhelds with controllers (first target: Retroid Pocket 4
Pro, MediaTek Dimensity 1100, Mali-G77, Android 13).

Status: **not yet built with the NDK or run on a device.** The changes are
checked by the desktop Linux build (unchanged behaviour), by the Android
code paths compiled for syntax, and by building the game through
`android/app/jni/CMakeLists.txt` (SDL3, SDL3_ttf and SDL3_sound added as
subdirectories) on Linux.

## Building (macOS, Linux or Windows with Android Studio)

Needs Android Studio with, from its SDK Manager: the NDK `28.2.13676358`
(SDL 3.4.12's) and CMake 3.22.1 or later. Also `glslangValidator`
(macOS: `brew install glslang`; it compiles the Vulkan shaders on the
computer, as for Linux).

```sh
sh android/fetch_deps.sh      # SDL3, SDL3_ttf, SDL3_sound and SDL's Gradle wrapper into android/
cd android
./gradlew assembleDebug        # or open android/ in Android Studio
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

If Gradle cannot find `glslangValidator`, give CMake its path in
`app/build.gradle`'s cmake arguments: `"-DGLSLANG=/usr/local/bin/glslangValidator"`.

## The game's files

The APK has no game data. Everything lives in the app's external files
folder, which a computer reaches over USB (macOS: Android File Transfer or
OpenMTP) or `adb push`:

```
/storage/emulated/0/Android/data/com.mw3re.game/files/
  install/     your MechWarrior 3 1.2 install (Mech3.exe's folder)
  movies, music and saves: where the desktop program puts them in its write dir
  assets/      copied from the APK at each start (fonts, launcher art)
  mw3.log      the log of the last start
```

The simplest route: set the game up with the desktop program first
(SETUP from the disc image, the 1.2 patch, the movie and music import), then
copy its install to `files/install/` and its write dir's contents
(`movies/`, `music/`, `mw3.cfg`) to `files/`.

```sh
adb push ~/path/to/mw3-1.2 /storage/emulated/0/Android/data/com.mw3re.game/files/install
adb logcat -s SDL SDL/APP mw3   # SDL's own messages; the game's are in files/mw3.log
adb pull /storage/emulated/0/Android/data/com.mw3re.game/files/mw3.log
```

## What is different on Android (ours)

| Where | What | Why |
|---|---|---|
| CMakeLists.txt | `ANDROID`: `mw3` is a shared library named `main`; no launcher, tests, probes or tools; `MW3_PACKAGE` on | SDLActivity loads libSDL3.so then libmain.so and calls `SDL_main` |
| CMakeLists.txt | `CMAKE_CURRENT_SOURCE_DIR`/`_BINARY_DIR` in place of `CMAKE_SOURCE_DIR`/`_BINARY_DIR`; SDL3, SDL3_ttf and SDL3_sound taken as targets when they already exist | the root file is added from android/app/jni; the same on the desktop |
| mw3_main.cpp | `SDL_main.h` included; no crash box; no restart into a MODS entry (said in the log) | an app cannot fork and exec itself |
| mw3_main.cpp | back button trapped, landscape only, taps as mouse clicks | the menus are the original's, mouse-driven |
| sdl_input.cpp | Android's back button is Escape | backs out of menus, opens the pause menu |
| data_folder_start.h | the write dir is the app's external files folder | reachable over USB, unlike SDL_GetPrefPath's |
| android_support.cpp | `program_base()` stands for SDL_GetBasePath (launcher.cpp, sdl_twod_host.cpp, sdl_install.cpp); the APK's assets copied out by `assets_manifest.txt` | no folder beside the program on Android |
| font_search.cpp | `/system/fonts` | no HOME or XDG folders |
| vulkan_check.cpp | `libvulkan.so` (no `.1`) and a message for a device | Android's loader's name; Android defines `__linux__` |
| app_engine.cpp | no `MW3_TRACE_UNPORTED` backtrace | bionic has `execinfo.h` from API 33 only (minSdk is 29) |
| zmth_matrix.cpp | the matrix-stack fault's callers walked with `_Unwind_Backtrace` | the same |
| sdl_restart.cpp | `sysconf(_SC_OPEN_MAX)` in place of `getdtablesize()` (on the desktop too, the same value) | bionic has no `getdtablesize` |
| sdl_platform.cpp, vulkan_check.cpp | the GPU device made, and the check asked, without clip distances (`SDL_PROP_GPU_DEVICE_CREATE_FEATURE_CLIP_DISTANCE_BOOLEAN` false); the check logs SDL's reason for a refusal | the Retroid Pocket 4 Pro's Mali-G77 has no `shaderClipDistance` (the other three opt-outs it has), and SDL_GPU asks for it by default; the shaders write none |
| sdl_input.cpp | outside a mission the mouse is not held: zInput's mouse goes where the finger is, as the menus in a window | a tap has a place and no motion, which a held (relative) mouse never sees |
| sdl_input.cpp | outside a mission the gamepad works the menus: the left stick and the d-pad move the cursor, A clicks, B is Escape, Start is Enter (a movie's skip); each as the event a mouse or key would send. The buttons come as joystick events through the pad's map (SDL_GetGamepadBindings): SDL is started with joysticks only, so no gamepad events come | a handheld has no mouse or keyboard; a movie skips only on Space, Escape or Enter |

## Open questions (to settle on a device)

- **Mali's Vulkan driver.** Settled on the Retroid Pocket 4 Pro (2026-11-20):
  with clip distances off, SDL_GPU's Vulkan device is made and the launcher,
  the menus and the movies draw. Wireframe (`FILLMODE_LINE`, the editor's)
  needs `fillModeNonSolid`, not yet checked.
- **Menus with a pad.** Done (above); the cursor's speed is to be tuned by
  hand.
- **Screen.** 4:3 frames on a 16:9 screen; the menus' 16:9 (widemenus=) is on.
- **Movies.** Imported on a computer (mw3import, FFmpeg); the importer is not
  part of the APK, so the first start's import is skipped.
- **Suspend and resume.** SDL keeps the GPU device across a pause on recent
  versions; to be seen.
