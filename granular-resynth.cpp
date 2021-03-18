/* granular-resynth.cpp written by Stejara Dinulescu
 * MAT240B 2021, Final Project
 * This file is the main file for this allolib project. 
 * References: granulator-source-material.cpp, synths.h, Scatter-Sequence.cpp, frequency-modulation-grains.cpp written by Karl Yerkes
 *             various max patches showing FM synth
 *             PickRay examples, GUI examples (for parameter usage), and Gamma oscillator files were referenced from allolib
 */

#include "al/app/al_App.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"
#include "al/ui/al_PresetHandler.hpp"
#include "al/ui/al_PresetServer.hpp"
#include "al/math/al_Ray.hpp" // Ray
#include "al/graphics/al_Shapes.hpp"
#include "al/math/al_Random.hpp"
#include "grains.h"
#include "sequence.h"

using namespace al;

struct MyApp : App {
  ControlGUI gui; // gui
  Granulator granulator; // handles grains
  //Sequencer sequencer; // handles sequence
  std::vector<Sequencer> sequencers;
  std::mutex mutex; // mutex for audio callback

  al::Light light;

  MyApp() {} // this is called from the main thread

  void onCreate() override {
    gui.init();
    gui << granulator.gain << granulator.nGrains << 
           granulator.carrier_mean << granulator.carrier_stdv << 
           granulator.modulator_mean << granulator.modulator_stdv << 
           granulator.modulation_depth << granulator.moddepth_stdv <<
           granulator.envelope;
           
    for (int i = 0; i < NUM_SEQUENCERS; i++) {  // init sequencers
      Sequencer s; 
      sequencers.push_back(s); 
    } 
    
    for (int i = 0; i < NUM_SEQUENCERS; i++) { gui << sequencers[i].rate; } // add their rates to the GUI
    
    nav().pos(0, 0, 25);
    light.pos(0, 0, 25);
    light.diffuse();
  }

  void onAnimate(double dt) override {
    navControl().active(!gui.usingInput());

    for (int i = 0; i < NUM_SEQUENCERS; i++) {
      sequencers[i].setTimer();
    }
  }

  Vec3d unproject(Vec3d screenPos) { // copied from Scatter-Sequence.cpp by Karl Yerkes
    auto& g = graphics();
    auto mvp = g.projMatrix() * g.viewMatrix() * g.modelMatrix();
    Matrix4d invprojview = Matrix4d::inverse(mvp);
    Vec4d worldPos4 = invprojview.transform(screenPos);
    return worldPos4.sub<3>(0) / worldPos4.w;
  }

  Rayd getPickRay(int screenX, int screenY) { // copied from Scatter-Sequence.cpp by Karl Yerkes
    Rayd r;
    Vec3d screenPos;
    screenPos.x = (screenX * 1. / width()) * 2. - 1.;
    screenPos.y = ((height() - screenY) * 1. / height()) * 2. - 1.;
    screenPos.z = -1.;
    Vec3d worldPos = unproject(screenPos);
    r.origin().set(worldPos);

    screenPos.z = 1.;
    worldPos = unproject(screenPos);
    r.direction().set(worldPos);
    r.direction() -= r.origin();
    r.direction().normalize();
    return r;
  }

  bool onMouseDown(const Mouse& m) override {  // adapted from Scatter-Sequence.cpp by Karl Yerkes
    al::Rayd r = getPickRay(m.x(), m.y());

    for (int i = 0; i < granulator.nGrains; i++) {
      float t = r.intersectSphere(granulator.settings[i].position, 0.2);

      if (t > 0.0f) {
        //if(mutex.try_lock()) {
          for (int j = 0; j < NUM_SEQUENCERS; j++) {
            //sequencers[j].sayName();
            if (sequencers[j].active) {
              bool found = sequencers[j].checkIntersection(granulator.settings[i].position); 

              if (!found) {
                granulator.settings[i].color = sequencers[j].color; // turn it whatever color is assigned to the sequencer
                sequencers[j].addSample(granulator.settings[i]);
                //sequencers[j].printSamples();
                break;
              } else { granulator.settings[i].color = al::Vec3f(1.0, 1.0, 1.0); } // turn it white again 
            }
          }
        
          //mutex.unlock(); // unblocks
        //}
      }
    }
    return true;
  }

  bool onMouseMove(const Mouse& m) override { // adapted from Scatter-Sequence.cpp by Karl Yerkes
    al::Rayd r = getPickRay(m.x(), m.y());
    for (int i = 0; i < granulator.nGrains; i++) {
      float t = r.intersectSphere(granulator.settings[i].position, 0.1);
      // only trigger once; no re-trigger when hovering
      if (granulator.settings[i].hover == false && t > 0.0f) {
        // trigger grain
        auto* voice = granulator.polySynth.getVoice<Grain>(); // grab one of the voices
        granulator.set(voice, granulator.settings[i]); // set properties of the voice -> taken from settings[playhead index]
        granulator.polySynth.triggerOn(voice); //trigger it on
      }
      granulator.settings[i].hover = (t > 0.f);
    }
    return true;
  }

  virtual bool onKeyDown(const Keyboard &k) override {
    if (k.key() == ' ') {
      granulator.resetSettings();
      //std::cout << "space" << std::endl;
    } else {
      for (int i = 0; i < sequencers.size(); i++) {
        //std::cout << k.key() - 48 << " " << i << std::endl;
        if (k.key() - 48 == i) { sequencers[i].enable(); }
        else { sequencers[i].disable(); }
      }
    }
    
    return true;
  }

  void onDraw(Graphics& g) override {
    g.clear(0.1); // background color
    gl::depthTesting(true); // if this is enabled, color disappears
    g.lighting(true);
    gui.draw(g); // draw GUI
    granulator.displayGrainSettings(g);
    granulator.polySynth.render(g);  // Call render for PolySynth to generate its output
  }

  void onSound(AudioIOData& io) override {    
    granulator.polySynth.render(io); // render all active synth voices (grains) into the output buffer
    io.frame(0); // reset the frame so we can go over the frame again below

    while (io()) {
      for (int i = 0; i < NUM_SEQUENCERS; i++) {
        if (sequencers[i].timer()) {
          if (mutex.try_lock()) {
            if (sequencers[i].sequence.size() > 0) { // if there is something in the sequence
              auto* voice = granulator.polySynth.getVoice<Grain>(); // grab one of the voices
              granulator.set(voice, sequencers[i].grabSample()); // set properties of the voice -> taken from settings[playhead index]
              granulator.polySynth.triggerOn(voice); //trigger it on

              sequencers[i].increment();
            }
            mutex.unlock();
          }
        }
      }

      io.out(0) = tanh(io.out(0) * granulator.gain);
      io.out(1) = tanh(io.out(1) * granulator.gain);
    }
  }
};

int main() {
  MyApp app;
  app.dimensions(1000, 700);
  app.configureAudio(SAMPLE_RATE, 768, OUTPUT_CHANNELS);
  app.start();
}