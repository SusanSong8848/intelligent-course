@echo off
chcp 65001 >nul
echo ==========================================
echo   智能课程规划系统 - 启动中...
echo ==========================================
cd /d "%~dp0"
where python >nul 2>&1
if %errorlevel% neq 0 (
    echo [错误] 未找到 Python，请先安装 Python 3
    echo 下载地址: https://www.python.org/downloads/
    pause
    exit /b 1
)
start http://localhost:8000
python server.py 8000
pause