#pragma once
// Android (ours, docs/android.md): where the program's files live on a phone
// or handheld, and the APK's own files put where the rest of the program
// looks for them.
//
// On Android there is no folder beside the program: the APK is one zip.
// Everything goes in the app's external files folder instead
// (/storage/emulated/0/Android/data/<package>/files), which a computer can
// reach over USB:
//   files/            the write dir (saves, mw3.cfg, mw3.log)
//   files/install/    the player's MechWarrior 3 1.2 install (SETUP's own folder)
//   files/assets/     the APK's assets/ (fonts, launcher art), copied at start
#include <string>

namespace sdlfe {

// The folder that stands for "beside the program": SDL_GetBasePath() on the
// desktop, the app's external files folder (with a trailing '/') on Android.
// nullptr when there is none.
const char* program_base();

#ifdef __ANDROID__
// The app's external files folder, no trailing '/'; "" when Android gives none.
std::string android_files_dir();
// The APK's assets (listed in its assets_manifest.txt, written by the Gradle
// build) copied to android_files_dir()/assets, each only when missing or a
// different size. Lines for the log.
void android_copy_assets();
#endif

}  // namespace sdlfe
