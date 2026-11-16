$ErrorActionPreference = 'Stop'
$version = '4.14.3'
$directory = Join-Path $env:RUNNER_TEMP 'eepp-ccache-tools'
New-Item -ItemType Directory -Force -Path $directory | Out-Null
$archive = Join-Path $directory 'ccache.zip'
Invoke-WebRequest "https://github.com/ccache/ccache/releases/download/v$version/ccache-$version-windows-x86_64.zip" -OutFile $archive
Expand-Archive -LiteralPath $archive -DestinationPath $directory -Force
$ccache = Join-Path $directory "ccache-$version-windows-x86_64/ccache.exe"
if (-not (Test-Path -LiteralPath $ccache)) { throw 'ccache executable missing from release archive' }
$wrapper = Join-Path $directory 'msvc'
New-Item -ItemType Directory -Force -Path $wrapper | Out-Null
Copy-Item -LiteralPath $ccache -Destination (Join-Path $wrapper 'cl.exe') -Force

# MSBuild needs single-file compiler invocations and embedded object debug information.
# The linker still produces the final PDBs used by CI crash diagnostics.
$repoRoot = (Resolve-Path "$PSScriptRoot/../../..").Path
$buildDirectory = Join-Path $repoRoot 'make/windows'
New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null
$escapedWrapper = [Security.SecurityElement]::Escape($wrapper)
@"
<Project>
  <PropertyGroup>
    <CLToolPath>$escapedWrapper</CLToolPath>
    <UseMultiToolTask>true</UseMultiToolTask>
    <EnforceProcessCountAcrossBuilds>true</EnforceProcessCountAcrossBuilds>
  </PropertyGroup>
  <ItemDefinitionGroup>
    <ClCompile>
      <DebugInformationFormat Condition="'%(ClCompile.DebugInformationFormat)' == 'ProgramDatabase' Or '%(ClCompile.DebugInformationFormat)' == 'EditAndContinue'">OldStyle</DebugInformationFormat>
      <ObjectFileName>`$(IntDir)%(Filename).obj</ObjectFileName>
      <MultiProcessorCompilation>false</MultiProcessorCompilation>
    </ClCompile>
  </ItemDefinitionGroup>
</Project>
"@ | Set-Content -LiteralPath (Join-Path $buildDirectory 'Directory.Build.targets') -Encoding utf8
(Split-Path $ccache) | Out-File -FilePath $env:GITHUB_PATH -Append -Encoding utf8
& $ccache --zero-stats
if ($LASTEXITCODE -ne 0) { throw "ccache setup failed: $LASTEXITCODE" }
