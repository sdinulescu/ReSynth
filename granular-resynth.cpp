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

  MyApp() {} // this is called from the main thread

  void onCreate() override {
    gui.init();
    gui << granulator.nGrains << granulator.carrier_mean << granulator.carrier_stdv << 
           granulator.modulator_mean << granulator.modulator_stdv << granulator.modulation_depth << 
           sequencer.steps << sequencer.rate <<
           granulator.envelope << granulator.gain;
  }

  void onAnimate(double dt) override {
    navControl().active(!gui.usingInput());
  }

  void onDraw(Graphics& g) override {
    g.clear(0.2); // background color
    gui.draw(g);
  }

  void onSound(AudioIOData& io) override {    
    granulator.polySynth.render(io); // render all active synth voices (grains) into the output buffer
    io.frame(0); // reset the frame so we can go over the frame again below

    while (io()) {
      if (sequencer.timer()) {
        if (mutex.try_lock()) {
          if (sequencer.steps > 0) {
            auto* voice = granulator.polySynth.getVoice<Grain>(); // grab one of the voices
            if (sequencer.sequence.size() == 0) { granulator.set(voice); } // synthesize properties 
            else {
              Grain* g = sequencer.sequence[sequencer.playhead]; // get the grain held in the sequencer
              granulator.set(voice, g); // set the voice to that grain
            }
            granulator.polySynth.triggerOn(voice); //trigger it on

            sequencer.playhead++;
            if (sequencer.playhead >= sequencer.steps) {
              sequencer.playhead -= sequencer.steps;
              if (sequencer.playhead < 0) { sequencer.playhead = 0; }
            }
          }
          mutex.unlock();
        }
      }

      io.out(0) *= granulator.gain;
      io.out(1) *= granulator.gain;
    }
  }
};

int main() {
  MyApp app;
  app.configureAudio(SAMPLE_RATE, 768, OUTPUT_CHANNELS);
  app.start();
}