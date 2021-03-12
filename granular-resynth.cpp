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
#include "al/math/al_Ray.hpp" // Ray
#include "grains.h"
//#include "sequence.h"

using namespace al;

struct MyApp : App {
  ControlGUI gui; 
  Granulator granulator; // handles grains
  //Sequencer sequencer; // handles sequence
  std::mutex mutex; // mutex for audio callback

  //al::ParameterInt steps{"/sequencer steps", "", 4, "", 0, 10}; // user input for number of "beats" in the sequencer (can think of sequencer as a measure)
  al::Parameter rate{"/sequencer rate", "", 1.0, "", 0.1, 10.0}; // user input for mean frequency value of granulator, in Hz
  gam::Accum<> timer; // rate timer
  int playhead = 0; // where we are in the sequencer
  std::vector<GrainSettings> sequence;

  MyApp() {} // this is called from the main thread

  void onCreate() override {
    gui.init();
    gui << granulator.nGrains << granulator.carrier_mean << granulator.carrier_stdv << 
           granulator.modulator_mean << granulator.modulator_stdv << granulator.modulation_depth << 
           granulator.envelope << granulator.gain << rate;

    timer.freq(rate);

    nav().pos(0, 0, 4);
  }

  void onAnimate(double dt) override {
    navControl().active(!gui.usingInput());

    if (timer.freq() != rate) {timer.freq(rate);} // reset the timer rate if it changes
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

    for (int i = 0; i < granulator.settings.size(); i++) {
      float t = r.intersectSphere(granulator.settings[i].position, 0.1);

      if (t > 0.0f) {
        mutex.lock();  // blocks

        bool found = false;
        for (auto iterator = sequence.begin(); iterator != sequence.end();) {
          if (iterator->position == granulator.settings[i].position) {
            found = true;

            // actually, this would erase ALL the copies of point[i] in
            // sequence.
            sequence.erase(iterator);
          } else {
            ++iterator;  // only advance the iterator when we don't erase
          }
        }

        if (!found) {
          sequence.push_back(granulator.settings[i]);
          for (int i = 0; i < sequence.size(); i++) {
            std::cout << sequence[i].position << " ";
          }
          std::cout << std::endl;
          mutex.unlock();
          break;
        }

        mutex.unlock();
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

  void onDraw(Graphics& g) override {
    g.clear(0.2); // background color
    gui.draw(g); // draw GUI
    granulator.displayGrainSettings(g);
    granulator.polySynth.render(g);  // Call render for PolySynth to generate its output
  }

  void onSound(AudioIOData& io) override {    
    granulator.polySynth.render(io); // render all active synth voices (grains) into the output buffer
    io.frame(0); // reset the frame so we can go over the frame again below

    while (io()) {
      if (timer()) {
        if (mutex.try_lock()) {
          if (sequence.size() > 0) { // if there is something in the sequence
            auto* voice = granulator.polySynth.getVoice<Grain>(); // grab one of the voices
            granulator.set(voice, sequence[playhead]); // set properties of the voice -> taken from settings[playhead index]
            granulator.polySynth.triggerOn(voice); //trigger it on

            playhead++; // increment the playhead
            if (playhead >= sequence.size()) {
              playhead -= sequence.size();
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