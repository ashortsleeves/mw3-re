#pragma once
#include <cstdint>
#include <vector>

// The debug commands' own pieces (app_key's 0x89 and 0x94; keybindings.md). Original file unknown: between player.cpp
// and turret.cpp, beside the teleport 0x004a37a0 (teleport.h). No key reaches them in the release; ported for
// preservation.
namespace mech3 {

// Mech3.exe 1.2 @ 0x004a3220 (command 0x94): "Marker # n" and the place to stdout, n from 0 ([0x00609a24]).
void debug_marker(const float pos[3]);
int32_t& debug_marker_count();  // [0x00609a24]

// Mech3.exe 1.2 @ 0x004a33f0 (command 0x89): the developer lines emptied; then, in zEntity's list ([0x0073655c]),
// the mech after `target`, wrapping to the list's first mech (the first too when `target` is not after it). `mech_of`
// is an entity's vt+0x2c, then that one's vt+0x40 (null: none). The caller runs the answer's vt+0x108 (the view
// onto it); none when it is `target`.
void* next_view_mech(const std::vector<void*>& entities, void* target, void* (*mech_of)(void*));

}  // namespace mech3
