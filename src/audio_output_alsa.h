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

#ifndef SRC_ASSISTANT_AUDIO_OUTPUT_ALSA_H_
#define SRC_ASSISTANT_AUDIO_OUTPUT_ALSA_H_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

// Audio output using ALSA.
class AudioOutputALSA
{
public:
	bool start();

	void stop();

	void send(std::shared_ptr<std::vector<unsigned char>> data);

	// friend void fill_audio(void* userdata, unsigned char* stream, int len);

private:
	std::vector<std::shared_ptr<std::vector<unsigned char>>> audioData_;
	std::mutex audioDataMutex_;
	std::condition_variable audioDataCv_;
	std::unique_ptr<std::thread> alsaThread_;
	bool isRunning_ = false;
	std::mutex isRunningMutex_;
};

#endif // SRC_ASSISTANT_AUDIO_OUTPUT_ALSA_H_
