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
  al::Parameter gain{"/gain", "", 0.0, "", 0.0, 1.0}; // user input for volume of the playing program. starts at 0 for no sound.
  
  MyApp() {} // this is called from the main thread

  void onCreate() override {
    gui.init();
    gui << granulator.nGrains << granulator.mean << granulator.stdv << gain;
  }

  void onAnimate(double dt) override {
    navControl().active(!gui.usingInput());

    // update which grains are active if need be
    granulator.updateActiveGrains();
  }

  void onDraw(Graphics& g) override {
    g.clear(0.2); // background color
    gui.draw(g);
  }

  void onSound(AudioIOData& io) override {    
    int sampleIndex = 0;
    while (io()) {
      float f = 0.0;
      for (int i = 0; i < granulator.activeGrains; i++) {
        f += granulator.grains[i].getSample(sampleIndex);
      }
      f /= float(SAMPLE_RATE);
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