# DMRBot - ChatGPT Voice Bot for DMR
This software connects to a DMR "master" (using MMDVM protocol) and provides a ChatGPT voice capable bot.

OpenAI Whisper API is used for speech recognition and Google Translate's text-to-speech API for speech synthesis. Speech recognition supports multiple languages, though currently it seems to work better for English, for other languages it seems to be problematic with very short sentences like just one or two words, when talking with the bot it's always better to formalize normal/polite questions as you would do when speaking with another person, the more words you say the best the speech recognition will work.

# How to get your own OpenAI API keys
1. Log in (or Sign up) to the OpenAI Platform website at https://platform.openai.com/
2. Click your profile icon at the top-right corner of the page and select "View API Keys".
3. Click "Create new secret key" to generate a new API key.
4. Create a file named "openai_api_key.txt" with your API key on the DMRBot directory.

# Requirements
Python is required to run the dmrbot.py helper script, also gTTS python package must be installed:
```
pip install gTTS
```

# Build
Main program is a single C file, no makefile is required. To build, simply run gcc:
```
gcc -o dmrbot dmrbot.c
```

# Usage
```
./dmrbot [CALLSIGN] [DMRID] [DMRHost1IP:PORT:TG:PW] [AMBEServerIP:PORT]
```

# Notes
If you have no AMBEServer available you may use md380-emu from this repository: https://github.com/narspt/md380tools

You need to run this on an ARM cpu system like Raspberry Pi (or use QEMU on x86_64), to compile it you should just need to "cd emulator" and run "make", then you can get the "md380-emu" standalone binary and run it (with "-s port" parameter) to create an emulated AMBEServer.

Please note that despite the source code available on the above mentioned repository doesn't itself infringe patents, after compiling it (along with MD380 firmware) or using this emulator may infringe patent rights in your jurisdiction, you are strongly advised to check for any patent restrictions or licencing requirements before compiling or using this!
