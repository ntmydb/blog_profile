import gradio as gr
import torch
import os
import sys
from datetime import datetime
import soundfile as sf
import librosa
import numpy as np

# 注意：这里需要根据Fish Speech库的实际API导入相应函数
# 假设的导入方式，可能需要根据库的文档调整
# from fish_speech.infer import generate_audio_from_text
fish_speech_path = os.path.abspath(os.path.join(os.path.dirname(__file__),'../fish-speech'))
if fish_speech_path not in sys.path:
    sys.path.insert(0,fish_speech_path)

from fish_speech.inference_engine import TTSInferenceEngine
from fish_speech.models.dac.inference import load_model as load_decoder_model
from fish_speech.models.text2semantic.inference import launch_thread_safe_queue
from fish_speech.utils.schema import ServeTTSRequest, ServeReferenceAudio

# 预定义的“语音包”配置 (音频文件需放在 `voice_packs/` 目录下)
VOICE_PACKS = {
    "丁真语音包": "/Users/huangsonghao/off_work/tts/voice_packs/dingzhen.mp3",
    "成熟男声": "voice_packs/deep_male.wav",
    "温柔女声": "/Users/huangsonghao/off_work/tts/voice_packs/girl_multi_emotion.wav",
    # 可在此添加更多语音包...
}

# 情感标签映射
EMOTION_MAP = {
    "默认 (Default)": "",
    "开心 (Happy)": "(happy)",
    "兴奋 (Excited)": "(excited)",
    "愤怒 (Angry)": "(angry)",
    "悲伤 (Sad)": "(sad)",
    "惊讶 (Surprised)": "(surprised)",
    "恐惧 (Scared)": "(scared)",
    "担忧 (Worried)": "(worried)",
    "紧张 (Nervous)": "(nervous)",
    "沮丧 (Depressed)": "(depressed)",
    "自信 (Confident)": "(confident)",
    "轻松 (Relaxed)": "(relaxed)",
    "严肃 (Serious)": "(serious)",
}

MODEL_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__),'../model'))
LLAMA_CHECKPOINT_PATH = MODEL_DIR
DECODER_CHECKPOINT_PATH = os.path.join(MODEL_DIR,'codec.pth')
DECODER_CONFIG_NAME = 'modded_dac_vq'

tts_engine = None
device = None
precision = None

def initialize_models():
    global tts_engine, device, precision
    if torch.cuda.is_available():
        device = "cuda"
    elif torch.backends.mps.is_available():
        device = "mps"
    else:
        device = "cpu"
    precision = torch.half if device == "cuda" else torch.bfloat16

    llama_queue = launch_thread_safe_queue(
        checkpoint_path=LLAMA_CHECKPOINT_PATH,
        device=device,
        precision=precision,
        compile=False,
    )
    decoder_model = load_decoder_model(
        config_name=DECODER_CONFIG_NAME,
        checkpoint_path=DECODER_CHECKPOINT_PATH,
        device=device,
    )
    tts_engine = TTSInferenceEngine(
        llama_queue=llama_queue,
        decoder_model=decoder_model,
        precision=precision,
        compile=False,
    )
    warmup_request = ServeTTSRequest(
        text="你好",
        references=[],
        reference_id=None,
        max_new_tokens=1024,
        chunk_length=200,
        top_p=0.7,
        repetition_penalty=1.2,
        temperature=0.7,
        format="wav",
        streaming=False,
    )
    list(tts_engine.inference(warmup_request))

def synthesize_speech(text, voice_pack_name, emotion, speed, temperature, top_p):
    """
    核心合成函数：将文本和指定语音包合成为语音。
    """
    if not text.strip():
        return None, "请输入有效文本。"
    
    # 1. 获取选定的参考音频路径
    reference_audio_path = VOICE_PACKS.get(voice_pack_name)
    if not reference_audio_path or not os.path.exists(reference_audio_path):
        return None, f"未找到语音包 '{voice_pack_name}' 的参考音频文件。"
    
    try:
        # 2. 处理情感标签
        emotion_tag = EMOTION_MAP.get(emotion, "")
        final_text = f"{emotion_tag} {text}" if emotion_tag else text

        # 3. 生成唯一文件名
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        safe_text = text[:20].replace(" ", "_")  # 取前20字符用于文件名
        output_filename = f"output_{safe_text}_{timestamp}.wav"
        output_path = os.path.join("generated_audio", output_filename)
        os.makedirs("generated_audio", exist_ok=True)

        with open(reference_audio_path,"rb") as audio_file:
            audio_bytes = audio_file.read()
        
        reference = [ServeReferenceAudio(
            audio=audio_bytes,
            text=""
        )]

        request = ServeTTSRequest(
            text=final_text,
            references=reference,
            reference_id=None,
            max_new_tokens=1024,
            chunk_length=200,
            top_p=top_p,
            repetition_penalty=1.2,
            temperature=temperature,
            format="wav",
            streaming=False,
            normalize=True
        )

        audio_data = None
        sample_rate = None

        for result in tts_engine.inference(request):
            if result.code == "error":
                error_msg = str(result.message) if result.error else "未知错误"
                return None,f"合成失败: {error_msg}"
            elif result.code == "final":
                sample_rate,audio_data = result.audio
                break

        if audio_data is None:
            return None, "生成音频失败，没有音频数据"
        
        # 语速调整 (使用 librosa)
        if speed != 1.0:
            try:
                # 确保数据类型为 float32
                if audio_data.dtype != np.float32:
                    audio_data = audio_data.astype(np.float32)
                
                # librosa.effects.time_stretch 需要 (n_samples,) 或 (n_channels, n_samples)
                # 如果是 (n_samples, n_channels) 且 n_channels < n_samples，则转置
                is_transposed = False
                if len(audio_data.shape) > 1 and audio_data.shape[1] < audio_data.shape[0]:
                    audio_data = audio_data.T
                    is_transposed = True
                
                # 进行变速处理
                audio_data = librosa.effects.time_stretch(audio_data, rate=speed)
                
                # 如果之前转置过，转置回来
                if is_transposed:
                    audio_data = audio_data.T
                    
            except Exception as e:
                print(f"语速调整失败: {e}")
                # 失败时继续使用原音频，不中断流程

        sf.write(output_path,audio_data,sample_rate)
        return output_path, "语音生成成功！"
    
    except Exception as e:
        # 错误处理
        print(f"合成过程中出现错误: {e}")
        return None, f"合成失败: {str(e)}"

def create_gradio_interface():
    """
    创建Gradio Web界面。
    """
    # 确保输出目录存在
    os.makedirs("generated_audio", exist_ok=True)
    os.makedirs("voice_packs", exist_ok=True)

    try:
        initialize_models()
    except Exception as e:
        print(f"初始化模型失败: {e}")
        return None, f"初始化模型失败: {str(e)}"
    
    # 界面布局
    with gr.Blocks(title="OpenAudio S1 文字转语音系统", theme=gr.themes.Soft()) as app:
        gr.Markdown("## 🎤 OpenAudio S1-mini 文字转语音系统")
        gr.Markdown("输入文本，选择语音包，即可生成并下载语音。")
        
        with gr.Row():
            with gr.Column(scale=2):
                # 文本输入
                text_input = gr.Textbox(
                    label="请输入要转换的文本",
                    placeholder="例如：大家好，我是丁真...",
                    lines=5,
                    max_lines=10
                )
                
                # 语音包选择
                voice_pack_dropdown = gr.Dropdown(
                    label="选择语音包",
                    choices=list(VOICE_PACKS.keys()),
                    value="丁真语音包"  # 默认值
                )
                
                # 情感选择
                emotion_dropdown = gr.Dropdown(
                    label="选择情感 (Emotion)",
                    choices=list(EMOTION_MAP.keys()),
                    value="默认 (Default)",
                    info="选择特定的情感风格"
                )
                
                # 高级设置
                with gr.Accordion("高级设置 (语速与参数)", open=True):
                    speed_slider = gr.Slider(
                        label="语速 (Speed)",
                        minimum=0.5,
                        maximum=2.0,
                        value=1.0,
                        step=0.1,
                        info="1.0为正常速度，<1.0变慢，>1.0变快"
                    )
                    with gr.Row():
                        temperature_slider = gr.Slider(
                            label="情感变化度 (Temperature)",
                            minimum=0.1,
                            maximum=1.0,
                            value=0.7,
                            step=0.1,
                            info="值越高语音越富有感情变化，但也越不稳定"
                        )
                        top_p_slider = gr.Slider(
                            label="采样阈值 (Top-P)",
                            minimum=0.1,
                            maximum=1.0,
                            value=0.7,
                            step=0.1,
                            info="控制生成文本的多样性"
                        )
                
                # 生成按钮
                generate_btn = gr.Button("生成语音", variant="primary")
                
                # 状态信息显示
                status_output = gr.Textbox(label="状态", interactive=False)
            
            with gr.Column(scale=3):
                # 音频输出组件
                audio_output = gr.Audio(
                    label="生成的语音",
                    type="filepath",
                    interactive=False
                )
                
                # 下载链接（通过音频组件已包含下载功能，这里可添加额外说明）
                gr.Markdown("**使用说明：** 生成后，可点击上方音频播放器右侧的下载按钮保存文件。")
        
        # 示例文本
        gr.Examples(
            examples=[
                ["今天天气真好，我们一起去玩吧！", "丁真语音包", "开心 (Happy)"],
                ["欢迎使用本语音合成系统。", "成熟男声", "默认 (Default)"],
                ["这是一个多语音包合成的演示。", "温柔女声", "轻松 (Relaxed)"]
            ],
            inputs=[text_input, voice_pack_dropdown, emotion_dropdown],
            label="示例（点击试试）"
        )
        
        # 按钮点击事件
        generate_btn.click(
            fn=synthesize_speech,
            inputs=[text_input, voice_pack_dropdown, emotion_dropdown, speed_slider, temperature_slider, top_p_slider],
            outputs=[audio_output, status_output]
        )
    
    return app

if __name__ == "__main__":
    # 创建界面并启动
    app = create_gradio_interface()
    # 设置服务器名称和端口，`share=True`可生成临时公网链接用于测试
    app.launch(server_name="0.0.0.0", server_port=7860, share=False)