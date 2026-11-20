# The Android build's native part (docs/android.md): SDL3 as libSDL3.so,
# SDL3_ttf and SDL3_sound linked in statically, and the game as libmain.so
# from the repository's own CMakeLists.txt. Gradle runs this with the NDK.
cmake_minimum_required(VERSION 3.20)
project(mw3android LANGUAGES C CXX)

set(MW3_DEPS "${CMAKE_CURRENT_SOURCE_DIR}/../../deps")
if(NOT EXISTS "${MW3_DEPS}/SDL/CMakeLists.txt")
  message(FATAL_ERROR "android/deps is empty: run sh android/fetch_deps.sh first")
endif()

# SDL3: shared, as SDLActivity loads libSDL3.so before libmain.so.
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory("${MW3_DEPS}/SDL" SDL)
# SDL_sound asks for SDL3's package: the stand-in says it is here.
set(SDL3_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sdl3-config" CACHE PATH "" FORCE)

# The companions: static, into libmain.so.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SDLTTF_VENDORED ON CACHE BOOL "" FORCE)
set(SDLTTF_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDLTTF_INSTALL OFF CACHE BOOL "" FORCE)
set(SDLTTF_PLUTOSVG OFF CACHE BOOL "" FORCE)  # colour emoji: not needed
add_subdirectory("${MW3_DEPS}/SDL_ttf" SDL_ttf)

set(SDLSOUND_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SDLSOUND_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(SDLSOUND_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(SDLSOUND_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory("${MW3_DEPS}/SDL_sound" SDL_sound)

# The game (the repository root): ANDROID makes its mw3 target libmain.so.
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../.." mw3re)
