#pragma once
#include "cocos2d.h"
using namespace cocos2d;

// ── LED 패널 ─────────────────────────────────────────────────────────────────
void setupLedBackground(DrawNode* node, float width, float height, float border = 3.0f);
void setupLedDotOverlay(DrawNode* node, float width, float height, float gap = 2.0f, float radius = 0.5f);

// ── 키캡 버튼 ────────────────────────────────────────────────────────────────
// grayStyle=true → 회색 톤 (기어 등 보조 버튼용)
void drawKeycap(DrawNode* node, float cx, float cy, float w, float h, bool grayStyle = false);

// ── 벡터 아이콘 ──────────────────────────────────────────────────────────────
void drawVecGear(DrawNode* node, float cx, float cy, float r, const Color4F& col);
void drawVecRestore_CircArrow(DrawNode* node, float cx, float cy, float sz, const Color4F& col);
void drawVecTriangle(DrawNode* node, float cx, float cy, float sz, bool leftward, const Color4F& col);
void drawVecReset(DrawNode* node, float cx, float cy, float sz, const Color4F& col);
void drawVecCartIcon(DrawNode* node, float cx, float cy, float sz, const Color4F& col);
void drawVecLock(DrawNode* node, float cx, float cy, float sz, const Color4F& col);

// ── BGM 플레이어 드로우 ──────────────────────────────────────────────────────
void drawCassetteBody(DrawNode* dn, float cx, float cy, float w, float h, bool on);
void drawSpeakerGrille(DrawNode* dn, float r, bool on);
void drawIconPrev(DrawNode* dn, float cx, float cy, float sz, const Color4F& col);
void drawIconNext(DrawNode* dn, float cx, float cy, float sz, const Color4F& col);
void drawIconPlay(DrawNode* dn, float cx, float cy, float sz, const Color4F& col);
void drawIconPause(DrawNode* dn, float cx, float cy, float sz, const Color4F& col);

// ── 아이콘 노드 팩토리 ───────────────────────────────────────────────────────
Node* makeVecCartNode(float w, float h, float sz);
Node* makeVecIcon(float w, float h, const std::function<void(DrawNode*, float, float)>& drawFn);
Node* makeVecNavKeycap(float w, float h, bool leftward, bool enabled);

// ── 색상 유틸리티 ────────────────────────────────────────────────────────────
Color4F hsv2rgb(float h, float s, float v);

// ── 팝업 컬러 팔레트 (docs/popup_design_plan.md §1) ───────────────────────────
//    지정 팔레트 밖 색상 사용 금지. UI 폰트 컬러는 "역할"로 고정.
const Color4B kPopupBG    (10, 15, 50, 240);   // 모든 팝업 배경(네이비 강제 통일)
// 프레임 테두리 — 톤: 성취=골드 / 유틸=그레이 / 파괴적 위험=레드
const Color3B kBorderGold (255, 215, 0);
const Color3B kBorderGray (152, 166, 188);
const Color3B kBorderRed  (235, 90, 90);
// 버튼 — 역할별(폰트색 = 테두리색)
const Color3B kBtnFunc    (120, 200, 255);     // 긍정·실행 (REPLAY/WATCH/OK/UPDATE/EDIT)
const Color3B kBtnRace    (255, 180, 90);      // 경쟁·공격 (RACE/REVENGE/RETRY)
const Color3B kBtnDismiss (170, 178, 195);     // 해제·취소 (CLOSE/CANCEL/LATER/HOME)
const Color3B kBtnDanger  (235, 90, 90);       // 파괴·확정 (RESET)
// 메시지 폰트 — 역할별
const Color3B kTextTitle  (255, 215, 0);       // 제목(성취)
const Color3B kTextTitleBad(235, 90, 90);      // 제목(부정: LOSE/TIME OUT)
const Color3B kTextData   (255, 255, 255);     // 핵심 데이터(기록·이름)
const Color3B kTextAccent (120, 200, 255);     // 라벨/보조 강조
const Color3B kTextMuted  (150, 160, 175);     // 부가·캡션
const Color3B kTextError  (255, 120, 120);     // 에러

// 백드롭 딤 알파 — 표준 160 / 강제 업데이트만 210
const int kDimStd   = 160;
const int kDimForce = 210;

// 하단 버튼 행 표준 중심 Y (박스 바닥 기준) — 팝업 전체 통일. 칩 하단 여백 ~8px.
const float kPopupBtnY = 26.f;

// ── 팝업 칩 버튼 ─────────────────────────────────────────────────────────────
// 텍스트 뒤에 컬러 테두리 칩 배경을 깔아 히트 영역을 w×h로 확대(미스터치 감소).
// 반환된 MenuItemSprite를 Menu에 넣어 사용. 내부 라벨은 kChipLabelTag로 접근 가능
// (동적 색/문구 변경이 필요한 버튼용, 예: 이름 입력창 OK 유효성 표시).
const int kChipLabelTag = 0x0C41;
MenuItemSprite* makePopupChipButton(const std::string& text, Color3B col,
    const ccMenuCallback& cb, float w = 130.f, float h = 36.f, float fontSize = 14.f);

// ── 공용 팝업 프레임 (docs/popup_design_plan.md §2) ───────────────────────────
// 백드롭(딤) + 박스(네이비 배경 + 톤 테두리) + 타이틀 + 구분선 + 스케일인 애니를 표준 생성.
// 반환: backdrop(씬에 tag와 함께 add) / box(본문·버튼을 자식으로 add).
struct PopupFrame { LayerColor* backdrop; LayerColor* box; Label* title; };
PopupFrame makePopupFrame(const std::string& title, Color3B borderCol, Color3B titleCol,
    float w, float h, float titleSize = 20.f, int dimAlpha = kDimStd,
    bool modal = true, bool divider = true);

// 배경 터치 차단(모달). 팝업 백드롭에 부착 → 뒤 UI 비활성.
void attachModalBlocker(Node* backdrop);
