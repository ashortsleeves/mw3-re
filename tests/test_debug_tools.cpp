// The developers' tools no key reaches in the release (S5): the GameZ Interpreter console (zint::InterpConsole,
// 0x005960a0..0x00596190) and the stealth object's command slot (0x004a50e0). The key switch's debug commands and
// the next-mech pick run against the exe in test_debug_keys.py.
#include <cstring>
#include <string>

#include "check.h"
#include "gamez/zinterp/zinterp.h"
#include "gamez/zinterp/zinterp_console.h"
#include "mech3/mech3_interp.h"

namespace {
bool console_has(ztwod::TextBox& box, const char* text) {
  for (size_t i = 0; i < box.lines.size(); ++i)
    if (std::strstr(box.lines[i].text, text)) return true;
  return false;
}
}  // namespace

// Made on the interpreter, it takes its messages (+0x70) and sets its +0x04; a line entered runs after the "> "; at
// its end both are cleared.
TEST(interp_console_runs_its_lines_and_prints_the_interpreters_messages) {
  zint::Interp in;
  mech3::interp_create(in);
  auto* c = new zint::InterpConsole(&in);
  EXPECT(zint::interp_console() == c && in.log_fn == zint::interp_console_log && in.console == 1);
  EXPECT(std::strcmp(c->kept, "> ") == 0);
  zint::log(in, "matrix %d", 7);
  EXPECT(console_has(*c, "matrix 7"));
  c->line().set_text("> Quit");
  in.quit = 0;
  c->line_entered();
  EXPECT(std::strcmp(c->kept, "> Quit") == 0);
  EXPECT(in.quit == 1);
  delete c;
  EXPECT(in.log_fn == nullptr && in.console == 0);
  zint::log(in, "after %d", 1);  // nothing to print to: no call
  zint::destroy(in);
}

#include <filesystem>
#include <fstream>

#include "gamez/zsiscript/si_script.h"
#include "gamez/zutil/zutl_path.h"
#include "mech3/mech_anim.h"

// The developers' loose motion (0x005a9ad0's end, 0x005a98d0 -> 0x005a9e10): a motion no archive has is read from
// "<name>.zan" on the search path as text; a name with neither stays missing (docs/motion.md, The text side).
TEST(loose_zan_motion_loads_when_no_archive_has_it) {
  namespace fs = std::filesystem;
  const std::string wd = std::string(MW3_TEST_TMP) + "/loose_zan";
  std::error_code ec;
  fs::remove_all(wd, ec);
  fs::create_directories(wd, ec);
  std::ofstream(wd + "/dev_wave.zan") << "Frame 0\n Object hip\n  Scaling 1 1 1\n  Rotation 0 0 0\n  Translation 1 2 3\n"
                                         "Frame 1\n Object hip\n  Scaling 1 1 1\n  Rotation 0 1.5 0\n  Translation 4 5 6\n";
  const zutl::Mounts keep = zutl::mounts();
  zutl::mounts() = zutl::Mounts{};
  zutl::set_write_dir(wd);
  mech3::close_motions();
  zsis::Script s;
  EXPECT(mech3::load_motion(s, "motion\\dev_wave"));
  EXPECT(s.frame_count == 2 && s.part_count == 1 && std::strcmp(s.parts[0]->name, "hip") == 0);
  EXPECT(s.parts[0]->translations[3] == 4.0f && s.parts[0]->translations[6] == 1.0f);  // the loop closed on frame 0
  mech3::script_free_parts(s);
  zsis::Script none;
  EXPECT(!mech3::load_motion(none, "motion\\no_such_motion"));
  zutl::mounts() = keep;
  fs::remove_all(wd, ec);
}

#include "gamez/zreader/zreader.h"
#include "gamez/zutil/zutl_zar.h"
#include "mech3/mech_names.h"
#include "mech3/node_vehicle_dlg.h"

namespace {
struct ZrdBytes {
  std::string b;
  ZrdBytes& u32(uint32_t v) { b.append(reinterpret_cast<const char*>(&v), 4); return *this; }
  ZrdBytes& i(int32_t v) { return u32(1).u32(static_cast<uint32_t>(v)); }
  ZrdBytes& s(const std::string& v) { u32(3).u32(static_cast<uint32_t>(v.size())); b += v; return *this; }
  ZrdBytes& list(uint32_t items) { return u32(4).u32(items + 1); }
};
}  // namespace

// The release's Add veh to node type list (0x0049d350): the 19 mechs, then vehicle.zrd's record names, made once.
TEST(node_vehicle_types_are_the_mechs_then_vehicle_zrd_records) {
  const std::string body = ZrdBytes().list(1).list(4).s("ttruck").i(1).s("hovertank").i(2).b;
  zutl_zar::Entry e{};
  e.length = static_cast<uint32_t>(body.size());
  std::strncpy(e.name, "vehicle.zrd", sizeof e.name - 1);
  std::string data = body;
  data.append(reinterpret_cast<const char*>(&e), sizeof e);
  data += ZrdBytes().u32(1).u32(1).b;
  const std::string path = std::string(MW3_TEST_TMP) + "/test_vehicle_types.zbd";
  std::FILE* f = std::fopen(path.c_str(), "wb");
  std::fwrite(data.data(), 1, data.size(), f);
  std::fclose(f);
  zrdr::init();
  EXPECT(zrdr::open_archive(path.c_str(), true));
  const std::vector<std::string>& t = mech3::node_vehicle_types();
  EXPECT(t.size() == 21 && t[0] == mech3::mech_name(0) && t[18] == mech3::mech_name(18));
  EXPECT(t[19] == "ttruck" && t[20] == "hovertank");
  zrdr::close_archives(true);
  EXPECT(&mech3::node_vehicle_types() == &t && t.size() == 21);  // made once
  std::remove(path.c_str());
}
