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

using namespace al;

struct MyApp : App {
  ControlGUI gui; 
  Granulator granulator; // handles grains
  al::Parameter gain{"/gain", "", 0.8, "", 0.0, 1.0}; // user input for volume of the playing program. starts at 0 for no sound.
  
  MyApp() {} // this is called from the main thread

  void onCreate() override {
    gui.init();
    gui << granulator.nGrains << granulator.mean << granulator.stdv << granulator.modulation_depth << gain;
  }

  void onAnimate(double dt) override {
    navControl().active(!gui.usingInput());
    // check if gui inputs for mean and stddev actually makes sense (i.e. restrict Hz vals between 0 and ~2000)
    if (granulator.mean - granulator.stdv < 0) { granulator.stdv.set(granulator.mean); } // if stddev is greater than mean, set to the mean value
    if (granulator.mean + granulator.stdv > 2000) { granulator.stdv.set(2000 - granulator.mean); } // if mean + stddev > max Hz value, reduce stddev to a value that makes sense

    // update which grains are active if need be
    granulator.updateActiveGrains();
  }

  void onDraw(Graphics& g) override {
    g.clear(0.2); // background color
    gui.draw(g);
  }

  void onSound(AudioIOData& io) override {    
    while (io()) {
      float f = 0.0;
      for (int i = 0; i < granulator.activeGrains; i++) {
        f += granulator.grains[i].calculateSample();
      }
      //f /= granulator.activeGrains; //f /= float(SAMPLE_RATE);
      std::cout << f << std::endl;
      if (f > 1.0) {f = 1.0;} else if (f < 0.0) { f = 0.0; } // bounds checking
      io.out(0) = f * gain;
      io.out(1) = f * gain;
    }
  }
};

int main() {
  MyApp app;
  app.configureAudio(SAMPLE_RATE, BLOCK_SIZE, OUTPUT_CHANNELS);
  app.start();
}