<#
.SYNOPSIS
    수상소감(Award Comment) 관리 도구 — PlayFab CloudScript 경유.

.DESCRIPTION
    UGC 신고 대응용. 전 레벨(L03~L10) 수상소감을 검색하거나, 특정 유저의 소감을 삭제한다.
    CloudScript 핸들러 dumpAwardComments / adminDeleteAwardComment 를 호출하며,
    Title Internal Data 의 admin_key 로 권한을 검증한다.

.EXAMPLE
    .\admin_award.ps1
        전체 소감 검색 (한글 자동 디코딩, 표 출력)

.EXAMPLE
    .\admin_award.ps1 -Delete -Level 3 -TargetId 38F18129ABAD5A47
        L3 의 해당 유저 소감 삭제 (확인 프롬프트 있음)

.EXAMPLE
    .\admin_award.ps1 -Delete -Level 3 -TargetId 38F18129ABAD5A47 -Force
        확인 없이 즉시 삭제
#>
[CmdletBinding()]
param(
    [switch] $Delete,
    [int]    $Level,
    [string] $TargetId,
    [switch] $Force,
    [string] $Title    = "119C4E",
    [string] $AdminKey = "@wlsrnr4387"
)

$ErrorActionPreference = "Stop"
$Base = "https://$Title.playfabapi.com"

# ── 로그인 → SessionTicket (adminKey 가 실제 권한, 익명 계정으로 충분) ──
$login = Invoke-RestMethod -Uri "$Base/Client/LoginWithCustomID" -Method Post `
    -ContentType "application/json" `
    -Body (@{ TitleId = $Title; CustomId = "admin-console"; CreateAccount = $true } | ConvertTo-Json)
$ticket = $login.data.SessionTicket

function Invoke-Award([string]$Fn, [hashtable]$Params) {
    $Params.adminKey = $AdminKey
    $resp = Invoke-RestMethod -Uri "$Base/Client/ExecuteCloudScript" -Method Post `
        -ContentType "application/json" -Headers @{ "X-Authorization" = $ticket } `
        -Body (@{ FunctionName = $Fn; FunctionParameter = $Params; GeneratePlayStreamEvent = $false } | ConvertTo-Json -Depth 5)
    if ($resp.data.Error) {
        throw "CloudScript error: $($resp.data.Error.Error) - $($resp.data.Error.Message)"
    }
    return $resp.data.FunctionResult
}

# PowerShell 콘솔이 UTF-8 을 CP1252 로 오독해 생기는 한글 깨짐 복원
$latin1 = [System.Text.Encoding]::GetEncoding(28591)
$utf8   = [System.Text.Encoding]::UTF8
function Fix([string]$s) { if ($null -eq $s) { return "" }; return $utf8.GetString($latin1.GetBytes($s)) }

if ($Delete) {
    # ── 삭제 모드 ──
    if (-not ($Level -ge 3 -and $Level -le 10) -or [string]::IsNullOrWhiteSpace($TargetId)) {
        throw "삭제하려면 -Level (3~10) 과 -TargetId 를 지정해야 합니다."
    }
    if (-not $Force) {
        $ans = Read-Host "L$Level / $TargetId 의 소감을 삭제합니다. 진행할까요? (y/N)"
        if ($ans -ne "y" -and $ans -ne "Y") { Write-Host "취소됨."; return }
    }
    $r = Invoke-Award "adminDeleteAwardComment" @{ level = $Level; targetId = $TargetId }
    if ($r.ok) { Write-Host "삭제 완료: L$($r.level) / $($r.targetId)" -ForegroundColor Green }
    else       { Write-Host "삭제 실패: reason=$($r.reason)" -ForegroundColor Red }
    return
}

# ── 검색 모드 (기본) ──
$res = Invoke-Award "dumpAwardComments" @{}
if (-not $res.ok) { Write-Host "조회 실패: reason=$($res.reason)" -ForegroundColor Red; return }

$rows = @()
foreach ($lvlProp in $res.data.PSObject.Properties) {
    foreach ($idProp in $lvlProp.Value.PSObject.Properties) {
        $val = $idProp.Value.Value
        try {
            $j = $val | ConvertFrom-Json
            $t = Fix $j.t
            $when = [DateTimeOffset]::FromUnixTimeMilliseconds([int64]$j.ts).ToLocalTime().ToString("yyyy-MM-dd HH:mm")
        } catch { $t = "(parse fail) $val"; $when = "" }
        $rows += [pscustomobject]@{ Level = $lvlProp.Name; targetId = $idProp.Name; Comment = $t; When = $when }
    }
}

if ($rows.Count -eq 0) { Write-Host "등록된 수상소감이 없습니다." }
else {
    $rows | Sort-Object Level, When | Format-Table -AutoSize -Wrap
    Write-Host "총 $($rows.Count) 건. 삭제: .\admin_award.ps1 -Delete -Level <n> -TargetId <id>" -ForegroundColor DarkGray
}

