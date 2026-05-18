param(
    [string]$App = ".\build\Debug\rvc_app.exe",
    [string]$HostName = "127.0.0.1",
    [int]$Port = 18765
)

python system_tests\run_system_tests.py --app $App --host $HostName --port $Port
