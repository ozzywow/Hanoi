#pragma once

// ──────────────────────────────────────────────────────────────
// MainScene 구현 파일 공유 심볼 (Phase 3 분할)
//   MainScene 클래스 하나를 4개 번역단위로 나눠 구현한다:
//     MainScene.cpp         코어(씬 생명주기/타이틀/START 애니/IAP/nameplate)
//     MainScene_Rank.cpp    온라인 랭킹보드(drawOnlineRank) + 페이지 콜백
//     MainScene_Dialog.cpp  이름·설정·복수·업데이트·수상소감·버전 게이트 다이얼로그
//     MainScene_Bgm.cpp     BGM 플레이어 + 상/하단 티커
//   아래는 둘 이상의 파일에서 참조하는 심볼만 모은다.
// ──────────────────────────────────────────────────────────────

#include <string>

// 앱스토어/Play 스토어 URL (정의: MainScene_Dialog.cpp). core init + rank drawOnlineRank 에서 참조.
std::string appStoreUrl();

// UTF-8 코드포인트 단위 자르기 (정의: MainScene_Dialog.cpp). rank 표시 + dialog 입력에서 공유.
std::string utf8TruncateCP(const std::string& s, int maxCP);
