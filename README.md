# Google Assistant SDK for devices - C++

## Requirements

This project is officially supported on Ubuntu 14.04. Other Linux distributions may be able to run
this sample.

Refer to the [Assistant SDK documentation](https://developers.google.com/assistant/sdk/) for more information.

## Setup instructions

### Build Project

1. Install dependencies
```
sudo apt-get install build-essential cmake libtool curl unzip pkg-config
sudo apt-get install libasound2-dev  # For ALSA sound output
sudo apt-get install libcurl4-openssl-dev # CURL development library
```

2. Clone this project and submodules
```
git clone https://github.com/UltraCoderRU/my-assistant.git
cd my-assistant
git submodule update --init --recursive
```

3. Configure and build project
```
mkdir build
cd build
cmake -DUSE_SYSTEM_SSL=OFF .. # Change to 'ON', if you want to link with system OpenSSL/LibreSSL/etc.
cmake --build . -- -jN # N = number of cores
```

6. Get credentials file. It must be an end-user's credentials.

* Go to the [Actions Console](https://console.actions.google.com/) and register your device model, following [these instructions](https://developers.google.com/assistant/sdk/guides/library/python/embed/register-device)
* Move it to project folder and rename it to `client_secret.json`
* run `get_credentials.sh` in this folder. It will create the file `credentials.json`.

7. Start one of the `run_assistant` samples:

On a Linux workstation, you can use ALSA for audio input:

```bash
./run_assistant_audio --credentials ../credentials.json
```

You can use a text-based query instead of audio. This allows you to continually enter text queries to the Assistant.

```bash
./run_assistant_text --credentials ../credentials.json
```

This takes input from `cin`, so you can send data to the program when it starts.

```bash
echo "what time is it" | ./run_assistant_text --credentials ../credentials.json
```

To change the locale, include a `locale` parameter:

```bash
echo "Привет" | ./run_assistant_text --credentials ./credentials.json --locale "ru-RU"
```

## Enabling screen output

To get a visual output from the Assistant, provide a command to be run alongside every step of the conversation. It will execute that command along along with a provided argument of a temporary HTML file.

```bash
echo "what time is it" | ./run_assistant_text --credentials ../credentials.json --html_out google-chrome
```

After you enter text, it will run `google-chrome /tmp/google-assistant-cpp-screen-out.html`.
If you prefer a different program, use that argument instead.
