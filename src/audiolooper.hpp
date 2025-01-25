#pragma once

#include "SDL3/SDL_audio.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_timer.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"
#include "imgui_memory_editor.h"
#include "imgui_stdlib.h"
#include "implot.h"
#include "implot3d.h"
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
    int sample_frames;
  };

  struct AudioSample {
    std::string name;
    std::vector<uint8_t> buffer;
    SDL_AudioStream *stream = nullptr;
    bool enabled = false;
  };

  std::vector<AudioDevice> in_devices;
  std::vector<AudioDevice> out_devices;

  std::vector<AudioSample> samples;

  SDL_AudioSpec in_spec{};
  SDL_AudioSpec out_spec{};

  int selected_in_device;
  int selected_out_device;
  int selected_sample;

  bool recording = false;
  bool playback = false;

  SDL_AudioDeviceID virtual_output_device;
  SDL_AudioDeviceID virtual_input_device;
  SDL_AudioStream *input_stream = nullptr;

  uint64_t last_timestamp;

  bool Init() {
    int in_count = 0;
    int out_count = 0;

    SDL_AudioDeviceID *in_devices = SDL_GetAudioRecordingDevices(&in_count);
    SDL_AudioDeviceID *out_devices = SDL_GetAudioPlaybackDevices(&out_count);

    InitDevices(this->in_devices, in_count, in_devices);
    InitDevices(this->out_devices, out_count, out_devices);

    for (int i = 0; i < 10; i++) {
      samples.push_back({});
    }

    for(auto& s : samples) {
      s.buffer.reserve(4*1024*1024);
    }

    selected_sample = 0;

    SelectOutputDevice(0);
    SelectInputDevice(0);

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
          sample_frames,
      });
    }
  }

  void SelectOutputDevice(int selected_id) {
    selected_out_device = selected_id;
    if (virtual_output_device != 0) {
      SDL_CloseAudioDevice(virtual_output_device);
    }

    auto &dev = out_devices[selected_id];
    virtual_output_device = SDL_OpenAudioDevice(dev.id, &dev.spec);
    if (virtual_output_device == 0) {
      SDL_Log("Couldn't open audio device: %s, %s", dev.name.data(),
              SDL_GetError());
      return;
    }
    SDL_AudioSpec spec;
    int sample_frames;
    SDL_GetAudioDeviceFormat(virtual_output_device, &spec, &sample_frames);
    std::vector<SDL_AudioStream *> streams;
    for (auto &s : samples) {
      if (s.stream != nullptr) {
        SDL_DestroyAudioStream(s.stream);
      }
      s.stream = SDL_CreateAudioStream(&spec, &spec);
      if (!s.stream) {
        SDL_Log("Failed to create audio stream: %s, %s", dev.name.data(),
                SDL_GetError());
      }
      streams.push_back(s.stream);
    }

      if(streams.size() > 0 && !SDL_BindAudioStreams(
          virtual_output_device, streams.data(), streams.size()))
        {
          SDL_Log("Failed to bind audio streams: %s, %s", dev.name.data(), SDL_GetError());
        }
    
  }

  void SelectInputDevice(int selected_id) {
    selected_in_device = selected_id;
    if (virtual_input_device != 0) {
      SDL_CloseAudioDevice(virtual_input_device);
    }

    auto &dev = in_devices[selected_id];
    virtual_input_device = SDL_OpenAudioDevice(dev.id, nullptr);
    if (virtual_input_device == 0) {
      SDL_Log("Couldn't open recording device: %s, %s", dev.name.data(),
              SDL_GetError());
      return;
    }

    input_stream = SDL_CreateAudioStream(&dev.spec, &dev.spec);
    if (!input_stream) {
      SDL_Log("Failed to create audio stream: %s, %s", dev.name.data(),
              SDL_GetError());
    }

    if(!SDL_BindAudioStream(virtual_output_device, input_stream)) {
      SDL_Log("Failed to bind audio stream: %s, %s", dev.name.data(),
              SDL_GetError());
    }

    SDL_AudioSpec spec;
    int sample_frames;
    if(!SDL_GetAudioDeviceFormat(virtual_output_device, &spec, &sample_frames)) {
      SDL_Log("Failed to bind audio device format: %s, %s", dev.name.data(),
              SDL_GetError());
    }

    SDL_SetAudioStreamFormat(input_stream, nullptr, &spec);
  }

  void OnEvent(const SDL_Event &e) {
    if(e.type == SDL_EVENT_KEY_DOWN) {
      int key = e.key.key;
      if(key >= SDLK_0 && key <= SDLK_9) {

        int sample_key = key - SDLK_0;
        if(sample_key == selected_sample) {
          if(recording) {
            StopRecording();
          } else {
            StartRecording();
          }
        } else {
          StopRecording();
          selected_sample = sample_key;
        }
      } else if(key == SDLK_RETURN) {
        if(playback) {
          StopPlayback();
        } else {
          StartPlayback();
        }
      } else if(key == SDLK_SPACE) {
        samples[selected_sample].enabled = !samples[selected_sample].enabled;
      }
    }
  }

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

  void StartRecording() {
    recording = true;
    if(SDL_ResumeAudioStreamDevice(input_stream)) {
      SDL_Log("Failed to resume audio stream: %s", SDL_GetError());
    }
  }

  void StopRecording() {
    recording = false;
    SDL_PauseAudioStreamDevice(input_stream);
  }

  void StartPlayback() {
    playback = true;
  }

  void StopPlayback() {
    playback = false;
  }


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
        auto [id, name, spec, samples] = target_devices[i];
        ImGui::TableNextColumn();
        if (ImGui::Button(name.data())) {
          if (!strcmp(label, "Output")) {
            SelectOutputDevice(i);
          } else {
            SelectInputDevice(i);
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
        samples.end()->buffer.reserve(4*1024*1024);
      }
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_Button, recording ? ImVec4(1.0f, 0.0f, 0.0f, 1.0f) : ImVec4(0.0f, 0.0f, 1.0f, 1.0f));
      if (ImGui::Button(recording ? "(R) Recording " : "(R) Record   ")) {
        if(recording) {
          StopRecording();
        } else {
          StartRecording();
        }
      }
      ImGui::PopStyleColor();
      
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_Button, playback ? ImVec4(1.0f, 0.0f, 0.0f, 1.0f) : ImVec4(0.0f, 0.0f, 1.0f, 1.0f));
      if (ImGui::Button(playback ? "(P) Stop" : "(P) Play")) {
        if(playback) {
          StopPlayback();
        } else {
          StartPlayback();
        }
      }
      ImGui::PopStyleColor();

      ImGui::SameLine();
      if (ImGui::Button("Clear")) {
      }
      ImGui::SameLine();
      if (ImGui::Button("Delete")) {
        samples.erase(samples.begin() + selected_sample);
      }
      ImGuiTableFlags flags =
            ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable
            | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti
            | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_NoBordersInBody
            | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY
            | ImGuiTableFlags_SizingFixedFit;

      if (ImGui::BeginTable("SampleTable", 3, flags)) {
          ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.0f, 0);
          ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 0.0f, 1);
          ImGui::TableSetupColumn("Capacity", ImGuiTableColumnFlags_WidthFixed, 0.0f, 2);
          ImGui::TableHeadersRow();
        
        char wlabel[32] = {0};
        for (int i = 0; i < samples.size(); i++) {
          
          ImGui::PushID(i);
          ImGui::TableNextRow(ImGuiTableRowFlags_None, 20.0f);
          

          auto &[name, data, stream, enabled] = samples[i];
          
          if(ImGui::TableSetColumnIndex(0)) {
            sprintf(wlabel, "cb%04d", i);
            
            ImGui::Checkbox(wlabel, &samples[i].enabled);
            ImGui::SameLine();
            sprintf(wlabel, "%04d", i);
            ImGui::PushStyleColor(ImGuiCol_Button, selected_sample == i ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            if (ImGui::Button(wlabel)) {
              selected_sample = i;
            }
            ImGui::PopStyleColor();
          }
          if(ImGui::TableSetColumnIndex(1)) {
            ImGui::Text("%" SDL_PRIu64, data.size());
          }
               if(ImGui::TableSetColumnIndex(2)) {
            ImGui::Text("%" SDL_PRIu64, data.capacity());
          }
          ImGui::PopID();
        }
        ImGui::EndTable();
      }
    }
    ImGui::End();
  }

  
  void Update() { 
    last_timestamp = SDL_GetTicksNS();
    
    if(input_stream != nullptr) {
      int res = SDL_GetAudioStreamAvailable(input_stream);
      SDL_Log("%i\n", res);
      while(res > 0) {
        Uint8 buf[1024];
        const int buf_read = SDL_GetAudioStreamData(input_stream, buf, sizeof(buf));
        if(buf_read < 0) {
          SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to read from input audio stream: %s", SDL_GetError());
        } else {
          auto& sample_buf = samples[selected_sample].buffer;
          sample_buf.insert(sample_buf.end(), buf, buf+sizeof(buf));
        }
        res = SDL_GetAudioStreamAvailable(input_stream);
      }

      if(res < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get avaiable stream: %s", SDL_GetError());
      }
    }
  }

};