#include "Enemy.h"
#include "Shader.h"
#include <cmath>

Enemy::Enemy(XMFLOAT2 pos, float left, float right, float speed)
    : Entity(pos, { 30.f, 34.f }), patrolLeft(left), patrolRight(right),
      m_speed(speed), m_dir(1)
{}

void Enemy::Update(float dt)
{
    if (!active) return;

    m_animTimer += dt;

    position.x += m_speed * m_dir * dt;

    if (position.x >= patrolRight) { position.x = patrolRight; m_dir = -1; }
    else if (position.x <= patrolLeft) { position.x = patrolLeft; m_dir = 1; }
}

void Enemy::Draw(Shader* shader, ID3D11DeviceContext* ctx) const
{
    if (!active) return;

    float px = position.x, py = position.y;
    float hw = size.x * 0.5f, hh = size.y * 0.5f;

    // Walk bobbing
    float bob = std::sinf(m_animTimer * 8.f) * 2.f;

    // --- Body: fiery red-orange ---
    shader->DrawRect(ctx, { px, py + bob }, size,
                     { 0.90f, 0.25f, 0.08f, 1.f });

    // --- Chest stripe ---
    shader->DrawRect(ctx, { px, py + bob }, { size.x * 0.6f, 8.f },
                     { 0.95f, 0.55f, 0.10f, 1.f });

    // --- Legs ---
    float legY = py + hh + 5.f + bob;
    shader->DrawRect(ctx, { px - 6.f, legY }, { 9.f, 10.f },
                     { 0.70f, 0.18f, 0.05f, 1.f });
    shader->DrawRect(ctx, { px + 6.f, legY }, { 9.f, 10.f },
                     { 0.70f, 0.18f, 0.05f, 1.f });

    // --- Head ---
    float headY = py - hh - 12.f + bob;
    shader->DrawCircle(ctx, { px, headY }, 12.f,
                       { 1.0f, 0.45f, 0.10f, 1.f });

    // --- Horns ---
    shader->DrawRect(ctx, { px - 8.f, headY - 18.f }, { 5.f, 12.f },
                     { 0.80f, 0.10f, 0.05f, 1.f });
    shader->DrawRect(ctx, { px + 8.f, headY - 18.f }, { 5.f, 12.f },
                     { 0.80f, 0.10f, 0.05f, 1.f });

    // --- Eyes: angry yellow ---
    float eyeDir = (m_dir > 0) ? 1.f : -1.f;
    shader->DrawCircle(ctx, { px + eyeDir * 4.f, headY },
                       3.5f, { 1.0f, 0.85f, 0.0f, 1.f });
    shader->DrawCircle(ctx, { px + eyeDir * 4.f, headY },
                       1.5f, { 0.1f, 0.0f, 0.0f, 1.f });
}
