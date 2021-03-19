/* sequence.h written by Stejara Dinulescu
 * MAT240B 2021, Final Project
 * This file defines the Sequencer stuct used in granular-resynth.cpp
 * References: granulator-source-material.cpp, frequency-modulation-grains.cpp, Scatter-Sequence.cpp written by Karl Yerkes
 */

# pragma once

#include <iostream>
#include <string>
#include <vector>
#include "al/ui/al_Parameter.hpp"
#include "al/math/al_Random.hpp"  // rnd::uniform()
#include "Gamma/Oscillator.h"
#include "al/math/al_Functions.hpp"  // al::clip
#include "grains.h"

const int NUM_SEQUENCERS = 3;
int count_id = 0; // keeps track of how many sequencers have already been added to give the sequencer a unique id (and parameter name)

struct Sequencer {
  int id = count_id; 
  al::Parameter rate{"/sequencer rate " + std::to_string(id), "", (float)al::rnd::uniform(0.1, 1.0), "", -30.0, 50.0};  // user input for rate of sequencer
  gam::Accum<> timer; // rate timer
  int playhead = 0; // where we are in the sequencer
  std::vector<GrainSettings> sequence; // stores the grain settings
  bool active = false; // whether the sequencer is active for adding/removing grains
  al::Vec3f color; // each sequencer has a unique color

  Sequencer() { 
    timer.freq(rate); // set the timer
    if (id == 0) {color = al::Vec3f(0, 0, 1);} // blue
    else if (id == 1) {color = al::Vec3f(0, 1, 0);} // green 
    else if (id == 2) {color = al::Vec3f(1, 1, 0);} // yellow
    count_id += 1; 
  }

  void enable() { active = true; }
  void disable() { active = false; }

  void setTimer() {  if (timer.freq() != rate) timer.freq(rate); } // set the timer's frequency to the specified rate, also acts as a reset if rate changes
  
  GrainSettings grabSample() { return sequence[playhead]; }
  void addSample(GrainSettings g) { sequence.push_back(g); printSamples(); }

  void increment() { 
    playhead++; // increment playhead
    if (playhead >= sequence.size()) { // reset if need be
      playhead -= sequence.size();
      if (playhead < 0) { playhead = 0; } // bounds check
    }
  }

  bool checkIntersection(al::Vec3f pos) { // for when using the mouse to select grains
    bool found = false;
    for (auto iterator = sequence.begin(); iterator != sequence.end();) {
      if (iterator->position == pos) {
        found = true;
        sequence.erase(iterator); // erases all the copies in the sequence
      } else {
        ++iterator;  // only advance the iterator when we don't erase
      }
    }
    return found;
  }

  // debugging purposes
  void sayName() {std::cout << "i am sequence " << id << " " << active << std::endl;} 
  void printSamples() {
    for (int i = 0; i < sequence.size(); i++) {
      std::cout << sequence[i].position << " ";
    }
    std::cout << std::endl;
  }
};