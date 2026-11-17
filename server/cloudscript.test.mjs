// cloudscript.js 보안 회귀 테스트 — PlayFab 없이 가짜 `server` 로 핸들러를 직접 호출한다.
// 실행: node --test server/cloudscript.test.mjs   (Node 18+, 외부 패키지 없음)
import test from 'node:test';
import assert from 'node:assert/strict';
import vm from 'node:vm';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
// CS_PATH 로 다른 버전(예: 수정 전 리비전)을 지정해 테스트가 실제로 취약점을 잡는지 확인할 수 있다.
const code = fs.readFileSync(process.env.CS_PATH || path.join(here, '..', 'cloudscript.js'), 'utf8');

const ALPHA = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
/** 클라 encodeReplayBlob 헤더와 같은 형식의 Base64 블롭 (events 는 비워 둔다) */
function blob(discus, finalMs) {
  const b = [discus, 0, 0, finalMs & 255, (finalMs >> 8) & 255, (finalMs >> 16) & 255];
  let out = '';
  for (let i = 0; i < b.length; i += 3) {
    const n = (b[i] << 16) | (b[i + 1] << 8) | b[i + 2];
    out += ALPHA[(n >> 18) & 63] + ALPHA[(n >> 12) & 63] + ALPHA[(n >> 6) & 63] + ALPHA[n & 63];
  }
  return out;
}

function makeEnv() {
  const internal = {}; // pid -> { key: {Value} }
  const groups = {}; // groupId -> { key: {Value} }
  const profiles = {}; // pid -> { name, cc }
  const customIds = {}; // pid -> customId
  const lb = {}; // `${stat}|${pid}` -> { Position, StatValue }
  const calls = { sharedGroupWrites: [] };

  const server = {
    GetTitleData: () => ({ Data: {} }),
    GetTitleInternalData: () => ({ Data: {} }),
    GetPlayerProfile: ({ PlayFabId }) => {
      const p = profiles[PlayFabId];
      return { PlayerProfile: p ? { DisplayName: p.name, Locations: p.cc ? [{ CountryCode: p.cc }] : [] } : {} };
    },
    GetUserAccountInfo: ({ PlayFabId }) => ({ UserInfo: { CustomIdInfo: { CustomId: customIds[PlayFabId] } } }),
    GetUserInternalData: ({ PlayFabId, Keys }) => {
      const d = internal[PlayFabId] || {};
      const out = {};
      for (const k of Keys || Object.keys(d)) if (d[k]) out[k] = d[k];
      return { Data: out };
    },
    UpdateUserInternalData: ({ PlayFabId, Data, KeysToRemove }) => {
      internal[PlayFabId] = internal[PlayFabId] || {};
      for (const k in Data || {}) internal[PlayFabId][k] = { Value: Data[k] };
      for (const k of KeysToRemove || []) delete internal[PlayFabId][k];
    },
    GetSharedGroupData: ({ SharedGroupId, Keys }) => {
      const g = groups[SharedGroupId] || {};
      const out = {};
      for (const k of Keys || Object.keys(g)) if (g[k]) out[k] = g[k];
      return { Data: out };
    },
    UpdateSharedGroupData: ({ SharedGroupId, Data }) => {
      calls.sharedGroupWrites.push(SharedGroupId);
      groups[SharedGroupId] = groups[SharedGroupId] || {};
      for (const k in Data) {
        if (Data[k] === null) delete groups[SharedGroupId][k];
        else groups[SharedGroupId][k] = { Value: Data[k] };
      }
    },
    GetLeaderboardAroundUser: ({ StatisticName, PlayFabId }) => {
      const e = lb[`${StatisticName}|${PlayFabId}`];
      return { Leaderboard: e ? [{ PlayFabId, Position: e.Position, StatValue: e.StatValue }] : [] };
    },
  };

  const sandbox = { handlers: {}, server, currentPlayerId: '', log: { debug() {}, info() {}, error() {} } };
  vm.createContext(sandbox);
  vm.runInContext(code, sandbox, { filename: 'cloudscript.js' });
  const call = (pid, name, args) => {
    sandbox.currentPlayerId = pid;
    return sandbox.handlers[name](args || {}, {});
  };
  return { call, internal, groups, profiles, customIds, lb, calls };
}

const ALICE = 'A8B08D8743AD67C2';
const BOB = 'B1C2D3E4F5061728';

test('토큰: 발급 → 교환 성공, 공개 Shared Group 에는 아무것도 쓰지 않는다', () => {
  const env = makeEnv();
  env.profiles[ALICE] = { name: 'Alice', cc: 'kr' };
  const issued = env.call(ALICE, 'issueWebToken');
  assert.equal(issued.ok, true);
  assert.match(issued.token, /^[A-Za-z0-9]+$/, '랜딩 정규식 /wt=([A-Za-z0-9]+)/ 와 호환');
  assert.ok(issued.token.endsWith(ALICE));
  assert.ok(!env.calls.sharedGroupWrites.includes('web_link'), 'web_link 그룹 미사용');

  const redeemed = env.call('anyone', 'redeemWebToken', { token: issued.token });
  assert.equal(redeemed.ok, true);
  assert.equal(redeemed.name, 'Alice');
  assert.ok(redeemed.sessionKey.startsWith(ALICE + '.'));
});

test('토큰: 일회용 — 재사용 불가', () => {
  const env = makeEnv();
  env.profiles[ALICE] = { name: 'Alice', cc: 'kr' };
  const { token } = env.call(ALICE, 'issueWebToken');
  assert.equal(env.call('x', 'redeemWebToken', { token }).ok, true);
  assert.equal(env.call('x', 'redeemWebToken', { token }).reason, 'invalid');
});

test('토큰: 틀린 비밀로는 실패하고 진짜 토큰을 소각하지 않는다', () => {
  const env = makeEnv();
  env.profiles[ALICE] = { name: 'Alice', cc: '' };
  const { token } = env.call(ALICE, 'issueWebToken');
  const forged = 'z'.repeat(24) + ALICE;
  assert.equal(env.call('x', 'redeemWebToken', { token: forged }).reason, 'invalid');
  assert.equal(env.call('x', 'redeemWebToken', { token }).ok, true, '원래 토큰은 아직 유효');
});

test('토큰: 만료/형식 오류/존재하지 않는 계정', () => {
  const env = makeEnv();
  env.profiles[ALICE] = { name: 'Alice', cc: '' };
  const { token } = env.call(ALICE, 'issueWebToken');
  const rec = JSON.parse(env.internal[ALICE].web_token.Value);
  rec.e = Date.now() - 1;
  env.internal[ALICE].web_token = { Value: JSON.stringify(rec) };
  assert.equal(env.call('x', 'redeemWebToken', { token }).reason, 'expired');
  assert.equal(env.call('x', 'redeemWebToken', { token: 'short' }).reason, 'invalid');
  assert.equal(env.call('x', 'redeemWebToken', { token: 'a'.repeat(24) + 'ZZZZZZZZ' }).reason, 'invalid');
  assert.equal(env.call('x', 'redeemWebToken', { token: '' }).reason, 'empty');
});

test('토큰: 이름 없는 플레이어는 발급 거부', () => {
  const env = makeEnv();
  assert.equal(env.call(ALICE, 'issueWebToken').reason, 'no_name');
});

test('공용 뷰어 계정(CustomId)은 Title Data 설정 없이도 게시판 쓰기 차단', () => {
  const env = makeEnv();
  env.profiles[BOB] = { name: 'Admin', cc: '' }; // 누군가 뷰어 계정의 표시명을 바꿔 둔 상황
  env.customIds[BOB] = 'web_public_viewer';
  const r = env.call(BOB, 'writeBoardPost', { text: 'hello' });
  assert.equal(r.reason, 'read_only');
  assert.equal(env.call(BOB, 'writeBoardComment', { postId: '1', text: 'hi' }).reason, 'read_only');
});

test('writeReplay: 형식/헤더/기록 검증', () => {
  const env = makeEnv();
  env.lb['BestTime_L03|' + ALICE] = { Position: 0, StatValue: 5000 };
  const ok = env.call(ALICE, 'writeReplay', { level: 3, blob: blob(3, 5000) });
  assert.equal(ok.ok, true);

  assert.equal(env.call(ALICE, 'writeReplay', { level: 3, blob: 'http://evil.example/<x>' }).reason, 'bad_blob');
  assert.equal(env.call(ALICE, 'writeReplay', { level: 3, blob: blob(4, 5000) }).reason, 'bad_blob', '레벨과 디스크 수 불일치');
  assert.equal(env.call(ALICE, 'writeReplay', { level: 3, blob: blob(3, 3000) }).reason, 'time_mismatch', '서버 기록보다 빠른 리플레이');
  assert.equal(env.call(ALICE, 'writeReplay', { level: 3, blob: blob(3, 9000) }).ok, true, '더 느린 것은 허용(전파 지연 여유)');
  assert.equal(env.call(BOB, 'writeReplay', { level: 3, blob: blob(3, 5000) }).reason, 'not_top10', '비랭커 차단');
});

test('likeReplay: 형식 불량/리플레이 없는 대상은 키를 만들지 않는다', () => {
  const env = makeEnv();
  env.groups['replays_L03'] = { [ALICE]: { Value: '{}' } };
  assert.equal(env.call(BOB, 'likeReplay', { level: 3, targetId: '__proto__' }).reason, 'bad_target');
  assert.equal(env.call(BOB, 'likeReplay', { level: 3, targetId: 'C0FFEE00C0FFEE00' }).reason, 'no_replay');
  assert.ok(!env.groups['likes_L03'], '쓰기 없음');
  const ok = env.call(BOB, 'likeReplay', { level: 3, targetId: ALICE });
  assert.equal(ok.ok, true);
  assert.equal(ok.n, 1);
  assert.equal(env.call(BOB, 'likeReplay', { level: 3, targetId: ALICE }).already, true);
  assert.equal(env.call(ALICE, 'likeReplay', { level: 3, targetId: ALICE }).reason, 'self');
});

test('touchRecentPlayer: 프로필 이름이 없을 때 클라 힌트도 검증한다', () => {
  const env = makeEnv();
  assert.equal(env.call(ALICE, 'touchRecentPlayer', { name: 'check http://evil.example now' }).reason, 'no_name');
  assert.equal(env.call(ALICE, 'touchRecentPlayer', { name: 'x'.repeat(40) }).reason, 'no_name');
  const ok = env.call(ALICE, 'touchRecentPlayer', { name: 'Alice' });
  assert.equal(ok.ok, true);
  assert.equal(ok.name, 'Alice');
});

test('관리자 핸들러는 adminKey 없이 거부된다', () => {
  const env = makeEnv();
  for (const name of ['resetAllLeaderboards', 'clearRecentPlayers', 'dumpAwardComments', 'adminDeleteBoardPost', 'revokeWebSession', 'maintainLeaderboards', 'cleanupJunkPlayer']) {
    const r = env.call(ALICE, name, { playFabId: BOB });
    assert.ok(r.reason === 'denied' || r.ok === false, name + ' → ' + JSON.stringify(r));
    assert.equal(r.reason, 'denied', name);
  }
});
