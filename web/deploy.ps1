<#
  Copy the web build into the gh-pages branch under /play/ (one-way deploy).
  - Source lives in master web/; gh-pages play/ must never be edited by hand (docs/web_port_plan.md section 3)
  - gh-pages is handled in a separate worktree (../Hanoi-gh-pages) so the current working tree is untouched
  - This script does NOT commit or push. Review the result, then commit/push yourself.
  - Keep this file ASCII-only: Windows PowerShell 5.1 misreads UTF-8 without BOM.

  Usage (from the web folder):  .\deploy.ps1
#>
$ErrorActionPreference = 'Stop'

$webDir   = $PSScriptRoot
$repoRoot = (git -C $webDir rev-parse --show-toplevel).Trim()
$wtDir    = Join-Path (Split-Path $repoRoot -Parent) 'Hanoi-gh-pages'

Write-Host "==> 1/4 build" -ForegroundColor Cyan
Push-Location $webDir
try {
    if (-not (Test-Path (Join-Path $webDir 'node_modules'))) {
        npm install
        if ($LASTEXITCODE) { throw 'npm install failed' }
    }
    npm run build
    if ($LASTEXITCODE) { throw 'build failed' }
} finally { Pop-Location }

Write-Host "==> 2/4 prepare gh-pages worktree: $wtDir" -ForegroundColor Cyan
if (-not (Test-Path $wtDir)) {
    git -C $repoRoot fetch origin gh-pages
    git -C $repoRoot worktree add $wtDir gh-pages
    if ($LASTEXITCODE) { throw 'worktree add failed (is gh-pages checked out elsewhere? release it first)' }
}
# the local gh-pages branch may be behind origin - fast-forward before touching anything
git -C $wtDir pull --ff-only origin gh-pages
if ($LASTEXITCODE) { throw 'gh-pages pull failed - check the worktree state' }

Write-Host "==> 3/4 replace /play/" -ForegroundColor Cyan
$dest = Join-Path $wtDir 'play'
if (Test-Path $dest) { Remove-Item $dest -Recurse -Force }
Copy-Item (Join-Path $webDir 'dist') $dest -Recurse

Write-Host "==> 4/4 changes" -ForegroundColor Cyan
git -C $wtDir status --short

Write-Host ""
Write-Host "Copied. Review, then commit and push yourself:" -ForegroundColor Green
Write-Host "  cd `"$wtDir`""
Write-Host "  git add play"
Write-Host "  git commit -m `"[web] update play build`""
Write-Host "  git push origin gh-pages"
Write-Host "URL: https://ozzywow.github.io/Hanoi/play/"
