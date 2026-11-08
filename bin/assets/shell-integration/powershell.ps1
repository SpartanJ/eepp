# Profiles have already loaded when eterm sources this script through -NoExit -Command.
# Use PowerShell 5.1 syntax as well as PowerShell 7, without changing execution policies.
if ($env:ETERM_SHELL_INTEGRATION -eq '0' -or
    $ExecutionContext.SessionState.LanguageMode -ne 'FullLanguage' -or
    (Test-Path variable:global:__etermOriginalPrompt)) {
    return
}

$global:__etermOriginalPrompt = $function:prompt
function global:prompt {
    # Invoke the user's prompt first so it sees the original $? and LASTEXITCODE. Capture its
    # result before emitting OSC 7 so the visible prompt still ends at the input position.
    $etermPrompt = & $global:__etermOriginalPrompt
    if ($env:ETERM_HARNESS_DIRECTORY -and (Get-Command __etermClearHarness -CommandType Function -ErrorAction SilentlyContinue)) {
        __etermClearHarness
    }
    try {
        if ($pwd.Provider.Name -eq 'FileSystem') {
            $etermPath = $pwd.ProviderPath
            if ([System.IO.Path]::DirectorySeparatorChar -eq '\') {
                $etermPath = $etermPath.Replace('\', '/')
            }
            # eterm accepts local drive paths, not UNC shares or non-filesystem providers.
            if (-not $etermPath.StartsWith('//')) {
                if (-not $etermPath.StartsWith('/')) {
                    $etermPath = '/' + $etermPath
                }
                $etermPath = [uri]::EscapeDataString($etermPath).Replace('%2F', '/').Replace('%3A', ':')
                $Host.UI.Write("$([char]27)]7;file://localhost${etermPath}$([char]7)")
            }
        }
    } catch {
        # Reporting is optional; an unavailable output stream must not break the user's prompt.
    }
    $etermPrompt
}

# Recovery stays entirely absent when its separate option is disabled.
if ($env:ETERM_HARNESS_DIRECTORY -and $env:ETERM_HARNESS_POWERSHELL) {
    . $env:ETERM_HARNESS_POWERSHELL
}
