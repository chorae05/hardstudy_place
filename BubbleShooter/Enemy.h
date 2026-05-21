#pragma once
#include "Entity.h"

class Enemy : public Entity {
public:
    float patrolLeft;
    float patrolRight;

    Enemy(XMFLOAT2 pos, float left, float right, float speed = 85.f);

    void Update(float dt) override;
    void Draw(Shader* shader, ID3D11DeviceContext* ctx) const override;

private:
    float m_speed;
    int   m_dir = 1; // +1 right, -1 left
    float m_animTimer = 0.f;
};
