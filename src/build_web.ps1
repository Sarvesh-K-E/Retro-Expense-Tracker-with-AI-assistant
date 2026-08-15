param(
    [string]$OutDir = "web"
)

$ErrorActionPreference = "Stop"

$emcc = Get-Command emcc -ErrorAction SilentlyContinue
if (-not $emcc) {
    throw "Emscripten (emcc) is not in PATH. Activate emsdk first (for example: emsdk_env.bat)."
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null



$cmd = @(
    "main.c"
    "emscripten_compat/runtime_init.c"
    "-I", "emscripten_compat"
    "--shell-file", "web_shell.html"
    "--pre-js", "web_terminal.js"
    "-s", "ALLOW_MEMORY_GROWTH=1"
    "-s", "FORCE_FILESYSTEM=1"
    "-s", "ASYNCIFY=1"
    "-s", "EXIT_RUNTIME=1"
    "-o", (Join-Path $OutDir "index.html")
)

Write-Host "Running: emcc $($cmd -join ' ')"
& emcc @cmd

Write-Host ""
Write-Host "Build complete."
Write-Host "To test locally, run any static server in the output folder:"
Write-Host "  python -m http.server 8000 --directory $OutDir"
Write-Host "Then visit http://localhost:8000"
Write-Host ""
Write-Host "To deploy, push the '$OutDir' folder to GitHub Pages."
