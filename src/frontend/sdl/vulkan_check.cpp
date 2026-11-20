#include "frontend/sdl/vulkan_check.h"

#include <SDL3/SDL.h>

#include <cstdlib>
#include <cstring>

namespace sdlfe {

Vulkan vulkan_check() {
  if (const char* forced = std::getenv("MW3_VULKAN_CHECK")) {  // (the process's own: SDL_getenv keeps a copy)
    if (std::strcmp(forced, "noloader") == 0) return Vulkan::kNoLoader;
    if (std::strcmp(forced, "nodriver") == 0) return Vulkan::kNoDriver;
    return Vulkan::kOk;
  }
#if defined(__linux__)  // Android too (it defines __linux__)
#ifdef __ANDROID__
  SDL_SharedObject* lib = SDL_LoadObject("libvulkan.so");  // ours: Android's loader has no .1
#else
  SDL_SharedObject* lib = SDL_LoadObject("libvulkan.so.1");
#endif
  if (!lib) return Vulkan::kNoLoader;
  SDL_UnloadObject(lib);
  // SDL's own test for its Vulkan backend: an instance and a device with what SDL_GPU needs.
#ifdef __ANDROID__
  // ours (docs/android.md): asked as SdlWindow::init creates it (no clip distances: Mali has none,
  // the shaders write none); a refusal's reason goes to the log, the driver being the device's own
  SDL_SetLogPriority(SDL_LOG_CATEGORY_GPU, SDL_LOG_PRIORITY_VERBOSE);
  SDL_ClearError();
  SDL_PropertiesID p = SDL_CreateProperties();
  SDL_SetBooleanProperty(p, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);
  SDL_SetStringProperty(p, SDL_PROP_GPU_DEVICE_CREATE_NAME_STRING, "vulkan");
  SDL_SetBooleanProperty(p, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_CLIP_DISTANCE_BOOLEAN, false);
  const bool ok = SDL_GPUSupportsProperties(p);
  SDL_DestroyProperties(p);
  if (!ok) SDL_Log("mw3: vulkan: SDL_GPU refused the device: %s", SDL_GetError());
  return ok ? Vulkan::kOk : Vulkan::kNoDriver;
#else
  return SDL_GPUSupportsShaderFormats(SDL_GPU_SHADERFORMAT_SPIRV, "vulkan") ? Vulkan::kOk : Vulkan::kNoDriver;
#endif
#else
  return Vulkan::kOk;
#endif
}

std::string vulkan_help(Vulkan v) {
  if (v == Vulkan::kOk) return {};
#ifdef __ANDROID__
  return std::string("MechWarrior 3 needs Vulkan graphics, and this device has ") +
         (v == Vulkan::kNoLoader ? "no Vulkan library" : "no Vulkan driver with what the game needs") +
         ". The log (mw3.log in the app's files folder) has SDL's own reason.";
#endif
  const std::string what = v == Vulkan::kNoLoader ? "no Vulkan library (libvulkan)"
                                                  : "no Vulkan driver for its graphics card";
  return "MechWarrior 3 needs Vulkan graphics, and this computer has " + what +
         ". Install your system's 64-bit Vulkan packages (not the lib32 ones), then start again:\n"
         "Arch: vulkan-icd-loader and vulkan-intel, vulkan-radeon or nvidia-utils\n"
         "Debian or Ubuntu: libvulkan1 and mesa-vulkan-drivers\n"
         "Fedora: vulkan-loader and mesa-vulkan-drivers\n"
         "(NVIDIA's own driver brings its Vulkan with it.)";
}

}  // namespace sdlfe
