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

#include <grpc++/grpc++.h>

#include <fstream>
#include <getopt.h>
#include <sstream>
#include <string>

#include "assistant_config.h"
#include "base64_encode.h"
#include "embedded_assistant.grpc.pb.h"

namespace assistant = google::assistant::embedded::v1alpha2;

using assistant::AssistRequest;
using assistant::AssistResponse;
using assistant::AssistResponse_EventType_END_OF_UTTERANCE;
using assistant::AudioInConfig;
using assistant::AudioOutConfig;
using assistant::EmbeddedAssistant;
using assistant::ScreenOutConfig;

const char* DEFAULT_LOCALE = "en-US";
const char* DEVICE_MODEL_ID = "myassistant-fd356-myassistant-0zwd5h";
const char* DEVICE_INSTANCE_ID = "default";

// Creates a channel to be connected to Google.
std::shared_ptr<grpc::Channel> createChannel(const std::string& host)
{
	return grpc::CreateChannel(host, grpc::SslCredentials(grpc::SslCredentialsOptions()));
}

void printUsage()
{
	std::cerr << "Usage: ./run_assistant_text "
	          << "--credentials <credentials_file> "
	          << "[--api_endpoint <API endpoint>] "
	          << "[--locale <locale>]"
	          << "[--html_out <command to load HTML file>]" << std::endl;
}

bool getCommandLineFlags(int argc,
                         char** argv,
                         std::string* credentials_file_path,
                         std::string* locale,
                         std::string* html_out_command)
{
	const struct option long_options[] = {{"credentials", required_argument, nullptr, 'c'},
	                                      {"locale", required_argument, nullptr, 'l'},
	                                      {"html_out", required_argument, nullptr, 'h'},
	                                      {nullptr, 0, nullptr, 0}};
	while (true)
	{
		int option_index;
		int option_char = getopt_long(argc, argv, "c:l:h", long_options, &option_index);
		if (option_char == -1)
		{
			break;
		}
		switch (option_char)
		{
		case 'c':
			*credentials_file_path = optarg;
			break;
		case 'l':
			*locale = optarg;
			break;
		case 'h':
			*html_out_command = optarg;
			break;
		default:
			printUsage();
			return false;
		}
	}
	return true;
}

std::shared_ptr<grpc::CallCredentials> readCredentials(const std::string& credentialsFilePath)
{
	std::ifstream credentialsFile(credentialsFilePath);
	if (!credentialsFile)
	{
		std::cerr << "Credentials file \"" << credentialsFilePath << "\" does not exist."
		          << std::endl;
		return nullptr;
	}

	std::stringstream buffer;
	buffer << credentialsFile.rdbuf();
	std::string credentials = buffer.str();

	auto callCredentials = grpc::GoogleRefreshTokenCredentials(credentials);
	if (callCredentials == nullptr)
	{
		std::cerr << "Credentials file \"" << credentialsFilePath
		          << "\" is invalid. Check step 5 in README for how to get valid "
		          << "credentials." << std::endl;
		return nullptr;
	}
	return callCredentials;
}

int main(int argc, char** argv)
{
	grpc_init();

	std::string credentialsFilePath, locale, htmlOutCommand;
	if (!getCommandLineFlags(argc, argv, &credentialsFilePath, &locale, &htmlOutCommand))
	{
		return -1;
	}

	if (locale.empty())
	{
		locale = DEFAULT_LOCALE; // Default locale
	}
	std::cout << "Using locale " << locale << std::endl;

	// Read credentials file.
	auto callCredentials = readCredentials(credentialsFilePath);
	if (callCredentials == nullptr)
		return -1;

	// Connect a stream.
	auto channel = createChannel(ASSISTANT_ENDPOINT);
	auto assistant = EmbeddedAssistant::NewStub(channel);

	grpc::ClientContext context;
	context.set_fail_fast(false);
	context.set_credentials(callCredentials);

	// Create bidirectional stream
	auto stream = assistant->Assist(&context);

	std::string inputText;
	while (std::getline(std::cin, inputText))
	{
		// Create an AssistRequest
		AssistRequest request;
		auto* assistConfig = request.mutable_config();

		// Set the DialogStateIn of the AssistRequest
		assistConfig->mutable_dialog_state_in()->set_language_code(locale);

		// Set the DeviceConfig of the AssistRequest
		assistConfig->mutable_device_config()->set_device_id(DEVICE_INSTANCE_ID);
		assistConfig->mutable_device_config()->set_device_model_id(DEVICE_MODEL_ID);

		// Set parameters for audio output
		assistConfig->mutable_audio_out_config()->set_encoding(AudioOutConfig::LINEAR16);
		assistConfig->mutable_audio_out_config()->set_sample_rate_hertz(16000);
		assistConfig->set_text_query(inputText);

		// Set parameters for screen config
		assistConfig->mutable_screen_out_config()->set_screen_mode(
		    htmlOutCommand.empty() ? ScreenOutConfig::SCREEN_MODE_UNSPECIFIED :
		                             ScreenOutConfig::PLAYING);

		// Write config in first stream.
		std::cout << "tx: " << request.Utf8DebugString() << std::endl;
		stream->Write(request);

		// Read responses.
		AssistResponse response;
		while (stream->Read(&response)) // Returns false when no more to read.
		{
			for (int i = 0; i < response.speech_results_size(); i++)
			{
				const auto& result = response.speech_results(i);
				std::cout << "assistant_sdk request: \n"
				          << result.transcript() << " (" << std::to_string(result.stability())
				          << ")" << std::endl;
			}
			if (!htmlOutCommand.empty() && !response.screen_out().data().empty())
			{
				std::string html_out_base64 = base64_encode(response.screen_out().data());
				system(
				    (htmlOutCommand + " \"data:text/html;base64, " + html_out_base64 + "\"").c_str());
			}
			else if (htmlOutCommand.empty())
			{
				if (!response.dialog_state_out().supplemental_display_text().empty())
				{
					std::cout << "assistant_sdk response:" << std::endl;
					std::cout << response.dialog_state_out().supplemental_display_text() << std::endl;
				}
			}
		}

		grpc::Status status = stream->Finish();
		if (!status.ok())
		{
			// Report the RPC failure.
			std::cerr << "assistant_sdk failed, error: " << status.error_message() << std::endl;
			return -1;
		}
	}

	return 0;
}
