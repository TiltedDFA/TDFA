[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [ValidateSet("Routine", "Audit", "Fast", "Deep", "Coverage", "Adversarial", "Fuzz", "Mutation")]
    [string]$Tier = "Routine",

    [ValidateRange(1, 256)]
    [int]$Jobs = [Math]::Min(256, [Math]::Max(1, [Environment]::ProcessorCount)),

    [string]$UcrtRuntime = $(
        if ($env:TDFA_UCRT_RUNTIME) { $env:TDFA_UCRT_RUNTIME }
        else { "C:\msys64\ucrt64\bin" }
    ),

    [string]$ClangSanitizerRuntime = $(
        if ($env:TDFA_CLANG_SANITIZER_RUNTIME) { $env:TDFA_CLANG_SANITIZER_RUNTIME }
        else { "C:\Users\Student\Documents\clang+llvm-18.1.8-x86_64-pc-windows-msvc\lib\clang\18\lib\windows" }
    )
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$VerificationDirectory = Join-Path $ProjectRoot "build/verification"
$RunDirectory = $null
$DebugJunitPath = $null
$ReleaseJunitPath = $null
$CoverageDirectory = Join-Path $ProjectRoot "build/coverage-gcc/coverage"
$CoverageSummaryPath = Join-Path $CoverageDirectory "summary.json"
$CoverageTestResultPath = Join-Path $CoverageDirectory "test-result.txt"
$SanitizerBuildDirectory = Join-Path $ProjectRoot "build/sanitize-clang-msvc18"
$FuzzBuildDirectory = Join-Path $ProjectRoot "build/fuzz-clang-msvc18"
$MutationDirectory = Join-Path $ProjectRoot "build/mutation"
$ExpectedSourceSnapshot = $null
$ActualSourceSnapshot = $null
$VerificationInputStart = $null
$TerminalVerificationInputEvidence = $null
$FuzzArtifactsBefore = $null

foreach ($RuntimeDirectory in @($UcrtRuntime, $ClangSanitizerRuntime)) {
    if ($RuntimeDirectory -and (Test-Path -LiteralPath $RuntimeDirectory)) {
        $env:PATH = "$RuntimeDirectory;$env:PATH"
    }
}

function Resolve-Python3 {
    $Candidates = @(
        [pscustomobject]@{ executable = "py"; prefix = @("-3") },
        [pscustomobject]@{ executable = "python3"; prefix = @() },
        [pscustomobject]@{ executable = "python"; prefix = @() }
    )

    foreach ($Candidate in $Candidates) {
        if (-not (Get-Command $Candidate.executable -ErrorAction SilentlyContinue)) {
            continue
        }
        $ProbeArguments = @($Candidate.prefix) + @(
            "-c",
            "import sys; raise SystemExit(0 if sys.version_info.major == 3 else 1)"
        )
        $ProbeExitCode = 1
        try {
            & $Candidate.executable @ProbeArguments 2>&1 | Out-Null
            $ProbeExitCode = $LASTEXITCODE
        }
        catch {
            $ProbeExitCode = 1
        }
        if ($ProbeExitCode -eq 0) {
            return $Candidate
        }
    }
    throw "Python 3 is required, but py -3, python3, and python were unavailable."
}

$Python3 = Resolve-Python3

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Executable,

        [Parameter(Mandatory = $true)]
        [string[]]$ToolArguments
    )

    & $Executable @ToolArguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Executable exited with code $LASTEXITCODE"
    }
}

function Invoke-Python {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$ToolArguments
    )

    $PythonArguments = @($Python3.prefix) + $ToolArguments
    Invoke-Checked $Python3.executable $PythonArguments
}

function Invoke-PythonCapture {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$ToolArguments
    )

    $PythonArguments = @($Python3.prefix) + $ToolArguments
    $Output = & $Python3.executable @PythonArguments
    if ($LASTEXITCODE -ne 0) {
        throw "$($Python3.executable) exited with code $LASTEXITCODE"
    }
    return ($Output -join "`n")
}

function Invoke-PythonEvidence {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$ToolArguments
    )

    $PythonArguments = @($Python3.prefix) + $ToolArguments
    $Output = @(& $Python3.executable @PythonArguments 2>&1 | ForEach-Object { "$_" })
    $ExitCode = $LASTEXITCODE
    foreach ($Line in $Output) {
        Write-Host $Line
    }
    if ($ExitCode -ne 0) {
        $Detail = if ($Output.Count -ne 0) { $Output -join "; " } else { "no diagnostic output" }
        throw $Detail
    }
}

function Get-SnapshotFromText {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Text,

        [Parameter(Mandatory = $true)]
        [string]$Source
    )

    $Match = [regex]::Match(
        $Text,
        "(?m)^# Snapshot-SHA256:\s*([0-9a-f]{64})\s*$"
    )
    if (-not $Match.Success) {
        throw "Cannot read Snapshot-SHA256 from $Source"
    }
    return $Match.Groups[1].Value
}

function Get-ExpectedSourceSnapshot {
    $ManifestPath = Join-Path $ProjectRoot "tests/spec/source-snapshot.sha256"
    $Manifest = Get-Content -Raw -LiteralPath $ManifestPath
    return Get-SnapshotFromText $Manifest $ManifestPath
}

function Get-ActualSourceSnapshot {
    $Manifest = Invoke-PythonCapture @("tools/generate_api_dossier.py", "--print-manifest")
    return Get-SnapshotFromText $Manifest "the current src tree"
}

function Get-VerificationInputFingerprint {
    $Files = [System.Collections.Generic.List[System.IO.FileInfo]]::new()
    foreach ($DirectoryName in @("src", "tests")) {
        $Directory = Join-Path $ProjectRoot $DirectoryName
        foreach ($File in Get-ChildItem -LiteralPath $Directory -Recurse -File) {
            $Files.Add($File)
        }
    }
    $ToolsDirectory = Join-Path $ProjectRoot "tools"
    foreach ($File in Get-ChildItem -LiteralPath $ToolsDirectory -Recurse -File) {
        if ($File.FullName -notmatch "[/\\]__pycache__[/\\]" -and
                $File.Extension -in @(".py", ".ps1", ".cmd", ".cmake")) {
            $Files.Add($File)
        }
    }
    foreach ($RootFileName in @(
        "CMakeLists.txt",
        "CMakePresets.json",
        "perftsuite.epd"
    )) {
        $RootFile = Get-Item -LiteralPath (Join-Path $ProjectRoot $RootFileName)
        $Files.Add($RootFile)
    }

    $ManifestEntries = foreach ($File in $Files | Sort-Object FullName -Unique) {
        $Relative = $File.FullName.Substring($ProjectRoot.Length)
        $Relative = $Relative.TrimStart([char]'\', [char]'/').Replace('\', '/')
        $FileHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $File.FullName).Hash.ToLowerInvariant()
        "$Relative`0$FileHash"
    }
    $ManifestBytes = [Text.Encoding]::UTF8.GetBytes(($ManifestEntries -join "`n"))
    $Hasher = [Security.Cryptography.SHA256]::Create()
    try {
        $Digest = $Hasher.ComputeHash($ManifestBytes)
    }
    finally {
        $Hasher.Dispose()
    }
    return [pscustomobject]@{
        sha256 = -join ($Digest | ForEach-Object { $_.ToString("x2") })
        file_count = @($ManifestEntries).Count
    }
}

function Get-FileEvidence {
    param([string]$Path)

    $Evidence = [ordered]@{
        path = $Path
        exists = $false
        size_bytes = $null
        last_write_utc = $null
        sha256 = $null
        read_error = $null
    }
    if (-not $Path -or -not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return [pscustomobject]$Evidence
    }
    try {
        $File = Get-Item -LiteralPath $Path
        $Evidence.exists = $true
        $Evidence.size_bytes = $File.Length
        $Evidence.last_write_utc = $File.LastWriteTimeUtc.ToString("O")
        $Evidence.sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $Path).Hash.ToLowerInvariant()
    }
    catch {
        $Evidence.read_error = $_.Exception.Message
    }
    return [pscustomobject]$Evidence
}

function Get-DirectoryEvidence {
    param([string]$Path)

    $Evidence = [ordered]@{
        path = $Path
        exists = $false
        file_count = $null
        sha256 = $null
        read_error = $null
    }
    if (-not $Path -or -not (Test-Path -LiteralPath $Path -PathType Container)) {
        return [pscustomobject]$Evidence
    }
    try {
        $Directory = (Resolve-Path -LiteralPath $Path).Path.TrimEnd([char]'\', [char]'/')
        $Entries = foreach ($File in Get-ChildItem -LiteralPath $Directory -Recurse -File |
                Sort-Object FullName) {
            $Relative = $File.FullName.Substring($Directory.Length)
            $Relative = $Relative.TrimStart([char]'\', [char]'/').Replace('\', '/')
            $FileHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $File.FullName).Hash.ToLowerInvariant()
            "$Relative`0$FileHash"
        }
        $ManifestBytes = [Text.Encoding]::UTF8.GetBytes(($Entries -join "`n"))
        $Hasher = [Security.Cryptography.SHA256]::Create()
        try {
            $Digest = $Hasher.ComputeHash($ManifestBytes)
        }
        finally {
            $Hasher.Dispose()
        }
        $Evidence.exists = $true
        $Evidence.file_count = @($Entries).Count
        $Evidence.sha256 = -join ($Digest | ForEach-Object { $_.ToString("x2") })
    }
    catch {
        $Evidence.read_error = $_.Exception.Message
    }
    return [pscustomobject]$Evidence
}

function Write-AtomicUtf8 {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [string]$Text
    )

    $Directory = Split-Path -Parent $Path
    New-Item -ItemType Directory -Force -Path $Directory | Out-Null
    $Temporary = Join-Path $Directory ("." + (Split-Path -Leaf $Path) + ".$PID.tmp")
    [IO.File]::WriteAllText(
        $Temporary,
        $Text,
        [Text.UTF8Encoding]::new($false)
    )
    if (Test-Path -LiteralPath $Path) {
        $Backup = Join-Path $Directory ("." + (Split-Path -Leaf $Path) + ".$PID.backup")
        if (Test-Path -LiteralPath $Backup) {
            Remove-Item -Force -LiteralPath $Backup
        }
        [IO.File]::Replace($Temporary, $Path, $Backup, $true)
        if (Test-Path -LiteralPath $Backup) {
            Remove-Item -Force -LiteralPath $Backup
        }
    }
    else {
        [IO.File]::Move($Temporary, $Path)
    }
}

function Invoke-ConfigurePreset {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ConfigurePreset
    )

    $ConfigureArguments = @("--preset", $ConfigurePreset)
    if ($UcrtRuntime) {
        $ConfigureArguments += "-DTDFA_TEST_RUNTIME_PATHS=$UcrtRuntime"
    }
    if ($ClangSanitizerRuntime) {
        $ConfigureArguments += "-DTDFA_CLANG_SANITIZER_RUNTIME=$ClangSanitizerRuntime"
    }
    if ($ConfigurePreset -eq "coverage-gcc" -and $UcrtRuntime) {
        $GcovExecutable = Join-Path $UcrtRuntime "gcov.exe"
        $GcovrExecutable = Join-Path $UcrtRuntime "gcovr.exe"
        if (-not (Test-Path -LiteralPath $GcovExecutable -PathType Leaf)) {
            throw "The configured UCRT runtime does not contain gcov: $GcovExecutable"
        }
        if (-not (Test-Path -LiteralPath $GcovrExecutable -PathType Leaf)) {
            throw "The configured UCRT runtime does not contain gcovr: $GcovrExecutable"
        }
        $ConfigureArguments += "-DTDFA_GCOV_EXECUTABLE=$GcovExecutable"
        $ConfigureArguments += "-DTDFA_GCOVR_EXECUTABLE=$GcovrExecutable"
    }
    Invoke-Checked "cmake" $ConfigureArguments
}

function Invoke-ConfigureBuildTest {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ConfigurePreset,

        [Parameter(Mandatory = $true)]
        [string]$TestPreset,

        [string]$JunitPath
    )

    if ($JunitPath) {
        if (Test-Path -LiteralPath $JunitPath) {
            Remove-Item -Force -LiteralPath $JunitPath
        }
    }

    Invoke-ConfigurePreset $ConfigurePreset
    Invoke-Checked "cmake" @("--build", "--preset", $ConfigurePreset, "--parallel", $Jobs)
    $CTestArguments = @("--preset", $TestPreset)
    if ($JunitPath) {
        $CTestArguments += @("--output-junit", $JunitPath)
    }
    Invoke-Checked "ctest" $CTestArguments
}

function Invoke-StaticTraceability {
    $script:ActualSourceSnapshot = Get-ActualSourceSnapshot
    if ($ExpectedSourceSnapshot -ne $ActualSourceSnapshot) {
        throw (
            "Source snapshot mismatch: expected $ExpectedSourceSnapshot, " +
            "actual $ActualSourceSnapshot"
        )
    }
    Invoke-Python @("tools/generate_api_dossier.py", "--check")
    Invoke-Python @("tools/validate_traceability.py")
}

function Invoke-ExecutionTraceability {
    Invoke-Python @(
        "tools/validate_traceability.py",
        "--junit", $DebugJunitPath,
        "--junit", $ReleaseJunitPath
    )
}

function Invoke-CoverageAudit {
    if (-not (Get-Command "gcovr" -ErrorAction SilentlyContinue)) {
        throw "gcovr is required for Coverage. Install the UCRT64 gcovr package and retry."
    }
    Invoke-ConfigurePreset "coverage-gcc"
    Invoke-Checked "cmake" @(
        "--build", "--preset", "coverage-gcc", "--target", "coverage",
        "--parallel", $Jobs
    )
    if ($RunDirectory -and (Test-Path -LiteralPath $CoverageDirectory -PathType Container)) {
        Copy-Item -LiteralPath $CoverageDirectory -Destination $RunDirectory -Recurse -Force
    }
}

function Get-ActiveCoverageDirectory {
    if ($RunDirectory) {
        return Join-Path $RunDirectory "coverage"
    }
    return $CoverageDirectory
}

function Assert-CoverageInstrumentedSuite {
    $EvidenceDirectory = Get-ActiveCoverageDirectory
    Invoke-PythonEvidence @(
        "tools/check_coverage_evidence.py",
        "tests",
        (Join-Path $EvidenceDirectory "test-result.txt")
    )
}

function Assert-LineCoverage95 {
    $EvidenceDirectory = Get-ActiveCoverageDirectory
    Invoke-PythonEvidence @(
        "tools/check_coverage_evidence.py",
        "lines",
        (Join-Path $EvidenceDirectory "summary.json"),
        "--minimum", "95"
    )
}

function Invoke-FocusedCoverageAudit {
    Invoke-CoverageAudit
    $Failures = [System.Collections.Generic.List[string]]::new()
    foreach ($Check in @(
        [pscustomobject]@{ name = "instrumented suite"; action = { Assert-CoverageInstrumentedSuite } },
        [pscustomobject]@{ name = "95% line threshold"; action = { Assert-LineCoverage95 } }
    )) {
        try {
            & $Check.action
        }
        catch {
            $Message = "$($Check.name): $($_.Exception.Message)"
            Write-Warning $Message
            $Failures.Add($Message)
        }
    }
    if ($Failures.Count -ne 0) {
        throw ($Failures -join "; ")
    }
}

function Invoke-SanitizerAudit {
    $SanitizerJunit = if ($RunDirectory) { Join-Path $RunDirectory "sanitizer.xml" } else { $null }
    $LastTestLog = Join-Path $SanitizerBuildDirectory "Testing/Temporary/LastTest.log"
    $LastFailedLog = Join-Path $SanitizerBuildDirectory "Testing/Temporary/LastTestsFailed.log"
    foreach ($StaleArtifact in @($SanitizerJunit, $LastTestLog, $LastFailedLog)) {
        if ($StaleArtifact -and (Test-Path -LiteralPath $StaleArtifact -PathType Leaf)) {
            Remove-Item -Force -LiteralPath $StaleArtifact
        }
    }

    $Failure = $null
    try {
        Invoke-ConfigureBuildTest "sanitize-clang" "test-adversarial" $SanitizerJunit
    }
    catch {
        $Failure = $_
    }
    if ($RunDirectory) {
        $Destination = Join-Path $RunDirectory "sanitizer"
        New-Item -ItemType Directory -Force -Path $Destination | Out-Null
        foreach ($Source in @($LastTestLog, $LastFailedLog)) {
            if (Test-Path -LiteralPath $Source -PathType Leaf) {
                Copy-Item -LiteralPath $Source -Destination $Destination -Force
            }
        }
    }
    if ($null -ne $Failure) {
        throw $Failure
    }
}

function Invoke-FuzzAudit {
    $ResultPath = Join-Path $FuzzBuildDirectory "fuzz-results.txt"
    $ArtifactSource = Join-Path $FuzzBuildDirectory "fuzz-artifacts"
    $script:FuzzArtifactsBefore = Get-DirectoryEvidence $ArtifactSource
    if (Test-Path -LiteralPath $ResultPath -PathType Leaf) {
        Remove-Item -Force -LiteralPath $ResultPath
    }
    $Failure = $null
    try {
        Invoke-ConfigurePreset "fuzz-clang"
        Invoke-Checked "cmake" @(
            "--build", "--preset", "fuzz-clang", "--target", "fuzz-smoke",
            "--parallel", $Jobs
        )
    }
    catch {
        $Failure = $_
    }
    if ($RunDirectory) {
        $Destination = Join-Path $RunDirectory "fuzz"
        New-Item -ItemType Directory -Force -Path $Destination | Out-Null
        if (Test-Path -LiteralPath $ResultPath -PathType Leaf) {
            Copy-Item -LiteralPath $ResultPath -Destination $Destination -Force
        }
        if ((Test-Path -LiteralPath $ResultPath -PathType Leaf) -and
                (Test-Path -LiteralPath $ArtifactSource -PathType Container)) {
            Copy-Item -LiteralPath $ArtifactSource -Destination $Destination -Recurse -Force
        }
    }
    if ($null -ne $Failure) {
        throw $Failure
    }
}

function Invoke-MutationAudit {
    $MutationScript = Join-Path $PSScriptRoot "mutate.py"
    if (-not (Test-Path -LiteralPath $MutationScript)) {
        throw "Mutation runner is not installed at $MutationScript"
    }
    $ReportPath = Join-Path $MutationDirectory "report.json"
    if (Test-Path -LiteralPath $ReportPath -PathType Leaf) {
        Remove-Item -Force -LiteralPath $ReportPath
    }
    $Failure = $null
    try {
        Invoke-Python @($MutationScript)
    }
    catch {
        $Failure = $_
    }
    if ($RunDirectory -and (Test-Path -LiteralPath $ReportPath -PathType Leaf)) {
        $Destination = Join-Path $RunDirectory "mutation"
        New-Item -ItemType Directory -Force -Path $Destination | Out-Null
        Copy-Item -LiteralPath $ReportPath -Destination $Destination -Force
    }
    if ($null -ne $Failure) {
        throw $Failure
    }
}

function Get-CoverageEvidence {
    $EvidenceDirectory = Get-ActiveCoverageDirectory
    $SummaryPath = Join-Path $EvidenceDirectory "summary.json"
    $Evidence = [ordered]@{
        test_result = Get-FileEvidence (Join-Path $EvidenceDirectory "test-result.txt")
        summary_json = Get-FileEvidence $SummaryPath
        lcov = Get-FileEvidence (Join-Path $EvidenceDirectory "coverage.info")
        text = Get-FileEvidence (Join-Path $EvidenceDirectory "coverage.txt")
        html = Get-FileEvidence (Join-Path $EvidenceDirectory "index.html")
        metrics = $null
        read_error = $null
    }
    if (Test-Path -LiteralPath $SummaryPath -PathType Leaf) {
        try {
            $Summary = Get-Content -Raw -LiteralPath $SummaryPath | ConvertFrom-Json
            $Evidence.metrics = [ordered]@{
                lines = [ordered]@{
                    covered = $Summary.line_covered
                    total = $Summary.line_total
                    percent = $Summary.line_percent
                }
                functions = [ordered]@{
                    covered = $Summary.function_covered
                    total = $Summary.function_total
                    percent = $Summary.function_percent
                }
                branches = [ordered]@{
                    covered = $Summary.branch_covered
                    total = $Summary.branch_total
                    percent = $Summary.branch_percent
                }
                conditions = [ordered]@{
                    covered = $Summary.condition_covered
                    total = $Summary.condition_total
                    percent = $Summary.condition_percent
                }
                decisions = [ordered]@{
                    covered = $Summary.decision_covered
                    total = $Summary.decision_total
                    percent = $Summary.decision_percent
                }
            }
        }
        catch {
            $Evidence.read_error = $_.Exception.Message
        }
    }
    return [pscustomobject]$Evidence
}

function Get-AuditArtifactEvidence {
    $SanitizerDirectory = Join-Path $RunDirectory "sanitizer"
    $FuzzDirectory = Join-Path $RunDirectory "fuzz"
    $MutationRunDirectory = Join-Path $RunDirectory "mutation"
    $RetainedFuzzArtifacts = Get-DirectoryEvidence (Join-Path $FuzzDirectory "fuzz-artifacts")
    $FuzzArtifactsChanged = $null
    if ($FuzzArtifactsBefore -and $FuzzArtifactsBefore.exists -and
            $RetainedFuzzArtifacts.exists) {
        $FuzzArtifactsChanged = (
            $FuzzArtifactsBefore.file_count -ne $RetainedFuzzArtifacts.file_count -or
            $FuzzArtifactsBefore.sha256 -ne $RetainedFuzzArtifacts.sha256
        )
    }
    return [pscustomobject][ordered]@{
        sanitizer = [ordered]@{
            junit = Get-FileEvidence (Join-Path $RunDirectory "sanitizer.xml")
            last_test_log = Get-FileEvidence (Join-Path $SanitizerDirectory "LastTest.log")
            failed_tests = Get-FileEvidence (Join-Path $SanitizerDirectory "LastTestsFailed.log")
        }
        fuzz = [ordered]@{
            results = Get-FileEvidence (Join-Path $FuzzDirectory "fuzz-results.txt")
            retained_artifacts_before = $FuzzArtifactsBefore
            retained_artifacts_snapshot = $RetainedFuzzArtifacts
            retained_artifacts_changed = $FuzzArtifactsChanged
        }
        mutation = [ordered]@{
            report = Get-FileEvidence (Join-Path $MutationRunDirectory "report.json")
        }
    }
}

function Get-VerificationInputEvidence {
    $Evidence = [ordered]@{
        start_sha256 = if ($VerificationInputStart) { $VerificationInputStart.sha256 } else { $null }
        start_file_count = if ($VerificationInputStart) { $VerificationInputStart.file_count } else { $null }
        current_sha256 = $null
        current_file_count = $null
        matches_start = $false
        read_error = $null
    }
    try {
        $Current = Get-VerificationInputFingerprint
        $Evidence.current_sha256 = $Current.sha256
        $Evidence.current_file_count = $Current.file_count
        $Evidence.matches_start = (
            $VerificationInputStart -and
            $VerificationInputStart.sha256 -eq $Current.sha256 -and
            $VerificationInputStart.file_count -eq $Current.file_count
        )
    }
    catch {
        $Evidence.read_error = $_.Exception.Message
    }
    return [pscustomobject]$Evidence
}

function Assert-VerificationInputsStable {
    $script:TerminalVerificationInputEvidence = Get-VerificationInputEvidence
    if ($TerminalVerificationInputEvidence.read_error) {
        throw "Could not seal verification inputs: $($TerminalVerificationInputEvidence.read_error)"
    }
    if (-not $TerminalVerificationInputEvidence.matches_start) {
        throw (
            "Verification inputs changed during the run: start " +
            "$($TerminalVerificationInputEvidence.start_sha256)/" +
            "$($TerminalVerificationInputEvidence.start_file_count), end " +
            "$($TerminalVerificationInputEvidence.current_sha256)/" +
            "$($TerminalVerificationInputEvidence.current_file_count)"
        )
    }
    if (-not $ActualSourceSnapshot -or $ExpectedSourceSnapshot -ne $ActualSourceSnapshot) {
        throw (
            "Source snapshot is not coherent: expected $ExpectedSourceSnapshot, " +
            "actual $ActualSourceSnapshot"
        )
    }
}

function Write-VerificationReport {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Mode,

        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[object]]$Results,

        [Parameter(Mandatory = $true)]
        [string]$ReportPath,

        [Parameter(Mandatory = $true)]
        [string]$LatestReportPath,

        [Parameter(Mandatory = $true)]
        [bool]$IncludeCoverage,

        [Parameter(Mandatory = $true)]
        [ValidateSet("RUNNING", "PASS", "FAIL")]
        [string]$PipelineStatus
    )

    $SnapshotEvidence = [ordered]@{
        expected = $ExpectedSourceSnapshot
        actual = $ActualSourceSnapshot
        matches = (
            $null -ne $ActualSourceSnapshot -and
            $ExpectedSourceSnapshot -eq $ActualSourceSnapshot
        )
    }
    $Report = [ordered]@{
        schema_version = 4
        mode = $Mode
        run_id = Split-Path -Leaf $RunDirectory
        run_directory = $RunDirectory
        generated_utc = [DateTimeOffset]::UtcNow.ToString("O")
        pipeline_status = $PipelineStatus
        complete = $PipelineStatus -ne "RUNNING"
        source_snapshot = $SnapshotEvidence
        verification_inputs = if ($TerminalVerificationInputEvidence) {
            $TerminalVerificationInputEvidence
        }
        else {
            Get-VerificationInputEvidence
        }
        junit = [ordered]@{
            debug_fast = Get-FileEvidence $DebugJunitPath
            release_deep = Get-FileEvidence $ReleaseJunitPath
        }
        stages = $Results
    }
    if ($IncludeCoverage) {
        $Report["coverage"] = Get-CoverageEvidence
        $Report["audit_artifacts"] = Get-AuditArtifactEvidence
    }

    $Json = $Report | ConvertTo-Json -Depth 8
    $Serialized = $Json + [Environment]::NewLine
    Write-AtomicUtf8 $ReportPath $Serialized
    Write-AtomicUtf8 $LatestReportPath $Serialized
}

function Invoke-RecordedStage {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,

        [Parameter(Mandatory = $true)]
        [scriptblock]$Action,

        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[object]]$Results,

        [Parameter(Mandatory = $true)]
        [string]$Mode,

        [Parameter(Mandatory = $true)]
        [string]$ReportPath,

        [Parameter(Mandatory = $true)]
        [string]$LatestReportPath,

        [Parameter(Mandatory = $true)]
        [bool]$IncludeCoverage
    )

    Write-Host "`n=== $Mode stage: $Name ==="
    $Started = [DateTimeOffset]::UtcNow
    $Stage = [pscustomobject]@{
        name = $Name
        status = "RUNNING"
        started_utc = $Started.ToString("O")
        finished_utc = $null
        detail = $null
    }
    $Results.Add($Stage)
    Write-VerificationReport $Mode $Results $ReportPath $LatestReportPath $IncludeCoverage "RUNNING"
    try {
        & $Action
        $Stage.status = "PASS"
    }
    catch {
        $Detail = $_.Exception.Message
        Write-Warning "$Name failed: $Detail"
        $Stage.status = "FAIL"
        $Stage.detail = $Detail
    }
    finally {
        $Stage.finished_utc = [DateTimeOffset]::UtcNow.ToString("O")
        Write-VerificationReport $Mode $Results $ReportPath $LatestReportPath $IncludeCoverage "RUNNING"
    }
}

function Invoke-VerificationPipeline {
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet("Routine", "Audit")]
        [string]$Mode
    )

    New-Item -ItemType Directory -Force -Path $VerificationDirectory | Out-Null
    $RunId = [DateTimeOffset]::UtcNow.ToString("yyyyMMdd'T'HHmmssfffffff'Z'") + "-$PID"
    $script:RunDirectory = Join-Path $VerificationDirectory (Join-Path "runs" $RunId)
    New-Item -ItemType Directory -Force -Path $RunDirectory | Out-Null
    $script:DebugJunitPath = Join-Path $RunDirectory "debug-fast.xml"
    $script:ReleaseJunitPath = Join-Path $RunDirectory "release-deep.xml"
    $script:VerificationInputStart = Get-VerificationInputFingerprint
    $script:ExpectedSourceSnapshot = Get-ExpectedSourceSnapshot
    $script:TerminalVerificationInputEvidence = $null
    $script:FuzzArtifactsBefore = $null

    $IncludeCoverage = $Mode -eq "Audit"
    if ($IncludeCoverage -and
            (Test-Path -LiteralPath $CoverageDirectory -PathType Container)) {
        Remove-Item -LiteralPath $CoverageDirectory -Recurse -Force
    }

    $Results = [System.Collections.Generic.List[object]]::new()
    $ReportPath = Join-Path $RunDirectory "summary.json"
    $LatestReportPath = Join-Path $VerificationDirectory "$($Mode.ToLowerInvariant())-summary.json"
    Write-VerificationReport $Mode $Results $ReportPath $LatestReportPath $IncludeCoverage "RUNNING"

    Invoke-RecordedStage "static-traceability" {
        Invoke-StaticTraceability
    } $Results $Mode $ReportPath $LatestReportPath $IncludeCoverage
    Invoke-RecordedStage "debug-fast" {
        Invoke-ConfigureBuildTest "test-debug" "test-fast" $DebugJunitPath
    } $Results $Mode $ReportPath $LatestReportPath $IncludeCoverage
    Invoke-RecordedStage "optimized-fast-deep" {
        Invoke-ConfigureBuildTest "test-release" "test-deep" $ReleaseJunitPath
    } $Results $Mode $ReportPath $LatestReportPath $IncludeCoverage
    Invoke-RecordedStage "execution-traceability" {
        Invoke-ExecutionTraceability
    } $Results $Mode $ReportPath $LatestReportPath $IncludeCoverage

    if ($Mode -eq "Audit") {
        Invoke-RecordedStage "coverage-evidence" {
            Invoke-CoverageAudit
        } $Results $Mode $ReportPath $LatestReportPath $IncludeCoverage
        Invoke-RecordedStage "coverage-instrumented-suite" {
            Assert-CoverageInstrumentedSuite
        } $Results $Mode $ReportPath $LatestReportPath $IncludeCoverage
        Invoke-RecordedStage "line-coverage-95" {
            Assert-LineCoverage95
        } $Results $Mode $ReportPath $LatestReportPath $IncludeCoverage
        Invoke-RecordedStage "asan-ubsan" {
            Invoke-SanitizerAudit
        } $Results $Mode $ReportPath $LatestReportPath $IncludeCoverage
        Invoke-RecordedStage "persistent-fuzz" {
            Invoke-FuzzAudit
        } $Results $Mode $ReportPath $LatestReportPath $IncludeCoverage
        Invoke-RecordedStage "curated-mutation" {
            Invoke-MutationAudit
        } $Results $Mode $ReportPath $LatestReportPath $IncludeCoverage
    }

    Invoke-RecordedStage "input-coherence" {
        Assert-VerificationInputsStable
    } $Results $Mode $ReportPath $LatestReportPath $IncludeCoverage

    $Failed = @($Results | Where-Object { $_.status -eq "FAIL" })
    $PipelineStatus = if ($Failed.Count -eq 0) { "PASS" } else { "FAIL" }
    Write-VerificationReport $Mode $Results $ReportPath $LatestReportPath $IncludeCoverage $PipelineStatus

    Write-Host "`n$Mode summary:"
    $Results | Format-Table -AutoSize name, status, detail
    Write-Host "Immutable run summary: $ReportPath"
    Write-Host "Latest-run summary: $LatestReportPath"
    if ($Failed.Count -ne 0) {
        throw "$Mode completed with $($Failed.Count) failed stage(s); all stages were attempted."
    }
}

Push-Location $ProjectRoot
try {
    switch ($Tier) {
        "Routine" {
            Invoke-VerificationPipeline "Routine"
        }
        "Audit" {
            Invoke-VerificationPipeline "Audit"
        }
        "Fast" {
            Invoke-ConfigureBuildTest "test-debug" "test-fast"
        }
        "Deep" {
            Invoke-ConfigureBuildTest "test-release" "test-deep"
        }
        "Coverage" {
            Invoke-FocusedCoverageAudit
        }
        "Adversarial" {
            Invoke-ConfigureBuildTest "sanitize-clang" "test-adversarial"
        }
        "Fuzz" {
            Invoke-FuzzAudit
        }
        "Mutation" {
            Invoke-MutationAudit
        }
    }
}
finally {
    Pop-Location
}
