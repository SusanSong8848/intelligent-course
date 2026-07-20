#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
简易 HTTP 静态服务器
用法：python server.py [端口号]
默认端口：8000
启动后在浏览器访问 http://localhost:8000
"""
import http.server
import socketserver
import sys
import os

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 8000

os.chdir(os.path.dirname(os.path.abspath(__file__)))
# 也尝试从 build/Release 加载 data 目录
DATA_DIRS = ['data', '../build/Release/data']

class CORSRequestHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Cache-Control', 'no-cache')
        super().end_headers()

    def translate_path(self, path):
        path = super().translate_path(path)
        # 如果请求 data/schedule_result.json 但 frontend/data/ 没有，尝试从 build 目录找
        if 'schedule_result.json' in path or 'course_dataset.json' in path:
            if not os.path.exists(path):
                alt = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                   '..', 'build', 'Release', 'data',
                                   os.path.basename(path))
                if os.path.exists(alt):
                    return alt
        return path

with socketserver.TCPServer(("", PORT), CORSRequestHandler) as httpd:
    print(f"✅ 服务器已启动: http://localhost:{PORT}")
    print(f"📁 服务目录: {os.getcwd()}")
    if not os.path.exists('data'):
        os.makedirs('data', exist_ok=True)
        print("⚠ 已自动创建 frontend/data/ 目录")
    print("按 Ctrl+C 停止服务器")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n服务器已停止")