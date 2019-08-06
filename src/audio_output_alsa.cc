/*
Copyright 2017 Google Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "audio_output_alsa.h"

#include <alsa/asoundlib.h>

#include <iostream>
#include <memory>

bool AudioOutputALSA::start()
{
	std::unique_lock<std::mutex> lock(isRunningMutex_);

	if (isRunning_)
	{
		return true;
	}

	int result = 0;

	snd_pcm_t* pcmHandle;
	if ((result = snd_pcm_open(&pcmHandle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0))
	{
		std::cerr << "AudioOutputALSA snd_pcm_open returned " << result << std::endl;
		return false;
	}

	snd_pcm_hw_params_t* pcmParams;
	if ((result = snd_pcm_hw_params_malloc(&pcmParams) < 0))
	{
		std::cerr << "AudioOutputALSA snd_pcm_hw_params_malloc returned " << result << std::endl;
		return false;
	}

	snd_pcm_hw_params_any(pcmHandle, pcmParams);
	if ((result =
	         snd_pcm_hw_params_set_access(pcmHandle, pcmParams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0))
	{
		std::cerr << "AudioOutputALSA snd_pcm_hw_params_set_access returned " << result << std::endl;
		return false;
	}

	if ((result = snd_pcm_hw_params_set_format(pcmHandle, pcmParams, SND_PCM_FORMAT_S16_LE) < 0))
	{
		std::cerr << "AudioOutputALSA snd_pcm_hw_params_set_format returned " << result << std::endl;
		return false;
	}

	if ((result = snd_pcm_hw_params_set_channels(pcmHandle, pcmParams, 1) < 0))
	{
		std::cerr << "AudioOutputALSA snd_pcm_hw_params_set_channels returned " << result
		          << std::endl;
		return false;
	}

	unsigned int rate = 16000;
	if ((result = snd_pcm_hw_params_set_rate_near(pcmHandle, pcmParams, &rate, nullptr) < 0))
	{
		std::cerr << "AudioOutputALSA snd_pcm_hw_params_set_rate_near returned " << result
		          << std::endl;
		return false;
	}

	if ((result = snd_pcm_hw_params(pcmHandle, pcmParams) < 0))
	{
		std::cerr << "AudioOutputALSA snd_pcm_hw_params returned " << result << std::endl;
		return false;
	}

	snd_pcm_hw_params_free(pcmParams);

	isRunning_ = true;
	alsaThread_ = std::make_unique<std::thread>([this, pcmHandle]() {
		while (isRunning_)
		{
			std::unique_lock<std::mutex> lock(audioDataMutex_);

			while (audioData_.empty() && isRunning_)
			{
				audioDataCv_.wait_for(lock, std::chrono::milliseconds(100));
			}

			if (!isRunning_)
			{
				break;
			}

			std::shared_ptr<std::vector<unsigned char>> data = audioData_[0];
			audioData_.erase(audioData_.begin());
			// 1 channel, S16LE, so 2 bytes each frame.
			int frames = data->size() / 2;

			if (int writeRes = snd_pcm_writei(pcmHandle, &(*data.get())[0], frames) < 0)
			{
				if (int result = snd_pcm_recover(pcmHandle, writeRes, 0) < 0)
				{
					std::cerr << "AudioOutputALSA snd_pcm_recover returns " << result << std::endl;
					break;
				}
			}
		}

		// Wait for all data to be consumed.
		snd_pcm_drain(pcmHandle);
		snd_pcm_close(pcmHandle);
	});
	return true;
}

void AudioOutputALSA::stop()
{
	std::unique_lock<std::mutex> lock(isRunningMutex_);

	if (!isRunning_)
	{
		return;
	}

	isRunning_ = false;
	alsaThread_->join();
	alsaThread_.reset(nullptr);
}

void AudioOutputALSA::send(std::shared_ptr<std::vector<unsigned char>> data)
{
	std::unique_lock<std::mutex> lock(audioDataMutex_);
	audioData_.push_back(data);
	audioDataCv_.notify_one();
}
