#!/bin/sh
# Fetches what the Android build needs into android/deps (docs/android.md):
# SDL3, SDL3_ttf (with its vendored FreeType and HarfBuzz) and SDL3_sound at
# the versions the desktop builds use (docs/linux.md), and SDL's Gradle
# wrapper and Java glue. Run once from anywhere: sh android/fetch_deps.sh
set -eu
here=$(cd "$(dirname "$0")" && pwd)
deps="$here/deps"
mkdir -p "$deps"

fetch() {  # name url tag [extra git flags]
  if [ -d "$deps/$1/.git" ]; then
    echo "have $1"
  else
    git -c advice.detachedHead=false clone --depth 1 -b "$3" ${4:-} "$2" "$deps/$1"
  fi
}
fetch SDL https://github.com/libsdl-org/SDL release-3.4.12
fetch SDL_ttf https://github.com/libsdl-org/SDL_ttf release-3.2.2 "--recurse-submodules --shallow-submodules"
fetch SDL_sound https://github.com/icculus/SDL_sound v3.2.0

# SDL's Gradle wrapper, so ./gradlew works without a Gradle install.
cp "$deps/SDL/android-project/gradlew" "$deps/SDL/android-project/gradlew.bat" "$here/"
mkdir -p "$here/gradle/wrapper"
cp "$deps/SDL/android-project/gradle/wrapper/"* "$here/gradle/wrapper/"
chmod +x "$here/gradlew"
echo "done: open android/ in Android Studio, or run ./gradlew assembleDebug in android/"
