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

struct ADSR {
  Line attack, decay, release;
  int state = 0;

  void set(float a, float d, float s, float r) {
    attack.set(0, 1, a);
    decay.set(1, s, d);
    release.set(s, 0, r);
  }

  void on() {
    attack.value = 0;
    decay.value = 1;
    state = 1;
  }

  void off() {
    release.value = decay.target;
    state = 3;
  }

  float operator()() {
    switch (state) {
      default:
      case 0:
        return 0;
      case 1:
        if (!attack.done()) return attack();
        if (!decay.done()) return decay();
        state = 2;
      case 2:  // sustaining...
        return decay.target;
      case 3:
        return release();
    }
  }
  void print() {
    printf("  state:%d\n", state);
    printf("  attack:%f\n", attack.seconds);
    printf("  decay:%f\n", decay.seconds);
    printf("  sustain:%f\n", decay.target);
    printf("  release:%f\n", release.seconds);
  }
};

struct AttackDecay {
  Line attack, decay;

  void set(float riseTime, float fallTime, float peakValue) {
    attack.set(0, peakValue, riseTime);
    decay.set(peakValue, 0, fallTime);
  }

  float operator()() {
    if (!attack.done()) return attack();
    return decay();
  }
};

struct Grain : al::SynthVoice {
  gam::Sine<float> carrier; 
  gam::Sine<float> modulator; 
  Line moddepth; 
  ExpSeg alpha;
  ExpSeg beta;
  AttackDecay envelope;

  Grain() { } // empty constructor

  void synthesize(float cmean, float cstdv, float mmean, float mstdv, float mod_depth, float env) {
    float d = al::rnd::uniform(1, 3); // duration
    duration = d;

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

    envelope.set(env * d, (1 - env) * d, 0.8);
  }

  void onProcess(al::AudioIOData& io) override {
    while (io()) {
      modulator.freq(beta());
      carrier.freq(alpha() + moddepth() * modulator());
      
      float v = envelope() * carrier();
      io.out(0) += v;
      io.out(1) += v;

      if (alpha.line.done() || beta.line.done() || envelope.decay.done()) {
        free();
        break;
      }
    }
  }
};

struct Granulator {
  // GUI accessible parameters
  al::ParameterInt nGrains{"/number of grains", "", 5, "", 0, MAX_GRAINS}; // user input for active grains
  int activeGrains = nGrains;
  al::Parameter carrier_mean{"/carrier mean", "", 440.0, "", 0.0, 4000.0}; // user input for mean frequency value of granulator, in Hz
  float p_cmean = carrier_mean;
  al::Parameter carrier_stdv{"/carrier standard deviation", "", 0.2, "", 0.1, 1.0}; // user input for standard deviation frequency value of granulator
  float p_cstdv = carrier_stdv;

  al::Parameter modulator_mean{"/modulator mean", "", 800.0, "", 0.0, 4000.0}; // user input for mean frequency value of granulator, in Hz
  float p_mmean = modulator_mean;
  al::Parameter modulator_stdv{"/modulator standard deviation", "", 0.2, "", 0.1, 1.0}; // user input for standard deviation frequency value of granulator
  float p_mstdv = modulator_stdv;
  al::Parameter modulation_depth{"/modulation depth", "", 100.0, "", 0.0, 300.0}; // user input for standard deviation value of granulator

  al::Parameter envelope{"/envelope", "", 0.1, "", 0.0, 1.0}; // user input for volume of the playing program. starts at 0 for no sound.
  al::PolySynth polySynth; // replaces vector of grains

  
  Granulator() { polySynth.allocatePolyphony<Grain>(nGrains); } //synthesize(); 
  
  void set(Grain* voice) {
    voice->synthesize(carrier_mean, carrier_stdv, modulator_mean, modulator_stdv, modulation_depth, envelope);
  }

  void updateActiveGrains() {  // change the number of grains on-screen
    // if (nGrains < activeGrains) { // if new number is lower than previous number of active grains
    //   for (int i = 0; i < activeGrains - nGrains; i++) {
    //     grains[nGrains + i].turnOff(); // turn off the ones @ index > nGrains
    //   }
    // } else if (nGrains > activeGrains) { // if new number is higher than previous number of active grains
    //   for (int i = 0; i < nGrains - activeGrains; i++) {
    //     grains[activeGrains + i].turnOn(); // turn on the ones @ index > active Grains
    //   }
    // }
    // activeGrains = nGrains; // update number of active grains 
  }

  void updateParameters() {
    if (nGrains != activeGrains) {updateActiveGrains();}
    if (p_cmean != carrier_mean || p_cstdv != carrier_stdv || p_mmean != modulator_mean || p_mstdv != modulator_mean) { 
      for (int i = 0; i < grains.size(); i++) { grains[i].synthesize(carrier_mean, carrier_stdv, modulator_mean, modulator_stdv, modulation_depth, envelope);} // turn off all grains, resynth
      p_cmean = carrier_mean;
      p_cstdv = carrier_stdv;
      p_mmean = modulator_mean;
      p_mstdv = modulator_stdv;
    }
  }
};

