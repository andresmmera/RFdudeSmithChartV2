param([Parameter(Mandatory = $true)][string]$Installer)

$ErrorActionPreference = "Stop"
if ($env:GITHUB_ACTIONS -ne "true" -or $env:RUNNER_OS -ne "Windows") {
    throw "Run installer lifecycle tests only on a disposable Windows Actions runner"
}
$root = Join-Path $env:RUNNER_TEMP "RFdude installer test"
$logs = Join-Path $PWD "installer-logs"
New-Item -ItemType Directory -Force -Path $logs | Out-Null
foreach ($name in @("QT_PLUGIN_PATH", "QT_QPA_PLATFORM_PLUGIN_PATH", "QT_QPA_PLATFORMTHEME", "QT_QPA_PLATFORM", "QTDIR", "QT_ROOT_DIR")) {
    Remove-Item "Env:$name" -ErrorAction SilentlyContinue
}
$env:QT_QPA_PLATFORM = "windows"
$env:QT_DEBUG_PLUGINS = "1"
# This job never installs a Qt SDK; also exclude any runner Qt/Python plugin paths.
$env:PATH = (($env:PATH -split ";" | Where-Object { $_ -notmatch '(?i)(Qt|Python)' }) -join ";")

Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class RFdudeIconProbe {
    [DllImport("shell32.dll", CharSet = CharSet.Unicode)]
    public static extern uint ExtractIconEx(string file, int index,
        [Out] IntPtr[] largeIcons, [Out] IntPtr[] smallIcons, uint count);
    [DllImport("user32.dll")]
    public static extern bool DestroyIcon(IntPtr icon);
}
"@

function Assert-ExecutableIcon([string]$Executable) {
    $large = [IntPtr[]]::new(1)
    $small = [IntPtr[]]::new(1)
    try {
        $count = [RFdudeIconProbe]::ExtractIconEx($Executable, 0, $large, $small, 1)
        if ($count -eq 0 -or $count -eq [uint32]::MaxValue -or
            $large[0] -eq [IntPtr]::Zero -or $small[0] -eq [IntPtr]::Zero) {
            throw "Executable is missing extractable large/small Windows icons: $Executable (count=$count, large=$($large[0]), small=$($small[0]))"
        }
        Write-Output "PASS: executable icons extracted (count=$count, large=$($large[0]), small=$($small[0]))"
    } finally {
        foreach ($icon in @($large[0], $small[0])) {
            if ($icon -ne [IntPtr]::Zero) { [void][RFdudeIconProbe]::DestroyIcon($icon) }
        }
    }
}

function Get-SystemQtState {
    $state = @{}
    foreach ($directory in @("$env:WINDIR\System32", "$env:WINDIR\SysWOW64")) {
        Get-ChildItem "$directory\Qt*.dll" -ErrorAction SilentlyContinue | ForEach-Object {
            $state[$_.FullName] = (Get-FileHash $_.FullName -Algorithm SHA256).Hash
        }
    }
    foreach ($scope in @("Machine", "User")) {
        $state["$scope PATH"] = [Environment]::GetEnvironmentVariable("PATH", $scope)
    }
    return $state
}

function Assert-SystemQtUnchanged($Before) {
    $after = Get-SystemQtState
    $keys = @($Before.Keys) + @($after.Keys)
    foreach ($key in ($keys | Select-Object -Unique)) {
        if ($Before[$key] -ne $after[$key]) { throw "Installer changed shared runtime or PATH: $key" }
    }
}

function Invoke-Installer([string]$Program, [string[]]$Arguments) {
    $process = Start-Process -FilePath $Program -ArgumentList $Arguments -PassThru -Wait
    if ($process.ExitCode -ne 0) { throw "$Program failed with exit $($process.ExitCode)" }
}

function Test-Application {
    $executable = Join-Path $root "bin\RFdudeSmithChartV2.exe"
    foreach ($file in @($executable, (Join-Path $root "plugins\platforms\qwindows.dll"))) {
        if (-not (Test-Path $file)) { throw "Missing installed runtime: $file" }
    }
    Assert-ExecutableIcon $executable
    $example = Join-Path $root "share\RFdudeSmithChartV2\examples\network.rfs_45Mfilter.json"
    if (-not (Test-Path $example)) { throw "Installed example is missing" }
    $process = Start-Process -FilePath $executable -ArgumentList "`"$example`"" -PassThru
    try {
        $deadline = (Get-Date).AddSeconds(30)
        do {
            $process.Refresh()
            if ($process.HasExited) { throw "Installed application exited early: $($process.ExitCode)" }
            if ($process.MainWindowHandle -ne 0) { break }
            Start-Sleep -Milliseconds 250
        } while ((Get-Date) -lt $deadline)
        if ($process.MainWindowHandle -eq 0) { throw "Installed application never created a native window" }
        $modules = @($process.Modules)
        $modules | ForEach-Object { $_.FileName } | Set-Content (Join-Path $logs "loaded-modules.txt")
        $qt = @($modules | Where-Object { $_.ModuleName -match '^Qt6.*\.dll$|^qwindows\.dll$' })
        if (-not ($qt | Where-Object { $_.ModuleName -eq "Qt6Widgets.dll" }) -or
            -not ($qt | Where-Object { $_.ModuleName -eq "qwindows.dll" })) {
            throw "Qt Widgets and the Windows platform plugin were not loaded"
        }
        foreach ($module in $qt) {
            if (-not $module.FileName.StartsWith("$root\", [StringComparison]::OrdinalIgnoreCase)) {
                throw "Qt module loaded outside installation: $($module.FileName)"
            }
        }
        if (-not $process.CloseMainWindow()) { throw "Could not close the installed application" }
        if (-not $process.WaitForExit(15000)) { throw "Application did not exit after closing its window" }
        if ($process.ExitCode -ne 0) { throw "Application exited with $($process.ExitCode)" }
    } finally {
        if (-not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
    }
}

$installerPath = (Resolve-Path $Installer).Path
$before = Get-SystemQtState
Invoke-Installer $installerPath @("/VERYSILENT", "/SUPPRESSMSGBOXES", "/NORESTART", "/SP-", "/TASKS=desktopicon", "/DIR=`"$root`"", "/LOG=`"$logs\install.txt`"")
Assert-SystemQtUnchanged $before
Test-Application
$shortcut = Join-Path ([Environment]::GetFolderPath("CommonPrograms")) "RFdude Smith Chart V2.lnk"
$desktopShortcut = Join-Path ([Environment]::GetFolderPath("CommonDesktopDirectory")) "RFdude Smith Chart V2.lnk"
$shell = New-Object -ComObject WScript.Shell
foreach ($path in @($shortcut, $desktopShortcut)) {
    if (-not (Test-Path $path)) { throw "Installed shortcut is missing: $path" }
    $link = $shell.CreateShortcut($path)
    $executable = Join-Path $root "bin\RFdudeSmithChartV2.exe"
    if ($link.TargetPath -ne $executable -or $link.IconLocation -ne "$executable,0") {
        throw "Shortcut does not target the installed executable and its icon: $path"
    }
}
Invoke-Installer $installerPath @("/VERYSILENT", "/SUPPRESSMSGBOXES", "/NORESTART", "/SP-", "/TASKS=desktopicon", "/DIR=`"$root`"", "/LOG=`"$logs\reinstall.txt`"")
Assert-SystemQtUnchanged $before
Test-Application
Invoke-Installer (Join-Path $root "unins000.exe") @("/VERYSILENT", "/SUPPRESSMSGBOXES", "/NORESTART", "/LOG=`"$logs\uninstall.txt`"")
Assert-SystemQtUnchanged $before
if ((Test-Path $root) -or (Test-Path $shortcut) -or (Test-Path $desktopShortcut)) {
    throw "Uninstall left the installation directory or a shortcut behind"
}
Write-Output "PASS: silent install, native icons and shortcuts, native window, private Qt modules, reinstall, uninstall"
