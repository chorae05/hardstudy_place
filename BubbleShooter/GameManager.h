#pragma once
#include "Shader.h"   // unique_ptr<Shader> 소멸자가 완전한 타입 요구
#include "Player.h"
#include "Enemy.h"
#include "Bubble.h"
#include <vector>
#include <memory>

class GameManager {
public:
    GameManager()  = default;
    ~GameManager() = default;

    bool Init(ID3D11Device* dev, ID3D11DeviceContext* ctx);
    void Update(float dt);
    void Draw(ID3D11DeviceContext* ctx);

    int GetScore() const { return m_score; }

private:
    void SpawnBubble();
    void CheckBubbleEnemyCollision();
    void DrawBackground(ID3D11DeviceContext* ctx);
    void DrawTerrain(ID3D11DeviceContext* ctx);
    void DrawHUD(ID3D11DeviceContext* ctx);
    void DrawDigit(ID3D11DeviceContext* ctx, int digit, float x, float y, float scale);

    std::unique_ptr<Shader> m_shader;
    std::unique_ptr<Player> m_player;

    std::vector<std::unique_ptr<Enemy>>  m_enemies;
    std::vector<std::unique_ptr<Bubble>> m_bubbles;
    std::vector<TilePlatform>            m_terrain;

    int   m_score      = 0;
    float m_spawnTimer = 0.f;
    float m_spawnDelay = 0.55f;
    float m_bgScroll   = 0.f;  // parallax star scroll

    static constexpr int MAX_BUBBLES = 40;
};
