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
#include "Gamma/Oscillator.h"
#include "al/math/al_Functions.hpp"  // al::clip


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

struct ExpSeg {
  Line line;
  void set(double v, double t, double s) { line.set(log2(v), log2(t), s); }
  void set(double t, double s) { line.set(log2(t), s); }
  void set(double t) { line.set(log2(t)); }
  float operator()() { return pow(2.0f, line()); }
};

struct Grain {
  int duration = 0; // how many samples does it live -> length of clip vector
  bool active = false; // whether or not to sound

  // FM SYNTHESIS PARAMS
  
  gam::Sine<float> carrier; 
  gam::Sine<float> modulator; 
  Line moddepth; 
  ExpSeg alpha;
  ExpSeg beta;

  Grain() { } // empty constructor

  void synthesize(float cmean, float cstdv, float mmean, float mstdv, float mod_depth) {
    float d = al::rnd::uniform(1, 3); // duration

    float carrier_start = al::clip( ((double)al::rnd::normal() / cstdv) + (double)cmean, 4000.0, 0.0);
    float carrier_end = al::clip( ((double)al::rnd::normal() / cstdv) + (double)cmean, 4000.0, 0.0);
    float mod_start = al::clip((double)al::rnd::normal() / mstdv + (double)mmean, 4000.0, 0.0);
    float mod_end = al::clip((double)al::rnd::normal() / mstdv + (double)mmean, 4000.0, 0.0);

    alpha.set(carrier_start, carrier_end, d);
    beta.set(mod_start, mod_end, d);
    carrier.freq(0);
    modulator.freq(0);

    moddepth.set(al::clip((double)al::rnd::normal() + (double)mod_depth, 500.0, 0.0), 
                 al::clip((double)al::rnd::normal() + (double)mod_depth, 500.0, 0.0), d); // set start freq, target freq, duration in seconds
  }

  //float getSample(int index) { if (index >= 0 && index < duration) { return clip[index]; } else { return 0.0; } }
  float calculateSample() {
    modulator.freq(beta());
    carrier.freq(alpha() + moddepth() * modulator());

    float sampleValue = 0.0;
    //sampleValue = carrier(); // envelope * carrier here

    // if the grain is turned on to sound, return the audio sample value. otherwise it returns 0. 
    //if (active) std::cout<< "active" << std::endl;
    if (active) { // freq modulation. sampleValue = carrier() * envelope() once envelope is implemented.
      sampleValue = carrier(); 
    }
    checkDeath(); // turn off if the grain is supposed to die this sample
    return sampleValue;
  }

  void turnOn() { active = true; }
  void turnOff() { active = false; }
  void checkDeath() { if (alpha.line.done() || beta.line.done()) { turnOff(); } }
};

struct Granulator {
  // GUI accessible parameters
  al::ParameterInt nGrains{"/number of grains", "", 100, "", 0, MAX_GRAINS}; // user input for active grains
  al::Parameter carrier_mean{"/carrier mean", "", 440.0, "", 0.0, 4000.0}; // user input for mean frequency value of granulator, in Hz
  float p_cmean = carrier_mean;
  al::Parameter carrier_stdv{"/carrier standard deviation", "", 0.2, "", 0.1, 1.0}; // user input for standard deviation frequency value of granulator
  float p_cstdv = carrier_stdv;

   al::Parameter modulator_mean{"/modulator mean", "", 800.0, "", 0.0, 4000.0}; // user input for mean frequency value of granulator, in Hz
  float p_mmean = modulator_mean;
  al::Parameter modulator_stdv{"/modulator standard deviation", "", 0.2, "", 0.1, 1.0}; // user input for standard deviation frequency value of granulator
  float p_mstdv = modulator_stdv;
  al::Parameter modulation_depth{"/modulation depth", "", 100.0, "", 0.0, 300.0}; // user input for standard deviation value of granulator

  int activeGrains = 0; // keep track of active grains, turn on and off based on nGrains
  std::vector<Grain> grains; //stores 1000 grains on init, activeGrains specifies number of active grains

  // creates and stores 1000 from the get go. activates nGrains out of the 1000. 
  Granulator() { synthesize(); }
  
  void synthesize() {
    for (int i = 0; i < MAX_GRAINS; i++) {
      Grain g;
      g.synthesize(carrier_mean, carrier_stdv, modulator_mean, modulator_stdv, modulation_depth);
      if (i < nGrains) {
        //std::cout << "turn on" << std::endl;
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
    if (nGrains != activeGrains) {updateActiveGrains();}
    if (p_cmean != carrier_mean || p_cstdv != carrier_stdv || p_mmean != modulator_mean || p_mstdv != modulator_mean) { 
      for (int i = 0; i < grains.size(); i++) { grains[i].synthesize(carrier_mean, carrier_stdv, modulator_mean, modulator_stdv, modulation_depth);} // turn off all grains, resynth
      p_cmean = carrier_mean;
      p_cstdv = carrier_stdv;
      p_mmean = modulator_mean;
      p_mstdv = modulator_stdv;
    }
  }
};

