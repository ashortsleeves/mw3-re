#include "mech3/app_keys.h"

#include "gamez/zinput/zinput.h"
#include "gamez/zmodel/gmod_cull.h"
#include "gamez/zutil/ztimer.h"
#include "mech3/cheats.h"
#include "mech3/dev_text.h"
#include "mech3/game_editor.h"
#include "mech3/message_panel.h"
#include "upgrades/editor/mods_edit.h"

namespace mech3 {
namespace {
AppKeyCalls g_calls;
void debug_7e(int32_t) {
  if (g_calls.debug_7e) g_calls.debug_7e();
}
}  // namespace

AppKeyCalls& app_key_calls() { return g_calls; }
AppKeyState& app_key_state() {
  static AppKeyState s;
  return s;
}

// Mech3.exe 1.2 @ 0x0040db60
void register_app_keys() {
  zin::set_callback(0x61, app_key);   // CYCLE_CAM (X)
  zin::set_callback(0x70, app_key);   // MESSAGE_TOGGLE (`)
  zin::set_callback(0x7e, debug_7e);  // 0x004abbc0
  zin::set_callback(0x6c, app_key);   // VMODE_TOGGLE (Shift+V)
  register_cheat_key();               // ours: the cheat box on Shift+Enter (cheats.h)
  // Ours: the game editor on Shift+F12, which does something only in a
  // mission the MODS screen launched (editor_reachable, mods_edit.h).
  zin::bind(kEditorCommand, nullptr, kEditorKey, 0, 0, 0);
  zin::set_callback(kEditorCommand, app_key);
}

// Mech3.exe 1.2 @ 0x0040dba0
void app_key(int32_t cmd) {
  AppKeyState& st = app_key_state();
  switch (cmd) {
    case 0x61:
      if (g_calls.cycle_camera) g_calls.cycle_camera();
      break;
    case 0x6c:
      if (g_calls.vmode_cycle) g_calls.vmode_cycle();
      break;
    case 0x6d:
      if (g_calls.mech_debug_bit) g_calls.mech_debug_bit();
      break;
    case 0x70:
      if (g_calls.message_toggle) g_calls.message_toggle();
      break;
    case 0x82:  // the frame rate and the developer lines on or off
      if (st.dev_text) {
        fps_text().hide();
        dev_lines().set_active(0);
        st.dev_text = 0;
      } else {
        fps_text().show_for(-1.0f);
        dev_lines().set_active(1);
        st.dev_text = 1;
      }
      break;
    case 0x83:
      st.readout = st.readout == 0;
      dev_readout(st.readout);
      break;
    case 0x84:
      if (g_calls.mech_list_dialog) {
        const int32_t roster = g_calls.mech_list_dialog();
        if (roster != -1 && g_calls.mech_remake) g_calls.mech_remake(roster);
      }
      break;
    case 0x86:
      st.clamp = st.clamp == 0;
      ztimer::set_clamp_enabled(st.clamp);
      break;
    case 0x87:  // the constant is -5, subtracted
      st.max_rate = ztimer::set_max_rate(st.max_rate - -5.0f);
      break;
    case 0x88:
      st.max_rate = ztimer::set_max_rate(st.max_rate - 5.0f);
      break;
    case 0x89:
      if (g_calls.next_view_mech) g_calls.next_view_mech();
      break;
    case 0x8a:
      if (g_calls.vehicle_dialog) g_calls.vehicle_dialog();
      break;
    case 0x8b:
    case 0x8c:
    case 0x8e:
      if (g_calls.camera_mode) g_calls.camera_mode(cmd == 0x8b ? 0 : cmd == 0x8c ? 3 : 1);
      break;
    case 0x8f:
      if (g_calls.camera_dialog) g_calls.camera_dialog();
      break;
    case 0x93:  // the cursor shown or hidden by the flag, the flag then whether its count went below 0 (hidden)
      st.cursor = (g_calls.show_cursor ? g_calls.show_cursor(st.cursor) : 0) < 0;
      break;
    case 0x94:
      if (g_calls.marker) g_calls.marker();
      break;
    case 0x95:
      if (g_calls.teleport) g_calls.teleport();
      break;
    case 0x97:
      message_panel_add(g_calls.soft_mip_toggle && g_calls.soft_mip_toggle() ? "MIP Mapping Enabled"
                                                                             : "MIP Mapping Disabled");
      break;
    case 0x98: {  // zModel's zone mode stepped 0, 1, 2 (gmod_cull.h)
      int32_t& mode = zmod::zones().mode;
      mode = (mode + 1) % 3;
      message_panel_add(mode == 1   ? "App Zone Switching Enabled"
                        : mode == 2 ? "Camera Zone Switching Enabled"
                                    : "Zone Switching Disabled");
      break;
    }
    case 0x9a:  // 0x0040e430; ours: again, the exit (docs/editor.md S4); only from MODS's missions
      editor_from_level();
      break;
    case 0x9d:
      if (g_calls.edit_vehicles) g_calls.edit_vehicles();
      break;
    case 0x9e:  // the constant is -0.2, subtracted
      if (g_calls.set_soft_mip_bias) g_calls.set_soft_mip_bias((g_calls.soft_mip_bias ? g_calls.soft_mip_bias() : 0.0f) - -0.2f);
      break;
    case 0x9f: {
      float bias = (g_calls.soft_mip_bias ? g_calls.soft_mip_bias() : 0.0f) - 0.2f;
      if (bias < 0.0f) bias = 0.0f;
      if (g_calls.set_soft_mip_bias) g_calls.set_soft_mip_bias(bias);
      break;
    }
    case 0xa3:
      if (g_calls.interp_console) g_calls.interp_console();
      break;
    case 0xa4:
      if (g_calls.mech_lab) g_calls.mech_lab();
      break;
    default:
      break;
  }
}

}  // namespace mech3
