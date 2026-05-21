#pragma once
#include <DirectXMath.h>
using namespace DirectX;

struct TilePlatform {
    XMFLOAT2 pos;   // center
    XMFLOAT2 size;  // width, height
    XMFLOAT4 color; // RGBA

    float Top()    const { return pos.y - size.y * 0.5f; }
    float Bottom() const { return pos.y + size.y * 0.5f; }
    float Left()   const { return pos.x - size.x * 0.5f; }
    float Right()  const { return pos.x + size.x * 0.5f; }
};
