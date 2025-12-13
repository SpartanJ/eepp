Set-Location $PSScriptRoot

$COMMIT_NUMBER = git rev-list "$((git tag --sort=-creatordate | Select-String ecode | Select-Object -First 1).Line)..HEAD" --count

if ($LASTEXITCODE) { exit $LASTEXITCODE }

$FILE_PATH = "../../src/tools/ecode/version.hpp"

try {
  (Get-Content $FILE_PATH) -replace '#define ECODE_COMMIT_NUMBER \d+', "#define ECODE_COMMIT_NUMBER $COMMIT_NUMBER" | Set-Content $FILE_PATH
} catch {
  exit 1
}

Set-Location (Resolve-Path "$PSScriptRoot\..\..")
