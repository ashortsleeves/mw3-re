#pragma once
#include <cstdint>

// The app's own keys (between briefing.cpp and mech.cpp, after the options;
// the file is not known): the commands whose keys reach the app, not the
// mech (docs/keybindings.md).
namespace mech3 {

// Where the app's keys go; null calls are skipped.
struct AppKeyCalls {
  void (*cycle_camera)() = nullptr;    // 0x0040aed0 on the player's view [0x005f4c8c]
  void (*vmode_cycle)() = nullptr;     // 0x004ab910: the next screen mode, saved as InGameVMode
  void (*message_toggle)() = nullptr;  // 0x0040f070: the message panel
  void (*debug_7e)() = nullptr;        // 0x004abbc0 (command 0x7e, which no key reaches)
  // Ours: --debug-camera / debugcamera=1: Shift+F12 to command 0x9a, the
  // game editor's (docs/editor.md S4; the exe binds no key to it).
  bool debug_camera = false;
  // The debug commands' targets (0x6d, 0x82..0xa4: no key or file binds them in the release; keybindings.md).
  void (*mech_debug_bit)() = nullptr;           // 0x6d: 0x0041ca90 on the pilot's mech ([0x0060861c] +0x14)
  int32_t (*mech_list_dialog)() = nullptr;      // 0x84: 0x00432c60 on its roster (an MFC list; -1 none)
  void (*mech_remake)(int32_t roster) = nullptr;  // 0x84: 0x004783c0 on the pilot
  void (*next_view_mech)() = nullptr;           // 0x89: 0x004a33f0
  void (*vehicle_dialog)() = nullptr;           // 0x8a: 0x004a3550 (an MFC list)
  void (*camera_mode)(int32_t id) = nullptr;    // 0x8b, 0x8c, 0x8e: 0x0040ae70 on the view [0x005f4c8c]
  void (*camera_dialog)() = nullptr;            // 0x8f: 0x0040bb00 on the view's camera (+0x58; MFC)
  int32_t (*show_cursor)(int32_t show) = nullptr;  // 0x93: ShowCursor
  void (*marker)() = nullptr;                   // 0x94: 0x004a3220
  void (*teleport)() = nullptr;                 // 0x95: 0x004a37a0
  int32_t (*soft_mip_toggle)() = nullptr;       // 0x97: 0x00557f70 (the software renderer's): on after
  void (*edit_vehicles)() = nullptr;            // 0x9d: 0x004b86e0
  float (*soft_mip_bias)() = nullptr;           // 0x9e, 0x9f: 0x00535ef0 (the software renderer's)
  void (*set_soft_mip_bias)(float bias) = nullptr;  // 0x00535ed0
  void (*interp_console)() = nullptr;           // 0xa3: 0x00480b20 -> 0x004810d0 on the app
  void (*mech_lab)() = nullptr;                 // 0xa4: 0x00448830 (the pilot, its +0x6c's +8, 0)
};
// The debug commands' own state.
struct AppKeyState {
  float max_rate = 8.0f;  // [0x005e0c98] the clamp's lowest frame rate, 0x87 and 0x88 five apart
  int32_t clamp = 1;      // [0x005e0c9c] 0x86
  int32_t cursor = 1;     // [0x005e0ca0] 0x93: the last ShowCursor's count was below 0
  int32_t dev_text = 0;   // [0x005f4d6c] 0x82: the frame rate and the developer lines' layer
  int32_t readout = 0;    // [0x005f4d70] 0x83
};
AppKeyState& app_key_state();
AppKeyCalls& app_key_calls();

// Mech3.exe 1.2 @ 0x0040db60 (from the mission's load): CYCLE_CAM (0x61),
// MESSAGE_TOGGLE (0x70) and VMODE_TOGGLE (0x6c) to 0x0040dba0, 0x7e to 0x004abbc0.
void register_app_keys();

// Mech3.exe 1.2 @ 0x0040dba0 (ECX the command; Ghidra made no function of it): the commands bound to it and
// the debug commands no key or file binds (0x6d, 0x82..0xa4: ported for preservation; keybindings.md).
void app_key(int32_t cmd);

}  // namespace mech3
