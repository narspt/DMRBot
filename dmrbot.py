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
import re
import json
import time
from gtts import gTTS

def get_current_weather(location, units="metric"):

    try:
        url = "https://www.google.com/search"
        params = { "hl": "en", "lr": "lang_en", "ie": "UTF-8", "q": "weather " + location }
        headers = {
            "User-Agent": "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:89.0) Gecko/20100101 Firefox/89.0",
            "Accept-Language": "en-US,en;q=0.5"
        }
        response = requests.get(url, params=params, headers=headers, timeout=60)

        data = dict()
        data["temp"] = float(re.findall("(?:<span.*?id=\"wob_tm\".*?>)(.*?)(?:<\\/span>)", response.text)[0])
        data["weather"] = re.findall("(?:<span.*?id=\"wob_dc\".*?>)(.*?)(?:<\\/span>)", response.text)[0]
        data["precip_prob"] = float(re.findall("(?:<span.*?id=\"wob_pp\".*?>)(.*?)(?:<\\/span>)", response.text)[0].replace("%", ""))
        data["humidity"] = float(re.findall("(?:<span.*?id=\"wob_hm\".*?>)(.*?)(?:<\\/span>)", response.text)[0].replace("%", ""))
        data["wind"] = re.findall("(?:<span.*?id=\"wob_ws\".*?>)(.*?)(?:<\\/span>)", response.text)[0]

        try:
            data["wind_dir"] = int(re.findall("(?:\\\\x3cimg.*?id\\\\x3d\\\\x22wind_image_\\d\\\\x22 style\\\\x3d\\\\x22.*?transform:rotate\\()(\\d+)(?:deg\\))", response.text)[0]) - 90
            compass_direction = ["N","NE","E","SE","S","SW","W","NW"]
            data["wind_dir"] = compass_direction[int((data["wind_dir"]/45)+0.5) % 8]
        except:
            data["wind_dir"] = ""

        try:
            data["location"] = re.findall("(?:<span class=\"BBwThe\">)(.*?)(?:<\\/span>)", response.text)[0]
        except:
            data["location"] = location

        if "km/h" in data["wind"]:
            data["wind"] = float(data["wind"].replace("km/h", ""))
            if units == "imperial":
                data["wind"] = round(data["wind"] / 1.6, 1)
                data["temp"] = round(data["temp"] * (9 / 5) + 32, 1)
        elif "m/s" in data["wind"]:
            data["wind"] = float(data["wind"].replace("m/s", "")) * 3.6
            if units == "imperial":
                data["wind"] = round(data["wind"] / 1.6, 1)
                data["temp"] = round(data["temp"] * (9 / 5) + 32, 1)
        elif "mph" in data["wind"]:
            data["wind"] = float(data["wind"].replace("mph", ""))
            if units == "metric":
                data["wind"] = round(data["wind"] * 1.6, 1)
                data["temp"] = round((data["temp"] - 32) * (5 / 9), 1)
        else:
            data["wind"] = 0

        data["weather"] = data["weather"].replace("Clear", "Clear sky")

        weather_info = {
            "location": data["location"],
            "temperature": round(data["temp"], 1),
            "temperature_unit": "fahrenheit" if units == "imperial" else "celsius",
            "humidity": round(data["humidity"], 0),
            "weather_conditions": data["weather"],
            "precipitation_probability": round(data["precip_prob"], 0),
            "wind_speed": round(data["wind"], 1),
            "wind_unit": "mph" if units == "imperial" else "km/h",
            "wind_direction": data["wind_dir"]
        }
        return json.dumps(weather_info)
    except:
        return json.dumps({"error": "Could not fetch weather for " + location})


def main():
    
    print("* " + time.strftime("%Y-%m-%d %H:%M:%S") + " *")
    
    try:
        srcid = int(sys.argv[1])
    except:
        srcid = 0
    
    callsign = ""
    name = ""
    city = ""
    state = ""
    country = ""
    if (srcid > 0):
        print("* Query radioid.net to get user info")
        for i in range(2):
            if i > 0:
                time.sleep(5)
                print("retrying...")
            try:
                response = requests.get("https://radioid.net/api/dmr/user/?id=" + str(srcid), timeout=20)
                response_result = response.json()["results"][0]
                print(response_result)
                callsign = response_result["callsign"]
                name = response_result["fname"]
                city = response_result["city"]
                state = response_result["state"]
                country = response_result["country"]
                break
            except requests.exceptions.Timeout:
                print("connection timeout")
            except requests.exceptions.RequestException:
                print("connection error")
            except Exception:
                pass
        else:
            print("failed to fetch info from radioid.net")

    # fix cities missing special characters
    if (country == "Portugal") and (city == "Canecas"):
        city = "Cane\u00e7as"


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
    
    # try to hint whisper with callsign, location and probable language
    data["prompt"] = "callsign:" + callsign
    if len(city) > 0:
        data["prompt"] += ";city:" + city
    if len(country) > 0:
        data["prompt"] += ";country:" + country
    if str(srcid)[0:3] in {"268", "724"}:
        data["prompt"] += ";language:portuguese"
    elif str(srcid)[0:3] == "214":
        data["prompt"] += ";language:spanish"
    elif str(srcid)[0:3] == "206":
        data["prompt"] += ";language:dutch"
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
                time.sleep(5)
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
        speech_to_text = ""
        speech_language = ""

    # workaround frequent mistakes
    if str(srcid)[0:3] == "268":
        speech_to_text = speech_to_text.replace("Canessas", "Cane\u00e7as").replace("Canessa", "Cane\u00e7as")
        if speech_language == "welsh":
            speech_language = "portuguese"
    elif str(srcid)[0:3] == "206":
        if speech_language == "afrikaans":
            speech_language = "dutch"
    if speech_language == "maori":
        speech_language = "english"


    ########## Query ChatGPT ##########

    print("* Query ChatGPT model with the text")
    url = openaiurl + "/chat/completions"

    tools = [
        {
            "type": "function",
            "function": {
                "name": "get_current_weather",
                "description": "Get the current weather in a given location",
                "parameters": {
                    "type": "object",
                    "properties": {
                        "location": {"type": "string", "description": "The city and state, e.g. San Francisco, California."}
                    },
                    "required": ["location"]
                }
            }
        }
    ]

    data = {
        "model": "gpt-3.5-turbo",
        "messages": [],
        "tools": tools
    }
    
    if os.path.isfile("conversation.json"):
        if time.time() - os.stat("conversation.json").st_mtime < 300:
            with open("conversation.json", "r") as read_file:
                data["messages"] = json.load(read_file)[-8:]

    if len(speech_to_text) < 1:
        if (len(data["messages"]) >= 2) and (data["messages"][-2]["role"] == "tool"):
            speech_to_text = "answer this text with some variations (talking to the user in a formal way): Hello " + name + ", I am an artificial intelligence assistant, I am here to assist you by providing information and answering your questions. Please speak your questions in clear speech, slowly and formalize them as full questions, just as you would when speaking with another person, avoid to transmit very short sentences like just one or two words because I may have trouble to understand them. How can I help you?"
        else:
            speech_to_text = "Hello"
            if len(name) > 0:
                speech_to_text += ", I'm " + name
            if len(city) > 0:
                speech_to_text += ", How is the current weather in " + city
                if (str(srcid)[0:3] in {"310","311","312","313","314","315","316","317", "724"}) and (len(state) > 0):
                    speech_to_text += ", " + state
                if (len(country) > 0):
                    speech_to_text += ", " + country
                speech_to_text += "?"
            speech_to_text += " (suffix your response by asking user if you can supply any other information)"
        if str(srcid)[0:3] in {"268", "724"}:
            speech_language = "portuguese"
        elif str(srcid)[0:3] == "214":
            speech_language = "spanish"
        elif str(srcid)[0:3] == "206":
            speech_language = "dutch"
        else:
            speech_language = "english"

    youAreTalkingWith = "You (assistant) know that you are talking with a ham radio operator"
    if len(name) > 0:
        youAreTalkingWith += ", his name is " + name
    if len(callsign) > 0:
        youAreTalkingWith += ", his callsign is " + callsign
    if len(city) > 0:
        youAreTalkingWith += ", he is located at " + city
        if (str(srcid)[0:3] in {"310","311","312","313","314","315","316","317", "724"}) and (len(state) > 0):
            youAreTalkingWith += ", " + state
        if (len(country) > 0):
            youAreTalkingWith += ", " + country
    youAreTalkingWith += ". "

    data["messages"].insert(0,{"role": "system", "content": "You are a voice capable bot, users talk with you using radios, input is processed by speech recognition and output by voice synthetizer. Users may say their callsign on start or end of their communications, ignore that. " + youAreTalkingWith + "If asked for information about a location, answer in detail but don't include current weather information unless user explicitly asks for that. Current UTC date/time: " + time.strftime("%Y-%m-%d %H:%M",time.gmtime()) })
    #print("system_message=" + data["messages"][0]["content"])
    
    # ask for reply in the same language as input
    speech_to_text += " (reply in " + speech_language + ")"
    
    data["messages"].append({"role": "user", "content": speech_to_text})
    
    response = None
    def post_to_chatgpt_api():
        nonlocal response
        for i in range(2):
            if i > 0:
                time.sleep(5)
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
    post_to_chatgpt_api()
    
    if response.json()["choices"][0]["message"].get("tool_calls"):
        data["messages"].append(response.json()["choices"][0]["message"])
        for tool_call in response.json()["choices"][0]["message"]["tool_calls"]:
            tool_response = ""
            if tool_call.get("function"):
                function_name = tool_call["function"]["name"]
                function_args = json.loads(tool_call["function"]["arguments"])
                print("function: " + function_name + ", arguments: " + json.dumps(function_args))
                if function_name == "get_current_weather":
                    units = "imperial" if str(srcid)[0:3] in {"310","311","312","313","314","315","316","317"} else "metric"
                    tool_response = get_current_weather(function_args.get("location", ""), units)
            print(tool_response)
            data["messages"].append({"role": "tool", "tool_call_id": tool_call["id"], "content": tool_response})
        del data["tools"]
        post_to_chatgpt_api()
    
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
        "galician": "gl",
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
    
    # workaround for unavailable tts
    if tts_lang == "gl":
        tts_lang = "es"
    
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
            time.sleep(5)
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