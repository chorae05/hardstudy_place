#include "Bubble.h"
#include "Shader.h"
#include <cmath>

Bubble::Bubble()
    : Entity({ 0.f, 0.f }, { RADIUS * 2.f, RADIUS * 2.f })
{
    active = false;
}

void Bubble::Spawn(XMFLOAT2 pos, XMFLOAT2 vel)
{
    position = pos;
    velocity = vel;
    m_life   = 0.f;
    active   = true;
}

void Bubble::Update(float dt)
{
    if (!active) return;

    m_life     += dt;
    velocity.y += GRAVITY * dt;
    position.x += velocity.x * dt;
    position.y += velocity.y * dt;

    if (position.x < -25.f || position.x > 825.f ||
        position.y < -25.f || position.y > 625.f)
    {
        active = false;
    }
}

void Bubble::Draw(Shader* shader, ID3D11DeviceContext* ctx) const
{
    if (!active) return;

    // Pulsing hue shift: pale blue <-> aqua
    float pulse = std::sinf(m_life * 4.f) * 0.5f + 0.5f;
    float r = 0.30f + pulse * 0.10f;
    float g = 0.75f + pulse * 0.15f;
    float b = 1.0f;
    float a = 0.88f;

    // Outer glow (slightly larger, more transparent)
    shader->DrawCircle(ctx, position, RADIUS + 4.f,
                       { r * 0.6f, g * 0.6f, b, 0.22f });

    // Main bubble sphere
    shader->DrawCircle(ctx, position, RADIUS, { r, g, b, a });
}
