param(
  [string]$arch = "x64"
)

Set-Location (Resolve-Path "$PSScriptRoot\..\..\..")

if ($arch -eq "arm64") {
  Invoke-WebRequest -Uri "https://github.com/premake/premake-core/releases/download/v5.0.0-beta6/premake-5.0.0-beta6-windows.zip" -OutFile "premake-5.0.0-beta6-windows.zip"
  Expand-Archive -LiteralPath "premake-5.0.0-beta6-windows.zip" -DestinationPath .
  .\premake5.exe --windows-vc-build --with-text-shaper --arch=arm64 --disable-static-build vs2022
  $SDLVER = (Select-String 'remote_sdl2_version_number =' .\premake5.lua | ForEach-Object { $_.Line.Split()[2].Trim('"') })
  Remove-Item ".\src\thirdparty\SDL2-$SDLVER" -Recurse -Force
  Invoke-WebRequest -Uri "https://github.com/mmozeiko/build-sdl2/releases/download/2025-12-07/SDL2-arm64-2025-12-07.zip" -OutFile "SDL2-arm64-2025-12-07.zip"
  Expand-Archive -LiteralPath "SDL2-arm64-2025-12-07.zip" -DestinationPath .\src\thirdparty
  Move-Item ".\src\thirdparty\SDL2-arm64" ".\src\thirdparty\SDL2-$SDLVER"
  & "$env:MSBUILD_PATH/MSBuild.exe" .\make\windows\eepp.sln -m /t:ecode /p:Platform=ARM64 /p:Configuration=release
  New-Item -Name "ecode" -ItemType Directory
  .\projects\scripts\copy_ecode_assets.ps1 .\bin .\ecode
  Copy-Item -Path ".\bin\SDL2.dll", ".\bin\eepp.dll", ".\bin\ecode.exe" -Destination ".\ecode"
  Compress-Archive -LiteralPath ".\ecode" -DestinationPath .\ecode-windows-nightly-msvc-arm64.zip -Force
  Compress-Archive -LiteralPath ".\bin\ecode.pdb" -DestinationPath .\ecode-windows-nightly-msvc-arm64-pdb.zip -Force
}
else {
  Invoke-WebRequest -Uri "https://github.com/premake/premake-core/releases/download/v5.0.0-beta6/premake-5.0.0-beta6-windows.zip" -OutFile "premake-5.0.0-beta6-windows.zip"
  Expand-Archive -LiteralPath "premake-5.0.0-beta6-windows.zip" -DestinationPath .
  .\premake5.exe --windows-vc-build --with-text-shaper --disable-static-build vs2022
  & "$env:MSBUILD_PATH/MSBuild.exe" .\make\windows\eepp.sln -m /t:ecode /p:Platform=x64 /p:Configuration=release
  New-Item -Name "ecode" -ItemType Directory
  .\projects\scripts\copy_ecode_assets.ps1 .\bin .\ecode
  Copy-Item -Path ".\bin\SDL2.dll", ".\bin\eepp.dll", ".\bin\ecode.exe" -Destination ".\ecode"
  Compress-Archive -LiteralPath ".\ecode" -DestinationPath .\ecode-windows-nightly-msvc-x86_64.zip -Force
  Compress-Archive -LiteralPath ".\bin\ecode.pdb" -DestinationPath .\ecode-windows-nightly-msvc-x86_64-pdb.zip -Force
}
