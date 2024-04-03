# DMRBot - ChatGPT Voice Bot for DMR
This software connects to a DMR "master" (using MMDVM protocol) and provides a ChatGPT voice-capable AI bot that can answer questions and provide information. The software consists of a main program, which is essentially a DMR client responsible for connecting to the master and decoding/encoding audio using AMBEServer, and a Python helper script to process the received audio and interface with remote APIs.

OpenAI Whisper API is used for speech recognition and Google Translate's text-to-speech API for speech synthesis. Speech recognition supports multiple languages and the bot is ready to answer in the same language the user speaks. Speech recognition tends to perform better in English, for other languages it may sometimes be problematic with very short sentences, such as those with only one or two words. When interacting with the bot, it is always better to speak clearly and formalize normal/polite questions, as you would when speaking to another person, the more words you say the better the speech recognition will function.

An additional feature is that if the user transmits for 1 second without speaking, the bot will default to replying with the current weather information for the user's location.

You can test the bot live at BrandMeister TG 268940 or TG 214990, as well as on FreeDMR TG 21490.

# How to get your own OpenAI API keys
1. Log in (or Sign up) to the OpenAI Platform website at https://platform.openai.com/
2. Click your profile icon at the top-right corner of the page and select "View API Keys".
3. Click "Create new secret key" to generate a new API key.
4. Create a file named "openai_api_key.txt" with your API key on the DMRBot directory.

# Requirements
Python 3.6+ is required to run the dmrbot.py helper script, also gTTS python package must be installed:
```
pip3 install gTTS
```
ffmpeg 4.1+ is also required.

# Build
Main program is a single C file, no makefile is required. To build, simply run gcc:
```
gcc -o dmrbot dmrbot.c
```

# Usage
```
./dmrbot [CALLSIGN] [DMRID] [DMRHostIP:PORT:TG:PW] [AMBEServerIP:PORT]
```

# Additional Notes
If you do not have an AMBEServer available, you may use md380-emu for testing purposes, from this repository: https://github.com/narspt/md380tools

You need to run md380-emu on an ARM cpu system like Raspberry Pi (or use qemu-user on x86_64), to compile it you should just need to "cd emulator" and run "make", then you can get the "md380-emu" standalone binary and run it (with "-s port" parameter) to create an emulated AMBEServer.

Please be aware that although the source code in the aforementioned repository does not infringe any patents on its own, compiling it (together with MD380 firmware) or using the resulting emulator may violate patent rights in your jurisdiction. It is strongly recommended to verify any patent restrictions or licensing requirements before proceeding with compilation or use!
