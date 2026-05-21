#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;

class Shader;

class Entity {
public:
    XMFLOAT2 position; // center position (screen space)
    XMFLOAT2 size;     // width, height
    XMFLOAT2 velocity;
    bool     active;

    Entity(XMFLOAT2 pos, XMFLOAT2 sz)
        : position(pos), size(sz), velocity({ 0.f, 0.f }), active(true)
    {}
    virtual ~Entity() = default;

    virtual void Update(float dt) = 0;
    virtual void Draw(Shader* shader, ID3D11DeviceContext* ctx) const = 0;

    bool IsActive() const { return active; }
    void Deactivate()     { active = false; }

    float Left()   const { return position.x - size.x * 0.5f; }
    float Right()  const { return position.x + size.x * 0.5f; }
    float Top()    const { return position.y - size.y * 0.5f; }
    float Bottom() const { return position.y + size.y * 0.5f; }
};
