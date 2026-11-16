param(
  [Parameter(Mandatory = $true)][string]$Label,
  [Parameter(Mandatory = $true)][scriptblock]$Command
)
$ErrorActionPreference = 'Stop'
$metricsDirectory = Join-Path $PSScriptRoot '../../../.ci-metrics'
New-Item -ItemType Directory -Force -Path $metricsDirectory | Out-Null
$watch = [Diagnostics.Stopwatch]::StartNew()
$result = 0
$global:LASTEXITCODE = 0
try {
  & $Command
  $result = $LASTEXITCODE
} catch {
  $result = 1
  Write-Error -ErrorRecord $_ -ErrorAction Continue
} finally {
  $watch.Stop()
  $seconds = [int][Math]::Ceiling($watch.Elapsed.TotalSeconds)
  "$Label`t$seconds`t$result" | Add-Content -LiteralPath (Join-Path $metricsDirectory 'timings.tsv')
  Write-Output "${Label}: ${seconds}s (exit $result)"
}
exit $result
