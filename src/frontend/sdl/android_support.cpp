#include "frontend/sdl/android_support.h"

#include <SDL3/SDL.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace sdlfe {

#ifdef __ANDROID__

std::string android_files_dir() {
  const char* p = SDL_GetAndroidExternalStoragePath();
  if (!p) {
    std::fprintf(stderr, "mw3: android: no external files folder: %s\n", SDL_GetError());
    return {};
  }
  std::string dir = p;
  while (dir.size() > 1 && dir.back() == '/') dir.pop_back();
  return dir;
}

const char* program_base() {
  static const std::string base = [] {
    const std::string d = android_files_dir();
    return d.empty() ? d : d + "/";
  }();
  return base.empty() ? nullptr : base.c_str();
}

void android_copy_assets() {
  namespace fs = std::filesystem;
  const std::string root = android_files_dir();
  if (root.empty()) return;
  // SDL reads a relative path from the APK's assets/.
  size_t size = 0;
  void* manifest = SDL_LoadFile("assets_manifest.txt", &size);
  if (!manifest) {
    std::fprintf(stderr, "mw3: android: the APK has no assets_manifest.txt: %s\n", SDL_GetError());
    return;
  }
  std::istringstream lines(std::string(static_cast<const char*>(manifest), size));
  SDL_free(manifest);
  int copied = 0, kept = 0, failed = 0;
  for (std::string rel; std::getline(lines, rel);) {
    if (!rel.empty() && rel.back() == '\r') rel.pop_back();
    if (rel.empty() || rel.find("..") != std::string::npos) continue;
    size_t n = 0;
    void* data = SDL_LoadFile(rel.c_str(), &n);  // e.g. "assets/fonts/Anton-Regular.ttf"
    if (!data) {
      ++failed;
      std::fprintf(stderr, "mw3: android: %s not in the APK: %s\n", rel.c_str(), SDL_GetError());
      continue;
    }
    const fs::path to = fs::path(root) / rel;
    std::error_code ec;
    if (fs::exists(to, ec) && fs::file_size(to, ec) == n && !ec) {
      ++kept;
    } else {
      fs::create_directories(to.parent_path(), ec);
      std::ofstream f(to, std::ios::binary | std::ios::trunc);
      f.write(static_cast<const char*>(data), static_cast<std::streamsize>(n));
      if (f) ++copied;
      else ++failed, std::fprintf(stderr, "mw3: android: cannot write %s\n", to.string().c_str());
    }
    SDL_free(data);
  }
  std::printf("mw3: android: files folder %s; assets %d copied, %d already there, %d failed\n", root.c_str(), copied,
              kept, failed);
}

#else

const char* program_base() { return SDL_GetBasePath(); }

#endif

}  // namespace sdlfe
