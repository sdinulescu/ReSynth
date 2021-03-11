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
//#include "sequence.h"

using namespace al;

struct MyApp : App {
  ControlGUI gui; 
  Granulator granulator; // handles grains
  //Sequencer sequencer; // handles sequence
  std::mutex mutex; // mutex for audio callback

  al::ParameterInt steps{"/sequencer steps", "", 4, "", 0, 10}; // user input for number of "beats" in the sequencer (can think of sequencer as a measure)
  al::Parameter rate{"/sequencer rate", "", 1.0, "", 0.1, 10.0}; // user input for mean frequency value of granulator, in Hz
  gam::Accum<> timer; // rate timer
  int playhead = 0; // where we are in the sequencer

  MyApp() {} // this is called from the main thread

  void onCreate() override {
    gui.init();
    gui << granulator.nGrains << granulator.carrier_mean << granulator.carrier_stdv << 
           granulator.modulator_mean << granulator.modulator_stdv << granulator.modulation_depth << 
           granulator.envelope << granulator.gain << steps << rate;

    timer.freq(rate);

    nav().pos(0, 0, 4);
  }

  void onAnimate(double dt) override {
    navControl().active(!gui.usingInput());

    if (timer.freq() != rate) {timer.freq(rate);} // reset the timer rate if it changes
  }

  void onDraw(Graphics& g) override {
    g.clear(0.2); // background color
    gui.draw(g); // draw GUI
    granulator.polySynth.render(g);  // Call render for PolySynth to generate its output
  }

  void onSound(AudioIOData& io) override {    
    granulator.polySynth.render(io); // render all active synth voices (grains) into the output buffer
    io.frame(0); // reset the frame so we can go over the frame again below

    while (io()) {
      if (timer()) {
        if (mutex.try_lock()) {
          if (steps > 0) {
            auto* voice = granulator.polySynth.getVoice<Grain>(); // grab one of the voices
            granulator.set(voice, playhead); // set properties of the voice -> taken from settings[playhead index]
            granulator.polySynth.triggerOn(voice); //trigger it on

            playhead++; // increment the playhead
            if (playhead >= steps) {
              playhead -= steps;
              if (playhead < 0) { playhead = 0; }
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
  app.dimensions(800, 600);
  app.configureAudio(SAMPLE_RATE, 768, OUTPUT_CHANNELS);
  app.start();
}