/* grains.h written by Stejara Dinulescu
 * MAT240B 2021, Final Project
 * This file defines the grain stuct and granulator struct
 */

# pragma once

#include <iostream>
#include <string>
#include <vector>
#include "al/ui/al_Parameter.hpp"
#include "al/math/al_Random.hpp"  // rnd::uniform()

const int SAMPLE_RATE = 48000; 
const int BLOCK_SIZE = 2048; 
const int OUTPUT_CHANNELS = 2;

const int MAX_GRAINS = 1000;

struct Grain {
  std::vector<float> clip; 
  int duration = al::rnd::uniform() * SAMPLE_RATE; // how many frames does it live -> length of clip vector
  bool active = false; // whether or not to sound

  Grain(float m, float sd) { synthesize(m, sd); } // synthesize clip of sound on init

  void synthesize(float mean, float stdv) {
    //given a mean and a standard deviation, synthesize a buffer of float values
    for (int i = 0; i < duration; i++) {
      float sample_value = al::rnd::uniform(mean - stdv, mean + stdv); // generate a sample value between mean +- stdv
      clip.push_back(sample_value);
    }
  }

  float getSample(int index) { if (index >= 0 && index < clip.size()) { return clip[index]; } else { return 0.0; } }

  void turnOn() { active = true; }
  void turnOff() { active = false; }
};

struct Granulator {
  // GUI accessible parameters
  al::ParameterInt nGrains{"/number of grains", "", 100, "", 0, MAX_GRAINS}; // user input for active grains
  al::Parameter mean{"/mean", "", 0.5, "", 0.0, 1.0}; // user input for mean value of granulator
  al::Parameter stdv{"/standard deviation", "", 0.3, "", 0.0, 1.0}; // user input for standard deviation value of granulator

  int activeGrains = 0; // keep track of active grains, turn on and off based on nGrains
  std::vector<Grain> grains; //stores 1000 grains on init, activeGrains specifies number of active grains

  // creates and stores 1000 from the get go. activates nGrains out of the 1000. 
  Granulator() { synthesize(); }
  
  void synthesize() {
    for (int i = 0; i < MAX_GRAINS; i++) {
      Grain g(mean, stdv);
      if (i < nGrains) {
        g.turnOn(); // turn on grain if its index is from 0 to nGrains
      }
      grains.push_back(g);
    }
    activeGrains = nGrains;
  }

  void updateActiveGrains() { // turn grains on/off if need be
    if (nGrains < activeGrains) { // if new number is lower than previous number of active grains
      for (int i = 0; i < activeGrains - nGrains; i++) {
        grains[nGrains + i].turnOff(); // turn off the ones @ index > nGrains
      }
    } else if (nGrains > activeGrains) { // if new number is higher than previous number of active grains
      for (int i = 0; i < nGrains - activeGrains; i++) {
        grains[activeGrains + i].turnOn(); // turn on the ones @ index > active Grains
      }
    }
    activeGrains = nGrains; // update number of active grains 
  }

  void updateGranulatorParams() {
    if (nGrains.hasChange()) { updateActiveGrains(); }
    if (mean.hasChange() || stdv.hasChange()) { 
      grains.erase(grains.begin());
      synthesize();
    }
  }
};

