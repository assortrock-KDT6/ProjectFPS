[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$DiversionWorkspacePath,

    [Parameter(Mandatory = $true)]
    [string]$GitWorkspacePath,

    [string]$RequestedBranch = '*',
    [string]$FallbackBaseBranch = 'develop',
    [string]$RemoteName = 'origin',
    [string[]]$IncludeDirectories = @('Config', 'Source')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Invoke-ExternalCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Command,

        [Parameter()]
        [string[]]$Arguments = @(),

        [Parameter()]
        [string]$WorkingDirectory
    )

    if ($WorkingDirectory) {
        Push-Location -LiteralPath $WorkingDirectory
    }

    try {
        $previousErrorActionPreference = $ErrorActionPreference
        try {
            $ErrorActionPreference = 'Continue'
            $output = & $Command @Arguments 2>&1
            $exitCode = $LASTEXITCODE
        }
        finally {
            $ErrorActionPreference = $previousErrorActionPreference
        }
        if ($exitCode -ne 0) {
            $details = ($output | ForEach-Object { $_.ToString() }) -join [Environment]::NewLine
            throw "Command failed ($exitCode): $Command $($Arguments -join ' ')`n$details"
        }

        return @($output | ForEach-Object { $_.ToString() })
    }
    finally {
        if ($WorkingDirectory) {
            Pop-Location
        }
    }
}

function Test-PathIsInsideRoot {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [string]$Root
    )

    $fullPath = [IO.Path]::GetFullPath($Path).TrimEnd('\')
    $fullRoot = [IO.Path]::GetFullPath($Root).TrimEnd('\')
    return $fullPath.StartsWith("$fullRoot\", [StringComparison]::OrdinalIgnoreCase)
}

function Copy-MirroredDirectory {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Source,

        [Parameter(Mandatory = $true)]
        [string]$Destination,

        [Parameter(Mandatory = $true)]
        [string]$DestinationRoot
    )

    if (-not (Test-PathIsInsideRoot -Path $Destination -Root $DestinationRoot)) {
        throw "Refusing to mirror outside the Git workspace: $Destination"
    }

    if (-not (Test-Path -LiteralPath $Source -PathType Container)) {
        if (Test-Path -LiteralPath $Destination) {
            Remove-Item -LiteralPath $Destination -Recurse -Force
        }
        return
    }

    New-Item -ItemType Directory -Path $Destination -Force | Out-Null
    & robocopy $Source $Destination /MIR /R:2 /W:1 /NFL /NDL /NJH /NJS /NP
    $robocopyExitCode = $LASTEXITCODE
    if ($robocopyExitCode -ge 8) {
        throw "Robocopy failed with exit code $robocopyExitCode while mirroring $Source"
    }
}

function Get-DiversionBranches {
    param([string]$WorkspacePath)

    $rawLines = @(Invoke-ExternalCommand -Command 'dv' -Arguments @('branch') -WorkingDirectory $WorkspacePath)
    $branches = foreach ($line in $rawLines) {
        $cleanLine = [regex]::Replace($line, '\x1B\[[0-?]*[ -/]*[@-~]', '').Trim()
        $cleanLine = $cleanLine -replace '^[*]>?\s*', ''
        if ($cleanLine -and $cleanLine -notmatch '^(Branches|Branch)\s*:?$') {
            $cleanLine
        }
    }

    return @($branches | Sort-Object -Unique)
}

function Test-GitRefExists {
    param(
        [string]$WorkspacePath,
        [string]$Ref
    )

    Push-Location -LiteralPath $WorkspacePath
    try {
        & git show-ref --verify --quiet $Ref
        return ($LASTEXITCODE -eq 0)
    }
    finally {
        Pop-Location
    }
}

$diversionRoot = [IO.Path]::GetFullPath($DiversionWorkspacePath).TrimEnd('\')
$gitRoot = [IO.Path]::GetFullPath($GitWorkspacePath).TrimEnd('\')

if (-not (Test-Path -LiteralPath (Join-Path $diversionRoot '.diversion'))) {
    throw "Not a Diversion workspace: $diversionRoot"
}
if (-not (Test-Path -LiteralPath (Join-Path $gitRoot '.git'))) {
    throw "Not a Git workspace: $gitRoot"
}

if (-not (Get-Command dv -ErrorAction SilentlyContinue)) {
    throw 'Diversion CLI (dv) was not found on PATH.'
}
if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw 'Git was not found on PATH.'
}
if (-not (Get-Command robocopy -ErrorAction SilentlyContinue)) {
    throw 'Robocopy was not found on PATH.'
}

$lockName = 'Global\ProjectFPS-Diversion-GitHub-Source-Sync'
$mutex = [Threading.Mutex]::new($false, $lockName)
$lockAcquired = $false

try {
    $lockAcquired = $mutex.WaitOne([TimeSpan]::FromSeconds(5))
    if (-not $lockAcquired) {
        throw 'Another ProjectFPS source export is already running.'
    }

    $pendingChanges = @(Invoke-ExternalCommand -Command 'dv' -Arguments @('diff', '--name-status') -WorkingDirectory $diversionRoot)
    if ($pendingChanges.Count -gt 0) {
        throw "The Diversion export workspace has pending changes. Use a clean, dedicated workspace.`n$($pendingChanges -join [Environment]::NewLine)"
    }

    $branches = @(Get-DiversionBranches -WorkspacePath $diversionRoot)
    if ($RequestedBranch -ne '*') {
        $branches = @($branches | Where-Object { $_ -eq $RequestedBranch })
        if ($branches.Count -eq 0) {
            throw "Diversion branch was not found: $RequestedBranch"
        }
    }

    if ($branches.Count -eq 0) {
        throw 'No Diversion branches were found.'
    }

    Invoke-ExternalCommand -Command 'git' -Arguments @('config', 'user.name', 'github-actions[bot]') -WorkingDirectory $gitRoot | Out-Null
    Invoke-ExternalCommand -Command 'git' -Arguments @('config', 'user.email', '41898282+github-actions[bot]@users.noreply.github.com') -WorkingDirectory $gitRoot | Out-Null
    Invoke-ExternalCommand -Command 'git' -Arguments @('fetch', $RemoteName, '--prune') -WorkingDirectory $gitRoot | Out-Null

    if (-not (Test-GitRefExists -WorkspacePath $gitRoot -Ref "refs/remotes/$RemoteName/$FallbackBaseBranch")) {
        throw "Fallback branch does not exist on GitHub: $RemoteName/$FallbackBaseBranch"
    }

    $summaryRows = [Collections.Generic.List[string]]::new()

    foreach ($branch in $branches) {
        if ($branch -notmatch '^[A-Za-z0-9._/-]+$' -or $branch.StartsWith('-')) {
            throw "Diversion branch cannot be represented safely as a Git branch: $branch"
        }

        Write-Host "::group::Exporting Diversion branch '$branch'"
        try {
            Invoke-ExternalCommand -Command 'dv' -Arguments @('checkout', $branch, '--ignore-shelf') -WorkingDirectory $diversionRoot | Out-Null
            Invoke-ExternalCommand -Command 'dv' -Arguments @('update', '--conflict_resolution', 'accept-incoming') -WorkingDirectory $diversionRoot | Out-Null
            $commitOutput = @(Invoke-ExternalCommand -Command 'dv' -Arguments @('status', '--commit-id-only') -WorkingDirectory $diversionRoot)
            $diversionCommit = ($commitOutput | Where-Object { $_ -match 'dv\.commit\.' } | Select-Object -Last 1)
            if (-not $diversionCommit) {
                $diversionCommit = ($commitOutput | Select-Object -Last 1)
            }
            $diversionCommit = $diversionCommit.Trim()

            $remoteRef = "refs/remotes/$RemoteName/$branch"
            if (Test-GitRefExists -WorkspacePath $gitRoot -Ref $remoteRef) {
                Invoke-ExternalCommand -Command 'git' -Arguments @('checkout', '-B', $branch, "$RemoteName/$branch") -WorkingDirectory $gitRoot | Out-Null
            }
            else {
                Invoke-ExternalCommand -Command 'git' -Arguments @('checkout', '-B', $branch, "$RemoteName/$FallbackBaseBranch") -WorkingDirectory $gitRoot | Out-Null
            }

            foreach ($directory in $IncludeDirectories) {
                $sourcePath = Join-Path $diversionRoot $directory
                $destinationPath = Join-Path $gitRoot $directory
                Copy-MirroredDirectory -Source $sourcePath -Destination $destinationPath -DestinationRoot $gitRoot
            }

            Invoke-ExternalCommand -Command 'git' -Arguments (@('add', '-A', '--') + $IncludeDirectories) -WorkingDirectory $gitRoot | Out-Null

            Push-Location -LiteralPath $gitRoot
            try {
                & git diff --cached --quiet
                $diffExitCode = $LASTEXITCODE
            }
            finally {
                Pop-Location
            }

            if ($diffExitCode -eq 0) {
                Write-Host "No source changes for '$branch'."
                $summaryRows.Add("| ``$branch`` | No changes | ``$diversionCommit`` |")
                continue
            }
            if ($diffExitCode -ne 1) {
                throw "git diff --cached failed with exit code $diffExitCode"
            }

            $commitMessage = "sync: Diversion/$branch source ($diversionCommit)"
            Invoke-ExternalCommand -Command 'git' -Arguments @('commit', '-m', $commitMessage) -WorkingDirectory $gitRoot | Out-Null
            Invoke-ExternalCommand -Command 'git' -Arguments @('push', $RemoteName, "HEAD:refs/heads/$branch") -WorkingDirectory $gitRoot | Out-Null
            $summaryRows.Add("| ``$branch`` | Pushed | ``$diversionCommit`` |")
        }
        finally {
            Write-Host '::endgroup::'
        }
    }

    if ($env:GITHUB_STEP_SUMMARY) {
        @(
            '# Diversion source export'
            ''
            '| Branch | Result | Diversion commit |'
            '|---|---|---|'
            $summaryRows
        ) | Add-Content -LiteralPath $env:GITHUB_STEP_SUMMARY -Encoding utf8
    }
}
finally {
    if ($lockAcquired) {
        $mutex.ReleaseMutex()
    }
    $mutex.Dispose()
}

