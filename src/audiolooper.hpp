#pragma once

#include "SDL3/SDL_audio.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_timer.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"
#include "imgui_stdlib.h"
#include "implot.h"
#include "implot3d.h"
#include "imgui_memory_editor.h"
#include <SDL3/SDL.h>
#include <memory>
#include <stdio.h>

#include <string>
#include <unordered_map>
#include <vector>

const char *GetAudioFormatString(SDL_AudioFormat format) {
  switch (format) {
  case SDL_AUDIO_UNKNOWN:
    return "SDL_AUDIO_UNKNOWN";
  case SDL_AUDIO_U8:
    return "SDL_AUDIO_U8";
  case SDL_AUDIO_S8:
    return "SDL_AUDIO_S8";
  case SDL_AUDIO_S16LE:
    return "SDL_AUDIO_S16LE";
  case SDL_AUDIO_S16BE:
    return "SDL_AUDIO_S16BE";
  case SDL_AUDIO_S32LE:
    return "SDL_AUDIO_S32LE";
  case SDL_AUDIO_S32BE:
    return "SDL_AUDIO_S32BE";
  case SDL_AUDIO_F32LE:
    return "SDL_AUDIO_F32LE";
  case SDL_AUDIO_F32BE:
    return "SDL_AUDIO_F32BE";
  default:
    return "Unknown";
  }
}

class AudioLooper {
public:

    MemoryEditor mem_editor;

  struct AudioDevice {
    SDL_AudioDeviceID id;
    std::string name;
    SDL_AudioSpec spec;
    SDL_AudioStream *stream;
    int sample_frames;
  };

  struct AudioSample {
    std::string name;
    std::vector<uint8_t> buffer;
  };

  std::vector<AudioDevice> in_devices;
  std::vector<AudioDevice> out_devices;

  std::vector<AudioSample> samples;

  SDL_AudioSpec in_spec;
  SDL_AudioSpec out_spec;

  int selected_in_device;
  int selected_out_device;
  int selected_sample;

  SDL_AudioDeviceID virtual_output_device;
  SDL_AudioDeviceID virtual_input_device;
  

  uint64_t last_timestamp;

  bool Init() {
    int in_count = 0;
    int out_count = 0;

    SDL_AudioDeviceID *in_devices = SDL_GetAudioRecordingDevices(&in_count);
    SDL_AudioDeviceID *out_devices = SDL_GetAudioPlaybackDevices(&out_count);

    InitDevices(this->in_devices, in_count, in_devices);
    InitDevices(this->out_devices, out_count, out_devices);
    

    uint64_t last_timestamp = SDL_GetTicksNS();
    return true;
  }

  void InitDevices(std::vector<AudioDevice> &target_devices, int count,
                   SDL_AudioDeviceID *devices) {
    for (int i = 0; i < count; i++) {
      const char *name = SDL_GetAudioDeviceName(devices[i]);
      SDL_AudioSpec spec;
      int sample_frames = 0;
      if (!SDL_GetAudioDeviceFormat(devices[i], &spec, &sample_frames)) {
        SDL_Log("Could't obtain format for %s", name);
      }

      target_devices.push_back(AudioDevice{
          devices[i],
          std::string(name),
          spec,
          nullptr,
          sample_frames,
      });
    }
  }

  void SelectOutputDevice(int selected_id) {

  }

  void SelectInputDevice(int selected_id) {
    // Rebind output stream and destination stream for each sample.
  }

  void OnEvent(const SDL_Event &e) {}

  void Draw() {
    DrawDevices();
    DrawSamples();
  }

  void DrawDevices() {
    if (ImGui::Begin("Devices")) {
      DrawDeviceGroup("Input", this->in_devices, selected_in_device);
      DrawDeviceGroup("Output", this->out_devices, selected_out_device);
    }
    ImGui::End();
  }

  void Update() { last_timestamp = SDL_GetTicksNS(); }

  void DrawDeviceGroup(const char *label,
                       std::vector<AudioDevice> &target_devices,
                       int &selected_device) {
    if (selected_device >= 0 && selected_device < target_devices.size()) {
      ImGui::Text("Selected %s: %s", label,
                  target_devices[selected_device].name.data());
    } else {
      ImGui::Text("Selected %s: None", label);
    }

    if (ImGui::BeginTable(label, 6, ImGuiTableFlags_Resizable)) {
      ImGui::TableSetupColumn("Name");
      ImGui::TableSetupColumn("ID");
      ImGui::TableSetupColumn("SampleFrames");

      ImGui::TableSetupColumn("Channels");
      ImGui::TableSetupColumn("Frequency");
      ImGui::TableSetupColumn("Format");
      ImGui::TableHeadersRow();

      for (int i = 0; i < target_devices.size(); i++) {
        auto [id, name, spec,stream, samples] = target_devices[i];
        ImGui::TableNextColumn();
        if (ImGui::Button(name.data())) {
          selected_device = i;
          if(strcmp(label, "Output")) {
            UpdateAudioSteams(this->out_devices[selected_device].spec);
          }
        }
        ImGui::TableNextColumn();
        ImGui::Text("%u", id);
        ImGui::TableNextColumn();
        ImGui::Text("%i", samples);
        ImGui::TableNextColumn();
        ImGui::Text("%i", spec.channels);
        ImGui::TableNextColumn();
        ImGui::Text("%i", spec.freq);
        ImGui::TableNextColumn();
        ImGui::Text("%s", GetAudioFormatString(spec.format));
      }
      ImGui::TableNextRow();
      ImGui::EndTable();
    }
  }

  void DrawSamples() {
    if (ImGui::Begin("Samples")) {
      if (ImGui::Button("New")) {
        samples.push_back({});
      }

      if (ImGui::BeginTable("SampleTable", 4, ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Length (s)");
        ImGui::TableSetupColumn("Actions");

        ImGui::TableHeadersRow();

        for (int i = 0; i < samples.size(); i++) {
          auto &[name, data] = samples[i];
          ImGui::TableNextColumn();
          ImGui::Text("%i", i);
          ImGui::TableNextColumn();
          ImGui::InputText((std::string("Text") + std::to_string(i)).data(),
                           &name);
          ImGui::Text("%" SDL_PRIu64, data.size());
          ImGui::TableNextColumn();

          if (ImGui::Button("Record")) {
          }
          ImGui::SameLine();
          if (ImGui::Button("Clear")) {
          }
          ImGui::SameLine();
          if (ImGui::Button("Delete")) {
            samples.erase(samples.begin() + i);
          }
          
          ImGui::TableNextRow();
        }
        ImGui::EndTable();
      }
    }
    ImGui::End();
  }
};