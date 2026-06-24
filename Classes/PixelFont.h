#pragma once
#include "cocos2d.h"
using namespace cocos2d;

// 5x7 픽셀 폰트 (S·T·A·R·T 정의)
const char* pixelGlyph(char c);
float       pixelTextWidth(const std::string& s, float px);

// 단색 렌더링
void drawPixelGlyphs(DrawNode* node, const std::string& s, float px,
                     const Color4F& col, const Vec2& off);

// 픽셀별 색 콜백 버전
void drawPixelGlyphsFn(DrawNode* node, const std::string& s, float px, const Vec2& off,
                       const std::function<Color4F(int gx, int gy, float normX)>& fn);

// 드롭섀도 + 재색칠 가능 본체 노드 생성
Node* makePixelText(const std::string& s, float px, Color3B col, DrawNode** outMain = nullptr);
