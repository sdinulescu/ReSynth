/* buffer.h compiled by Stejara Dinulescu
 * MAT240B 2021, Final Project
 * This file defines the Buffer struct used in granular-resynth.cpp to record the sound from the session
 * References: frequency-modulation-grains.cpp, synths.h, dr_wav.h from Karl Yerkes
 */

# pragma once

#include <string>
#include <vector>
#include "grains.h"
#include "dr_wav.h" // copied from MAT240B-2021 repo

//copied and pasted from synths.h
struct Buffer : std::vector<float> {
  int sampleRate{SAMPLE_RATE};

  void operator()(float f) {
    push_back(f);
  }
  void save(const std::string& fileName) const { save(fileName.c_str()); }
  void save(const char* fileName) const {
    drwav_data_format format;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels = 1;
    format.sampleRate = sampleRate;
    format.bitsPerSample = 32;

    drwav wav;
    if (!drwav_init_file_write(&wav, fileName, &format, nullptr)) {
      std::cerr << "filed to init file " << fileName << std::endl;
      return;
    }
    drwav_uint64 framesWritten = drwav_write_pcm_frames(&wav, size(), data());
    if (framesWritten != size()) {
      std::cerr << "failed to write all samples to " << fileName << std::endl;
    }
    drwav_uninit(&wav);
  }

  bool load(const std::string& fileName) { return load(fileName.c_str()); }
  bool load(const char* filePath) {
    unsigned int channels;
    unsigned int sampleRate;
    drwav_uint64 totalPCMFrameCount;
    float* pSampleData = drwav_open_file_and_read_pcm_frames_f32(
        filePath, &channels, &sampleRate, &totalPCMFrameCount, NULL);
    if (pSampleData == NULL) {
      printf("failed to load %s\n", filePath);
      return false;
    }

    //
    if (channels == 1)
      for (int i = 0; i < totalPCMFrameCount; i++) {
        push_back(pSampleData[i]);
      }
    else if (channels == 2) {
      for (int i = 0; i < totalPCMFrameCount; i++) {
        push_back((pSampleData[2 * i] + pSampleData[2 * i + 1]) / 2);
      }
    } else {
      printf("can't handle %d channels\n", channels);
      return false;
    }

    drwav_free(pSampleData, NULL);
    return true;
  }

  float get(float index) const {
    // correct indices that are out or range i.e., outside the interval [0.0,
    // size) in such a way that the buffer data seems to repeat forever to the
    // left and forever to the right.
    //
    index = al::wrap(index, (float)size());
    assert(index >= 0.0f);
    assert(index < size());

    const int i =
        floor(index);  // XXX modf would give us i and t; faster or slower?
    const int j = i == (size() - 1) ? 0 : i + 1;
    // const unsigned j = (i + 1) % size();
    // XXX is the ternary op (?:) faster or slower than the mod op (%) ?
    assert(j < size());

    const float x0 = std::vector<float>::operator[](i);
    const float x1 = std::vector<float>::operator[](j);  // loop around
    const float t = index - i;
    return x1 * t + x0 * (1 - t);
  }

  float operator[](const float index) const { return get(index); }
  float phasor(float index) const { return get(size() * index); }

  void add(float index, const float value) {
    index = al::wrap(index, (float)size());
    assert(index >= 0.0f);
    assert(index < size());

    const int i = floor(index);
    const int j = (i == (size() - 1)) ? 0 : i + 1;
    const float t = index - i;
    std::vector<float>::operator[](i) += value * (1 - t);
    std::vector<float>::operator[](j) += value * t;
  }
};