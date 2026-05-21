#include "GameManager.h"
#include "Shader.h"
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>

// ============================================================
//  Init
// ============================================================
bool GameManager::Init(ID3D11Device* dev, ID3D11DeviceContext* ctx)
{
    srand(static_cast<unsigned>(time(nullptr)));

    m_shader = std::make_unique<Shader>();
    if (!m_shader->Init(dev, ctx)) return false;

    // ---- Terrain layout (center pos, size, color) ----
    // Floor
    m_terrain.push_back({ {400.f, 575.f}, {800.f,  50.f}, {0.12f,0.28f,0.08f,1.f} });
    // Left aerial platform
    m_terrain.push_back({ {130.f, 400.f}, {200.f,  18.f}, {0.48f,0.30f,0.10f,1.f} });
    // Right aerial platform
    m_terrain.push_back({ {670.f, 400.f}, {200.f,  18.f}, {0.48f,0.30f,0.10f,1.f} });
    // Upper mid platform
    m_terrain.push_back({ {400.f, 293.f}, {380.f,  18.f}, {0.40f,0.25f,0.08f,1.f} });

    // Pyramid (stacked, each piece sits on the one below)
    // Base  : top=518, bottom=550
    m_terrain.push_back({ {400.f, 534.f}, {260.f,  32.f}, {0.70f,0.55f,0.20f,1.f} });
    // Mid   : top=490, bottom=518
    m_terrain.push_back({ {400.f, 504.f}, {180.f,  28.f}, {0.78f,0.63f,0.25f,1.f} });
    // Top   : top=462, bottom=490  -> player feet land at 462+...
    //         We want player center y=450 => feet=470 => top at 470
    m_terrain.push_back({ {400.f, 480.f}, {100.f,  20.f}, {0.86f,0.72f,0.30f,1.f} });

    // ---- Player: (400, 450) => feet at 470 = pyramid top ----
    m_player = std::make_unique<Player>(XMFLOAT2{ 400.f, 450.f }, &m_terrain);

    // ---- Enemies ----
    // Left platform  top=391 => enemy (34px tall) center y = 391-17=374
    m_enemies.push_back(std::make_unique<Enemy>(
        XMFLOAT2{ 100.f, 374.f },  40.f, 220.f,  78.f));
    // Right platform top=391
    m_enemies.push_back(std::make_unique<Enemy>(
        XMFLOAT2{ 700.f, 374.f }, 580.f, 760.f,  92.f));
    // Upper mid platform top=284 => center y=284-17=267
    m_enemies.push_back(std::make_unique<Enemy>(
        XMFLOAT2{ 350.f, 267.f }, 215.f, 585.f,  65.f));

    // ---- Bubble object pool ----
    m_bubbles.reserve(MAX_BUBBLES);
    for (int i = 0; i < MAX_BUBBLES; ++i)
        m_bubbles.push_back(std::make_unique<Bubble>());

    return true;
}

// ============================================================
//  Update
// ============================================================
void GameManager::Update(float dt)
{
    m_bgScroll = std::fmod(m_bgScroll + dt * 12.f, 800.f);

    m_player->Update(dt);

    for (auto& e : m_enemies)
        if (e->IsActive()) e->Update(dt);

    // ---- Bubble spawning ----
    m_spawnTimer += dt;
    if (m_spawnTimer >= m_spawnDelay) {
        m_spawnTimer = 0.f;

        // 15-20% random chance for a double burst
        float roll = static_cast<float>(rand() % 100) / 100.f;
        SpawnBubble();
        if (roll < 0.18f) SpawnBubble();

        // Randomise next interval
        m_spawnDelay = 0.35f + static_cast<float>(rand() % 55) / 100.f;
    }

    for (auto& b : m_bubbles)
        if (b->IsActive()) b->Update(dt);

    CheckBubbleEnemyCollision();
}

void GameManager::SpawnBubble()
{
    Bubble* slot = nullptr;
    for (auto& b : m_bubbles) {
        if (!b->IsActive()) { slot = b.get(); break; }
    }
    if (!slot) return;

    float spawnX, spawnY;
    int choice = rand() % 3;
    if (choice == 0) {
        // Near player
        spawnX = m_player->position.x + static_cast<float>(rand() % 80 - 40);
        spawnY = m_player->position.y + 24.f;
    } else {
        // Random position along the bottom strip
        spawnX = static_cast<float>(rand() % 680 + 60);
        spawnY = 558.f;
    }

    float vx = static_cast<float>(rand() % 320 - 160);  // -160..+160
    float vy = -static_cast<float>(rand() % 200 + 260);  // -260..-460 (upward)

    slot->Spawn({ spawnX, spawnY }, { vx, vy });
}

void GameManager::CheckBubbleEnemyCollision()
{
    for (auto& b : m_bubbles) {
        if (!b->IsActive()) continue;
        for (auto& e : m_enemies) {
            if (!e->IsActive()) continue;

            // Distance-based: bubble circle vs enemy AABB approximate circle
            float dx   = b->position.x - e->position.x;
            float dy   = b->position.y - e->position.y;
            float dist = std::sqrtf(dx * dx + dy * dy);
            float hitR = Bubble::RADIUS + 20.f; // enemy hit radius

            if (dist < hitR) {
                b->Deactivate();
                e->Deactivate();
                m_score += 100;
                break;
            }
        }
    }
}

// ============================================================
//  Draw helpers
// ============================================================
void GameManager::DrawBackground(ID3D11DeviceContext* ctx)
{
    // Deep night sky gradient (two rects)
    m_shader->DrawRect(ctx, { 400.f, 160.f }, { 800.f, 320.f },
                       { 0.03f, 0.05f, 0.18f, 1.f });
    m_shader->DrawRect(ctx, { 400.f, 420.f }, { 800.f, 260.f },
                       { 0.06f, 0.10f, 0.15f, 1.f });

    // Moon
    m_shader->DrawCircle(ctx, { 720.f, 70.f }, 28.f,
                         { 1.0f, 0.97f, 0.85f, 1.f });
    m_shader->DrawCircle(ctx, { 732.f, 62.f }, 22.f,
                         { 0.03f, 0.05f, 0.18f, 1.f }); // crescent cutout

    // Scrolling star layer 1 (fast)
    static const float sx1[] = { 40,110,200,320,450,570,680,760, 30,400,620 };
    static const float sy1[] = { 25, 65, 40, 15, 50, 30, 75, 15,130, 95,120 };
    for (int i = 0; i < 11; ++i) {
        float x = std::fmod(sx1[i] + m_bgScroll * 0.5f, 800.f);
        m_shader->DrawCircle(ctx, { x, sy1[i] }, 1.5f,
                             { 1.f, 1.f, 1.f, 0.80f });
    }
    // Star layer 2 (slow)
    static const float sx2[] = { 80,170,280,390,500,610,730, 55,340,700 };
    static const float sy2[] = { 45,100, 20, 80, 10, 55, 35,155, 70,145 };
    for (int i = 0; i < 10; ++i) {
        float x = std::fmod(sx2[i] + m_bgScroll * 0.2f, 800.f);
        m_shader->DrawCircle(ctx, { x, sy2[i] }, 1.0f,
                             { 0.85f, 0.90f, 1.f, 0.60f });
    }

    // Distant mountains silhouette
    static const float mtnX[] = { 0,100,200,300,400,500,600,700,800 };
    static const float mtnH[] = {90, 60,120, 75,100, 85,110, 65, 90 };
    for (int i = 0; i < 8; ++i) {
        float cx = (mtnX[i] + mtnX[i + 1]) * 0.5f;
        float h  = mtnH[i];
        float w  = mtnX[i + 1] - mtnX[i] + 2.f;
        float ty = 230.f - h;
        m_shader->DrawRect(ctx, { cx, ty + h * 0.5f }, { w, h },
                           { 0.08f, 0.09f, 0.14f, 1.f });
    }
}

void GameManager::DrawTerrain(ID3D11DeviceContext* ctx)
{
    for (const auto& t : m_terrain) {
        // Main surface
        m_shader->DrawRect(ctx, t.pos, t.size, t.color);

        // Top edge bright highlight
        XMFLOAT4 hi = { std::min(1.f, t.color.x + 0.28f),
                         std::min(1.f, t.color.y + 0.28f),
                         std::min(1.f, t.color.z + 0.28f), 1.f };
        m_shader->DrawRect(ctx,
            { t.pos.x, t.pos.y - t.size.y * 0.5f + 3.f },
            { t.size.x, 6.f }, hi);

        // Bottom shadow
        XMFLOAT4 sh = { t.color.x * 0.55f, t.color.y * 0.55f,
                         t.color.z * 0.55f, 1.f };
        m_shader->DrawRect(ctx,
            { t.pos.x, t.pos.y + t.size.y * 0.5f - 3.f },
            { t.size.x, 6.f }, sh);
    }

    // Grass tufts on the floor top edge
    float grassY = 553.f;
    for (int i = 0; i < 16; ++i) {
        float gx = 25.f + i * 50.f;
        m_shader->DrawRect(ctx, { gx, grassY }, { 8.f, 7.f },
                           { 0.22f, 0.60f, 0.12f, 1.f });
        m_shader->DrawRect(ctx, { gx + 12.f, grassY + 2.f }, { 6.f, 5.f },
                           { 0.18f, 0.52f, 0.10f, 1.f });
    }
}

// Minimal 7-segment style digit renderer (tiny quads)
void GameManager::DrawDigit(ID3D11DeviceContext* ctx,
                             int digit, float x, float y, float s)
{
    // Segment masks: top, top-left, top-right, middle, bot-left, bot-right, bottom
    static const bool seg[10][7] = {
        {1,1,1,0,1,1,1}, // 0
        {0,0,1,0,0,1,0}, // 1
        {1,0,1,1,1,0,1}, // 2
        {1,0,1,1,0,1,1}, // 3
        {0,1,1,1,0,1,0}, // 4
        {1,1,0,1,0,1,1}, // 5
        {1,1,0,1,1,1,1}, // 6
        {1,0,1,0,0,1,0}, // 7
        {1,1,1,1,1,1,1}, // 8
        {1,1,1,1,0,1,1}, // 9
    };
    if (digit < 0 || digit > 9) return;

    XMFLOAT4 c = { 1.f, 0.95f, 0.3f, 1.f };  // golden yellow

    float hw = s * 5.f, hh = s * 2.f;
    float mx = x, my = y;

    if (seg[digit][0]) // top horiz
        m_shader->DrawRect(ctx, { mx,          my - s*7.f  }, { hw*2, hh }, c);
    if (seg[digit][1]) // top-left vert
        m_shader->DrawRect(ctx, { mx - s*5.5f, my - s*3.5f }, { hh, s*6.f }, c);
    if (seg[digit][2]) // top-right vert
        m_shader->DrawRect(ctx, { mx + s*5.5f, my - s*3.5f }, { hh, s*6.f }, c);
    if (seg[digit][3]) // middle horiz
        m_shader->DrawRect(ctx, { mx,          my          }, { hw*2, hh }, c);
    if (seg[digit][4]) // bot-left vert
        m_shader->DrawRect(ctx, { mx - s*5.5f, my + s*3.5f }, { hh, s*6.f }, c);
    if (seg[digit][5]) // bot-right vert
        m_shader->DrawRect(ctx, { mx + s*5.5f, my + s*3.5f }, { hh, s*6.f }, c);
    if (seg[digit][6]) // bottom horiz
        m_shader->DrawRect(ctx, { mx,          my + s*7.f  }, { hw*2, hh }, c);
}

void GameManager::DrawHUD(ID3D11DeviceContext* ctx)
{
    // Score panel background
    m_shader->DrawRect(ctx, { 90.f, 22.f }, { 160.f, 32.f },
                       { 0.0f, 0.0f, 0.0f, 0.55f });
    m_shader->DrawRect(ctx, { 90.f, 22.f }, { 158.f, 30.f },
                       { 0.05f, 0.08f, 0.25f, 0.75f });

    // "SCORE" label (tiny rects approximating letters is too complex;
    //  draw the numeric score with 7-segment digits instead)
    int score  = m_score;
    int digits[6] = { 0,0,0,0,0,0 };
    int count  = 0;
    int tmp    = score;
    if (tmp == 0) { digits[0] = 0; count = 1; }
    else {
        while (tmp > 0 && count < 6) {
            digits[count++] = tmp % 10;
            tmp /= 10;
        }
    }

    float startX = 150.f;
    float digitY = 22.f;
    float sc     = 1.2f;
    for (int i = count - 1; i >= 0; --i) {
        DrawDigit(ctx, digits[i], startX, digitY, sc);
        startX -= 18.f * sc;
    }

    // Lives / remaining enemies indicator
    int alive = 0;
    for (auto& e : m_enemies) if (e->IsActive()) ++alive;
    m_shader->DrawRect(ctx, { 710.f, 22.f }, { 150.f, 32.f },
                       { 0.0f, 0.0f, 0.0f, 0.55f });
    m_shader->DrawRect(ctx, { 710.f, 22.f }, { 148.f, 30.f },
                       { 0.25f, 0.05f, 0.05f, 0.75f });
    for (int i = 0; i < static_cast<int>(m_enemies.size()); ++i) {
        float ex = 645.f + i * 22.f;
        bool  a  = m_enemies[i]->IsActive();
        m_shader->DrawCircle(ctx, { ex, 22.f }, 7.f,
            a ? XMFLOAT4{ 1.f, 0.3f, 0.1f, 1.f }
              : XMFLOAT4{ 0.3f, 0.3f, 0.3f, 0.4f });
    }
}

// ============================================================
//  Draw (main)
// ============================================================
void GameManager::Draw(ID3D11DeviceContext* ctx)
{
    DrawBackground(ctx);
    DrawTerrain(ctx);

    // Bubbles (behind characters)
    for (const auto& b : m_bubbles)
        if (b->IsActive()) b->Draw(m_shader.get(), ctx);

    // Enemies
    for (const auto& e : m_enemies)
        if (e->IsActive()) e->Draw(m_shader.get(), ctx);

    // Player
    m_player->Draw(m_shader.get(), ctx);

    // HUD on top
    DrawHUD(ctx);
}
