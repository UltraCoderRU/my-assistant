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

#ifndef SRC_ASSISTANT_AUDIO_INPUT_H_
#define SRC_ASSISTANT_AUDIO_INPUT_H_

#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

// Base class for audio input. Input data should be mono, s16_le, 16000kz.
// This class uses a separate thread to send audio data to listeners.
class AudioInput
{
public:
	virtual ~AudioInput() = default;

	// Listeners might be called in different thread.
	void addDataListener(std::function<void(std::shared_ptr<std::vector<unsigned char>>)> listener)
	{
		dataListeners_.push_back(std::move(listener));
	}
	void addStopListener(std::function<void()> listener)
	{
		stopListeners_.push_back(std::move(listener));
	}

	// This thread should:
	// 1. Initialize necessary resources;
	// 2. If |is_running_| is still true, keep sending audio data;
	// 3. Finalize necessary resources.
	// 4. Call |OnStop|.
	virtual std::unique_ptr<std::thread> getBackgroundThread() = 0;

	// Asynchronously starts audio input. Starts internal thread to send audio.
	void start()
	{
		std::unique_lock<std::mutex> lock(isRunningMutex_);
		if (isRunning_)
		{
			return;
		}

		sendThread_ = std::move(getBackgroundThread());
		isRunning_ = true;
	}

	// Synchronously stops audio input.
	void stop()
	{
		std::unique_lock<std::mutex> lock(isRunningMutex_);
		if (!isRunning_)
		{
			return;
		}
		isRunning_ = false;
		// |send_thread_| might have finished.
		if (sendThread_->joinable())
		{
			sendThread_->join();
		}
	}

	// Whether audio input is being sent to listeners.
	bool isRunning()
	{
		std::unique_lock<std::mutex> lock(isRunningMutex_);
		return isRunning_;
	}

protected:
	// Function to call when audio input is stopped.
	void onStop()
	{
		for (auto& listener : stopListeners_)
		{
			listener();
		}
	}

	// Listeners which will be called when audio input data arrives.
	std::vector<std::function<void(std::shared_ptr<std::vector<unsigned char>>)>> dataListeners_;

	// Whether audio input is being sent to listeners.
	bool isRunning_ = false;

private:
	std::vector<std::function<void()>> stopListeners_;
	std::mutex isRunningMutex_;
	std::unique_ptr<std::thread> sendThread_;
};

#endif // SRC_ASSISTANT_AUDIO_INPUT_H_
