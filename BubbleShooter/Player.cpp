#include "Player.h"
#include "Shader.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <algorithm>

Player::Player(XMFLOAT2 pos, const std::vector<TilePlatform>* terrain)
    : Entity(pos, { 28.f, 38.f }), m_terrain(terrain)
{}

void Player::Update(float dt)
{
    // --- Horizontal input ---
    velocity.x = 0.f;
    if (GetAsyncKeyState(VK_LEFT)  & 0x8000) { velocity.x = -MOVE_SPD; m_facingRight = false; }
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000) { velocity.x =  MOVE_SPD; m_facingRight = true;  }

    // --- Jump ---
    if ((GetAsyncKeyState(VK_UP)    & 0x8000 ||
         GetAsyncKeyState(VK_SPACE) & 0x8000) && onGround)
    {
        velocity.y = JUMP_VEL;
        onGround   = false;
    }

    // --- Gravity ---
    velocity.y += GRAVITY * dt;

    // --- Integrate ---
    position.x += velocity.x * dt;
    position.y += velocity.y * dt;

    // --- Screen X boundary ---
    float hw = size.x * 0.5f;
    if (position.x < hw)        position.x = hw;
    if (position.x > SCR_W - hw) position.x = SCR_W - hw;

    ResolveTerrainCollision();
}

void Player::ResolveTerrainCollision()
{
    onGround = false;
    for (const auto& t : *m_terrain) {
        float pL = Left(), pR = Right(), pT = Top(), pB = Bottom();
        float tL = t.Left(), tR = t.Right(), tT = t.Top(), tB = t.Bottom();

        if (pR <= tL || pL >= tR || pB <= tT || pT >= tB) continue;

        float overlapR = pR - tL;
        float overlapL = tR - pL;
        float overlapB = pB - tT;
        float overlapT = tB - pT;

        float minX = std::min(overlapR, overlapL);
        float minY = std::min(overlapB, overlapT);

        if (minY <= minX) {
            if (overlapB <= overlapT) {
                // Player fell onto the top of the platform
                position.y -= overlapB;
                if (velocity.y > 0.f) { velocity.y = 0.f; onGround = true; }
            } else {
                // Player hit the underside
                position.y += overlapT;
                if (velocity.y < 0.f) velocity.y = 0.f;
            }
        } else {
            if (overlapR <= overlapL) position.x -= overlapR;
            else                      position.x += overlapL;
            velocity.x = 0.f;
        }
    }
}

void Player::Draw(Shader* shader, ID3D11DeviceContext* ctx) const
{
    if (!active) return;

    float px = position.x, py = position.y;
    float hw = size.x * 0.5f, hh = size.y * 0.5f;

    // --- Body: vibrant blue ---
    shader->DrawRect(ctx, { px, py }, size, { 0.15f, 0.40f, 0.95f, 1.f });

    // --- Belt highlight ---
    shader->DrawRect(ctx, { px, py + hh * 0.1f }, { size.x, 6.f },
                     { 0.05f, 0.25f, 0.75f, 1.f });

    // --- Legs ---
    float legY = py + hh + 5.f;
    shader->DrawRect(ctx, { px - 7.f, legY }, { 10.f, 12.f },
                     { 0.10f, 0.25f, 0.70f, 1.f });
    shader->DrawRect(ctx, { px + 7.f, legY }, { 10.f, 12.f },
                     { 0.10f, 0.25f, 0.70f, 1.f });

    // --- Head ---
    float headY = py - hh - 13.f;
    shader->DrawCircle(ctx, { px, headY }, 13.f, { 1.0f, 0.82f, 0.60f, 1.f });

    // --- Hair ---
    shader->DrawRect(ctx, { px, headY - 10.f }, { 22.f, 8.f },
                     { 0.30f, 0.15f, 0.05f, 1.f });

    // --- Eyes ---
    float eyeDir = m_facingRight ? 1.f : -1.f;
    shader->DrawCircle(ctx, { px + eyeDir * 4.f,  headY - 1.f }, 3.2f,
                       { 0.05f, 0.05f, 0.05f, 1.f });
    shader->DrawCircle(ctx, { px + eyeDir * 4.f,  headY - 1.f }, 1.2f,
                       { 1.f,  1.f,  1.f,  1.f });

    // --- Arms ---
    float armX = m_facingRight ? px + hw + 5.f : px - hw - 5.f;
    shader->DrawRect(ctx, { armX, py - 4.f }, { 10.f, 20.f },
                     { 0.15f, 0.40f, 0.95f, 1.f });
}
