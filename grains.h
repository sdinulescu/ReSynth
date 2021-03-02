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

// struct Phasor {
//   float phase{0};      // on the interval [0, 1)
//   float increment{0};  // led to an low F

//   void frequency(float hertz) {
//     // this function may run per-sample. all this stuff costs performance
//     // assert(hertz < SAMPLE_RATE && hertz > -SAMPLE_RATE);
//     increment = hertz / SAMPLE_RATE;
//   }
//   void period(float seconds) { frequency(1 / seconds); }
//   float frequency() const { return SAMPLE_RATE * increment; }
//   void zero() { phase = increment = 0; }

//   // add some Hertz to the current frequency
//   //
//   void modulate(float hertz) { increment += hertz / SAMPLE_RATE; }

//   float operator()() {
//     // increment and wrap phase; this only works correctly for frequencies in
//     // (-SAMPLE_RATE, SAMPLE_RATE) because otherwise increment will be greater
//     // than 1 or less than -1 and phase will get away from us.
//     //
//     phase += increment;
// #if 0
//     // must me >= 1.0 to stay on [0.0, 1.0)
//     if (phase >= 1.0) phase -= 1.0;
//     // must me < 0.0 to stay on [0.0, 1.0)
//     if (phase < 0) phase += 1.0;
// #else
//     // XXX should we use fmod instead? frequencies outside of the norm
//     // (-SAMPLE_RATE, SAMPLE_RATE)? might put phase outside [0, 1.0) unless we
//     // use fmod.
//     phase = fmod(phase, 1.0f);
// #endif
//     return phase;
//   }
// };

// struct Buffer : std::vector<float> {
//   int sampleRate{SAMPLE_RATE};

//   void operator()(float f) {
//     push_back(f);
//     //
//   }
//   void save(const std::string& fileName) const { save(fileName.c_str()); }
//   void save(const char* fileName) const {
//     drwav_data_format format;
//     format.container = drwav_container_riff;
//     format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
//     format.channels = 1;
//     format.sampleRate = sampleRate;
//     format.bitsPerSample = 32;

//     drwav wav;
//     if (!drwav_init_file_write(&wav, fileName, &format, nullptr)) {
//       std::cerr << "filed to init file " << fileName << std::endl;
//       return;
//     }
//     drwav_uint64 framesWritten = drwav_write_pcm_frames(&wav, size(), data());
//     if (framesWritten != size()) {
//       std::cerr << "failed to write all samples to " << fileName << std::endl;
//     }
//     drwav_uninit(&wav);
//   }

//   bool load(const std::string& fileName) { return load(fileName.c_str()); }
//   bool load(const char* filePath) {
//     unsigned int channels;
//     unsigned int sampleRate;
//     drwav_uint64 totalPCMFrameCount;
//     float* pSampleData = drwav_open_file_and_read_pcm_frames_f32(
//         filePath, &channels, &sampleRate, &totalPCMFrameCount, NULL);
//     if (pSampleData == NULL) {
//       printf("failed to load %s\n", filePath);
//       return false;
//     }

//     //
//     if (channels == 1)
//       for (int i = 0; i < totalPCMFrameCount; i++) {
//         push_back(pSampleData[i]);
//       }
//     else if (channels == 2) {
//       for (int i = 0; i < totalPCMFrameCount; i++) {
//         push_back((pSampleData[2 * i] + pSampleData[2 * i + 1]) / 2);
//       }
//     } else {
//       printf("can't handle %d channels\n", channels);
//       return false;
//     }

//     drwav_free(pSampleData, NULL);
//     return true;
//   }

//   // raw lookup
//   float raw(const float index) const {
//     // XXX don't use unsigned unless you really need to. See
//     // https://jacobegner.blogspot.com/2019/11/unsigned-integers-are-dangerous.html
//     // Using unsigned indicies here will fail when we do FM using a table-based
//     // sine. (?questionable?) Furthermore, we need large ints to prevent
//     // overflow which also happens when doing FM.
//     const int i = floor(index);
//     const int j = i == (size() - 1) ? 0 : i + 1;
//     // const unsigned j = (i + 1) % size(); // is this faster or slower than the
//     // line above?
// #if 1
//     const float x0 = std::vector<float>::operator[](i);
//     const float x1 = std::vector<float>::operator[](j);  // loop around
// #else
//     const float x0 = at(i);  // at() may throw std::out_of_range exception
//     const float x1 = at(j);  // loop around
// #endif
//     const float t = index - i;
//     return x1 * t + x0 * (1 - t);
//   }

//   // void resize(unsigned n) { data.resize(n, 0); }
//   // float& operator[](unsigned index) { return data[index]; }

//   // allow for sloppy indexing (e.g., negative, huge) by fixing the index to
//   // within the bounds of the array
//   float get(float index) const {
//     index = fmod(index, (float)size());
//     if (index < 0.0f) {
//       index += size();
//     }
//     return raw(index);
//   }
//   float operator[](const float index) const { return get(index); }
//   float phasor(float index) const { return get(size() * index); }

//   void add(const float index, const float value) {
//     const unsigned i = floor(index);
//     // XXX i think this next bit is wrong!
//     const unsigned j = (i == (size() - 1)) ? 0 : i + 1;  // looping semantics
//     const float t = index - i;
//     at(i) += value * (1 - t);
//     at(j) += value * t;
//   }
// };

// float sine(float phase) {
//   struct SineBuffer : Buffer {
//     SineBuffer() {
//       resize(100000);
//       for (unsigned i = 0; i < size(); ++i)  //
//         at(i) = sin(i * M_PI * 2 / size());
//     }
//     float operator()(float phase) { return phasor(phase); }
//   };

//   static SineBuffer instance;
//   return instance(phase);
// }

// struct Sine : Phasor {
//   float operator()() { return sine(Phasor::operator()()); }
// };

struct ExpSeg {
  Line line;
  void set(double v, double t, double s) { line.set(log2(v), log2(t), s); }
  void set(double t, double s) { line.set(log2(t), s); }
  void set(double t) { line.set(log2(t)); }
  float operator()() { return pow(2.0f, line()); }
};

struct Grain {
  // std::vector<float> clip; 
  //int clipIndex = 0; // if grain is on

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
    float d = al::rnd::uniform(1, 10); // duration

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
      //clipIndex++; 
      //std::cout<< "calc sample " << sampleValue <<std::endl; 
    }
    //if (active && clipIndex < duration) { sampleValue = clip[clipIndex]; clipIndex++; }
    //checkDeath(); // turn off if the grain is supposed to die this sample
    return sampleValue;
  }

  void turnOn() { active = true; }
  void turnOff() { active = false; }
  //void checkDeath() { if (clipIndex >= duration) { turnOff(); } }
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

