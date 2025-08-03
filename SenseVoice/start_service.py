#!/usr/bin/env python3
# -*- encoding: utf-8 -*-
"""
SenseVoice HTTP服务启动脚本
用于启动语音识别API服务，供Qt应用调用
"""

import os
import sys
import uvicorn
import argparse
from pathlib import Path

def check_dependencies():
    """
    函数名称：`check_dependencies`
    功能描述：检查必要的依赖是否已安装
    参数说明：无
    返回值：bool，依赖是否满足
    """
    try:
        import torch
        import funasr
        import fastapi
        print("✓ 依赖检查通过")
        return True
    except ImportError as e:
        print(f"✗ 依赖检查失败: {e}")
        print("请运行: pip install -r requirements.txt")
        return False

def check_model():
    """
    函数名称：`check_model`
    功能描述：检查模型文件是否存在
    参数说明：无
    返回值：bool，模型是否可用
    """
    model_path = Path("model/iic/SenseVoiceSmall")
    if model_path.exists():
        print("✓ 模型文件检查通过")
        return True
    else:
        print(f"✗ 模型文件未找到: {model_path}")
        print("请确保模型已正确下载到指定路径")
        return False

def start_service(host="127.0.0.1", port=8000, reload=False):
    """
    函数名称：`start_service`
    功能描述：启动SenseVoice HTTP服务
    参数说明：
        - host：str，服务绑定的主机地址
        - port：int，服务端口
        - reload：bool，是否启用热重载（开发模式）
    返回值：无
    """
    print(f"正在启动 SenseVoice 服务...")
    print(f"服务地址: http://{host}:{port}")
    print(f"API文档: http://{host}:{port}/docs")
    print("按 Ctrl+C 停止服务\n")
    
    try:
        uvicorn.run(
            "api:app",
            host=host,
            port=port,
            log_level="info",
            reload=reload,
            access_log=True
        )
    except KeyboardInterrupt:
        print("\n服务已停止")
    except Exception as e:
        print(f"服务启动失败: {e}")
        sys.exit(1)

def main():
    """
    函数名称：`main`
    功能描述：主函数，解析命令行参数并启动服务
    参数说明：无
    返回值：无
    """
    parser = argparse.ArgumentParser(description="SenseVoice HTTP服务启动器")
    parser.add_argument("--host", default="127.0.0.1", help="服务主机地址 (默认: 127.0.0.1)")
    parser.add_argument("--port", type=int, default=8000, help="服务端口 (默认: 8000)")
    parser.add_argument("--reload", action="store_true", help="启用热重载 (开发模式)")
    parser.add_argument("--skip-checks", action="store_true", help="跳过依赖和模型检查")
    
    args = parser.parse_args()
    
    print("=" * 50)
    print("SenseVoice 语音识别服务")
    print("=" * 50)
    
    # 检查运行环境
    if not args.skip_checks:
        if not check_dependencies():
            sys.exit(1)
        if not check_model():
            sys.exit(1)
    
    # 设置环境变量
    os.environ.setdefault("SENSEVOICE_HOST", args.host)
    os.environ.setdefault("SENSEVOICE_PORT", str(args.port))
    
    # 启动服务
    start_service(args.host, args.port, args.reload)

if __name__ == "__main__":
    main() 