$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$build = Join-Path $env:TEMP 'meda-race-native-tests'
New-Item -ItemType Directory -Force $build | Out-Null
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$install = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$install) { throw 'Visual Studio C++ tools are required.' }
$version = (Get-Content (Join-Path $install 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt')).Trim()
$tool = Join-Path $install "VC/Tools/MSVC/$version"
$kits = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits/10'
$sdk = (Get-ChildItem (Join-Path $kits 'Include') -Directory | Sort-Object Name -Descending | Select-Object -First 1).Name
$env:INCLUDE = "$tool/include;$kits/Include/$sdk/ucrt;$kits/Include/$sdk/shared;$kits/Include/$sdk/um"
$env:LIB = "$tool/lib/x64;$kits/Lib/$sdk/ucrt/x64;$kits/Lib/$sdk/um/x64"
& "$tool/bin/Hostx64/x64/cl.exe" /nologo /std:c++14 /EHsc /W4 /WX "$PSScriptRoot/race_control_test.cpp" "/Fe:$build/race_control_test.exe" "/Fo:$build/race_control_test.obj"
if ($LASTEXITCODE) { throw 'Compilation failed.' }
& "$build/race_control_test.exe"
if ($LASTEXITCODE) { throw 'Tests failed.' }

& "$tool/bin/Hostx64/x64/cl.exe" /nologo /std:c++14 /EHsc /W4 /WX /D_CRT_SECURE_NO_WARNINGS /utf-8 "/I$PSScriptRoot/stubs" "$PSScriptRoot/remote_control_test.cpp" "/Fe:$build/remote_control_test.exe" "/Fo:$build/remote_control_test.obj"
if ($LASTEXITCODE) { throw 'Remote command test compilation failed.' }
& "$build/remote_control_test.exe"
if ($LASTEXITCODE) { throw 'Remote command tests failed.' }
