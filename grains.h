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


const int SAMPLE_RATE = 48000; 
const int BLOCK_SIZE = 2048; 
const int OUTPUT_CHANNELS = 2;

const int MAX_GRAINS = 1000;
const int MAX_DURATION = 3;
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

  void set(float cm, float csd, float mm, float msd, float md, float e, float g) {
    duration = al::rnd::normal() / 3 + 1.3;
    size = duration/MAX_DURATION;
    modulator_depth = md; 
    envelope = e;
    gain = g;

    carrier_start = al::clip( ((double)al::rnd::normal() / csd) + (double)cm, MAX_FREQUENCY, 0.0);
    carrier_end = al::clip( ((double)al::rnd::normal() / csd) + (double)cm, MAX_FREQUENCY, 0.0);
    modulator_start = al::clip((double)al::rnd::normal() / msd + (double)mm, MAX_FREQUENCY, 0.0);
    modulator_end = al::clip((double)al::rnd::normal() / msd + (double)mm, MAX_FREQUENCY, 0.0);
    md_start = al::clip((double)al::rnd::normal() + (double)md, MAX_FREQUENCY, 0.0);
    md_end = al::clip((double)al::rnd::normal() + (double)md, MAX_FREQUENCY, 0.0);

    float x = (carrier_end - carrier_start)/10;
    float y = (modulator_end - modulator_start)/10;
    float z = (md_end - md_start)/10; // normalized duration
    position = al::Vec3f(x, y, z);
    //std::cout << position << std::endl;

    // draw circle, taken from Scatter-Sequence.cpp by Karl Yerkes
    const int N = 100;
    for (int i = 1; i < N + 1; i++) {
      float a0 = i * M_PI * 2 / N;
      float a1 = (i - 1) * M_PI * 2 / N;
      float r = 0.1;
      mesh.vertex(0, 0);
      mesh.vertex(r * sin(a0), r * cos(a0));
      mesh.vertex(r * sin(a1), r * cos(a1));
    }
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

  al::Mesh mesh;
  al::Vec3f position;
  al::Vec3f color = al::Vec3f(1.0, 0.0, 0.0);

  Grain() { 
    //al::addCone(mesh);  // Prepare mesh to draw a cone
    mesh.primitive(al::Mesh::TRIANGLE_STRIP); 
  }

  void set(GrainSettings g) {
    duration = g.duration;

    alpha.set(g.carrier_start, g.carrier_end, g.duration);
    beta.set(g.modulator_start, g.modulator_end, g.duration);
    carrier.freq(0);
    modulator.freq(0);

    moddepth.set(g.md_start, g.md_end, duration); // set start freq, target freq, duration in seconds

    envelope.set(g.envelope * g.duration, (1 - g.envelope) * g.duration, g.gain);

    // draw circle, taken from Scatter-Sequence.cpp by Karl Yerkes
    const int N = 100;
    for (int i = 1; i < N + 1; i++) {
      float a0 = i * M_PI * 2 / N;
      float a1 = (i - 1) * M_PI * 2 / N;
      float r = 0.1;
      mesh.vertex(0, 0);
      mesh.vertex(r * sin(a0), r * cos(a0));
      mesh.vertex(r * sin(a1), r * cos(a1));
    }
    
    position = g.position;
  }
  
  // void setPosition(al::Vec3f pos) { position = pos; }

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
    g.color(color.x, color.y, color.z); // grain flashes red whenever it plays
    g.translate(position);
    g.scale(duration/MAX_DURATION); // scale based on duration (normalized)
    //g.meshColor();
    g.draw(mesh);  // Draw the mesh
    g.popMatrix();
  }
};

struct Granulator {
  // GUI accessible parameters
  al::ParameterInt nGrains{"/number of grains", "", 10, "", 0, MAX_GRAINS}; // user input for grains on-screen
  al::Parameter carrier_mean{"/carrier mean", "", 440.0, "", 0.0, 4000.0}; // user input for mean frequency value of granulator, in Hz
  al::Parameter carrier_stdv{"/carrier standard deviation", "", 0.2, "", 0.1, 1.0}; // user input for standard deviation frequency value of granulator

  al::Parameter modulator_mean{"/modulator mean", "", 800.0, "", 0.0, 4000.0}; // user input for mean frequency value of granulator, in Hz
  al::Parameter modulator_stdv{"/modulator standard deviation", "", 0.2, "", 0.1, 1.0}; // user input for standard deviation frequency value of granulator
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

  void set(Grain* voice, GrainSettings settings) {
    voice->set(settings);
  }

  void displayGrainSettings(al::Graphics &g) {
    for (int i = 0; i < nGrains; i++) {
      g.pushMatrix();
      g.color(settings[i].color.x, settings[i].color.y, settings[i].color.z); // TODO: CHANGE COLOR BASED ON HOVER
      g.translate(settings[i].position);
      g.scale(settings[i].size); // scale based on duration (normalized)
      g.draw(settings[i].mesh);  // Draw the mesh
      g.popMatrix();
    }
  }

  void resetSettings() {
    // whenever the mean or stdv values are changed, reset grain settings
  }
};

