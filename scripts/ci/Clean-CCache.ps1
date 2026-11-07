# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: Apache-2.0

param(
    [Parameter(Mandatory)]
    [ValidatePattern('^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$')]
    [string]$Repository,

    [Parameter(Mandatory)]
    [string]$DefaultBranch
)

$ErrorActionPreference = 'Stop'
$headers = @{
    Accept = 'application/vnd.github+json'
    Authorization = "Bearer $env:GH_TOKEN"
    'X-GitHub-Api-Version' = '2022-11-28'
}
$apiUrl = "https://api.github.com/repos/$Repository/actions/caches"
$caches = [System.Collections.Generic.Dictionary[long, object]]::new()
for ($page = 1; ; $page++) {
    $response = Invoke-RestMethod -Uri "${apiUrl}?per_page=100&page=$page" -Headers $headers
    foreach ($cache in $response.actions_caches) {
        $caches[[long]$cache.id] = $cache
    }
    if ($response.actions_caches.Count -lt 100) {
        break
    }
}

$groups = @{}
$deletions = [System.Collections.Generic.List[object]]::new()
$defaultRef = "refs/heads/$DefaultBranch"
foreach ($cache in $caches.Values) {
    if ($cache.key -cnotmatch '(^|-)ccache-') {
        continue
    }
    if ($cache.ref -ne $defaultRef -or $cache.key -cnotmatch '^ccache-(?<environment>[a-z0-9][a-z0-9.-]*-(?:amd64|arm64))-[0-9]+-[0-9]+$') {
        $deletions.Add($cache)
        continue
    }
    $environment = $Matches.environment
    if (-not $groups.ContainsKey($environment)) {
        $groups[$environment] = [System.Collections.Generic.List[object]]::new()
    }
    $groups[$environment].Add($cache)
}

foreach ($group in $groups.Values) {
    $ordered = @($group | Sort-Object -Property @{ Expression = { [System.DateTimeOffset]$_.created_at }; Descending = $true }, @{ Expression = { [long]$_.id }; Descending = $true })
    Write-Host "Retaining ccache snapshot: $($ordered[0].key)"
    for ($index = 1; $index -lt $ordered.Count; $index++) {
        $deletions.Add($ordered[$index])
    }
}

$deletedCount = 0
$deletedBytes = 0L
foreach ($cache in $deletions) {
    Write-Host "Deleting ccache snapshot: $($cache.key) ($($cache.ref))"
    Invoke-RestMethod -Uri "$apiUrl/$($cache.id)" -Method Delete -Headers $headers -SkipHttpErrorCheck -StatusCodeVariable deleteStatus | Out-Null
    if ($deleteStatus -eq 204) {
        $deletedCount++
        $deletedBytes += [long]$cache.size_in_bytes
    } elseif ($deleteStatus -ne 404) {
        throw "Cache deletion failed with HTTP status $deleteStatus"
    }
}

$deletedMiB = ($deletedBytes / 1MB).ToString('F1', [System.Globalization.CultureInfo]::InvariantCulture)
Write-Host "Deleted $deletedCount ccache snapshots and released $deletedMiB MiB"
if ($env:GITHUB_STEP_SUMMARY) {
    "Deleted $deletedCount ccache snapshots and released $deletedMiB MiB." >> $env:GITHUB_STEP_SUMMARY
}
