#pragma once
#include "Entity.h"

class Bubble : public Entity {
public:
    static constexpr float RADIUS = 11.f;

    Bubble();

    void Spawn(XMFLOAT2 pos, XMFLOAT2 vel);
    void Update(float dt) override;
    void Draw(Shader* shader, ID3D11DeviceContext* ctx) const override;

private:
    float m_life = 0.f; // seconds alive (for color pulse)
    static constexpr float GRAVITY = 180.f;
};
