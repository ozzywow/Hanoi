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
