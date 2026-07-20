#!/usr/bin/env node
/**
 * PlayFab 정크 플레이어 정리 도구 (Tower of Hanoi - Speedrun)
 *
 * 판별 기준 — "진짜 유저"는 아래 중 하나라도 해당하면 보존(KEEP):
 *   1) DisplayName 이 있음        (이름을 입력한 유저 → UpdateUserTitleDisplayName)
 *   2) BestTime_Lxx 통계가 있음    (랭킹 점수를 제출한 유저 → UpdatePlayerStatistics)
 *   3) 최근 GRACE_DAYS 이내 생성/로그인 (갓 설치/플레이 중일 수 있어 안전하게 보존)
 * 그 외(이름 없음 + 점수 없음 + 오래됨) → 정크(DELETE 후보).
 *
 * 리플레이/수상소감/격파기록은 모두 점수 제출(Top10)이 전제라, 위 2번(통계)으로 함께 보존됨.
 *
 * ── 사용법 ─────────────────────────────────────────────────────────────
 *  준비물(환경변수):
 *    PLAYFAB_SECRET_KEY   : Game Manager → Settings → Secret Keys 의 타이틀 시크릿 (필수, 절대 커밋 금지)
 *    PLAYFAB_SEGMENT_ID   : Game Manager 에서 만든 "All Players" 세그먼트 ID (필수)
 *    PLAYFAB_TITLE_ID     : 기본 119C4E (생략 가능)
 *    GRACE_DAYS           : 최근 며칠 이내 계정 보존 (기본 7)
 *
 *  1) 감사(삭제 안 함, 분류만):   node tools/playfab_cleanup.js audit
 *       → junk_players.json 생성 + 요약 출력. 이 파일을 눈으로 검토.
 *  2) 실제 삭제(검토 후):        node tools/playfab_cleanup.js delete
 *       → junk_players.json 을 읽어 하나씩 삭제. deleted.log 기록(재실행 시 이어서/중복방지).
 *
 *  PowerShell 예:
 *    $env:PLAYFAB_SECRET_KEY="..."; $env:PLAYFAB_SEGMENT_ID="..."; node tools/playfab_cleanup.js audit
 */

'use strict';
const https = require('https');
const fs = require('fs');
const path = require('path');

const TITLE_ID   = process.env.PLAYFAB_TITLE_ID  || '119C4E';
const SECRET_KEY = process.env.PLAYFAB_SECRET_KEY || '';
const SEGMENT_ID = process.env.PLAYFAB_SEGMENT_ID || '';
const GRACE_DAYS = parseInt(process.env.GRACE_DAYS || '7', 10);
const MODE       = (process.argv[2] || 'audit').toLowerCase();

const HOST        = `${TITLE_ID}.playfabapi.com`;
const OUT_JUNK    = path.join(__dirname, 'junk_players.json');
const OUT_KEEP    = path.join(__dirname, 'keep_sample.json');
const DELETED_LOG = path.join(__dirname, 'deleted.log');
const DELETE_DELAY_MS = 250;   // 삭제 호출 간 간격 (429 시 자동 백오프)

function die(msg) { console.error('ERROR: ' + msg); process.exit(1); }
if (!SECRET_KEY) die('PLAYFAB_SECRET_KEY 환경변수가 필요합니다.');

// PlayFab Admin API 호출 (X-SecretKey 헤더)
function adminPost(apiPath, body) {
  return new Promise((resolve, reject) => {
    const payload = JSON.stringify(body);
    const req = https.request({
      host: HOST, path: apiPath, method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Content-Length': Buffer.byteLength(payload),
        'X-SecretKey': SECRET_KEY,
      },
    }, (res) => {
      let data = '';
      res.on('data', (c) => data += c);
      res.on('end', () => {
        let json = null;
        try { json = JSON.parse(data); } catch (_) {}
        resolve({ status: res.statusCode, json, raw: data });
      });
    });
    req.on('error', reject);
    req.write(payload);
    req.end();
  });
}

const sleep = (ms) => new Promise(r => setTimeout(r, ms));
const daysAgo = (n) => Date.now() - n * 86400000;

// 1차 분류(세그먼트 데이터 기준, API 호출 없음): 이름/최근이면 즉시 보존, 아니면 점수 재확인 필요.
function preClassify(p) {
  const name = (p.DisplayName || '').trim();
  const created = p.Created ? Date.parse(p.Created) : 0;
  const lastLogin = p.LastLogin ? Date.parse(p.LastLogin) : 0;
  const recent = (created && created >= daysAgo(GRACE_DAYS)) ||
                 (lastLogin && lastLogin >= daysAgo(GRACE_DAYS));
  if (name)   return { decision: 'keep', reason: 'name' };
  if (recent) return { decision: 'keep', reason: 'recent' };
  return { decision: 'verify', reason: 'need-score-check' };   // 이름 없고 오래됨 → 점수 있는지 개별 확인
}

// 개별 통계 조회로 BestTime 점수 유무 확정 (Server API, X-SecretKey). 세그먼트 통계 누락에 안전.
async function hasBestTimeScore(playFabId) {
  const r = await adminPost('/Server/GetPlayerStatistics', { PlayFabId: playFabId });
  if (r.status !== 200 || !r.json || !r.json.data) {
    // 조회 실패 시 "점수 있음"으로 간주(보수적 → 오삭제 방지)
    console.log(`  ⚠ 통계조회 실패 ${playFabId}: HTTP ${r.status} → 안전하게 보존`);
    return true;
  }
  const stats = r.json.data.Statistics || [];
  return stats.some(s => (s.StatisticName || '').startsWith('BestTime_'));
}

// 자격증명/엔드포인트 격리 테스트 — 어느 단계에서 막히는지 확인용.
async function test() {
  console.log(`[TEST] host=${HOST}  titleId=${TITLE_ID}`);
  console.log(`SecretKey 길이=${SECRET_KEY.length} (앞4=${SECRET_KEY.slice(0,4)}...)  SegmentId=${SEGMENT_ID || '(미설정)'}`);

  const a = await adminPost('/Admin/GetTitleData', {});
  console.log(`\n[1] /Admin/GetTitleData → HTTP ${a.status}` +
    (a.status === 200 ? '  ✅ 자격증명 OK (Admin API 접근됨)' : `\n    ${(a.raw || '').slice(0, 400)}`));

  if (SEGMENT_ID) {
    const b = await adminPost('/Admin/GetPlayersInSegment',
      { SegmentId: SEGMENT_ID, SecondsToLive: 300, MaxBatchSize: 10 });
    console.log(`\n[2] /Admin/GetPlayersInSegment → HTTP ${b.status}`);
    console.log(`    응답: ${(b.raw || '(빈 응답)').slice(0, 600)}`);
    if (b.json && b.json.data) {
      const n = (b.json.data.PlayerProfiles || []).length;
      console.log(`    ✅ 세그먼트 조회 성공 — 첫 배치 ${n}명 / total ${b.json.data.ProfilesInSegment}`);
      if (b.json.data.PlayerProfiles && b.json.data.PlayerProfiles[0])
        console.log(`    샘플 프로필 키: ${Object.keys(b.json.data.PlayerProfiles[0]).join(', ')}`);
    }
  }
  console.log('\n해석: [1]이 200인데 [2]가 404면 → 세그먼트 export API 접근/설정 문제. [1]도 실패면 → 시크릿키/호스트 문제.');
}

async function getAllPlayers() {
  if (!SEGMENT_ID) die('PLAYFAB_SEGMENT_ID 환경변수가 필요합니다 (Game Manager에서 "All Players" 세그먼트 생성 후 ID).');
  const all = [];
  let token = null, batch = 0;
  do {
    const body = { SegmentId: SEGMENT_ID, SecondsToLive: 300, MaxBatchSize: 10000 };
    if (token) body.ContinuationToken = token;
    const r = await adminPost('/Admin/GetPlayersInSegment', body);
    if (r.status !== 200 || !r.json || !r.json.data) {
      die('GetPlayersInSegment 실패: HTTP ' + r.status + ' 응답=[' + (r.raw || '') + ']  (자세히는 `node tools/playfab_cleanup.js test` 실행)');
    }
    const d = r.json.data;
    const profiles = d.PlayerProfiles || [];
    all.push(...profiles);
    token = d.ContinuationToken || null;
    console.log(`  batch ${++batch}: +${profiles.length} (누적 ${all.length})`);
  } while (token);
  return all;
}

async function audit() {
  console.log(`[AUDIT] Title=${TITLE_ID}  Segment=${SEGMENT_ID}  graceDays=${GRACE_DAYS}`);
  const players = await getAllPlayers();

  // 1차: 세그먼트 데이터로 즉시 보존(이름/최근) 판별, 나머지는 점수 재확인 목록으로.
  const toVerify = [], keepSample = [];
  const tally = { name: 0, score: 0, recent: 0, junk: 0 };
  for (const p of players) {
    const c = preClassify(p);
    if (c.decision === 'keep') {
      tally[c.reason]++;
      if (keepSample.length < 30) keepSample.push({ PlayerId: p.PlayerId, reason: c.reason, name: (p.DisplayName || '').trim(), created: p.Created || '' });
    } else {
      toVerify.push(p);
    }
  }

  // 2차: 이름 없고 오래된 계정만 개별 통계조회로 점수 유무 확정(랭커 오삭제 방지).
  console.log(`\n점수 재확인 대상 ${toVerify.length}명 개별 조회 중...`);
  const junk = [];
  for (let i = 0; i < toVerify.length; i++) {
    const p = toVerify[i];
    const scored = await hasBestTimeScore(p.PlayerId);
    if (scored) {
      tally.score++;
      if (keepSample.length < 30) keepSample.push({ PlayerId: p.PlayerId, reason: 'score', name: '', created: p.Created || '' });
    } else {
      tally.junk++;
      junk.push({ PlayerId: p.PlayerId, created: p.Created || '', lastLogin: p.LastLogin || '' });
    }
    if ((i + 1) % 50 === 0) console.log(`  ...${i + 1}/${toVerify.length}`);
    await sleep(60);   // 통계조회 레이트리밋 완화
  }

  fs.writeFileSync(OUT_JUNK, JSON.stringify(junk, null, 2));
  fs.writeFileSync(OUT_KEEP, JSON.stringify(keepSample, null, 2));
  console.log('\n===== 분류 결과 =====');
  console.log(`전체:        ${players.length}`);
  console.log(`보존(이름):   ${tally.name}`);
  console.log(`보존(점수):   ${tally.score}`);
  console.log(`보존(최근):   ${tally.recent}`);
  console.log(`삭제(정크):   ${tally.junk}`);
  console.log(`\n→ 삭제 후보 ${junk.length}명을 ${OUT_JUNK} 에 기록했습니다.`);
  console.log(`  보존 샘플은 ${OUT_KEEP} 에서 확인 (reason=score 항목이 보이면 점수 판별 정상 작동).`);
  console.log(`\n검토 후 실제 삭제: node tools/playfab_cleanup.js delete`);
}

async function runDelete() {
  if (!fs.existsSync(OUT_JUNK)) die(`${OUT_JUNK} 없음. 먼저 'audit'를 실행하세요.`);
  const junk = JSON.parse(fs.readFileSync(OUT_JUNK, 'utf8'));
  const done = new Set(
    fs.existsSync(DELETED_LOG)
      ? fs.readFileSync(DELETED_LOG, 'utf8').split('\n').map(s => s.trim()).filter(Boolean)
      : []
  );
  console.log(`[DELETE] 대상 ${junk.length}명, 이미 삭제됨 ${done.size}명 (건너뜀).`);
  let ok = 0, fail = 0, delay = DELETE_DELAY_MS;
  for (let i = 0; i < junk.length; i++) {
    const id = junk[i].PlayerId;
    if (done.has(id)) continue;
    let r = await adminPost('/Admin/DeletePlayer', { PlayerId: id });
    // 429(레이트리밋) → 백오프 후 재시도
    while (r.status === 429) {
      delay = Math.min(delay * 2, 10000);
      console.log(`  429 rate-limit → ${delay}ms 대기 후 재시도`);
      await sleep(delay);
      r = await adminPost('/Admin/DeletePlayer', { PlayerId: id });
    }
    if (r.status === 200) {
      ok++;
      fs.appendFileSync(DELETED_LOG, id + '\n');
    } else {
      fail++;
      console.log(`  실패 ${id}: HTTP ${r.status} ${(r.raw || '').slice(0, 200)}`);
    }
    if ((i + 1) % 25 === 0) console.log(`  진행 ${i + 1}/${junk.length} (성공 ${ok}, 실패 ${fail})`);
    await sleep(delay);
  }
  console.log(`\n완료: 성공 ${ok}, 실패 ${fail}. 로그: ${DELETED_LOG}`);
}

(async () => {
  try {
    if (MODE === 'audit') await audit();
    else if (MODE === 'delete') await runDelete();
    else if (MODE === 'test') await test();
    else die(`알 수 없는 모드 '${MODE}' (test | audit | delete)`);
  } catch (e) { die(e && e.stack ? e.stack : String(e)); }
})();
