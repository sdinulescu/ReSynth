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
const int MAX_DURATION = 10;
const double MAX_FREQUENCY = 4000;

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

struct GrainSettings {
  float carrier_mean;
  float carrier_stdv;
  float modulator_mean;
  float modulator_stdv;
  float modulator_depth;
  float envelope; 
  float gain;

  void set(float cm, float csd, float mm, float msd, float md, float e, float g) {
    carrier_mean = cm; 
    carrier_stdv = csd;
    modulator_mean = mm;
    modulator_stdv = msd;
    modulator_depth = md; 
    envelope = e;
    gain = g;
  }
};

struct Grain : al::SynthVoice {
  float duration = 0.0;
  gam::Sine<float> carrier; 
  gam::Sine<float> modulator; 
  Line moddepth; 
  ExpSeg alpha;
  ExpSeg beta;
  AttackDecay envelope;

  al::Vec3f position = al::Vec3f(al::rnd::uniform(), al::rnd::uniform(), al::rnd::uniform()); 
  al::Mesh mesh;

  Grain() { 
    //al::addCone(mesh);  // Prepare mesh to draw a cone
    mesh.primitive(al::Mesh::LINE_STRIP); 
  }

  void set(GrainSettings g) {
    float d = al::rnd::uniform(1, MAX_DURATION); // duration
    duration = d;
    float carrier_start = al::clip( ((double)al::rnd::normal() / g.carrier_stdv) + (double)g.carrier_mean, MAX_FREQUENCY, 0.0);
    float carrier_end = al::clip( ((double)al::rnd::normal() / g.carrier_stdv) + (double)g.carrier_mean, MAX_FREQUENCY, 0.0);
    float mod_start = al::clip((double)al::rnd::normal() / g.modulator_stdv + (double)g.modulator_mean, MAX_FREQUENCY, 0.0);
    float mod_end = al::clip((double)al::rnd::normal() / g.modulator_stdv + (double)g.modulator_mean, MAX_FREQUENCY, 0.0);

    alpha.set(carrier_start, carrier_end, d);
    beta.set(mod_start, mod_end, d);
    carrier.freq(0);
    modulator.freq(0);

    moddepth.set(al::clip((double)al::rnd::normal() + (double)g.modulator_depth, 500.0, 0.0), 
                 al::clip((double)al::rnd::normal() + (double)g.modulator_depth, 500.0, 0.0), d); // set start freq, target freq, duration in seconds

    envelope.set(g.envelope * d, (1 - g.envelope) * d, g.gain);

    float x = d/MAX_DURATION; // normalized duration
    float y = (carrier_end - carrier_start)/MAX_FREQUENCY;
    float z = (mod_end - mod_start)/MAX_FREQUENCY;
    position = al::Vec3f(x, y, z);
  }
  
  void setPosition(al::Vec3f pos) { position = pos; }

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

  void onProcess(al::Graphics &g) override {
    g.pushMatrix();
    g.color(1.0);
    g.translate(position);
    g.scale(duration); // scale based on duration
    g.draw(mesh);  // Draw the mesh
    g.popMatrix();
  }
};

struct Granulator {
  // GUI accessible parameters
  al::ParameterInt nGrains{"/number of grains", "", 5, "", 0, MAX_GRAINS}; // user input for grains on-screen
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
  al::Parameter gain{"/gain", "", 0.8, "", 0.0, 1.0}; // user input for volume of the playing program. starts at 0 for no sound.

  al::PolySynth polySynth; 
  std::vector<GrainSettings> settings;
  
  Granulator() { 
    for (int i = 0; i < MAX_GRAINS; i++) { // push back all of the different settings for MAX_GRAINS at the start of the program
      GrainSettings g;
      g.set(carrier_mean, carrier_stdv, modulator_mean, modulator_stdv, modulation_depth, envelope, gain);
      settings.push_back(g);
    }
    polySynth.allocatePolyphony<Grain>(nGrains); //this handles all grains that can happen at once
  } 

  void set(Grain* voice, int index) {
    voice->set(settings[index]);
  }
};

