param(
  [string]$arch = "x64"
)

Set-Location (Resolve-Path "$PSScriptRoot\..\..\..")

if ($arch -eq "arm64") {
  Invoke-WebRequest -Uri "https://github.com/premake/premake-core/releases/download/v5.0.0-beta6/premake-5.0.0-beta6-windows.zip" -OutFile "premake-5.0.0-beta6-windows.zip"
  Expand-Archive -LiteralPath "premake-5.0.0-beta6-windows.zip" -DestinationPath .
  .\premake5.exe --windows-vc-build --with-text-shaper --arch=arm64 --disable-static-build vs2022
  .\projects\scripts\patch_commit_number.ps1
  & "$env:MSBUILD_PATH/MSBuild.exe" .\make\windows\eepp.sln -m /t:ecode /p:Platform=ARM64 /p:Configuration=release
  New-Item -Name "ecode" -ItemType Directory
  .\projects\scripts\copy_ecode_assets.ps1 .\bin .\ecode
  Copy-Item -Path ".\bin\assets", ".\bin\SDL2.dll", ".\bin\eepp.dll", ".\bin\ecode.exe" -Destination ".\ecode"
  Compress-Archive -LiteralPath ".\ecode" -DestinationPath .\ecode-windows-nightly-msvc-arm64.zip -Force
  Compress-Archive -LiteralPath ".\bin\ecode.pdb" -DestinationPath .\ecode-windows-nightly-msvc-arm64-pdb.zip -Force
}
else {
  Invoke-WebRequest -Uri "https://github.com/premake/premake-core/releases/download/v5.0.0-beta6/premake-5.0.0-beta6-windows.zip" -OutFile "premake-5.0.0-beta6-windows.zip"
  Expand-Archive -LiteralPath "premake-5.0.0-beta6-windows.zip" -DestinationPath .
  .\premake5.exe --windows-vc-build --with-text-shaper --disable-static-build vs2022
  .\projects\scripts\patch_commit_number.ps1
  & "$env:MSBUILD_PATH/MSBuild.exe" .\make\windows\eepp.sln -m /t:ecode /p:Platform=x64 /p:Configuration=release
  New-Item -Name "ecode" -ItemType Directory
  .\projects\scripts\copy_ecode_assets.ps1 .\bin .\ecode
  Copy-Item -Path ".\bin\assets", ".\bin\SDL2.dll", ".\bin\eepp.dll", ".\bin\ecode.exe" -Destination ".\ecode"
  Compress-Archive -LiteralPath ".\ecode" -DestinationPath .\ecode-windows-nightly-msvc-x86_64.zip -Force
  Compress-Archive -LiteralPath ".\bin\ecode.pdb" -DestinationPath .\ecode-windows-nightly-msvc-x86_64-pdb.zip -Force
}
