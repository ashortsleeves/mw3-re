# A stand-in for SDL3's installed package file (docs/android.md): SDL3 is built
# alongside (add_subdirectory), so its targets already exist. SDL_sound's
# find_package(SDL3 ... COMPONENTS SDL3) finds this.
if(NOT TARGET SDL3::SDL3)
  set(SDL3_FOUND FALSE)
  set(SDL3_NOT_FOUND_MESSAGE "SDL3 must be added with add_subdirectory before this")
  return()
endif()
set(SDL3_FOUND TRUE)
set(SDL3_SDL3_FOUND TRUE)
set(SDL3_VERSION 3.4.12)
