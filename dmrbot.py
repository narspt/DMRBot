#!/usr/bin/env python3

#    DMRBot - ChatGPT Voice Bot for DMR
#    Copyright (C) 2023 Nuno Silva
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <https://www.gnu.org/licenses/>.

import os
import sys
import requests
import json
import time
from gtts import gTTS

def main():
    openaiurl = "https://api.openai.com/v1"
    
    #if len(sys.argv) > 1:
    #    print(sys.argv[1])
    
    try:
        with open('openai_api_key.txt', 'r') as file:
            openai_api_key = file.read().strip()
    except:
        print("failed to read file openai_api_key.txt")
        exit(1)
    
    headers = { "Authorization" : f"Bearer {openai_api_key}" }


    ########## Speech-to-Text ##########

    print("* Query Whisper API to get Speech-to-Text")
    url = f"{openaiurl}/audio/transcriptions"
    audio_file_path = "rx.wav"

    data = {
        "model": "whisper-1",
        "file": audio_file_path,
        "response_format": "verbose_json"
    }
    files = {
        "file": open(audio_file_path, "rb")
    }

    # uncomment the following line to force Speech-to-Text on specific language
    #data["language"] = "pt"
    
    response = requests.post(url, files=files, data=data, headers=headers)
    print("Status Code:", response.status_code)
    if response.status_code != 200:
        exit(1)
    
    speech_to_text = response.json()["text"]
    speech_language = response.json()["language"]
    print("Response from Whisper API:", speech_to_text)
    print("Language:", speech_language)


    ########## Query ChatGPT ##########

    print("* Query ChatGPT model with the text")
    url = f"{openaiurl}/chat/completions"

    data = {
        "model": "gpt-3.5-turbo",
        "messages": [
        ]
    }
    
    if os.path.isfile("conversation.json"):
        if time.time() - os.stat("conversation.json").st_mtime < 300:
            with open("conversation.json", "r") as read_file:
                data["messages"] = json.load(read_file)[-8:]
    
    data["messages"].insert(0,{"role": "system", "content": "You are a voice capable bot, users talk with you using radios, input is processed by speech recognition and output by voice synthetizer. Users may say their callsign on start or end of their communications, ignore that. Current UTC date/time: " + time.strftime("%Y-%m-%d %H:%M",time.gmtime()) })
    
    # ask for reply in the same language as input
    speech_to_text += " (reply in " + speech_language + ")"
    
    data["messages"].append({"role": "user", "content": speech_to_text})

    response = requests.post(url, json=data, headers=headers)
    print("Status Code:", response.status_code)
    if response.status_code != 200:
        exit(1)
    
    chatgpt_response = response.json()["choices"][0]["message"]["content"]
    print("Response from ChatGPT:", chatgpt_response)
    print("Total Tokens:", response.json()['usage']['total_tokens'])
    
    data["messages"].append({"role": "assistant", "content": chatgpt_response})
    if data["messages"][0]["role"] == "system":
        data["messages"].pop(0)
    with open("conversation.json", "w") as write_file:
        json.dump(data["messages"], write_file, indent = 2)


    ########## Text-to-Speech ##########

    print("* Convert Text-to-Speech from the response")

    lang_code = {
        "afrikaans": "af",
        "arabic": "ar",
        "bulgarian": "bg",
        "bengali": "bn",
        "bosnian": "bs",
        "catalan": "ca",
        "czech": "cs",
        "danish": "da",
        "german": "de",
        "greek": "el",
        "english": "en",
        "spanish": "es",
        "estonian": "et",
        "finnish": "fi",
        "french": "fr",
        "gujarati": "gu",
        "hindi": "hi",
        "croatian": "hr",
        "hungarian": "hu",
        "indonesian": "id",
        "icelandic": "is",
        "italian": "it",
        "hebrew": "iw",
        "japanese": "ja",
        "javanese": "jw",
        "khmer": "km",
        "kannada": "kn",
        "korean": "ko",
        "latin": "la",
        "latvian": "lv",
        "malayalam": "ml",
        "marathi": "mr",
        "malay": "ms",
        "nepali": "ne",
        "dutch": "nl",
        "norwegian": "no",
        "polish": "pl",
        "portuguese": "pt",
        "romanian": "ro",
        "russian": "ru",
        "sinhala": "si",
        "slovak": "sk",
        "albanian": "sq",
        "serbian": "sr",
        "sundanese": "su",
        "swedish": "sv",
        "swahili": "sw",
        "tamil": "ta",
        "telugu": "te",
        "thai": "th",
        "filipino": "tl",
        "turkish": "tr",
        "ukrainian": "uk",
        "urdu": "ur",
        "vietnamese": "vi",
        "chinese": "zh"
    }
    if speech_language.lower() in lang_code:
        tts_lang = lang_code[speech_language.lower()]
    else:
        tts_lang = "en"
    
    lang_tld = {
        "en": "us",
        "fr": "fr",
        "pt": "pt",
        "es": "es"
    }
    if tts_lang in lang_tld:
        tts_tld = lang_tld[tts_lang]
    else:
        tts_tld = "com"

    print(f"lang={tts_lang}, tld={tts_tld}")
    tts = gTTS(chatgpt_response, lang=tts_lang, tld=tts_tld, slow=False)
    
    try:
        tts.save('tx.mp3')
    except:
        print("failed to convert text-to-speech")
        exit(1)
    
    if os.system("ffmpeg -y -loglevel error -i tx.mp3 -ar 8000 tx.wav") != 0:
        print("failed to call ffmpeg")
        exit(1)

    print("done")
    
if __name__ == "__main__":
    main()