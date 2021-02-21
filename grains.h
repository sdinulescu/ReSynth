/* grains.h written by Stejara Dinulescu
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

const int SAMPLE_RATE = 48000; 
const int BLOCK_SIZE = 2048; 
const int OUTPUT_CHANNELS = 2;

const int MAX_GRAINS = 1000;

// Line struct taken and adapted from Karl Yerkes' synths.h sample code
struct Line {
  float value = 0, target = 0, seconds = 1 / SAMPLE_RATE, increment = 0;

  void set() {
    if (seconds <= 0) seconds = 1 / SAMPLE_RATE;
    // slope per sample
    increment = (target - value) / (seconds * SAMPLE_RATE);
  }
  void set(float v, float t, float s) {
    value = v;
    target = t;
    seconds = s;
    set();
  }
  void set(float t, float s) {
    target = t;
    seconds = s;
    set();
  }
  void set(float t) {
    target = t;
    set();
  }

  bool done() { return value == target; }

  float operator()() {
    if (value != target) {
      value += increment;
      if ((increment < 0) ? (value < target) : (value > target)) value = target;
    }
    return value;
  }
};

struct Grain {
  std::vector<float> clip; 
  int clipIndex = 0; // if grain is on

  int duration = 0; // how many samples does it live -> length of clip vector
  bool active = false; // whether or not to sound

  // FM SYNTHESIS PARAMS
  Line carrier; 
  Line modulator; 
  Line moddepth; 

  Grain(float m, float sd, float mod_index) { 
    synthesize(m, sd, mod_index); // synthesize clip of sound on init
  } 

  void synthesize(float mean, float stdv, float mod_depth) { // REWORK THIS TO FM SYNTH
    float d = al::rnd::uniform(0, 5); // duration is randomly set on grain generation, consistent for carrier, modulator, and moddepth
    // first, set grain params based on mean and stddev (positive or negative value) of distribution
    carrier.set(al::rnd::uniform(mean + stdv), al::rnd::uniform(mean + stdv), d); // set start freq, target freq, duration in seconds
    modulator.set(al::rnd::uniform(mean + stdv), al::rnd::uniform(mean + stdv), d); // set start freq, target freq, duration in seconds
    moddepth.set(al::rnd::uniform(mod_depth + stdv), al::rnd::uniform(mod_depth + stdv), d); // set start freq, target freq, duration in seconds
    // moddepth NEEDS to be less than mean I think. Consider using exposing mod_depth as a slider to control this? 

    // given a mean and a standard deviation, synthesize a buffer of float values. This was just randomly generated noise for initial test.
    // for (int i = 0; i < duration; i++) {
    //   float sample_value = al::rnd::uniform(mean - stdv, mean + stdv); // generate a sample value between mean +- stdv
    //   clip.push_back(sample_value);
    // }
  }

  //float getSample(int index) { if (index >= 0 && index < duration) { return clip[index]; } else { return 0.0; } }
  float calculateSample() {
    float sampleValue = 0.0;
    // if the grain is turned on to sound, return the audio sample value. otherwise it returns 0. 
    if (active && clipIndex < duration) { sampleValue = clip[clipIndex]; clipIndex++; }
    checkDeath(); // turn off if the grain is supposed to die this sample
    return sampleValue;
  }

  void turnOn() { active = true; clipIndex = 0; }
  void turnOff() { active = false; clipIndex = 0; }
  void checkDeath() { if (clipIndex >= duration) { turnOff(); } }
};

struct Granulator {
  // GUI accessible parameters
  al::ParameterInt nGrains{"/number of grains", "", 100, "", 0, MAX_GRAINS}; // user input for active grains
  al::Parameter mean{"/mean", "", 0.5, "", 0.0, 1.0}; // user input for mean value of granulator
  al::Parameter stdv{"/standard deviation", "", 0.3, "", 0.0, 1.0}; // user input for standard deviation value of granulator
  al::Parameter modulation_depth{"/modulation depth", "", 0.3, "", 0.0, 1.0}; // user input for standard deviation value of granulator

  int activeGrains = 0; // keep track of active grains, turn on and off based on nGrains
  std::vector<Grain> grains; //stores 1000 grains on init, activeGrains specifies number of active grains

  // creates and stores 1000 from the get go. activates nGrains out of the 1000. 
  Granulator() { synthesize(); }
  
  void synthesize() {
    for (int i = 0; i < MAX_GRAINS; i++) {
      Grain g(mean, stdv, modulation_depth);
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

