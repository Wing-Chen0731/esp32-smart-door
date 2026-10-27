$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $root 'desktop\build'
$gxx = Get-Command g++ -ErrorAction SilentlyContinue

if (-not $gxx) {
    Write-Host '没有找到 g++。请安装 MSYS2 UCRT64 的 GCC，并把 C:\msys64\ucrt64\bin 加入 PATH。' -ForegroundColor Yellow
    Write-Host '安装命令：pacman -S mingw-w64-ucrt-x86_64-gcc'
    Read-Host '按回车退出'
    exit 1
}

New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
$testsExe = Join-Path $buildDir 'simulator-tests.exe'
$simulatorExe = Join-Path $buildDir 'smartdoor-simulator.exe'
$includeDir = Join-Path $root 'desktop\include'
$testsSource = Join-Path $root 'desktop\tests\simulator_tests.cpp'
$simSource = Join-Path $root 'desktop\src\simulator.cpp'
$mainSource = Join-Path $root 'desktop\src\main.cpp'

Write-Host '编译并运行桌面状态模型测试……'
& $gxx.Source -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -pthread "-I$includeDir" $testsSource $simSource -o $testsExe
if ($LASTEXITCODE -ne 0) { throw '测试程序编译失败。' }
& $testsExe
if ($LASTEXITCODE -ne 0) { throw '桌面状态模型测试未通过。' }

Write-Host '构建交互式验证平台……'
& $gxx.Source -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -pthread "-I$includeDir" $mainSource $simSource -lws2_32 -o $simulatorExe
if ($LASTEXITCODE -ne 0) { throw '实验台编译失败。' }
Write-Host 'Build successful。启动本机实验台……' -ForegroundColor Green

$baseUrl = 'http://127.0.0.1:8765'
$alreadyRunning = $false
try {
    $existingState = Invoke-RestMethod -Uri "$baseUrl/api/state" -TimeoutSec 1
    $alreadyRunning = $null -ne $existingState.mode
} catch { }
if ($alreadyRunning) {
    Write-Host "复用已运行的智能门禁实验台：$baseUrl"
    Start-Process $baseUrl
    Write-Host '原服务仍由最初启动它的终端管理。'
    exit 0
}

$server = $null
$ready = $false
$url = $null
try {
    for ($port = 8765; $port -le 8860 -and -not $ready; $port++) {
        $candidateUrl = "http://127.0.0.1:$port"
        $server = Start-Process -FilePath $simulatorExe -ArgumentList @('--port', "$port") -WorkingDirectory $root -PassThru -NoNewWindow
        for ($attempt = 0; $attempt -lt 50; $attempt++) {
            if ($server.HasExited) { break }
            try {
                $state = Invoke-RestMethod -Uri "$candidateUrl/api/state" -TimeoutSec 1
                if ($null -ne $state.mode) {
                    $ready = $true
                    $url = $candidateUrl
                    break
                }
            } catch { Start-Sleep -Milliseconds 100 }
        }
        if (-not $ready -and -not $server.HasExited) {
            Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue
            $server.WaitForExit()
        }
    }
    if (-not $ready) { throw '未能在 8765-8860 范围内启动服务，请检查端口或防火墙设置。' }
    Write-Host "实验台已就绪：$url" -ForegroundColor Green
    Start-Process $url
    Write-Host '验证平台正在运行。按 Ctrl+C 停止本地服务。'
    while (-not $server.HasExited) { Start-Sleep -Milliseconds 500 }
} finally {
    if ($server -and -not $server.HasExited) { Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue }
}
