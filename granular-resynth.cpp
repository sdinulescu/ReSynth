/* granular-resynth.cpp written by Stejara Dinulescu
 * MAT240B 2021, Final Project
 * This file is the main file for this allolib project. 
 * References: granulator-source-material.cpp written by Karl Yerkes
 */

#include "al/app/al_App.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"
#include "al/ui/al_PresetHandler.hpp"
#include "al/ui/al_PresetServer.hpp"
#include "grains.h"
#include "sequence.h"

using namespace al;

struct MyApp : App {
  ControlGUI gui; 
  Granulator granulator; // handles grains
  Sequencer sequencer; // handles sequence
  std::mutex mutex; // mutex for audio callback
  al::Parameter gain{"/gain", "", 0.8, "", 0.0, 1.0}; // user input for volume of the playing program. starts at 0 for no sound.

  MyApp() {} // this is called from the main thread

  void onCreate() override {
    gui.init();
    gui << granulator.nGrains << granulator.carrier_mean << granulator.carrier_stdv << 
           granulator.modulator_mean << granulator.modulator_stdv << granulator.modulation_depth << 
           sequencer.steps << sequencer.rate <<
           granulator.envelope << gain;
  }

  void onAnimate(double dt) override {
    navControl().active(!gui.usingInput());
    // check if gui inputs for mean and stddev actually makes sense (i.e. restrict Hz vals between 0 and ~2000)
    //if (granulator.mean - granulator.stdv < 0) { granulator.stdv.set(granulator.mean); } // if stddev is greater than mean, set to the mean value
    //if (granulator.mean + granulator.stdv > 2000) { granulator.stdv.set(2000 - granulator.mean); } // if mean + stddev > max Hz value, reduce stddev to a value that makes sense

    // update params if mean, stdv, or grain num is changed
    granulator.updateParameters();
    sequencer.updateParameters();
  }

  void onDraw(Graphics& g) override {
    g.clear(0.2); // background color
    gui.draw(g);
  }

  void onSound(AudioIOData& io) override {    
    granulator.polySynth.render(io); // render all active synth voices (grains) into the output buffer
    io.frame(0); // reset the frame so we can go over the frame again below
    // we need a mutex -> put this in later
    while (io()) {
      float f = 0.0;
      if (sequencer.timer()) {
        if (mutex.try_lock()) {
          if (sequencer.steps > 0) {
            //Grain g = sequencer.sequence[sequencer.playhead]; // get the grain held in the sequencer
            auto* voice = granulator.polySynth.getVoice<Grain>(); // grab one of the voices
            granulator.set(voice);
            //voice->set()
            granulator.polySynth.triggerOn(voice); //trigger it on
            // f = (float)granulator.grains[playhead].calculateSample();
            // std::cout << f << std::endl;
            sequencer.playhead++;
            if (sequencer.playhead >= sequencer.steps) {
              sequencer.playhead -= sequencer.steps;
              if (sequencer.playhead < 0) { sequencer.playhead = 0; }
            }
          }
          mutex.unlock();
        }
      }

      //int n = 0; // keeps track of grains that are sounding
      // for (int i = 0; i < granulator.activeGrains; i++) {
      //   float newSample = granulator.grains[i].calculateSample();
      //   if (newSample != 0) {n++;}
      //   //if (newSample == 0) { granulator.activeGrains -= 1; } else {n++;}
      //   f += newSample;
      // }
      // //std::cout << n << " " << granulator.activeGrains << std::endl;
      // f /= granulator.activeGrains;
      
      
      //std::cout << f << std::endl;
      if (f > 1.0) {f = 1.0;} else if (f < 0.0) { f = 0.0; } // bounds checking
      io.out(0) *= gain;
      io.out(1) *= gain;
    }
  }
};

int main() {
  MyApp app;
  app.configureAudio(SAMPLE_RATE, 768, OUTPUT_CHANNELS);
  app.start();
}