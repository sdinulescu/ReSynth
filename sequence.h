/* sequence.h written by Stejara Dinulescu
 * MAT240B 2021, Final Project
 * This file defines the grain stuct and granulator struct used in granular-resynth.cpp
 * References: granulator-source-material.cpp written by Karl Yerkes
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

struct Sequencer{
  al::ParameterInt steps{"/sequencer steps", "", 4, "", 0, 10}; // user input for number of "beats" in the sequencer (can think of sequencer as a measure)
  int psteps = steps;
  al::Parameter rate{"/sequencer rate", "", 1.0, "", 0.1, 10.0}; // user input for mean frequency value of granulator, in Hz
  float prate = rate;
  
  gam::Accum<> timer; // rate timer
  int playhead = 0; // where we are in the sequencer

  std::vector<Grain> sequence;

  Sequencer() { init(); }

  void init() {
    timer.freq(rate);
  }

  void set(Grain g) {
    sequence[playhead] = g;
  }

  void updateParameters() {
    //if (psteps != steps) { init(); psteps = steps; }
    if (prate != rate) { timer.freq(rate); prate = rate;}
  }

};