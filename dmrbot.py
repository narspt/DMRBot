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

def get_current_weather(location, latitude, longitude, units="metric", forecast=False):

    try:
        with open("openweathermap_api_key.txt", "r") as file:
            openweathermap_api_key = file.read().strip()
    except:
        print("failed to read file openweathermap_api_key.txt")
        return json.dumps({"error": "Could not fetch weather for " + location})

    if re.search(",\\s[A-Z]{2}$", location):
        url = "https://api.openweathermap.org/geo/1.0/direct?q=" + location + "&limit=1&appid=" + openweathermap_api_key
        response = requests.get(url, timeout=60)
        if response.status_code == 200:
            data = response.json()
            if len(data) > 0:
                latitude = str(data[0]['lat'])
                longitude = str(data[0]['lon'])
                print("latitude=" + latitude + ", longitude=" + longitude)

    url = "https://api.openweathermap.org/data/2.5/weather?lat=" + latitude + "&lon=" + longitude + "&appid=" + openweathermap_api_key + "&units=" + units
    response = requests.get(url, timeout=60)
    if response.status_code == 200:
        data = response.json()
        #print(json.dumps(data, indent=2))

        # convert m/s to km/h
        if units == "metric":
            data['wind']['speed'] *= 3.6
            if "gust" in data['wind']:
                data['wind']['gust'] *= 3.6

        data['wind']['dir'] = round(data['wind']['deg'])
        compass_direction = ["N","NE","E","SE","S","SW","W","NW"]
        data['wind']['dir'] = compass_direction[int((data['wind']['dir']/45)+0.5) % 8]

        weather_info = {
            "location": data['name'] + ", " + data['sys']['country'],
            "temperature": round(data['main']['temp']),
            "temperature_unit": "fahrenheit" if units == "imperial" else "celsius",
            "humidity": round(data['main']['humidity']),
            "weather_conditions": data['weather'][0]['description'],
            "wind_speed": round(data['wind']['speed']),
            "wind_gust": round(data['wind']['gust']) if "gust" in data['wind'] else None,
            "wind_speed_unit": "miles per hour" if units == "imperial" else "kilometres per hour",
            "wind_direction": data['wind']['dir'],
            "pressure": round(data['main']['pressure']),
            "pressure_unit": "milibars"
        }

        if forecast:
            url = "https://api.openweathermap.org/data/2.5/forecast?lat=" + latitude + "&lon=" + longitude + "&cnt=32&appid=" + openweathermap_api_key + "&units=" + units
            response = requests.get(url, timeout=60)
            if response.status_code == 200:
                data = response.json()
                #print(json.dumps(data, indent=2))
                forecast_data = []
                for item in data['list']:
                    forecast_data.append({'time': time.strftime("%Y-%m-%d %H:%M", time.gmtime(item['dt']+data['city']['timezone'])), 'weather': item['weather'][0]['description'], 'temp': round(item['main']['temp'])})
                #print(json.dumps(forecast_data, indent=2))
                weather_info["forecast"] = forecast_data

        return json.dumps(weather_info)
    else:
        return json.dumps({"error": "Could not fetch weather for " + location})


def main():
    
    print("* " + time.strftime("%Y-%m-%d %H:%M:%S") + " *")
    
    srcid = 0
    callsign = ""
    try:
        if sys.argv[1].isnumeric():
            srcid = int(sys.argv[1])
        else:
            callsign = sys.argv[1].strip()
    except:
        pass
    
    name = ""
    city = ""
    state = ""
    country = ""
    if (srcid > 0) or (len(callsign) > 0):
        print("* Query radioid.net to get user info")
        for i in range(2):
            if i > 0:
                time.sleep(5)
                print("retrying...")
            try:
                if (srcid > 0):
                    response = requests.get("https://radioid.net/api/dmr/user/?id=" + str(srcid), timeout=20)
                    response_result = response.json()["results"][0]
                    print(response_result)
                    callsign = response_result["callsign"]
                elif (len(callsign) > 0):
                    response = requests.get("https://radioid.net/api/dmr/user/?callsign=" + callsign, timeout=20)
                    response_result = response.json()["results"][0]
                    print(response_result)
                    srcid = int(response_result["id"])
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
    if (country == "Spain") and (city == "Murcia"):
        city = "M\u00farcia"
    if (country == "Spain") and (city == "La Guardia de Jan"):
        city = "La Guardia de Ja\u00e9n"
    if (country == "Spain") and (state == "Albacete"):
        city = "Albacete"


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
    
    mccLang = None
    if str(srcid)[:3] in {"268", "724"}:
        mccLang = "portuguese"
    elif int(str(srcid)[:3]) in {214,722,736,730,732,712,368,706,740,704,627,708,334,710,714,744,716,330,370,748,734}:
        mccLang = "spanish"
    elif str(srcid)[:3] == "208":
        mccLang = "french"
    elif str(srcid)[:3] in {"262", "263", "264", "265", "232"}:
        mccLang = "german"
    elif str(srcid)[:3] in {"204", "206"}:
        mccLang = "dutch"
    elif str(srcid)[:3] in {"222", "223", "224"}:
        mccLang = "italian"
    elif str(srcid)[:3] == "202":
        mccLang = "greek"
    elif str(srcid)[:3] == "250":
        mccLang = "russian"
    elif str(srcid)[:3] == "255":
        mccLang = "ukrainian"
    elif str(srcid)[:3] == "260":
        mccLang = "polish"
    elif str(srcid)[:3] == "284":
        mccLang = "bulgarian"
    elif str(srcid)[:3] == "286":
        mccLang = "turkish"
    elif str(srcid)[:3] in {"460", "461"}:
        mccLang = "chinese"
    elif str(srcid)[:3] in {"440", "441"}:
        mccLang = "japanese"
    elif str(srcid)[:3] == "450":
        mccLang = "korean"
    
    # try to hint whisper with callsign, location and probable language
    data["prompt"] = "callsign:" + callsign
    if len(city) > 0:
        data["prompt"] += ";city:" + city
    if len(country) > 0:
        data["prompt"] += ";country:" + country
    if mccLang is not None:
        data["prompt"] += ";language:" + mccLang
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
    
    ### Uncomment the following line to always force speech-to-text language ###
    #data["language"] = "pt"
    
    ### Always force speech-to-text language for specific src mcc ###
    #if str(srcid)[:3] in {"268", "724"}:
    #    data["language"] = "pt"
    
    ### Always force speech-to-text language for specific src ids ###
    if srcid in {2681009, 2680237, 2683226}:
        data["language"] = "pt"
    
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

    # workaround frequent whisper mistakes
    if mccLang in {"portuguese", "spanish"}:
        if speech_language == "galician":
            if str(callsign)[:3] not in {"EA1", "EB1", "EC1"}:
                speech_language = mccLang
    if mccLang == "dutch":
        if speech_language == "afrikaans":
            speech_language = mccLang
    if speech_language in {"maori", "latin", "welsh", "nynorsk"}:
        if mccLang is not None:
            speech_language = mccLang
        else:
            speech_language = "english"
    if (speech_to_text == "...") or speech_to_text.endswith("Amara.org") or ("cctexas.com" in speech_to_text) or \
       speech_to_text.startswith("http://") or speech_to_text.startswith("https://") or speech_to_text.startswith("www."):
        speech_to_text = "";


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
                       #"location": {"type": "string", "description": "The city and state, e.g. San Francisco, California."},
                       #"location": {"type": "string", "description": "The city, state (optional) and country, e.g. San Francisco, California, USA."},
                        "location": {"type": "string", "description": "The city, state code (optional) and country code, codes must be two-letter ISO3166, e.g. San Francisco, CA, US."},
                        "latitude": {"type": "string", "description": "Latitude of the location, e.g. 37.7792588"},
                        "longitude": {"type": "string", "description": "Longitude of the location, e.g. -122.4193286"},
                        "forecast": {"type": "boolean", "description": "Include forecast for next days."}
                    },
                    "required": ["location", "latitude", "longitude", "forecast"]
                }
            }
        }
    ]

    data = {
        #"model": "gpt-3.5-turbo",
        #"model": "gpt-4o-mini",
        "model": "gpt-4o",
        "messages": [],
        "tools": tools
    }
    
    if os.path.isfile("conversation.json"):
        if time.time() - os.stat("conversation.json").st_mtime < 300:
            with open("conversation.json", "r") as read_file:
                data["messages"] = json.load(read_file)

    if len(speech_to_text) < 1:
        speech_to_text = "Hello"
        if len(name) > 0:
            speech_to_text += ", I'm " + name
        if len(city) > 0:
            speech_to_text += ", How is the current weather in " + city
            if (str(srcid)[:3] in {"310","311","312","313","314","315","316","317","318","319","320", "724"}) and (len(state) > 0):
                speech_to_text += ", " + state
            if (len(country) > 0):
                speech_to_text += ", " + country
            speech_to_text += "?"
        speech_to_text += " (prefix your response with greeting and suffix it by asking user if you can supply any other information)"
        if (len(data["messages"]) >= 4):
            if (data["messages"][-2]["role"] == "tool") and str(data["messages"][-4].get("content", "") or "").startswith(speech_to_text):
                speech_to_text = "answer this text with some variations (talking to the user in a formal way): Hello " + name + ", I am an artificial intelligence assistant, I am here to assist you by providing information and answering your questions. Please speak your questions in clear speech, slowly and formalize them as full questions, just as you would when speaking with another person, avoid transmitting very short sentences like just one or two words because I may have trouble understanding them. How can I help you?"
        if mccLang is not None:
            speech_language = mccLang
        else:
            speech_language = "english"

    for msg in data["messages"].copy():
        if msg["role"] in {"system","tool"}:
            data["messages"].remove(msg)
        elif (msg["role"] == "assistant") and (msg.get("tool_calls")):
            data["messages"].remove(msg)
    data["messages"] = data["messages"][-8:]

    sm = "You are a voice-capable assistant, users talk to you using radios, input is processed by speech recognition and output by voice synthesizer. "
    if not data["model"].startswith("gpt-3.5"):
        sm += "Provide continuous text responses without markdown formatting such as bold or lists, suitable for voice synthesizer reading. "
    sm += "Users may say their callsign at the start or end of their communications, ignore that. "
    sm += "You know that you are talking to a ham radio operator"
    if len(name) > 0:
        sm += ", user's name is " + name
    if len(callsign) > 0:
        sm += ", user's callsign is " + callsign
    if len(city) > 0:
        sm += ", user is located in " + city
        if (str(srcid)[:3] in {"310","311","312","313","314","315","316","317","318","319","320", "724"}) and (len(state) > 0):
            sm += ", " + state
        if (len(country) > 0):
            sm += ", " + country
    sm += ". "
    sm += "If asked for information about a location, answer in detail but don't include current weather information unless the user explicitly asks for it. "
    sm += "Current UTC date/time: " + time.strftime("%Y-%m-%d %H:%M",time.gmtime())
    data["messages"].insert(0,{"role": "system", "content": sm})
    
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
                print(response.text)
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
                try:
                    function_args = json.loads(tool_call["function"]["arguments"])
                except:
                    function_args = {}
                print("function: " + function_name + ", arguments: " + json.dumps(function_args))
                if function_name == "get_current_weather":
                    units = "imperial" if str(srcid)[:3] in {"310","311","312","313","314","315","316","317","318","319","320"} else "metric"
                    tool_response = get_current_weather(function_args.get("location", ""), function_args.get("latitude", ""), function_args.get("longitude", ""), units, function_args.get("forecast", False))
            print(tool_response)
            data["messages"].append({"role": "tool", "tool_call_id": tool_call["id"], "content": tool_response})
        del data["tools"]
        post_to_chatgpt_api()
    
    chatgpt_response = response.json()["choices"][0]["message"]["content"]

    # remove markdown formatting
    chatgpt_response = re.sub("(?m)^### ", "", chatgpt_response)
    chatgpt_response = re.sub("\\*\\*(.*?)\\*\\*", "\\1", chatgpt_response)

    print("Response from ChatGPT:", chatgpt_response)
    print("Total Tokens:", response.json()["usage"]["total_tokens"])
    
    data["messages"].append({"role": "assistant", "content": chatgpt_response})
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
    
    # local language accent
    if tts_lang == "en":
        if str(srcid)[:3] in {"234", "235"}:
            tts_tld = "co.uk"
        else:
            tts_tld = "us"
    elif tts_lang == "pt":
        if str(srcid)[:3] == "724":
            tts_tld = "com.br"
        else:
            tts_tld = "pt"
    elif tts_lang == "es":
        tts_tld = "es"
    elif tts_lang == "fr":
        tts_tld = "fr"
    else:
        tts_tld = "com"
    
    # workaround google tts mistakes
    chatgpt_response = chatgpt_response.replace("\u00b0C.", "\u00b0C .")
    if tts_lang == "pt":
        chatgpt_response = chatgpt_response.replace("km/h", "quil\u00f4metros por hora")
    
    print("lang=" + tts_lang + ", tld=" + tts_tld)
    tts = gTTS(chatgpt_response, lang=tts_lang, tld=tts_tld, slow=False, lang_check=False)
    
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