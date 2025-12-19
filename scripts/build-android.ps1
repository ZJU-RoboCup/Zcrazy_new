param(
    [string]$AndroidSdkRoot = $env:ANDROID_SDK_ROOT,
    [string]$AndroidNdk     = $env:ANDROID_NDK,
    [string]$QtAndroidPrefix = 'F:/QT/6.5.3/android_arm64_v8a',
    [switch]$Clean
)

Write-Host "=== zcrazy Android build helper ==="

if (-not $AndroidSdkRoot -or -not (Test-Path $AndroidSdkRoot)) {
    Write-Host "ANDROID_SDK_ROOT is not set or path not found." -ForegroundColor Yellow
    Write-Host "Please set ANDROID_SDK_ROOT to your Android SDK, e.g.:" -ForegroundColor Yellow
    Write-Host "  $env:USERPROFILE\AppData\Local\Android\Sdk" -ForegroundColor Yellow
    exit 1
}
if (-not $AndroidNdk -or -not (Test-Path (Join-Path $AndroidNdk 'build/cmake/android.toolchain.cmake'))) {
    Write-Host "ANDROID_NDK is not set or invalid. It must point to NDK root containing build/cmake/android.toolchain.cmake" -ForegroundColor Yellow
    Write-Host "Example:" -ForegroundColor Yellow
    Write-Host "  $AndroidSdkRoot\ndk\26.1.10909125" -ForegroundColor Yellow
    exit 1
}

Write-Host "Using ANDROID_SDK_ROOT=$AndroidSdkRoot"
Write-Host "Using ANDROID_NDK=$AndroidNdk"
Write-Host "Using Qt Android prefix=$QtAndroidPrefix"

$env:ANDROID_SDK_ROOT = $AndroidSdkRoot
$env:ANDROID_NDK = $AndroidNdk

if ($Clean -and (Test-Path 'out/build/android-qt-arm64')) {
    Write-Host "Cleaning previous build directory..."
    Remove-Item -Recurse -Force 'out/build/android-qt-arm64'
}

# Ensure ninja is visible
$ninja = (Get-Command ninja -ErrorAction SilentlyContinue)
if (-not $ninja) {
    Write-Host "Warning: ninja not found in PATH. Make sure your environment provides ninja." -ForegroundColor Yellow
}

# Sanity check Qt package config exists
$qtConfig = Join-Path $QtAndroidPrefix 'lib/cmake/Qt6/Qt6Config.cmake'
if (-not (Test-Path $qtConfig)) {
    Write-Host "ERROR: Qt6Config.cmake not found at $qtConfig" -ForegroundColor Red
    Write-Host "Please install Qt for Android (arm64-v8a) base modules via Qt Maintenance Tool." -ForegroundColor Yellow
    Write-Host "Path should contain lib/cmake/Qt6/Qt6Config.cmake and modules like Qt6Core, Qt6Quick, Qt6Network." -ForegroundColor Yellow
    exit 1
}

Write-Host "Configuring (cmake --preset android-qt-arm64) ..." -ForegroundColor Cyan
cmake --preset android-qt-arm64
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Building (cmake --build --preset android-qt-arm64 -j 8) ..." -ForegroundColor Cyan
cmake --build --preset android-qt-arm64 -j 8
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# Print APK path hint
$apk = Get-ChildItem -Recurse -Filter *-debug.apk -ErrorAction SilentlyContinue | Where-Object { $_.FullName -like "*android-qt-arm64*" } | Select-Object -First 1 -ExpandProperty FullName
if ($apk) {
    Write-Host "APK built: $apk" -ForegroundColor Green
    Write-Host "Install with: adb install -r `"$apk`""
} else {
    Write-Host "Could not find debug APK automatically; check out/build/android-qt-arm64/android-build/build/outputs/apk/debug/" -ForegroundColor Yellow
}
