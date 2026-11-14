#include "mech3/debug_keys.h"

#include <cstdio>

#include "mech3/dev_text.h"

namespace mech3 {

int32_t& debug_marker_count() {
  static int32_t n = 0;
  return n;
}

// Mech3.exe 1.2 @ 0x004a3220: the place asked three times (0x005a85e0, each coordinate).
void debug_marker(const float pos[3]) {
  std::printf("\nMarker # %d\n", debug_marker_count()++);
  std::printf("Position: %g %g %g\n", static_cast<double>(pos[0]), static_cast<double>(pos[1]),
              static_cast<double>(pos[2]));
}

// Mech3.exe 1.2 @ 0x004a33f0
void* next_view_mech(const std::vector<void*>& entities, void* target, void* (*mech_of)(void*)) {
  for (DevText& r : dev_lines().rows) r.set_text("");  // vt+0x7c with [0x005f4c90]
  auto mech = [&](void* e) { return e ? mech_of(e) : nullptr; };
  auto first = entities.begin();
  while (first != entities.end() && !mech(*first)) ++first;
  void* found = nullptr;
  if (first != entities.end() && target) {
    auto at = first;  // the search starts at the first mech
    while (at != entities.end() && *at != target) ++at;
    if (at == entities.end()) {
      found = mech(*first);
    } else {
      do ++at;
      while (at != entities.end() && !mech(*at));
      found = mech(at == entities.end() ? *first : *at);
    }
  }
  return found && found != target ? found : nullptr;
}

}  // namespace mech3
