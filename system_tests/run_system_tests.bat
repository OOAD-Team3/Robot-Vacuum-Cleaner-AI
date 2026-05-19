@echo off
set APP_PATH=%1
if "%APP_PATH%"=="" set APP_PATH=.\build\Debug\rvc_app.exe
set HOST=127.0.0.1
set PORT=18765

python system_tests\run_system_tests.py --app "%APP_PATH%" --host %HOST% --port %PORT%
