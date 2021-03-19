/* grains.h written by Stejara Dinulescu
 * MAT240B 2021, Final Project
 * This file defines the grain stuct and granulator struct used in granular-resynth.cpp
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

// CONSTANTS

const int SAMPLE_RATE = 48000; 
const int BLOCK_SIZE = 2048; 
const int OUTPUT_CHANNELS = 2;

const int MAX_GRAINS = 1000;
const int MAX_DURATION = 1.0;
const double MAX_FREQUENCY = 127;

// SOME METHODS

inline float mtof(float m) { return 8.175799f * powf(2.0f, m / 12.0f); } // taken from synths.h by Karl Yerkes
inline float map(float value, float low, float high, float Low, float High) { // taken from synths.h by Karl Yerkes
  return Low + (High - Low) * ((value - low) / (high - low));
}

// SOME STRUCTS

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

// These structs created by Stejara, drawing from examples
struct GrainSettings {
  float carrier_start;
  float carrier_end;
  float modulator_start;
  float modulator_end;
  float modulator_depth;
  float md_start; // modulation index start
  float md_end; // modulation index end
  float envelope; 
  float gain;
  float duration;

  al::Vec3f position;
  al::Mesh mesh;
  float size;
  al::Vec3f color = al::Vec3f(1.0, 1.0, 1.0);

  bool hover = false;

  GrainSettings() { mesh.primitive(al::Mesh::TRIANGLE_STRIP); }

  void set(float cm, float csd, float mm, float msd, float md, float mdsd, float e, float g) {
    duration = al::rnd::uniform(0.01, double(MAX_DURATION)); //one second
    size = map(duration, 0.01, double(MAX_DURATION), 0.5, 5.0);
    modulator_depth = md; 
    envelope = e;
    gain = g;

    carrier_start = mtof(al::clip( ((double)al::rnd::normal() / csd) + (double)cm, MAX_FREQUENCY, 0.0));
    carrier_end = mtof(al::clip( ((double)al::rnd::normal() / csd) + (double)cm, MAX_FREQUENCY, 0.0));
    modulator_start = mtof(al::clip((double)al::rnd::normal() / msd + (double)mm, MAX_FREQUENCY, 0.0));
    modulator_end = mtof(al::clip((double)al::rnd::normal() / msd + (double)mm, MAX_FREQUENCY, 0.0));
    md_start = mtof(al::clip((double)al::rnd::normal() / mdsd + (double)md, MAX_FREQUENCY, 0.0));
    md_end = mtof(al::clip((double)al::rnd::normal() / mdsd + (double)md, MAX_FREQUENCY, 0.0));

    float x = map((carrier_end - carrier_start), -127.0, 354.0, -2.0, 2.0);
    float y = map((modulator_end - modulator_start), -127.0, 354.0, -2.0, 2.0);
    float z = map((md_end - md_start), -127.0, 354.0, -2.0, 2.0); 
    position = al::Vec3f(x, y, z);
    al::addSphere(mesh, 0.1);
    mesh.generateNormals();
  }
};

struct Grain : al::SynthVoice {
  gam::Sine<float> carrier; 
  gam::Sine<float> modulator; 
  Line moddepth; 
  ExpSeg alpha;
  ExpSeg beta;
  AttackDecay envelope;

  al::Mesh mesh;
  al::Vec3f position;
  al::Vec3f color = al::Vec3f(1.0, 0.0, 0.0);
  float size = 0.0;

  Grain() { mesh.primitive(al::Mesh::TRIANGLE_STRIP); }

  void set(GrainSettings g) {
    size = g.size;

    alpha.set(g.carrier_start, g.carrier_end, g.duration);
    beta.set(g.modulator_start, g.modulator_end, g.duration);
    carrier.freq(0);
    modulator.freq(0);

    moddepth.set(g.md_start, g.md_end, g.duration); // set start freq, target freq, duration in seconds

    envelope.set(g.envelope * g.duration, (1 - g.envelope) * g.duration, g.gain);

    al::addSphere(mesh, 0.1);
    mesh.generateNormals();
    position = g.position;
  }

  void onProcess(al::AudioIOData& io) override { // audio thread
    while (io()) {
      modulator.freq(beta());
      carrier.freq(alpha() + moddepth() * modulator());
      
      float v = envelope() * carrier();
      io.out(0) += v;
      io.out(1) += v;

      if (envelope.decay.done()) {
        free();
        break;
      }
    }
  }

  void onProcess(al::Graphics &g) override { // graphics thread
    g.pushMatrix();
    g.translate(position);
    g.scale(size); // scale based on duration (normalized)
    g.color(color.x, color.y, color.z); // grain flashes red whenever it plays
    g.draw(mesh);  // Draw the mesh
    g.popMatrix();
  }
};

struct Granulator {
  // GUI accessible parameters
  al::ParameterInt nGrains{"/number of grains", "", 100, "", 0, MAX_GRAINS}; // user input for number of grains on-screen
  al::Parameter carrier_mean{"/carrier mean", "", 40.0, "", 0.0, (float)MAX_FREQUENCY}; // user input for mean frequency value of carrier, in midi
  al::Parameter carrier_stdv{"/carrier standard deviation", "", 0.07, "", 0.01, 1.0}; // user input for standard deviation of carrier
  al::Parameter modulator_mean{"/modulator mean", "", 80.0, "", 0.0, (float)MAX_FREQUENCY}; // user input for mean frequency value of modulator, in midi
  al::Parameter modulator_stdv{"/modulator standard deviation", "", 0.34, "", 0.01, 1.0}; // user input for standard deviation frequency value of modulator
  al::Parameter modulation_depth{"/modulation depth mean", "", 20.0, "", 0.0, (float)MAX_FREQUENCY}; // user input for mean value of modulation depth
  al::Parameter moddepth_stdv{"/modulation depth standard deviation", "", 0.07, "", 0.01, 1.0}; // user input for standard deviation value of modulation depth
  al::Parameter envelope{"/envelope", "", 0.5, "", 0.01, 1.0}; // user input for volume of the playing program. starts at 0 for no sound.
  al::Parameter gain{"/gain", "", 0.5, "", 0.0, 1.0}; // user input for volume of the playing program. starts at 0 for no sound.

  al::PolySynth polySynth; 
  std::vector<GrainSettings> settings;
  
  Granulator() { 
    for (int i = 0; i < MAX_GRAINS; i++) { // push back all of the different settings for MAX_GRAINS at the start of the program
      GrainSettings g;
      g.set(carrier_mean, carrier_stdv, modulator_mean, modulator_stdv, modulation_depth, moddepth_stdv, envelope, gain);
      settings.push_back(g);
    }
    polySynth.allocatePolyphony<Grain>(nGrains); //this handles all grains that can happen at once
  } 

  void set(Grain* voice, GrainSettings settings) {
    voice->set(settings);
  }

  void displayGrainSettings(al::Graphics &g) {
    for (int i = 0; i < nGrains; i++) {
      g.pushMatrix();
      g.translate(settings[i].position);
      g.scale(settings[i].size * 0.99); // scale based on duration (normalized)
      g.color(settings[i].color.x, settings[i].color.y, settings[i].color.z);
      g.draw(settings[i].mesh);  // Draw the mesh
      g.popMatrix();
    }
  }

  void resetSettings() {
    // whenever the user hits the spacebar, reset grain settings based on the new slider parameters
    for (int i = 0; i < MAX_GRAINS; i++) {
      settings[i].set(carrier_mean, carrier_stdv, modulator_mean, modulator_stdv, modulation_depth, moddepth_stdv, envelope, gain);
    }
  }
};

