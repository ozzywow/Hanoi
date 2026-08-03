<#
.SYNOPSIS
    웹 게시판(Runners' Board) 관리 도구 — PlayFab 경유.

.DESCRIPTION
    UGC 신고 대응용. Shared Group "board" 의 글을 조회하고, 글 단위 / 작성자 단위로
    삭제하거나, 악용 계정의 웹 세션을 끊는다.
    조회는 Client/GetSharedGroupData 직접 호출(보드는 Permission=Public),
    삭제·차단은 CloudScript adminDeleteBoardPost / revokeWebSession 을 쓰며
    Title Internal Data 의 admin_key 로 권한을 검증한다.

    ⚠ 이 저장소는 public 이므로 admin_key 를 파일에 적지 않는다.
      환경변수로 넘기거나, 없으면 실행 시 입력받는다.
          $env:HANOI_ADMIN_KEY = "<키>"

.EXAMPLE
    .\admin_board.ps1
        전체 글 조회 (최신순, 한글 자동 디코딩)

.EXAMPLE
    .\admin_board.ps1 -Delete -Id 1754198400000abc123
        글 하나 삭제

.EXAMPLE
    .\admin_board.ps1 -Delete -TargetId 38F18129ABAD5A47
        해당 작성자의 글 전부 삭제 (조회 후 건별 삭제)

.EXAMPLE
    .\admin_board.ps1 -Delete -TargetId 38F18129ABAD5A47 -Revoke
        글 전부 삭제 + 웹 세션 무효화 — 도배 계정 대응 조합.
        세션이 끊기면 앱에서 💬 를 다시 눌러야 글을 쓸 수 있다.

.EXAMPLE
    .\admin_board.ps1 -Delete -All -Force
        게시판 전체 비우기 (확인 없이)
#>
[CmdletBinding()]
param(
    [switch] $Delete,
    [switch] $Revoke,
    [string] $Id,
    [string] $TargetId,
    [switch] $All,
    [switch] $Force,
    [string] $Title    = "119C4E",
    [string] $AdminKey = $env:HANOI_ADMIN_KEY
)

$ErrorActionPreference = "Stop"
$Base = "https://$Title.playfabapi.com"

if ([string]::IsNullOrWhiteSpace($AdminKey)) {
    $sec = Read-Host "admin_key (환경변수 HANOI_ADMIN_KEY 로 미리 넣어둘 수 있음)" -AsSecureString
    $AdminKey = [Runtime.InteropServices.Marshal]::PtrToStringAuto(
                    [Runtime.InteropServices.Marshal]::SecureStringToBSTR($sec))
    if ([string]::IsNullOrWhiteSpace($AdminKey)) { throw "admin_key 가 필요합니다." }
}

# ── 로그인 → SessionTicket (실제 권한은 adminKey. 익명 계정으로 충분) ──
$login = Invoke-RestMethod -Uri "$Base/Client/LoginWithCustomID" -Method Post `
    -ContentType "application/json" `
    -Body (@{ TitleId = $Title; CustomId = "admin-console"; CreateAccount = $true } | ConvertTo-Json)
$ticket = $login.data.SessionTicket

function Invoke-Board([string]$Fn, [hashtable]$Params) {
    $Params.adminKey = $AdminKey
    $resp = Invoke-RestMethod -Uri "$Base/Client/ExecuteCloudScript" -Method Post `
        -ContentType "application/json" -Headers @{ "X-Authorization" = $ticket } `
        -Body (@{ FunctionName = $Fn; FunctionParameter = $Params; GeneratePlayStreamEvent = $false } | ConvertTo-Json -Depth 5)
    if ($resp.data.Error) {
        throw "CloudScript error: $($resp.data.Error.Error) - $($resp.data.Error.Message)"
    }
    return $resp.data.FunctionResult
}

# PowerShell 콘솔이 UTF-8 을 CP1252 로 오독해 생기는 한글 깨짐 복원 (admin_award.ps1 과 동일)
$latin1 = [System.Text.Encoding]::GetEncoding(28591)
$utf8   = [System.Text.Encoding]::UTF8
function Fix([string]$s) { if ($null -eq $s) { return "" }; return $utf8.GetString($latin1.GetBytes($s)) }

# 보드 전체 조회 → 최신순 정렬된 행 목록. 보드가 Public 이라 CloudScript 없이 바로 읽는다.
function Get-BoardPosts {
    $resp = Invoke-RestMethod -Uri "$Base/Client/GetSharedGroupData" -Method Post `
        -ContentType "application/json" -Headers @{ "X-Authorization" = $ticket } `
        -Body (@{ SharedGroupId = "board"; GetMembers = $false } | ConvertTo-Json)

    $rows = @()
    if ($null -eq $resp.data -or $null -eq $resp.data.Data) { return $rows }
    foreach ($p in $resp.data.Data.PSObject.Properties) {
        try {
            $j = $p.Value.Value | ConvertFrom-Json
            $rows += [pscustomobject]@{
                When   = [DateTimeOffset]::FromUnixTimeMilliseconds([int64]$j.ts).ToLocalTime().ToString("yyyy-MM-dd HH:mm")
                Name   = Fix $j.n
                CC     = $j.cc
                Text   = Fix $j.t
                Id     = $p.Name
                Author = $j.p
                Ts     = [int64]$j.ts
            }
        } catch {
            $rows += [pscustomobject]@{
                When = ""; Name = "(parse fail)"; CC = ""; Text = $p.Value.Value
                Id = $p.Name; Author = ""; Ts = 0
            }
        }
    }
    return ($rows | Sort-Object -Property Ts -Descending)
}

function Confirm-Or-Exit([string]$Message) {
    if ($Force) { return $true }
    $ans = Read-Host "$Message 진행할까요? (y/N)"
    if ($ans -ne "y" -and $ans -ne "Y") { Write-Host "취소됨."; return $false }
    return $true
}

# ── 세션 무효화 단독 실행 (-Revoke 만 준 경우) ──
if ($Revoke -and -not $Delete) {
    if ([string]::IsNullOrWhiteSpace($TargetId)) { throw "-Revoke 는 -TargetId 가 필요합니다." }
    if (-not (Confirm-Or-Exit "$TargetId 의 웹 세션을 무효화합니다.")) { return }
    $r = Invoke-Board "revokeWebSession" @{ playFabId = $TargetId }
    if ($r.ok) { Write-Host "세션 무효화 완료: $TargetId" -ForegroundColor Green }
    else       { Write-Host "실패: reason=$($r.reason)" -ForegroundColor Red }
    return
}

# ── 삭제 모드 ──
if ($Delete) {
    if ($All) {
        if (-not (Confirm-Or-Exit "게시판의 모든 글을 삭제합니다.")) { return }
        $r = Invoke-Board "adminDeleteBoardPost" @{}
        if ($r.ok) { Write-Host "전체 삭제 완료: $($r.deleted) 건" -ForegroundColor Green }
        else       { Write-Host "실패: reason=$($r.reason)" -ForegroundColor Red }
        return
    }

    if (-not [string]::IsNullOrWhiteSpace($TargetId)) {
        $mine = @(Get-BoardPosts | Where-Object { $_.Author -eq $TargetId })
        if ($mine.Count -eq 0) { Write-Host "해당 작성자의 글이 없습니다: $TargetId"; }
        else {
            $mine | Format-Table When, Name, Text -AutoSize -Wrap
            if (Confirm-Or-Exit "위 $($mine.Count) 건을 삭제합니다.") {
                $done = 0
                foreach ($row in $mine) {
                    $r = Invoke-Board "adminDeleteBoardPost" @{ id = $row.Id }
                    if ($r.ok) { $done++ }
                    else { Write-Host "  실패 $($row.Id): reason=$($r.reason)" -ForegroundColor Red }
                }
                Write-Host "삭제 완료: $done / $($mine.Count) 건" -ForegroundColor Green
            }
        }
        # -Delete 와 -Revoke 를 함께 준 경우: 삭제 후 세션까지 끊는다(도배 계정 대응)
        if ($Revoke) {
            if (Confirm-Or-Exit "이어서 $TargetId 의 웹 세션도 무효화합니다.") {
                $r = Invoke-Board "revokeWebSession" @{ playFabId = $TargetId }
                if ($r.ok) { Write-Host "세션 무효화 완료: $TargetId" -ForegroundColor Green }
                else       { Write-Host "세션 무효화 실패: reason=$($r.reason)" -ForegroundColor Red }
            }
        }
        return
    }

    if (-not [string]::IsNullOrWhiteSpace($Id)) {
        if (-not (Confirm-Or-Exit "글 $Id 을(를) 삭제합니다.")) { return }
        $r = Invoke-Board "adminDeleteBoardPost" @{ id = $Id }
        if ($r.ok) { Write-Host "삭제 완료: $Id" -ForegroundColor Green }
        else       { Write-Host "실패: reason=$($r.reason)" -ForegroundColor Red }
        return
    }

    throw "삭제하려면 -Id / -TargetId / -All 중 하나를 지정해야 합니다."
}

# ── 조회 모드 (기본) ──
$rows = @(Get-BoardPosts)
if ($rows.Count -eq 0) {
    Write-Host "등록된 글이 없습니다."
    return
}

$rows | Format-Table When, Name, CC, Text, Id, Author -AutoSize -Wrap
Write-Host ""
Write-Host "총 $($rows.Count) 건 (서버 링버퍼 상한 50)." -ForegroundColor DarkGray
Write-Host "  글 삭제      : .\admin_board.ps1 -Delete -Id <Id>"           -ForegroundColor DarkGray
Write-Host "  작성자 일괄  : .\admin_board.ps1 -Delete -TargetId <Author>" -ForegroundColor DarkGray
Write-Host "  도배 대응    : .\admin_board.ps1 -Delete -TargetId <Author> -Revoke" -ForegroundColor DarkGray
