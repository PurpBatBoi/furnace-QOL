/**
 * Furnace Tracker - Standard MIDI file importer
 * Copyright (C) 2026 Furnace Tracker contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "fileOpsCommon.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

namespace {

int midiImportPatternRows=64;
int midiImportRowResolution=1;
int midiImportNoteTranspose=48;
bool midiImportKeepOldPolyphonyNote=false;

struct MidiReadError {};

struct MidiReader {
  const unsigned char* data;
  size_t len, pos;

  MidiReader(const unsigned char* d, size_t l): data(d), len(l), pos(0) {}

  uint8_t read8() {
    if (pos>=len) throw MidiReadError();
    return data[pos++];
  }

  uint16_t read16() {
    return ((uint16_t)read8()<<8)|read8();
  }

  uint32_t read32() {
    return ((uint32_t)read8()<<24)|((uint32_t)read8()<<16)|((uint32_t)read8()<<8)|read8();
  }

  uint32_t readVarLen() {
    uint32_t value=0;
    for (int i=0; i<4; i++) {
      uint8_t byte=read8();
      value=(value<<7)|(byte&0x7f);
      if (!(byte&0x80)) return value;
    }
    throw MidiReadError();
  }

  void skip(size_t amount) {
    if (amount>len-pos) throw MidiReadError();
    pos+=amount;
  }

  bool tag(const char* text) {
    if (len-pos<4) throw MidiReadError();
    bool same=memcmp(data+pos,text,4)==0;
    pos+=4;
    return same;
  }
};

struct MidiNoteEvent {
  uint32_t tick;
  uint8_t channel, note, velocity;
  bool on;
};

struct MidiProgramEvent {
  uint32_t tick;
  uint8_t channel, program;
};

struct MidiTempoEvent {
  uint32_t tick, usecPerQuarter;
};

static const char* gmProgramName(unsigned char program) {
  static const char* names[128]={
    "Acoustic Grand Piano", "Bright Acoustic Piano", "Electric Grand Piano", "Honky-tonk Piano", "Electric Piano 1", "Electric Piano 2", "Harpsichord", "Clavinet",
    "Celesta", "Glockenspiel", "Music Box", "Vibraphone", "Marimba", "Xylophone", "Tubular Bells", "Dulcimer",
    "Drawbar Organ", "Percussive Organ", "Rock Organ", "Church Organ", "Reed Organ", "Accordion", "Harmonica", "Tango Accordion",
    "Acoustic Guitar (nylon)", "Acoustic Guitar (steel)", "Electric Guitar (jazz)", "Electric Guitar (clean)", "Electric Guitar (muted)", "Overdriven Guitar", "Distortion Guitar", "Guitar harmonics",
    "Acoustic Bass", "Electric Bass (finger)", "Electric Bass (pick)", "Fretless Bass", "Slap Bass 1", "Slap Bass 2", "Synth Bass 1", "Synth Bass 2",
    "Violin", "Viola", "Cello", "Contrabass", "Tremolo Strings", "Pizzicato Strings", "Orchestral Harp", "Timpani",
    "String Ensemble 1", "String Ensemble 2", "SynthStrings 1", "SynthStrings 2", "Choir Aahs", "Voice Oohs", "Synth Voice", "Orchestra Hit",
    "Trumpet", "Trombone", "Tuba", "Muted Trumpet", "French Horn", "Brass Section", "SynthBrass 1", "SynthBrass 2",
    "Soprano Sax", "Alto Sax", "Tenor Sax", "Baritone Sax", "Oboe", "English Horn", "Bassoon", "Clarinet",
    "Piccolo", "Flute", "Recorder", "Pan Flute", "Blown Bottle", "Shakuhachi", "Whistle", "Ocarina",
    "Lead 1 (square)", "Lead 2 (sawtooth)", "Lead 3 (calliope)", "Lead 4 (chiff)", "Lead 5 (charang)", "Lead 6 (voice)", "Lead 7 (fifths)", "Lead 8 (bass + lead)",
    "Pad 1 (new age)", "Pad 2 (warm)", "Pad 3 (polysynth)", "Pad 4 (choir)", "Pad 5 (bowed)", "Pad 6 (metallic)", "Pad 7 (halo)", "Pad 8 (sweep)",
    "FX 1 (rain)", "FX 2 (soundtrack)", "FX 3 (crystal)", "FX 4 (atmosphere)", "FX 5 (brightness)", "FX 6 (goblins)", "FX 7 (echoes)", "FX 8 (sci-fi)",
    "Sitar", "Banjo", "Shamisen", "Koto", "Kalimba", "Bag pipe", "Fiddle", "Shanai",
    "Tinkle Bell", "Agogo", "Steel Drums", "Woodblock", "Taiko Drum", "Melodic Tom", "Synth Drum", "Reverse Cymbal",
    "Guitar Fret Noise", "Breath Noise", "Seashore", "Bird Tweet", "Telephone Ring", "Helicopter", "Applause", "Gunshot"
  };
  return names[program];
}

}

void DivEngine::setMidiImportOptions(int patternRows, int rowResolution, int noteTranspose, bool keepOldPolyphonyNote) {
  midiImportPatternRows=MIN(DIV_MAX_ROWS,MAX(1,patternRows));
  if (rowResolution>=8) {
    midiImportRowResolution=8;
  } else if (rowResolution>=4) {
    midiImportRowResolution=4;
  } else if (rowResolution>=2) {
    midiImportRowResolution=2;
  } else {
    midiImportRowResolution=1;
  }
  midiImportNoteTranspose=MIN(179,MAX(-127,noteTranspose));
  midiImportKeepOldPolyphonyNote=keepOldPolyphonyNote;
}

bool DivEngine::loadMidi(unsigned char* file, size_t len) {
  bool success=false;
  warnings="";

  try {
    MidiReader reader(file,len);
    if (!reader.tag("MThd") || reader.read32()<6) throw MidiReadError();
    uint16_t format=reader.read16();
    uint16_t trackCount=reader.read16();
    uint16_t division=reader.read16();
    if (format>1 || !trackCount || (division&0x8000)) {
      lastError="unsupported MIDI format or timing division";
      return false;
    }
    // Header chunks may contain extension data; consume it before the first track.
    // The six required bytes above have already been read.
    // Re-read the header size from the file to avoid accepting a malformed short header.
    uint32_t headerLen=((uint32_t)file[4]<<24)|((uint32_t)file[5]<<16)|((uint32_t)file[6]<<8)|file[7];
    reader.skip(headerLen-6);

    std::vector<MidiNoteEvent> notes;
    std::vector<MidiProgramEvent> programs;
    std::vector<MidiTempoEvent> tempos;
    uint32_t duration=0;

    for (unsigned int track=0; track<trackCount; track++) {
      if (!reader.tag("MTrk")) throw MidiReadError();
      uint32_t trackLen=reader.read32();
      if (trackLen>reader.len-reader.pos) throw MidiReadError();
      size_t trackEnd=reader.pos+trackLen;
      uint32_t tick=0;
      uint8_t status=0;

      while (reader.pos<trackEnd) {
        tick+=reader.readVarLen();
        if (reader.pos>=trackEnd) throw MidiReadError();
        uint8_t first=reader.read8();
        if (first&0x80) status=first;
        else {
          if (status<0x80 || status>=0xf0) throw MidiReadError();
          reader.pos--;
        }

        if (status==0xff) {
          uint8_t type=reader.read8();
          uint32_t metaLen=reader.readVarLen();
          if (metaLen>trackEnd-reader.pos) throw MidiReadError();
          if (type==0x51 && metaLen==3) {
            uint32_t tempo=((uint32_t)reader.read8()<<16)|((uint32_t)reader.read8()<<8)|reader.read8();
            if (tempo) tempos.push_back({tick,tempo});
          } else {
            reader.skip(metaLen);
          }
          if (type==0x2f) duration=MAX(duration,tick);
          status=0;
        } else if (status==0xf0 || status==0xf7) {
          uint32_t sysexLen=reader.readVarLen();
          if (sysexLen>trackEnd-reader.pos) throw MidiReadError();
          reader.skip(sysexLen);
          status=0;
        } else if (status<0xf0) {
          uint8_t type=status>>4;
          uint8_t channel=status&15;
          uint8_t data1=reader.read8();
          if (data1&0x80) throw MidiReadError();
          uint8_t data2=0;
          if (type!=0x0c && type!=0x0d) {
            data2=reader.read8();
            if (data2&0x80) throw MidiReadError();
          }
          if (type==0x08 || type==0x09) {
            notes.push_back({tick,channel,data1,data2,type==0x09 && data2!=0});
          } else if (type==0x0c) {
            programs.push_back({tick,channel,data1});
          }
        } else {
          // System common and real-time events do not carry song data. Their
          // exact byte count is defined by their status byte.
          switch (status) {
            case 0xf1: reader.skip(1); break;
            case 0xf2: reader.skip(2); break;
            case 0xf3: reader.skip(1); break;
            case 0xf6: case 0xf8: case 0xf9: case 0xfa: case 0xfb: case 0xfc: case 0xfd: case 0xfe: break;
            default: throw MidiReadError();
          }
          status=0;
        }
      }
      if (reader.pos!=trackEnd) throw MidiReadError();
      duration=MAX(duration,tick);
    }

    std::sort(notes.begin(),notes.end(),[](const MidiNoteEvent& a, const MidiNoteEvent& b) {
      return a.tick==b.tick ? a.on<b.on : a.tick<b.tick;
    });
    std::sort(programs.begin(),programs.end(),[](const MidiProgramEvent& a, const MidiProgramEvent& b) { return a.tick<b.tick; });
    std::sort(tempos.begin(),tempos.end(),[](const MidiTempoEvent& a, const MidiTempoEvent& b) { return a.tick<b.tick; });

    std::array<int,16> sourceToDest;
    sourceToDest.fill(-1);
    int channelCount=0;
    for (const MidiNoteEvent& event: notes) {
      if (event.on && sourceToDest[event.channel]<0) sourceToDest[event.channel]=channelCount++;
    }
    if (!channelCount) channelCount=1;

    // One tracker row is a sixteenth note. This matches FamiStudio's default
    // beat subdivision while keeping imported material readable in the pattern editor.
    const uint32_t rowTicks=MAX(1,(uint32_t)division/(4*midiImportRowResolution));
    uint32_t totalRows=(duration+rowTicks-1)/rowTicks;
    if (!totalRows) totalRows=1;
    uint32_t orderCount=(totalRows+midiImportPatternRows-1)/midiImportPatternRows;
    if (orderCount>DIV_MAX_PATTERNS) {
      orderCount=DIV_MAX_PATTERNS;
      warnings+="MIDI song is longer than 256 patterns; trailing data was discarded.\n";
    }

    uint32_t initialTempo=500000;
    for (const MidiTempoEvent& tempo: tempos) {
      if (tempo.tick>0) break;
      initialTempo=tempo.usecPerQuarter;
    }

    DivSong ds;
    ds.version=DIV_ENGINE_VERSION;
    ds.systemLen=1;
    ds.system[0]=DIV_SYSTEM_PCM_DAC;
    ds.systemChans[0]=channelCount;
    ds.subsong[0]->patLen=midiImportPatternRows;
    ds.subsong[0]->ordersLen=orderCount;
    ds.subsong[0]->speeds.len=1;
    ds.subsong[0]->speeds.val[0]=6;
    ds.subsong[0]->hz=MIN(999.0,MAX(1.0,(24000000.0*midiImportRowResolution)/(double)initialTempo));
    ds.name="MIDI Import";
    for (int channel=0; channel<channelCount; channel++) {
      ds.subsong[0]->chanName[channel]=fmt::sprintf("MIDI Channel %d",channel+1);
      ds.subsong[0]->chanShortName[channel]=fmt::sprintf("M%d",channel+1);
      ds.subsong[0]->pat[channel].effectCols=1;
      for (unsigned int order=0; order<orderCount; order++) ds.subsong[0]->orders.ord[channel][order]=order;
    }

    bool usedProgram[128]={false};
    usedProgram[0]=true;
    for (const MidiProgramEvent& event: programs) usedProgram[event.program]=true;
    int programToInstrument[128];
    memset(programToInstrument,-1,sizeof(programToInstrument));
    for (int program=0; program<128; program++) if (usedProgram[program]) {
      DivInstrument* ins=new DivInstrument;
      ins->type=DIV_INS_AMIGA;
      ins->amiga.initSample=-1;
      ins->name=gmProgramName(program);
      programToInstrument[program]=ds.ins.size();
      ds.ins.push_back(ins);
    }
    ds.insLen=ds.ins.size();

    std::array<int,16> currentProgram;
    std::array<int,16> activeNote;
    currentProgram.fill(0);
    activeNote.fill(-1);
    size_t programIndex=0;
    for (const MidiNoteEvent& event: notes) {
      while (programIndex<programs.size() && programs[programIndex].tick<=event.tick) {
        currentProgram[programs[programIndex].channel]=programs[programIndex].program;
        programIndex++;
      }
      int channel=sourceToDest[event.channel];
      if (channel<0) continue;
      uint32_t row=event.tick/rowTicks;
      if (row>=orderCount*(unsigned int)midiImportPatternRows) continue;
      DivPattern* pattern=ds.subsong[0]->pat[channel].getPattern(row/midiImportPatternRows,true);
      short* cell=pattern->newData[row%midiImportPatternRows];
      if (event.on) {
        if (activeNote[event.channel]>=0) {
          warnings+=fmt::sprintf("Polyphony on MIDI channel %d at tick %u; %s.\n",event.channel+1,event.tick,midiImportKeepOldPolyphonyNote?"keeping the currently playing note":"using the newest note");
          if (midiImportKeepOldPolyphonyNote) continue;
        }
        activeNote[event.channel]=event.note;
        int note=event.note+midiImportNoteTranspose;
        if (note<0 || note>179) {
          warnings+=fmt::sprintf("MIDI note %d at tick %u is outside Furnace's note range after transposition; ignoring.\n",event.note,event.tick);
          activeNote[event.channel]=-1;
          continue;
        }
        cell[DIV_PAT_NOTE]=note;
        cell[DIV_PAT_INS]=programToInstrument[currentProgram[event.channel]];
        cell[DIV_PAT_VOL]=(event.velocity*255+63)/127;
      } else if (activeNote[event.channel]==event.note) {
        activeNote[event.channel]=-1;
        cell[DIV_PAT_NOTE]=DIV_NOTE_OFF;
      }
    }

    // FamiStudio applies tempo changes on a pattern boundary. Do the same here,
    // using Furnace's Cxxx tick-rate command on the first channel.
    size_t tempoIndex=0;
    uint32_t activeTempo=500000;
    for (unsigned int order=0; order<orderCount; order++) {
      uint32_t orderTick=order*midiImportPatternRows*rowTicks;
      while (tempoIndex<tempos.size() && tempos[tempoIndex].tick<=orderTick) {
        activeTempo=tempos[tempoIndex++].usecPerQuarter;
      }
      if (order && activeTempo) {
        int hz=MIN(1023,MAX(1,(int)(((24000000.0*midiImportRowResolution/(double)activeTempo)+0.5))));
        DivPattern* pattern=ds.subsong[0]->pat[0].getPattern(order,true);
        pattern->newData[0][DIV_PAT_FX(0)]=0xc0|(hz>>8);
        pattern->newData[0][DIV_PAT_FXVAL(0)]=hz&255;
      }
    }

    ds.initDefaultSystemChans();
    // Generic PCM DAC has a dynamic channel count, so restore the imported value.
    ds.systemChans[0]=channelCount;
    ds.recalcChans();
    ds.findSubSongs();

    if (active) quitDispatch();
    BUSY_BEGIN_SOFT;
    saveLock.lock();
    song.unload();
    song=ds;
    hasLoadedSomething=true;
    changeSong(0);
    saveLock.unlock();
    BUSY_END;
    if (active) {
      initDispatch();
      BUSY_BEGIN;
      renderSamples();
      reset();
      BUSY_END;
    }
    success=true;
  } catch (MidiReadError&) {
    lastError="invalid or unsupported MIDI file";
  }
  return success;
}
