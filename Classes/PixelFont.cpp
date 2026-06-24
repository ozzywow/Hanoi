#include "PixelFont.h"

const char* pixelGlyph(char c)
{
    switch (c) {
    case 'S': return ".####" "#...." "#...." ".###." "....#" "....#" "####.";
    case 'T': return "#####" "..#.." "..#.." "..#.." "..#.." "..#.." "..#..";
    case 'A': return ".###." "#...#" "#...#" "#####" "#...#" "#...#" "#...#";
    case 'R': return "####." "#...#" "#...#" "####." "#.#.." "#..#." "#...#";
    default:  return nullptr;
    }
}

float pixelTextWidth(const std::string& s, float px)
{
    const float advance = 5 * px + px;
    return s.empty() ? 0 : (advance * (float)s.size() - px);
}

void drawPixelGlyphs(DrawNode* node, const std::string& s, float px,
                     const Color4F& col, const Vec2& off)
{
    const int GW = 5, GH = 7;
    const float advance = GW * px + px;
    const float cell = px - 0.6f;
    float x0 = 0;
    for (char ch : s) {
        const char* g = pixelGlyph(ch);
        if (g) {
            for (int row = 0; row < GH; ++row)
                for (int c2 = 0; c2 < GW; ++c2) {
                    if (g[row * GW + c2] != '#') continue;
                    float bx = x0 + c2 * px;
                    float by = (GH - 1 - row) * px;
                    node->drawSolidRect(Vec2(bx, by) + off, Vec2(bx + cell, by + cell) + off, col);
                }
        }
        x0 += advance;
    }
}

void drawPixelGlyphsFn(DrawNode* node, const std::string& s, float px, const Vec2& off,
                       const std::function<Color4F(int gx, int gy, float normX)>& fn)
{
    const int GW = 5, GH = 7;
    const float advance = GW * px + px;
    const float cell = px - 0.6f;
    const int nLetters = (int)s.size();
    const float totalCols = nLetters > 0 ? (float)(nLetters * (GW + 1) - 1) : 1.f;
    float x0 = 0;
    int letterIdx = 0;
    for (char ch : s) {
        const char* g = pixelGlyph(ch);
        if (g) {
            for (int row = 0; row < GH; ++row)
                for (int c2 = 0; c2 < GW; ++c2) {
                    if (g[row * GW + c2] != '#') continue;
                    float bx = x0 + c2 * px;
                    float by = (GH - 1 - row) * px;
                    int gx = letterIdx * (GW + 1) + c2;
                    int gy = GH - 1 - row;
                    float normX = gx / totalCols;
                    node->drawSolidRect(Vec2(bx, by) + off, Vec2(bx + cell, by + cell) + off, fn(gx, gy, normX));
                }
        }
        x0 += advance;
        letterIdx++;
    }
}

Node* makePixelText(const std::string& s, float px, Color3B col, DrawNode** outMain)
{
    auto node = Node::create();
    node->setContentSize(Size(pixelTextWidth(s, px), 7 * px));
    node->setCascadeOpacityEnabled(true);

    auto shadow = DrawNode::create();
    auto main   = DrawNode::create();
    node->addChild(shadow, 0);
    node->addChild(main, 1);

    drawPixelGlyphs(shadow, s, px, Color4F(0, 0, 0, 0.6f), Vec2(1.5f, -1.5f));
    drawPixelGlyphs(main, s, px,
        Color4F(col.r / 255.f, col.g / 255.f, col.b / 255.f, 1.0f), Vec2::ZERO);

    if (outMain) *outMain = main;
    return node;
}
