# Handoff: MechWarrior 3 Rebuild → Android (Retroid Pocket 4 Pro)

Paste this file into a new Claude Code session, or save it in the repo and say
"read HANDOFF-android.md and continue". It sums up the work from an earlier
Claude session, which wrote the changes in a cloud workspace that had no
Android SDK or NDK.

## Goal

Port the MechWarrior 3 Rebuild (https://github.com/telltaleatheist/mw3-re,
MIT, C++, SDL3) to Android as a sideloaded APK with controller support. The
target device is a **Retroid Pocket 4 Pro**:

- MediaTek Dimensity 1100 (4× Cortex-A78, 4× Cortex-A55), **Mali-G77 MC9 GPU**
- 8 GB RAM, Android 13
- 750×1334 touchscreen, Hall-effect sticks, analog triggers

The build machine is a **2019 Intel MacBook Pro** running Android Studio. The
user also has VS Code with Claude Code.

## Why the port is feasible (from the code audit)

- About 200k lines of C++, with no x86 asm or SSE intrinsics. It already ships
  native arm64 builds for Apple silicon.
- The original game's x87 math is emulated in portable code, and the build sets
  `-ffp-contract=off`, so ARM's fused multiply-add won't change results.
- The renderer uses the SDL3 GPU API. On Linux it uses Vulkan, with SPIR-V
  shaders compiled at build time by `glslangValidator`, and Android takes the
  same path.
- SDL gamepads are already mapped to the XInput layout the original game
  expected, with rumble.

## State of the work

All changes are in **one commit on a branch named `android-port`**. The user
applies it from a patch file with
`git am 0001-Android-build-the-game-as-libmain.so-for-SDL-s-SDLAc.patch`.

**Never built with the NDK and never run on the device.** Checks done so far:

1. The desktop Linux build (`mw3`, `mw3launcher`) still compiles and starts.
2. The Android-only `#ifdef __ANDROID__` code passed a syntax-only compile
   check with `-D__ANDROID__ -DSDL_PLATFORM_ANDROID` on host clang.
3. The game built and linked on Linux through `android/app/jni/CMakeLists.txt`,
   with SDL3, SDL3_ttf and SDL3_sound added as subdirectories. This is the same
   structure Gradle uses.

### New files

| File | Purpose |
|---|---|
| `android/fetch_deps.sh` | Clones SDL `release-3.4.12`, SDL_ttf `release-3.2.2` (with submodules) and SDL_sound `v3.2.0` into `android/deps/`, then copies SDL's `gradlew` and `gradle/wrapper` into `android/`. All of these are gitignored. |
| `android/settings.gradle`, `android/build.gradle`, `android/gradle.properties` | Root Gradle project. AGP 8.7.3, the version SDL 3.4.12 uses. |
| `android/app/build.gradle` | `com.mw3re.game`, minSdk 29, target/compile 35, NDK `28.2.13676358`, arm64-v8a only, `c++_static`. Java sources come from `deps/SDL/android-project/app/src/main/java`. A `mw3Assets` task copies the repo's `assets/` into the APK under `assets/...` and writes `assets_manifest.txt`. Release builds are signed with the debug key. |
| `android/app/jni/CMakeLists.txt` | Native build: SDL3 shared (`libSDL3.so`), SDL3_ttf (vendored FreeType/HarfBuzz, plutosvg off) and SDL3_sound static, then `add_subdirectory` of the repo root. |
| `android/app/jni/sdl3-config/SDL3Config.cmake` | Stand-in package config. SDL_sound calls `find_package(SDL3 ... COMPONENTS SDL3)`, which fails when SDL3 is a subdirectory, so this file reports SDL3 as found when the `SDL3::SDL3` target exists. `SDL3_DIR` points at it. |
| `android/app/src/main/AndroidManifest.xml` | Requires Vulkan. Landscape (`sensorLandscape`), VIBRATE, INTERNET, and the gamepad/touch features marked optional. |
| `android/app/src/main/java/com/mw3re/game/MW3Activity.java` | Empty subclass of `org.libsdl.app.SDLActivity`, which loads `SDL3` then `main`. |
| `android/app/src/main/res/values/strings.xml` | `app_name` = "MW3 Rebuild" (there is no icon yet). |
| `src/frontend/sdl/android_support.h/.cpp` | `sdlfe::program_base()` replaces `SDL_GetBasePath()`: on Android it returns the external files folder with a trailing `/`. Also `android_files_dir()` (`SDL_GetAndroidExternalStoragePath()`) and `android_copy_assets()`, which reads `assets_manifest.txt` from the APK with `SDL_LoadFile` and copies each file to `<files>/assets/...` when it is missing or its size differs. |
| `docs/android.md` | Build steps, file layout on the device, a table of every change, and the open questions. |

### Changed files

| File | Change |
|---|---|
| `CMakeLists.txt` | `CMAKE_SOURCE_DIR`/`CMAKE_BINARY_DIR` replaced with `CMAKE_CURRENT_*` everywhere, because the root file is now added as a subdirectory. This is a no-op on the desktop. With `if(ANDROID)`, `MW3_PACKAGE` is forced ON and `mw3` is `add_library(SHARED)` with `OUTPUT_NAME main`, linking `android log`. mw3probe, the tests, mw3launcher, mw3sdltests and the tools (mw3view, mw3import, etc.) are skipped. SDL3, SDL3_ttf and SDL3_sound count as found when their targets already exist (`if(TARGET ...)`) instead of always calling `find_package`. `android_support.cpp` was added to `mw3sdl`. |
| `src/frontend/sdl/mw3_main.cpp` | Includes `<SDL3/SDL_main.h>` on Android. Sets hints before `SDL_Init`: `SDL_HINT_ANDROID_TRAP_BACK_BUTTON=1`, `SDL_HINT_ORIENTATIONS="LandscapeLeft LandscapeRight"`, `SDL_HINT_TOUCH_MOUSE_EVENTS=1`. On Android, `android_copy_assets()` is called instead of `crash_report_install` (which forks/execs). A requested restart (the MODS menu) only logs a message, because execv isn't possible. |
| `src/frontend/sdl/data_folder_start.h` | On Android the usual write folder is `android_files_dir()` instead of `SDL_GetPrefPath`. |
| `src/frontend/sdl/launcher.cpp`, `sdl_twod_host.cpp`, `sdl_install.cpp` | `SDL_GetBasePath()` → `program_base()`. |
| `src/frontend/sdl/font_search.cpp` | On Android the only font folder is `/system/fonts`. |
| `src/frontend/sdl/vulkan_check.cpp` | On Android it loads `libvulkan.so`, not `libvulkan.so.1`. Android defines `__linux__`, so without this fix the game would always report "no Vulkan". Also adds an Android-specific error message. |
| `src/frontend/sdl/sdl_input.cpp` | `SDL_SCANCODE_AC_BACK` maps to DIK_ESCAPE (0x01) on Android. |
| `.gitignore` | Ignores `android/deps`, the Gradle and CMake build output, the wrapper files, `local.properties` and `*.apk`. |

Repo convention: the project's own additions are commented with "ours" and a
pointer to the relevant doc, e.g. `// ours (android_support.h): ...`. Keep
doing that.

## Files on the device

Everything lives in `/storage/emulated/0/Android/data/com.mw3re.game/files/`,
which the Mac can reach over USB (OpenMTP or Android File Transfer) or with
`adb push`:

- `install/`: the player's MechWarrior 3 1.2 install, i.e. Mech3.exe's folder. The game already looks in `<write dir>/install`.
- The write-dir contents from a desktop setup: `movies/`, `music/`, `mw3.cfg`. Movies must be converted on the Mac with the desktop build's `mw3import`, because the importer isn't in the APK.
- `assets/`: copied from the APK at each start.
- `mw3.log`: the game's log.

The easiest route is to set the game up on the Mac with the desktop rebuild
(Intel Macs are supported), then copy the install folder and the write-dir
contents to the device.

## Next steps

1. Run `brew install glslang`, then `sh android/fetch_deps.sh`.
2. Open `android/` (not the repo root) in Android Studio, install NDK `28.2.13676358` and CMake ≥ 3.22.1, and do a Gradle sync followed by Build → Make Project.
3. Fix build errors. Likely ones:
   - **`glslangValidator` not found.** CMake runs it on the host, but Android Studio's PATH may lack `/usr/local/bin` (Intel Homebrew). Add `"-DGLSLANG=/usr/local/bin/glslangValidator"` to the cmake `arguments` in `app/build.gradle`.
   - **Warnings treated as errors** or new clang warnings from the NDK's clang.
   - **Missing bionic functions** in `crash_report.cpp`, `sdl_restart.cpp` or `sdl_update.cpp` (they still compile on Android but aren't called).
   - **enet/miniupnpc** networking headers.
4. Install with `adb install -r app/build/outputs/apk/debug/app-debug.apk`, push the game files, and launch the app. Read `adb logcat -s SDL SDL/APP` and `adb pull .../files/mw3.log`.
5. Device questions:
   - **Does SDL_GPU's Vulkan backend start on Mali-G77?** The depth format is D32_FLOAT, which is fine. The editor's wireframe needs `fillModeNonSolid`.
   - **4:3 vs 16:9 scaling.**
   - **Suspend/resume.**
   - **Audio latency.**
6. Controls work:
   - **Menus:** they are mouse-driven (touch works as a mouse for now). Add a gamepad-driven virtual cursor in `sdl_input.cpp`: stick moves the pointer, A clicks, B is Escape.
   - **Mission layout:** check the default mapping on the Retroid's sticks and analog triggers (throttle, torso twist, weapon groups).

## Related decisions

- MW4 has no source port. The repo `Icehellionx/MechWarrior-4-Remastered` is
  only a Windows installer/patcher, so the MW4 route is Winlator or GameHub
  (Wine + Box64). That is out of scope here.
