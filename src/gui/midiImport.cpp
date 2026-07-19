/**
 * Furnace Tracker - MIDI import options
 * Copyright (C) 2026 Furnace Tracker contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "gui.h"

void FurnaceGUI::drawMidiImport() {
  ImGui::TextWrapped("%s",_("Import into Generic PCM DAC channels. Instruments are created blank and named after their General MIDI programs."));
  ImGui::Separator();

  ImGui::TextUnformatted(_("Rows per pattern:"));
  ImGui::SetNextItemWidth(120.0f*dpiScale);
  ImGui::InputInt("##MidiPatternRows",&midiImportPatternRows,1,16);
  midiImportPatternRows=MIN(DIV_MAX_ROWS,MAX(1,midiImportPatternRows));
  ImGui::SameLine();
  ImGui::TextDisabled("%s",_("(1 to 256)"));

  ImGui::TextUnformatted(_("Timing resolution:"));
  ImGui::RadioButton(_("Original (16th-note rows)"),&midiImportRowResolution,1);
  ImGui::RadioButton(_("Double (twice as many rows)"),&midiImportRowResolution,2);
  ImGui::RadioButton(_("4x (four times as many rows)"),&midiImportRowResolution,4);
  ImGui::RadioButton(_("8x (eight times as many rows)"),&midiImportRowResolution,8);
  ImGui::TextWrapped("%s",_("Higher values keep the MIDI playback speed but add more editable rows between notes. 4x uses 64th-note rows; 8x uses 128th-note rows."));

  ImGui::Separator();
  ImGui::TextUnformatted(_("Note transpose:"));
  ImGui::SetNextItemWidth(120.0f*dpiScale);
  ImGui::InputInt("##MidiNoteTranspose",&midiImportNoteTranspose,1,12);
  midiImportNoteTranspose=MIN(179,MAX(-127,midiImportNoteTranspose));
  ImGui::SameLine();
  ImGui::TextDisabled("%s",_("semitones"));
  ImGui::TextWrapped("%s",_("Default +48 maps MIDI C4 to Furnace C4. Use 0 to preserve the raw MIDI note numbers."));

  ImGui::Separator();
  ImGui::TextUnformatted(_("Overlapping notes:"));
  if (ImGui::RadioButton(_("Favor newest note"),!midiImportKeepOldPolyphonyNote)) {
    midiImportKeepOldPolyphonyNote=false;
  }
  if (ImGui::RadioButton(_("Favor currently playing note"),midiImportKeepOldPolyphonyNote)) {
    midiImportKeepOldPolyphonyNote=true;
  }
  ImGui::TextWrapped("%s",_("Generic PCM DAC channels are monophonic. This controls what happens when a MIDI channel starts another note before its current one ends."));

  ImGui::Separator();
  if (ImGui::Button(_("Import"))) {
    e->setMidiImportOptions(midiImportPatternRows,midiImportRowResolution,midiImportNoteTranspose,midiImportKeepOldPolyphonyNote);
    String path=midiImportPath;
    midiImportPath="";
    midiImportConfirmed=true;
    ImGui::CloseCurrentPopup();
    if (load(path)>0) {
      showError(fmt::sprintf(_("Error while loading file! (%s)"),lastError));
    }
  }
  ImGui::SameLine();
  if (ImGui::Button(_("Cancel"))) {
    midiImportPath="";
    ImGui::CloseCurrentPopup();
  }
}
