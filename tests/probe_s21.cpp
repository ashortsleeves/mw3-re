// test_s21_preserve.py: S21's preservation ports (the last of MW3's audit) on inputs the test also gives the exe.
#include <cstdint>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <vector>

#include "gamez/zimage/zimg_texture.h"
#include "gamez/zmodel/gmod_const.h"
#include "gamez/zmodel/gmod_matl.h"
#include "gamez/zreader/zreader.h"
#include "mech3/app_hooks.h"

namespace {

std::vector<int32_t> g_freed;
void log_free(void* p) { g_freed.push_back(static_cast<int32_t>(reinterpret_cast<intptr_t>(p))); }

struct PickEngine : mech3::Engine {
  std::unordered_map<uint32_t, uint32_t> mem;
  uint32_t hud = 0;
  uint32_t asked = 0xffffffff;
  uint32_t peek32(uint32_t a) override {
    const auto it = mem.find(a);
    return it == mem.end() ? 0 : it->second;
  }
  void poke32(uint32_t a, uint32_t v) override { mem[a] = v; }
  zgame::Handle pilot_hud(zgame::Handle data) override {
    asked = data;
    return hud;
  }
};

}  // namespace

extern "C" {

// entries: n of (state, image id, mip index or -1); mats: m of (word at +0 (alpha | flags << 8), texture index or
// -1, cycle frame count or -1 for none, then that many entry indices); polys: each polygon's material index. mode 0:
// material_set_flag2(material 0 or null when m is 0, on); 1: material_unload(material 0 or null); 2:
// model_set_flag2(on); 3: model_unload_textures; 4: chain_unload(entry 0). Out: the answer, the materials' words, the
// entries' states and images, the images freed in order.
int probe_s21_materials(const int32_t* entries, int n, const int32_t* mats, int m, const int32_t* polys, int np,
                        int mode, int on, int32_t* out) {
  std::vector<zimg::TextureEntry> e(static_cast<size_t>(n > 0 ? n : 1));
  for (int i = 0; i < n; ++i) {
    e[i] = zimg::TextureEntry{};
    e[i].state = entries[3 * i];
    e[i].image = reinterpret_cast<void*>(static_cast<intptr_t>(entries[3 * i + 1]));
    e[i].mip = entries[3 * i + 2] >= 0 ? &e[static_cast<size_t>(entries[3 * i + 2])] : nullptr;
  }
  std::vector<zmod::Material> mat(static_cast<size_t>(m > 0 ? m : 1));
  std::vector<std::unique_ptr<zmod::Cycle>> cycles;
  std::vector<std::vector<zimg::TextureEntry*>> frames;
  const int32_t* p = mats;
  for (int i = 0; i < m; ++i) {
    mat[i] = zmod::Material{};
    mat[i].alpha = static_cast<uint8_t>(*p & 0xff);
    mat[i].flags = static_cast<uint8_t>((*p++ >> 8) & 0xff);
    const int32_t tex = *p++;
    mat[i].texture = tex >= 0 ? &e[static_cast<size_t>(tex)] : nullptr;
    const int32_t count = *p++;
    if (count >= 0) {
      frames.emplace_back();
      for (int k = 0; k < count; ++k) frames.back().push_back(&e[static_cast<size_t>(*p++)]);
      cycles.push_back(std::make_unique<zmod::Cycle>());
      cycles.back()->texture_count = static_cast<uint32_t>(count);
      cycles.back()->textures = frames.back().data();
      mat[i].cycle = cycles.back().get();
    }
  }
  zmod::Model model;
  std::vector<zmod::Polygon> pv(static_cast<size_t>(np > 0 ? np : 1));
  for (int i = 0; i < np; ++i) pv[i].material = &mat[static_cast<size_t>(polys[i])];
  const uint32_t count = static_cast<uint32_t>(np);
  std::memcpy(model.header + 0x10, &count, 4);
  model.polygons = pv.data();
  g_freed.clear();
  int32_t answer = 0;
  zmod::Material* first = m > 0 ? &mat[0] : nullptr;
  if (mode == 0) answer = zmod::material_set_flag2(first, on);
  if (mode == 1) zmod::material_unload(first, log_free);
  if (mode == 2) zmod::model_set_flag2(model, on);
  if (mode == 3) zmod::model_unload_textures(model, log_free);
  if (mode == 4) zimg::chain_unload(n > 0 ? &e[0] : nullptr, log_free);
  model.polygons = nullptr;
  int32_t* o = out;
  *o++ = answer;
  for (int i = 0; i < m; ++i) *o++ = mat[i].alpha | mat[i].flags << 8;
  for (int i = 0; i < n; ++i)
    *o++ = e[i].state, *o++ = static_cast<int32_t>(reinterpret_cast<intptr_t>(e[i].image));
  *o++ = static_cast<int32_t>(g_freed.size());
  for (int32_t f : g_freed) *o++ = f;
  return static_cast<int>(o - out);
}

// zreader::free_pair on (a, b) as pointers (0 for none): the blocks freed in order.
int probe_s21_pair(int32_t a, int32_t b, int32_t* out) {
  void* pair[2] = {reinterpret_cast<void*>(static_cast<intptr_t>(a)), reinterpret_cast<void*>(static_cast<intptr_t>(b))};
  g_freed.clear();
  zrdr::free_pair(pair, log_free);
  for (size_t i = 0; i < g_freed.size(); ++i) out[i] = g_freed[i];
  return static_cast<int>(g_freed.size());
}

// Engine::view_pick: mech, the viewed mech ([[0x005f4c8c] + 0x64]), the mech's +0x684, the HUD pilot_hud answers,
// its three words at +0x1d48, out (0 for none). Out: the answer, the data asked (-1 none), out's three words.
int probe_s21_pick(uint32_t mech, uint32_t viewed, uint32_t data, uint32_t hud, const uint32_t* words, uint32_t dst,
                   uint32_t* out) {
  PickEngine eng;
  const uint32_t view = 0x00900000;
  eng.mem[0x5f4c8c] = view, eng.mem[view + 0x64] = viewed, eng.mem[mech + 0x684] = data;
  for (uint32_t k = 0; k < 3; ++k) eng.mem[hud + 0x1d48 + 4 * k] = words[k];
  eng.hud = hud;
  out[0] = eng.view_pick(mech, dst);
  out[1] = eng.asked;
  for (uint32_t k = 0; k < 3; ++k) out[2 + k] = dst ? eng.peek32(dst + 4 * k) : 0;
  return 5;
}

}  // extern "C"
