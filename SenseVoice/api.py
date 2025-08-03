# Set the device with environment, default is cuda:0
# export SENSEVOICE_DEVICE=cuda:1

import os, re
from fastapi import FastAPI, File, Form
from fastapi.responses import HTMLResponse
from typing_extensions import Annotated
from typing import List
from enum import Enum
import torchaudio
from model import SenseVoiceSmall
from funasr.utils.postprocess_utils import rich_transcription_postprocess
from io import BytesIO


class Language(str, Enum):
    auto = "auto"
    zh = "zh"
    en = "en"
    yue = "yue"
    ja = "ja"
    ko = "ko"
    nospeech = "nospeech"

model_dir = "iic/SenseVoiceSmall"
m, kwargs = SenseVoiceSmall.from_pretrained(model=model_dir, device=os.getenv("SENSEVOICE_DEVICE", "cuda:0"))
m.eval()

regex = r"<\|.*\|>"

app = FastAPI()

@app.get("/health")
async def health_check():
    """
    健康检查端点，用于Qt应用检测服务状态
    """
    return {"status": "ok", "service": "SenseVoice", "version": "1.0"}

@app.get("/")
async def root():
    return {"message": "SenseVoice API is running"}

@app.post("/api/v1/asr")
async def turn_audio_to_text(files: Annotated[List[bytes], File(description="wav or mp3 audios in 16KHz")], keys: Annotated[str, Form(description="name of each audio joined with comma")], lang: Annotated[Language, Form(description="language of audio content")] = "auto"):
    audios = []
    audio_fs = 0
    for file in files:
        file_io = BytesIO(file)
        data_or_path_or_list, audio_fs = torchaudio.load(file_io)
        data_or_path_or_list = data_or_path_or_list.mean(0)
        audios.append(data_or_path_or_list)
        file_io.close()
    if lang == "":
        lang = "auto"
    if keys == "":
        key = ["wav_file_tmp_name"]
    else:
        key = keys.split(",")
    res = m.inference(
        data_in=audios,
        language=lang, # "zh", "en", "yue", "ja", "ko", "nospeech"
        use_itn=True,  #输出结果中是否包含标点与逆文本正则化。
        ban_emo_unk=False,
        key=key,
        fs=audio_fs,
        **kwargs,
    )
    if len(res) == 0:
        return {"result": []}
    
    print("=" * 60)
    print("🎤 SenseVoice 识别结果调试信息")
    print("=" * 60)
    
    for it in res[0]:
        print(f"📍 处理音频: {it.get('key', 'unknown')}")
        print(f"🔤 原始识别文本: {repr(it['text'])}")
        
        it["raw_text"] = it["text"]
        it["clean_text"] = re.sub(regex, "", it["text"], 0, re.MULTILINE)
        print(f"🧹 清理标记后: {repr(it['clean_text'])}")
        
        it["text"] = rich_transcription_postprocess(it["text"])
        print(f"✨ 最终处理结果: {repr(it['text'])}")
        print("-" * 40)
    
    print("📤 返回给客户端的完整响应:")
    result = {"result": res[0]}
    print(result)
    print("=" * 60)
    
    return result
