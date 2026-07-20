#include "DrawUtils.h"

// ── LED 패널 ─────────────────────────────────────────────────────────────────

void setupLedBackground(DrawNode* node, float width, float height, float border)
{
    node->drawSolidRect(
        Vec2(-width/2, -height/2 - border), Vec2(width/2, height/2 + border),
        Color4F(0.03f, 0.03f, 0.03f, 1.0f));
    node->drawSolidRect(
        Vec2(-width/2, -height/2), Vec2(width/2, height/2),
        Color4F(0.03f, 0.09f, 0.09f, 1.0f));
}

void setupLedDotOverlay(DrawNode* node, float width, float height, float gap, float radius)
{
    const Color4F dotColor(0.30f, 0.30f, 0.30f, 1.0f);
    for (float dy = -height/2 + gap/2; dy < height/2; dy += gap)
        for (float dx = -width/2 + gap/2; dx < width/2; dx += gap)
            node->drawDot(Vec2(dx, dy), radius, dotColor);
}

// ── 키캡 버튼 ────────────────────────────────────────────────────────────────

void drawKeycap(DrawNode* node, float cx, float cy, float w, float h, bool grayStyle)
{
    float x0 = cx - w / 2, x1 = cx + w / 2;
    float y0 = cy - h / 2, y1 = cy + h / 2;
    float c = (w < h ? w : h) * 0.10f; if (c < 2.0f) c = 2.0f;
    Color4F base   = grayStyle ? Color4F(0.13f, 0.14f, 0.16f, 1.0f) : Color4F(0.08f, 0.19f, 0.19f, 1.0f);
    Color4F hi     = grayStyle ? Color4F(0.65f, 0.65f, 0.65f, 0.4f) : Color4F(0.5f,  1.0f,  0.85f, 0.5f);
    Color4F shadow = Color4F(0.0f, 0.0f, 0.0f, 0.5f);
    Color4F border = grayStyle ? Color4F(0.42f, 0.46f, 0.50f, 0.60f) : Color4F(0.31f, 0.86f, 0.70f, 0.55f);
    const float k = 0.293f;
    Vec2 poly[12] = {
        Vec2(x0+c,   y0),        Vec2(x1-c,   y0),
        Vec2(x1-c*k, y0+c*k),   Vec2(x1,     y0+c),
        Vec2(x1,     y1-c),      Vec2(x1-c*k, y1-c*k),
        Vec2(x1-c,   y1),        Vec2(x0+c,   y1),
        Vec2(x0+c*k, y1-c*k),   Vec2(x0,     y1-c),
        Vec2(x0,     y0+c),      Vec2(x0+c*k, y0+c*k),
    };
    node->drawSolidPoly(poly, 12, base);
    node->drawSolidRect(Vec2(x0+c, y0),   Vec2(x1-c, y0+2), shadow);
    node->drawSolidRect(Vec2(x1-2, y0+c), Vec2(x1,   y1-c), shadow);
    node->drawSolidRect(Vec2(x0+c, y1-2), Vec2(x1-c, y1),   hi);
    node->drawSolidRect(Vec2(x0,   y0+c), Vec2(x0+2, y1-c), hi);
    Vec2 tl[3] = { Vec2(x0,    y1-c), Vec2(x0+c, y1),    Vec2(x0+2, y1-2) };
    Vec2 br[3] = { Vec2(x1-c,  y0),   Vec2(x1,   y0+c),  Vec2(x1-2, y0+2) };
    node->drawSolidPoly(tl, 3, hi);
    node->drawSolidPoly(br, 3, shadow);
    node->drawPoly(poly, 12, true, border);
}

// ── 벡터 아이콘 ──────────────────────────────────────────────────────────────

void drawVecGear(DrawNode* node, float cx, float cy, float r, const Color4F& col)
{
    node->drawCircle(Vec2(cx, cy), r, 0, 40, false, col);
    const int TEETH = 8;
    const float toothW = r * 0.38f, toothH = r * 0.42f;
    for (int i = 0; i < TEETH; ++i) {
        float angle = (float)i / TEETH * M_PI * 2.f;
        float ca = cosf(angle), sa = sinf(angle);
        float tx = cx + (r + toothH * 0.35f) * ca;
        float ty = cy + (r + toothH * 0.35f) * sa;
        float hw = toothW / 2.f, hh = toothH / 2.f;
        Vec2 pts[4] = {
            Vec2(tx + (-hw * ca - (-hh) * sa), ty + (-hw * sa + (-hh) * ca)),
            Vec2(tx + ( hw * ca - (-hh) * sa), ty + ( hw * sa + (-hh) * ca)),
            Vec2(tx + ( hw * ca -   hh  * sa), ty + ( hw * sa +   hh  * ca)),
            Vec2(tx + (-hw * ca -   hh  * sa), ty + (-hw * sa +   hh  * ca)),
        };
        node->drawSolidPoly(pts, 4, col);
    }
    node->drawSolidCircle(Vec2(cx, cy), r, 0, 32, col);
    node->drawSolidCircle(Vec2(cx, cy), r * 0.38f, 0, 20, Color4F(0.08f, 0.19f, 0.19f, 1.0f));
}

void drawVecRestore_CircArrow(DrawNode* node, float cx, float cy, float sz, const Color4F& col)
{
    float archR  = sz * 0.40f;
    float archIR = archR * 0.72f;
    const int N  = 20;
    float startA = M_PI * 15.f / 180.f;
    float sweepA = M_PI * 285.f / 180.f;
    for (int i = 0; i < N; ++i) {
        float a0 = startA - sweepA *  i      / N;
        float a1 = startA - sweepA * (i + 1) / N;
        Vec2 pts[4] = {
            {cx + cosf(a0)*archIR, cy + sinf(a0)*archIR},
            {cx + cosf(a0)*archR,  cy + sinf(a0)*archR },
            {cx + cosf(a1)*archR,  cy + sinf(a1)*archR },
            {cx + cosf(a1)*archIR, cy + sinf(a1)*archIR},
        };
        node->drawSolidPoly(pts, 4, col);
    }
    float midR = (archR + archIR) * 0.5f;
    float asz  = sz * 0.21f;
    Vec2  tip  = {cx + 0.f, cy + midR};
    Vec2  fwd  = {1.f, 0.f};
    Vec2  perp = {0.f, 1.f};
    Vec2 arrowPts[3] = {
        {tip.x + fwd.x*asz,                          tip.y + fwd.y*asz},
        {tip.x - fwd.x*asz*0.3f + perp.x*asz*0.75f, tip.y - fwd.y*asz*0.3f + perp.y*asz*0.75f},
        {tip.x - fwd.x*asz*0.3f - perp.x*asz*0.75f, tip.y - fwd.y*asz*0.3f - perp.y*asz*0.75f},
    };
    node->drawSolidPoly(arrowPts, 3, col);
}

void drawVecTriangle(DrawNode* node, float cx, float cy, float sz, bool leftward, const Color4F& col)
{
    float hw = sz * 0.45f, hh = sz * 0.38f;
    Vec2 pts[3];
    if (leftward) {
        pts[0] = Vec2(cx + hw, cy + hh);
        pts[1] = Vec2(cx - hw, cy);
        pts[2] = Vec2(cx + hw, cy - hh);
    } else {
        pts[0] = Vec2(cx - hw, cy + hh);
        pts[1] = Vec2(cx + hw, cy);
        pts[2] = Vec2(cx - hw, cy - hh);
    }
    node->drawSolidPoly(pts, 3, col);
}

void drawVecReset(DrawNode* node, float cx, float cy, float sz, const Color4F& col)
{
    float barW = sz * 0.12f;
    float barH = sz * 0.70f;
    float triW = sz * 0.45f;
    float triH = sz * 0.65f;
    float gap  = sz * 0.10f;
    float totalW = barW + gap + triW;
    float leftX  = cx - totalW * 0.5f;
    node->drawSolidRect(Vec2(leftX, cy - barH/2), Vec2(leftX + barW, cy + barH/2), col);
    float triCx = leftX + barW + gap + triW * 0.5f;
    Vec2 pts[3] = {
        Vec2(triCx + triW*0.5f, cy + triH*0.5f),
        Vec2(triCx - triW*0.5f, cy),
        Vec2(triCx + triW*0.5f, cy - triH*0.5f),
    };
    node->drawSolidPoly(pts, 3, col);
}

void drawVecCartIcon(DrawNode* node, float cx, float cy, float sz, const Color4F& col)
{
    float lw = sz * 0.10f;
    if (lw < 2.0f) lw = 2.0f;
    Color4F bg(0.08f, 0.19f, 0.19f, 1.0f);
    float bx0 = cx - sz*0.42f, bx1 = cx + sz*0.18f;
    float by0  = cy - sz*0.20f, by1 = cy + sz*0.10f;
    node->drawSolidRect(Vec2(bx0,     by0), Vec2(bx0+lw, by1),    col);
    node->drawSolidRect(Vec2(bx1-lw,  by0), Vec2(bx1,    by1),    col);
    node->drawSolidRect(Vec2(bx0,     by0), Vec2(bx1,  by0+lw),   col);
    node->drawSolidRect(Vec2(bx0,  by1-lw), Vec2(bx1,    by1),    col);
    float hvTop = cy + sz*0.40f;
    node->drawSolidRect(Vec2(bx1-lw,  by1-lw), Vec2(bx1,         hvTop), col);
    node->drawSolidRect(Vec2(bx1-lw, hvTop-lw), Vec2(cx+sz*0.50f, hvTop), col);
    float wr = lw * 1.3f, wy = by0 - wr*0.8f;
    node->drawSolidCircle(Vec2(bx0+sz*0.12f, wy), wr,      0, 14, col);
    node->drawSolidCircle(Vec2(bx1-sz*0.08f, wy), wr,      0, 14, col);
    node->drawSolidCircle(Vec2(bx0+sz*0.12f, wy), wr*0.4f, 0, 10, bg);
    node->drawSolidCircle(Vec2(bx1-sz*0.08f, wy), wr*0.4f, 0, 10, bg);
}

void drawVecLock(DrawNode* node, float cx, float cy, float sz, const Color4F& col)
{
    Color4F bg(0.08f, 0.19f, 0.19f, 1.0f);
    float archR  = sz * 0.26f;
    float bw     = archR * 2.2f;
    float bh     = sz * 0.42f;
    float by0    = cy - sz * 0.34f;
    float by1    = by0 + bh;
    node->drawSolidRect(Vec2(cx-bw/2, by0), Vec2(cx+bw/2, by1), col);
    float archIR = archR - sz * 0.11f;
    const int N  = 12;
    for (int i = 0; i < N; ++i) {
        float a0 = (float)M_PI * i / N;
        float a1 = (float)M_PI * (i + 1) / N;
        Vec2 pts[4] = {
            Vec2(cx + cosf(a0)*archIR, by1 + sinf(a0)*archIR),
            Vec2(cx + cosf(a0)*archR,  by1 + sinf(a0)*archR),
            Vec2(cx + cosf(a1)*archR,  by1 + sinf(a1)*archR),
            Vec2(cx + cosf(a1)*archIR, by1 + sinf(a1)*archIR),
        };
        node->drawSolidPoly(pts, 4, col);
    }
    float kr  = sz * 0.10f;
    float kcy = by0 + bh * 0.62f;
    node->drawSolidCircle(Vec2(cx, kcy), kr, 0, 12, bg);
    node->drawSolidRect(Vec2(cx-kr*0.55f, by0+bh*0.20f), Vec2(cx+kr*0.55f, kcy), bg);
}

void drawVecShare(DrawNode* node, float cx, float cy, float sz, const Color4F& col)
{
    Vec2 pL (cx - sz * 0.55f, cy);                  // 좌측 노드
    Vec2 pTR(cx + sz * 0.55f, cy + sz * 0.62f);     // 우상 노드
    Vec2 pBR(cx + sz * 0.55f, cy - sz * 0.62f);     // 우하 노드
    float lw = sz * 0.10f;                          // 연결선 두께(반지름)
    float dr = sz * 0.28f;                          // 노드 반지름
    node->drawSegment(pL, pTR, lw, col);
    node->drawSegment(pL, pBR, lw, col);
    node->drawSolidCircle(pL,  dr, 0, 16, col);
    node->drawSolidCircle(pTR, dr, 0, 16, col);
    node->drawSolidCircle(pBR, dr, 0, 16, col);
}

// ── 아이콘 노드 팩토리 ───────────────────────────────────────────────────────

Node* makeVecCartNode(float w, float h, float sz)
{
    auto node = Node::create();
    node->setContentSize(Size(w, h));
    node->setCascadeOpacityEnabled(true);
    auto dn = DrawNode::create();
    drawVecCartIcon(dn, w/2.f, h/2.f, sz, Color4F(80/255.f, 220/255.f, 180/255.f, 1.f));
    node->addChild(dn);
    return node;
}

Node* makeVecIcon(float w, float h, const std::function<void(DrawNode*, float, float)>& drawFn)
{
    auto node = Node::create();
    node->setContentSize(Size(w, h));
    node->setCascadeOpacityEnabled(true);
    auto dn = DrawNode::create();
    drawFn(dn, w / 2.f, h / 2.f);
    node->addChild(dn);
    return node;
}

Node* makeVecNavKeycap(float w, float h, bool leftward, bool enabled)
{
    auto node = Node::create();
    node->setContentSize(Size(w, h));
    auto dn = DrawNode::create();
    drawKeycap(dn, w / 2.f, h / 2.f, w, h);
    Color4F col = enabled
        ? Color4F(80 / 255.f, 220 / 255.f, 180 / 255.f, 1.f)
        : Color4F(0.28f, 0.28f, 0.28f, 0.55f);
    drawVecTriangle(dn, w / 2.f, h / 2.f, h * 0.52f, leftward, col);
    node->addChild(dn);
    return node;
}

// ── 색상 유틸리티 ────────────────────────────────────────────────────────────

Color4F hsv2rgb(float h, float s, float v)
{
    h = h - floorf(h);
    float i = floorf(h * 6.0f);
    float f = h * 6.0f - i;
    float p = v * (1 - s), q = v * (1 - s * f), t = v * (1 - s * (1 - f));
    float r = v, g = v, b = v;
    switch (((int)i) % 6) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    case 5: r = v; g = p; b = q; break;
    }
    return Color4F(r, g, b, 1.0f);
}

// ── BGM 플레이어 드로우 ──────────────────────────────────────────────────────

void drawCassetteBody(DrawNode* dn, float cx, float cy, float w, float h, bool on)
{
    const Color4F bodyC = on ? Color4F(0.11f,0.13f,0.16f,1.f) : Color4F(0.22f,0.22f,0.25f,0.65f);
    const Color4F rimC  = on ? Color4F(0.30f,0.35f,0.40f,0.50f) : Color4F(0.40f,0.40f,0.40f,0.35f);
    const Color4F knobC = on ? Color4F(80/255.f,220/255.f,180/255.f,0.70f) : Color4F(0.5f,0.5f,0.5f,0.4f);
    const Color4F deckF = on ? Color4F(0.25f,0.28f,0.32f,1.f) : Color4F(0.32f,0.32f,0.34f,0.7f);
    const Color4F deckB = on ? Color4F(0.04f,0.04f,0.06f,1.f) : Color4F(0.14f,0.14f,0.16f,0.7f);
    const Color4F reelC = on ? Color4F(0.20f,0.55f,0.45f,0.70f) : Color4F(0.28f,0.28f,0.28f,0.5f);
    const Color4F hubC  = on ? Color4F(0.04f,0.04f,0.06f,1.f)   : Color4F(0.12f,0.12f,0.14f,0.8f);
    const Color4F tapeC = on ? Color4F(0.40f,0.75f,0.65f,0.40f)  : Color4F(0.30f,0.30f,0.30f,0.3f);

    float x0 = cx-w/2, x1 = cx+w/2, y0 = cy-h/2, y1 = cy+h/2;
    const float rr = 5.f, k = 0.293f;
    Vec2 body[12] = {
        {x0+rr, y0}, {x1-rr, y0}, {x1-rr*k, y0+rr*k}, {x1, y0+rr},
        {x1, y1-rr}, {x1-rr*k, y1-rr*k}, {x1-rr, y1}, {x0+rr, y1},
        {x0+rr*k, y1-rr*k}, {x0, y1-rr}, {x0, y0+rr}, {x0+rr*k, y0+rr*k},
    };
    dn->drawSolidPoly(body, 12, bodyC);
    dn->drawPoly(body, 12, true, rimC);

    const Color4F hiC = on ? Color4F(0.34f,0.40f,0.48f,0.60f) : Color4F(0.42f,0.42f,0.44f,0.40f);
    const Color4F shC = on ? Color4F(0.01f,0.01f,0.03f,0.90f) : Color4F(0.04f,0.04f,0.05f,0.65f);
    dn->drawSolidRect(Vec2(x0+rr, y1-2),  Vec2(x1-rr, y1),    hiC);
    dn->drawSolidRect(Vec2(x0,    y0+rr), Vec2(x0+2,  y1-rr), hiC);
    dn->drawSolidRect(Vec2(x0+rr, y0),    Vec2(x1-rr, y0+2),  shC);
    dn->drawSolidRect(Vec2(x1-2,  y0+rr), Vec2(x1,    y1-rr), shC);

    const Color4F glowC = on ? Color4F(0.28f,0.35f,0.42f,0.28f) : Color4F(0.30f,0.30f,0.32f,0.15f);
    float sH = h * 0.17f;
    dn->drawSolidRect(Vec2(cx-28, y1-sH*2.2f), Vec2(cx+28, y1-sH*0.6f), glowC);
    dn->drawSolidRect(Vec2(x0+rr, y1-sH), Vec2(x1-rr, y1),
        on ? Color4F(0.17f,0.20f,0.24f,1.f) : Color4F(0.28f,0.28f,0.30f,0.7f));

    float kY = y1 - sH/2;
    dn->drawSolidCircle(Vec2(cx-24,kY), 2.2f, 0, 8, knobC);
    dn->drawSolidCircle(Vec2(cx,   kY), 2.2f, 0, 8, knobC);
    dn->drawSolidCircle(Vec2(cx+24,kY), 2.2f, 0, 8, knobC);

    float dkW=28, dkH=16, dkY=cy-1;
    dn->drawSolidRect(Vec2(cx-dkW/2-2,dkY-dkH/2-2), Vec2(cx+dkW/2+2,dkY+dkH/2+2), deckF);
    dn->drawSolidRect(Vec2(cx-dkW/2-2,dkY-dkH/2-2), Vec2(cx+dkW/2+2,dkY-dkH/2-1),
        Color4F(0.0f,0.0f,0.0f,0.8f));
    dn->drawSolidRect(Vec2(cx-dkW/2-2,dkY+dkH/2+1), Vec2(cx+dkW/2+2,dkY+dkH/2+2),
        Color4F(0.35f,0.40f,0.45f,0.5f));
    dn->drawSolidRect(Vec2(cx-dkW/2,  dkY-dkH/2),   Vec2(cx+dkW/2,  dkY+dkH/2),   deckB);

    const float rR = 4.5f;
    dn->drawSolidCircle(Vec2(cx-6, dkY+1), rR,       0, 12, reelC);
    dn->drawSolidCircle(Vec2(cx+6, dkY+1), rR,       0, 12, reelC);
    dn->drawSolidCircle(Vec2(cx-6, dkY+1), rR*0.42f, 0,  8, hubC);
    dn->drawSolidCircle(Vec2(cx+6, dkY+1), rR*0.42f, 0,  8, hubC);
    dn->drawLine(Vec2(cx-6-rR,  dkY+1),         Vec2(cx-dkW/2+2, dkY-dkH/2+2), tapeC);
    dn->drawLine(Vec2(cx+6+rR,  dkY+1),         Vec2(cx+dkW/2-2, dkY-dkH/2+2), tapeC);
    dn->drawLine(Vec2(cx-dkW/2+2,dkY-dkH/2+2), Vec2(cx+dkW/2-2, dkY-dkH/2+2), tapeC);
}

void drawSpeakerGrille(DrawNode* dn, float r, bool on)
{
    const Color4F bg    = on ? Color4F(0.05f,0.06f,0.09f,1.f) : Color4F(0.15f,0.15f,0.17f,0.65f);
    const Color4F rim   = on ? Color4F(0.35f,0.45f,0.50f,0.55f) : Color4F(0.40f,0.40f,0.40f,0.35f);
    const Color4F ring  = on ? Color4F(80/255.f,220/255.f,180/255.f,0.40f) : Color4F(0.40f,0.40f,0.40f,0.28f);
    const Color4F inner = Color4F(0.0f,0.0f,0.0f,0.30f);
    const Color4F coneH = on ? Color4F(80/255.f,220/255.f,180/255.f,0.10f) : Color4F(0.35f,0.35f,0.35f,0.07f);
    const Color4F cap   = on ? Color4F(80/255.f,220/255.f,180/255.f,0.80f) : Color4F(0.55f,0.55f,0.55f,0.5f);

    dn->drawSolidCircle(Vec2::ZERO, r,      0, 32, bg);
    dn->drawCircle(Vec2::ZERO,      r,      0, 32, false, rim);
    dn->drawCircle(Vec2::ZERO,      r-1.5f, 0, 32, false, inner);
    dn->drawCircle(Vec2::ZERO, r*0.75f,     0, 28, false, ring);
    dn->drawCircle(Vec2::ZERO, r*0.52f,     0, 22, false, ring);
    dn->drawCircle(Vec2::ZERO, r*0.30f,     0, 16, false, ring);
    dn->drawSolidCircle(Vec2::ZERO, r*0.38f, 0, 20, coneH);
    dn->drawSolidCircle(Vec2::ZERO, r*0.12f, 0, 10, cap);
    dn->drawCircle(Vec2::ZERO,      r*0.12f, 0, 10, false, inner);
}

void drawIconPrev(DrawNode* dn, float cx, float cy, float sz, const Color4F& col)
{
    const float H=sz*0.40f, TW=sz*0.22f, BW=sz*0.08f, GAP=sz*0.04f;
    float x = cx - sz*0.36f;
    dn->drawSolidRect(Vec2(x, cy-H), Vec2(x+BW*2, cy+H), col);
    x += BW*2 + GAP;
    Vec2 t1[3]={{x+TW,cy+H},{x,cy},{x+TW,cy-H}}; dn->drawSolidPoly(t1,3,col);
    x += TW + GAP;
    Vec2 t2[3]={{x+TW,cy+H},{x,cy},{x+TW,cy-H}}; dn->drawSolidPoly(t2,3,col);
}

void drawIconNext(DrawNode* dn, float cx, float cy, float sz, const Color4F& col)
{
    const float H=sz*0.40f, TW=sz*0.22f, BW=sz*0.08f, GAP=sz*0.04f;
    float x = cx - sz*0.36f;
    Vec2 t1[3]={{x,cy+H},{x+TW,cy},{x,cy-H}}; dn->drawSolidPoly(t1,3,col);
    x += TW + GAP;
    Vec2 t2[3]={{x,cy+H},{x+TW,cy},{x,cy-H}}; dn->drawSolidPoly(t2,3,col);
    x += TW + GAP;
    dn->drawSolidRect(Vec2(x, cy-H), Vec2(x+BW*2, cy+H), col);
}

void drawIconPlay(DrawNode* dn, float cx, float cy, float sz, const Color4F& col)
{
    Vec2 t[3]={{cx-sz*0.35f,cy+sz*0.42f},{cx+sz*0.45f,cy},{cx-sz*0.35f,cy-sz*0.42f}};
    dn->drawSolidPoly(t, 3, col);
}

void drawIconPause(DrawNode* dn, float cx, float cy, float sz, const Color4F& col)
{
    float bw=sz*0.18f, bh=sz*0.42f, gap=sz*0.12f;
    dn->drawSolidRect(Vec2(cx-gap-bw,cy-bh), Vec2(cx-gap,    cy+bh), col);
    dn->drawSolidRect(Vec2(cx+gap,   cy-bh), Vec2(cx+gap+bw, cy+bh), col);
}

// ── 팝업 칩 버튼 ─────────────────────────────────────────────────────────────

static DrawNode* makeChipBg(Color3B col, float w, float h)
{
    auto bg = DrawNode::create();
    bg->drawSolidRect(Vec2(2, 2), Vec2(w - 2, h - 2),
        Color4F(col.r / 255.f * 0.20f, col.g / 255.f * 0.20f, col.b / 255.f * 0.20f, 0.92f));
    bg->drawRect(Vec2(2, 2), Vec2(w - 2, h - 2),
        Color4F(col.r / 255.f, col.g / 255.f, col.b / 255.f, 0.85f));
    bg->setContentSize(Size(w, h));   // DrawNode 기본 contentSize=0 → MenuItem 히트영역 확보 위해 설정
    return bg;
}

MenuItemSprite* makePopupChipButton(const std::string& text, Color3B col,
    const ccMenuCallback& cb, float w, float h, float fontSize)
{
    auto normal = makeChipBg(col, w, h);
    auto sel    = makeChipBg(col, w, h);   // 눌림 상태에서 버튼이 사라지지 않도록 동일 배경
    auto item = MenuItemSprite::create(normal, sel, cb);
    item->setContentSize(Size(w, h));
    auto lbl = Label::createWithSystemFont(text, "Arial", fontSize);
    lbl->setColor(col);
    lbl->setPosition(Vec2(w / 2, h / 2));
    lbl->setTag(kChipLabelTag);
    item->addChild(lbl);
    return item;
}

// ── 공용 팝업 프레임 ─────────────────────────────────────────────────────────

void attachModalBlocker(Node* backdrop)
{
    auto ls = EventListenerTouchOneByOne::create();
    ls->setSwallowTouches(true);
    ls->onTouchBegan = [](Touch*, Event*) -> bool { return true; };
    Director::getInstance()->getEventDispatcher()
        ->addEventListenerWithSceneGraphPriority(ls, backdrop);
}

PopupFrame makePopupFrame(const std::string& title, Color3B borderCol, Color3B titleCol,
    float w, float h, float titleSize, int dimAlpha, bool modal, bool divider)
{
    // 백드롭 — 전체화면 딤(LayerColor 인자 없이 생성 = 가시영역 전체)
    auto backdrop = LayerColor::create(Color4B(0, 0, 0, 0));
    backdrop->runAction(FadeTo::create(0.18f, dimAlpha));
    if (modal) attachModalBlocker(backdrop);

    // 박스 — 네이비 배경, 중앙 정렬 + 스케일 인
    Size scr = backdrop->getContentSize();
    auto box = LayerColor::create(kPopupBG, w, h);
    box->setPosition(Vec2((scr.width - w) / 2, (scr.height - h) / 2));
    box->setScale(0.7f);
    backdrop->addChild(box);
    box->runAction(Sequence::create(
        ScaleTo::create(0.15f, 1.05f), ScaleTo::create(0.08f, 1.0f), nullptr));

    // 테두리 — 단선 1종(톤 색)
    auto border = DrawNode::create();
    border->drawRect(Vec2(0, 0), Vec2(w, h),
        Color4F(borderCol.r / 255.f, borderCol.g / 255.f, borderCol.b / 255.f, 0.9f));
    box->addChild(border);

    // 타이틀
    auto titleLabel = Label::createWithSystemFont(title, "Arial", titleSize);
    titleLabel->setColor(titleCol);
    titleLabel->setPosition(Vec2(w / 2, h - titleSize - 6));
    box->addChild(titleLabel, 1);

    // 구분선 (헤더가 2행 이상인 팝업은 divider=false로 끄고 직접 그림)
    if (divider) {
        auto div = DrawNode::create();
        float dy = h - titleSize * 2 - 8;
        div->drawLine(Vec2(16, dy), Vec2(w - 16, dy), Color4F(0.5f, 0.5f, 0.5f, 0.8f));
        box->addChild(div);
    }

    return PopupFrame{ backdrop, box, titleLabel };
}
