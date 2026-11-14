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
