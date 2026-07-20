#pragma once

// ──────────────────────────────────────────────────────────────
// PlayScene 구현 파일 공유 심볼 (Phase 2 분할)
//   PlayScene 클래스 하나를 4개 번역단위로 나눠 구현한다:
//     PlayScene.cpp         코어(씬 생명주기/게임플레이/카운트다운/콜백/IAP)
//     PlayScene_Replay.cpp  리플레이·고스트 레이스·관전 + 직렬화
//     PlayScene_Hud.cpp     하단 패널/랭크 티커/이퀄라이저/결과 HUD
//     PlayScene_Fx.cpp      idle/cheer/guide 연출 + 포디움
//   아래는 둘 이상의 파일에서 참조하는 심볼만 모은다.
// ──────────────────────────────────────────────────────────────

#include "cocos2d.h"
#include "PlayScene.h"   // ReplayEvent
#include <vector>
#include <string>
#include <cstdint>

// 폴 3개의 화면 좌표 (정의: PlayScene.cpp). 코어/HUD/Fx/이퀄라이저 공유.
extern cocos2d::Point arrPosOfPole[3];

// 공유 상수
constexpr uint32_t REPLAY_MAX_DELTA_MS   = 120000;  // core DrawTime(무입력 포기) + replay 인코딩 clamp
constexpr int      REPLAY_BLOB_MAX_CHARS = 10000;   // core MessagePopup(저장 상한) + replay (서버 writeReplay와 동일)
constexpr float    BOTTOM_PANEL_Y        = 28.0f;   // hud 하단 패널 + fx 포디움
constexpr int      TAG_PODIUM_BASE       = 310;     // hud clearBottomPanels(정리) + fx 포디움 태그

// 리플레이 직렬화 (정의: PlayScene_Replay.cpp). core MessagePopup 에서 저장 시 참조.
std::string encodeReplayBlob(const std::vector<ReplayEvent>& ev, int discus, int finalMs);
std::string replayKey(int level);
