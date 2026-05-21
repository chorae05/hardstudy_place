#pragma once
#include "Entity.h"
#include "Terrain.h"
#include <vector>

class Player : public Entity {
public:
    bool onGround = false;

    Player(XMFLOAT2 pos, const std::vector<TilePlatform>* terrain);

    void Update(float dt) override;
    void Draw(Shader* shader, ID3D11DeviceContext* ctx) const override;

private:
    void ResolveTerrainCollision();

    const std::vector<TilePlatform>* m_terrain;
    bool m_facingRight = true;

    static constexpr float GRAVITY  = 950.f;
    static constexpr float MOVE_SPD = 230.f;
    static constexpr float JUMP_VEL = -520.f;
    static constexpr float SCR_W    = 800.f;
};
