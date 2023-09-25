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
import csv
import time
from gtts import gTTS

def main():
    
    try:
        srcid = int(sys.argv[1])
    except:
        srcid = 0
    
    callsign = ""
    name = ""
    if (srcid > 0) and os.path.isfile("DMRIds.dat"):
        with open("DMRIds.dat", "r", encoding="utf8", newline="") as ids_file:
            ids_reader = csv.reader(ids_file, delimiter="\t")
            for tmp_id in ids_reader:
                if tmp_id[0] == str(srcid):
                    print(tmp_id)
                    callsign = tmp_id[1]
                    name = tmp_id[2]
                    break
    
    if (not os.path.isfile("DMRIds.dat")) or (time.time() - os.stat("DMRIds.dat").st_mtime > 24*60*60):
        os.system("{ curl --fail -o DMRIds.tmp -s http://www.pistar.uk/downloads/DMRIds.dat && mv DMRIds.tmp DMRIds.dat; } &")
    
    
    openaiurl = "https://api.openai.com/v1"
    
    try:
        with open("openai_api_key.txt", "r") as file:
            openai_api_key = file.read().strip()
    except:
        print("failed to read file openai_api_key.txt")
        exit(1)
    
    headers = { "Authorization": "Bearer " + openai_api_key }


    ########## Speech-to-Text ##########

    print("* Query Whisper API to get Speech-to-Text")
    url = openaiurl + "/audio/transcriptions"
    audio_file_path = "rx.wav"
    
    # trim silence at start and end of file
    if os.system("ffmpeg -y -loglevel error -i " + audio_file_path + " -af 'areverse,silenceremove=start_periods=1:start_duration=0.05:start_silence=0.1:start_threshold=0.02,areverse,silenceremove=start_periods=1:start_duration=0.05:start_silence=0.1:start_threshold=0.02' rx-trim.wav") == 0:
        audio_file_path = "rx-trim.wav"
    else:
        print("failed to call ffmpeg")
    
    data = {
        "model": "whisper-1",
        "response_format": "verbose_json"
    }
    files = {
        "file": open(audio_file_path, "rb")
    }
    
    data["prompt"] = "callsign:" + callsign
    if str(srcid)[0:3] in {"268", "724"}:
        data["prompt"] += ";language:portuguese"
    data["prompt"] = "[" + data["prompt"] + "]"
    #print("prompt=" + data["prompt"])
    
    
    # language auto-lock after user talks with the bot 3 times using the same language
    last_languages = []
    if os.path.isfile("last_languages.json"):
        if time.time() - os.stat("last_languages.json").st_mtime < 300:
            with open("last_languages.json", "r") as read_file:
                last_languages = json.load(read_file)[-3:]
    if len(last_languages) >= 3:
        for i in range(0, len(last_languages)):
            if (last_languages[i]["srcid"] != srcid) or (last_languages[i]["lang"] != last_languages[-1]["lang"]):
                break
        else:
            data["language"] = last_languages[-1]["lang"]
    
    # uncomment the following line to force Speech-to-Text on specific language
    #data["language"] = "pt"
    
    if "language" in data:
        print("Forced Language: " + data["language"])
    

    if os.stat(audio_file_path).st_size > 44+16000*0.5:
        for i in range(3):
            if i > 0:
                time.sleep(2)
                print("retrying...")
            try:
                response = requests.post(url, files=files, data=data, headers=headers, timeout=30)
                print("Status Code:", response.status_code)
                if response.status_code == 200:
                    break
            except requests.exceptions.Timeout:
                print("connection timeout")
            except requests.exceptions.RequestException:
                print("connection error")
        else:
            exit(1)
        
        speech_to_text = response.json()["text"]
        speech_language = str(response.json()["language"] or "english")
        print("Response from Whisper API:", speech_to_text)
        print("Language:", speech_language)
    else:
        print("no rx audio")
        speech_to_text = "answer this text with some variations (talking to the user in a formal way): Hello " + name + ", I am an artificial intelligence assistant, I am here to assist you by providing information and answering your questions. Please speak your questions in clear speech, slowly and formalize them as full questions, just as you would when speaking with another person, avoid to transmit very short sentences like just one or two words because I may have trouble to understand them. How can I help you?"
        if str(srcid)[0:3] in {"268", "724"}:
            speech_language = "portuguese"
        else:
            speech_language = "english"


    ########## Query ChatGPT ##########

    print("* Query ChatGPT model with the text")
    url = openaiurl + "/chat/completions"

    data = {
        "model": "gpt-3.5-turbo",
        "messages": [
        ]
    }
    
    if os.path.isfile("conversation.json"):
        if time.time() - os.stat("conversation.json").st_mtime < 300:
            with open("conversation.json", "r") as read_file:
                data["messages"] = json.load(read_file)[-8:]
    
    if (len(callsign) > 0) and (len(name) > 0):
        youAreTalkingWith = "You (assistant) know that you are talking with a ham radio operator, his name is " + name + ", his callsign is " + callsign + ". "
    elif len(callsign) > 0:
        youAreTalkingWith = "You (assistant) know that you are talking with a ham radio operator, his callsign is " + callsign + ". "
    else:
        youAreTalkingWith = ""

    data["messages"].insert(0,{"role": "system", "content": "You are a voice capable bot, users talk with you using radios, input is processed by speech recognition and output by voice synthetizer. Users may say their callsign on start or end of their communications, ignore that. " + youAreTalkingWith + "Current UTC date/time: " + time.strftime("%Y-%m-%d %H:%M",time.gmtime()) })
    
    # ask for reply in the same language as input
    speech_to_text += " (reply in " + speech_language + ")"
    
    data["messages"].append({"role": "user", "content": speech_to_text})
    
    for i in range(2):
        if i > 0:
            time.sleep(2)
            print("retrying...")
        try:
            response = requests.post(url, json=data, headers=headers, timeout=60)
            print("Status Code:", response.status_code)
            if response.status_code == 200:
                break
        except requests.exceptions.Timeout:
            print("connection timeout")
        except requests.exceptions.RequestException:
            print("connection error")
    else:
        exit(1)
    
    chatgpt_response = response.json()["choices"][0]["message"]["content"]
    print("Response from ChatGPT:", chatgpt_response)
    print("Total Tokens:", response.json()["usage"]["total_tokens"])
    
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
    
    if os.stat(audio_file_path).st_size > 44+16000*2:
        last_languages.append({"srcid": srcid, "lang": tts_lang})
        with open("last_languages.json", "w") as write_file:
            json.dump(last_languages, write_file, indent = 2)
    
    # local language accent
    if tts_lang == "en":
        if str(srcid)[0:3] in {"234", "235"}:
            tts_tld = "co.uk"
        else:
            tts_tld = "us"
    elif tts_lang == "pt":
        if str(srcid)[0:3] == "724":
            tts_tld = "com.br"
        else:
            tts_tld = "pt"
    elif tts_lang == "es":
        tts_tld = "es"
    elif tts_lang == "fr":
        tts_tld = "fr"
    else:
        tts_tld = "com"
    
    print("lang=" + tts_lang + ", tld=" + tts_tld)
    tts = gTTS(chatgpt_response, lang=tts_lang, tld=tts_tld, slow=False)
    
    for i in range(2):
        if i > 0:
            time.sleep(2)
            print("retrying...")
        try:
            tts.save("tx.mp3")
            print("done")
            break
        except:
            print("failed to convert text-to-speech")
    else:
        exit(1)
    
    if os.system("ffmpeg -y -loglevel error -i tx.mp3 -ar 8000 tx.wav") != 0:
        print("failed to call ffmpeg")
        exit(1)

if __name__ == "__main__":
    main()