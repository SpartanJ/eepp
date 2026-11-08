# PowerShell adapter only; profiles have already loaded. No execution-policy changes or eval.
if (-not $env:ETERM_HARNESS_DIRECTORY -or -not $env:ETERM_HARNESS_BINARY -or
    $env:ETERM_SHELL_INTEGRATION -eq '0') { return }
& $env:ETERM_HARNESS_BINARY --harness-session attach $PID
if ($LASTEXITCODE -ne 0) { return }
$env:ETERM_HARNESS_SHELL_PID = [string]$PID

function global:__etermClearHarness {
    $etermRecord = Join-Path $env:ETERM_HARNESS_DIRECTORY 'active.json'
    if (Test-Path -LiteralPath $etermRecord) {
        try { [System.IO.File]::WriteAllText($etermRecord, '') } catch {}
    }
}
if (-not (Get-Command codex -CommandType Function,Alias -ErrorAction SilentlyContinue)) {
    function global:codex {
        & $env:ETERM_HARNESS_BINARY --harness-session codex -- @args
    }
}
if ($env:ETERM_RESUME_SESSION) {
    try { & $env:ETERM_HARNESS_BINARY --harness-session resume }
    finally { [Environment]::SetEnvironmentVariable('ETERM_RESUME_SESSION', $null, 'Process') }
}
