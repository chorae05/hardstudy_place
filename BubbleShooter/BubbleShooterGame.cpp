// ================================================================
// BubbleShooterGame.cpp  -  버블 슈터 2D 액션 플랫포머
// DirectX 11  |  수업시간 Component / GameObject 패턴 기반
//
// [빌드 방법]
//   Visual Studio 2022 에서 빈 C++ 프로젝트 생성 후 이 파일 하나 추가
//   구성 = x64 Debug / Release  ->  빌드
// ================================================================
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <string>
#include <algorithm>

// ================================================================
//  상수
// ================================================================
static constexpr int   SCR_W   = 800;
static constexpr int   SCR_H   = 600;
static constexpr float GRAVITY = 950.f;
static constexpr float PI      = 3.14159265f;

// ================================================================
//  정점 구조체 (수업 코드 동일)
// ================================================================
struct Vertex { float x, y, z, r, g, b, a; };

// ================================================================
//  지형 플랫폼 데이터
// ================================================================
struct Platform {
    float cx, cy, w, h;   // 화면 공간 중심 + 크기
    float r, g, b;        // 색상

    float Left()   const { return cx - w * 0.5f; }
    float Right()  const { return cx + w * 0.5f; }
    float Top()    const { return cy - h * 0.5f; }
    float Bottom() const { return cy + h * 0.5f; }
};

// ================================================================
//  컴포넌트 추상 클래스 (수업 코드 동일)
// ================================================================
class GameObject;

class Component {
public:
    GameObject* owner = nullptr;
    virtual ~Component() {}
    virtual void Update(float dt) {}
    virtual void Render() {}
};

// ================================================================
//  게임 오브젝트 (수업 코드 기반 + vx, vy, w, h 확장)
// ================================================================
class GameObject {
public:
    float x = 0.f, y = 0.f;   // 화면 공간 중심 위치
    float vx = 0.f, vy = 0.f; // 속도 (픽셀/초)
    float w = 30.f, h = 30.f; // 크기
    float speed = 230.f;       // 이동 속도 (수업 코드 speed 변수)
    bool  active   = true;
    bool  onGround = false;
    std::vector<Component*> comps;

    void Add(Component* c) { c->owner = this; comps.push_back(c); }
    void Update(float dt)  { if (active) for (auto c : comps) c->Update(dt); }
    void Render()          { if (active) for (auto c : comps) c->Render(); }
    ~GameObject()          { for (auto c : comps) delete c; }

    float Left()   const { return x - w * 0.5f; }
    float Right()  const { return x + w * 0.5f; }
    float Top()    const { return y - h * 0.5f; }
    float Bottom() const { return y + h * 0.5f; }
};

// ================================================================
//  전역 D3D11 변수 (수업 코드 동일)
// ================================================================
ID3D11Device*           g_device  = nullptr;
ID3D11DeviceContext*    g_context = nullptr;
IDXGISwapChain*         g_swap    = nullptr;
ID3D11RenderTargetView* g_rtv     = nullptr;
ID3D11Buffer*           g_vb      = nullptr;  // 동적 정점 버퍼 (256 Vertex)
ID3D11VertexShader*     g_vs      = nullptr;
ID3D11PixelShader*      g_ps      = nullptr;
ID3D11InputLayout*      g_layout  = nullptr;

// ================================================================
//  전역 게임 상태
// ================================================================
std::vector<Platform>    g_platforms;
GameObject*              g_player    = nullptr;
std::vector<GameObject*> g_enemies;
std::vector<GameObject*> g_bubbles;
int   g_score       = 0;
float g_bubbleTimer = 0.f;
float g_bubbleDelay = 0.55f;
float g_bgScroll    = 0.f;   // 배경 별 시차 스크롤
HWND  g_hwnd        = nullptr;

// ================================================================
//  좌표 변환: 화면 픽셀 -> NDC
// ================================================================
static float nx(float sx) { return  (sx / SCR_W) * 2.f - 1.f; }
static float ny(float sy) { return -(sy / SCR_H) * 2.f + 1.f; }

// ================================================================
//  렌더링 헬퍼 함수
// ================================================================
static void DrawVerts(const Vertex* v, UINT count)
{
    D3D11_MAPPED_SUBRESOURCE ms = {};
    g_context->Map(g_vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
    memcpy(ms.pData, v, sizeof(Vertex) * count);
    g_context->Unmap(g_vb, 0);
    g_context->Draw(count, 0);
}

// 직사각형: 6정점 (삼각형 2개)
static void DrawRect(float cx, float cy, float w, float h,
                     float r, float g, float b, float a = 1.f)
{
    float x1 = nx(cx - w * 0.5f), x2 = nx(cx + w * 0.5f);
    float y1 = ny(cy - h * 0.5f), y2 = ny(cy + h * 0.5f);
    Vertex v[6] = {
        {x1,y1,0.5f,r,g,b,a}, {x2,y1,0.5f,r,g,b,a}, {x2,y2,0.5f,r,g,b,a},
        {x1,y1,0.5f,r,g,b,a}, {x2,y2,0.5f,r,g,b,a}, {x1,y2,0.5f,r,g,b,a},
    };
    DrawVerts(v, 6);
}

// 원: 18개 삼각형 팬 (center + edge)
static void DrawCircle(float cx, float cy, float radius,
                       float r, float g, float b, float a = 1.f)
{
    const int N = 18;
    Vertex v[N * 3];
    for (int i = 0; i < N; ++i) {
        float a0 = (float) i      / N * 2.f * PI;
        float a1 = (float)(i + 1) / N * 2.f * PI;
        v[i*3  ] = { nx(cx),                  ny(cy),                  0.5f, r,       g,       b, a };
        v[i*3+1] = { nx(cx + cosf(a0)*radius), ny(cy + sinf(a0)*radius), 0.5f, r*0.65f, g*0.85f, b, a };
        v[i*3+2] = { nx(cx + cosf(a1)*radius), ny(cy + sinf(a1)*radius), 0.5f, r*0.65f, g*0.85f, b, a };
    }
    DrawVerts(v, N * 3);
}

// ================================================================
//  컴포넌트 구현 - 물리 (중력 + 지형 충돌)
// ================================================================
class PhysicsComponent : public Component {
public:
    void Update(float dt) override
    {
        owner->vy += GRAVITY * dt;
        owner->x  += owner->vx * dt;
        owner->y  += owner->vy * dt;

        // [요구사항] 화면 X 경계선 충돌
        float hw = owner->w * 0.5f;
        if (owner->x < hw)          owner->x = hw;
        if (owner->x > SCR_W - hw)  owner->x = SCR_W - hw;

        // 지형 AABB 충돌 해소
        owner->onGround = false;
        for (const auto& p : g_platforms) {
            float oL = owner->Left(), oR = owner->Right();
            float oT = owner->Top(),  oB = owner->Bottom();
            if (oR <= p.Left() || oL >= p.Right() || oB <= p.Top() || oT >= p.Bottom()) continue;

            float penR = oR - p.Left(),  penL = p.Right() - oL;
            float penB = oB - p.Top(),   penT = p.Bottom() - oT;
            float minX = (penR < penL) ? penR : penL;
            float minY = (penB < penT) ? penB : penT;

            if (minY <= minX) {
                if (penB <= penT) {
                    owner->y -= penB;
                    if (owner->vy > 0.f) { owner->vy = 0.f; owner->onGround = true; }
                } else {
                    owner->y += penT;
                    if (owner->vy < 0.f) owner->vy = 0.f;
                }
            } else {
                if (penR <= penL) owner->x -= penR;
                else              owner->x += penL;
                owner->vx = 0.f;
            }
        }
    }
};

// ================================================================
//  컴포넌트 구현 - 플레이어 입력 (수업 코드 InputComponent 동일 구조)
// ================================================================
class InputComponent : public Component {
public:
    void Update(float dt) override
    {
        // [요구사항] GetAsyncKeyState 비동기 입력
        owner->vx = 0.f;
        if (GetAsyncKeyState(VK_LEFT)  & 0x8000) owner->vx = -owner->speed;
        if (GetAsyncKeyState(VK_RIGHT) & 0x8000) owner->vx =  owner->speed;

        // [요구사항] 위쪽 점프 입력 -> 위의 플랫폼에 안착
        if ((GetAsyncKeyState(VK_UP)    & 0x8000 ||
             GetAsyncKeyState(VK_SPACE) & 0x8000) && owner->onGround)
        {
            owner->vy = -520.f;
            owner->onGround = false;
        }
    }
};

// ================================================================
//  컴포넌트 구현 - 적 패트롤 AI
// ================================================================
class PatrolComponent : public Component {
public:
    float left, right, spd;
    int   dir = 1;
    float animTimer = 0.f;

    PatrolComponent(float l, float r, float s) : left(l), right(r), spd(s) {}

    void Update(float dt) override
    {
        animTimer += dt;
        owner->x  += spd * dir * dt;
        if (owner->x >= right) { owner->x = right; dir = -1; }
        if (owner->x <= left)  { owner->x = left;  dir =  1; }
    }
};

// ================================================================
//  컴포넌트 구현 - 물방울 물리 (가벼운 중력, 화면 밖 비활성화)
// ================================================================
class BubblePhysicsComponent : public Component {
    static constexpr float BGRAVITY = 180.f;
public:
    void Update(float dt) override
    {
        owner->vy += BGRAVITY * dt;
        owner->x  += owner->vx * dt;
        owner->y  += owner->vy * dt;

        // [요구사항] 화면 밖 비활성화 (메모리 누수 방지)
        if (owner->x < -30.f || owner->x > SCR_W + 30.f ||
            owner->y < -30.f || owner->y > SCR_H + 30.f)
        {
            owner->active = false;
        }
    }
};

// ================================================================
//  컴포넌트 구현 - 렌더러 (수업 코드 RendererComponent 동일 구조)
// ================================================================

// 플레이어 렌더러
class PlayerRenderer : public Component {
    bool facingRight = true;
public:
    void Update(float dt) override
    {
        if (owner->vx > 0.f) facingRight = true;
        if (owner->vx < 0.f) facingRight = false;
    }
    void Render() override
    {
        float px = owner->x, py = owner->y;
        float hw = owner->w * 0.5f, hh = owner->h * 0.5f;

        // 몸통 (파란색)
        DrawRect(px, py, owner->w, owner->h, 0.15f, 0.42f, 0.95f);
        // 벨트
        DrawRect(px, py + 3.f, owner->w, 5.f, 0.05f, 0.22f, 0.72f);
        // 다리
        DrawRect(px - 7.f, py + hh + 6.f, 9.f, 12.f, 0.08f, 0.25f, 0.70f);
        DrawRect(px + 7.f, py + hh + 6.f, 9.f, 12.f, 0.08f, 0.25f, 0.70f);
        // 팔
        float ax = facingRight ? px + hw + 4.f : px - hw - 4.f;
        DrawRect(ax, py - 3.f, 9.f, 18.f, 0.15f, 0.42f, 0.95f);
        // 머리 (살색)
        float headY = py - hh - 13.f;
        DrawCircle(px, headY, 13.f, 1.0f, 0.82f, 0.62f);
        // 머리카락
        DrawRect(px, headY - 10.f, 24.f, 8.f, 0.28f, 0.14f, 0.04f);
        // 눈
        float eyeX = facingRight ? px + 4.f : px - 4.f;
        DrawCircle(eyeX, headY - 1.f, 3.2f, 0.05f, 0.05f, 0.05f);
        DrawCircle(eyeX, headY - 1.f, 1.1f, 1.f,   1.f,   1.f  );
    }
};

// 적 렌더러
class EnemyRenderer : public Component {
    PatrolComponent* patrol = nullptr;
public:
    void Update(float dt) override
    {
        if (!patrol)
            for (auto c : owner->comps)
                if ((patrol = dynamic_cast<PatrolComponent*>(c))) break;
    }
    void Render() override
    {
        float px = owner->x, py = owner->y;
        float hw = owner->w * 0.5f, hh = owner->h * 0.5f;
        float bob = sinf(patrol ? patrol->animTimer * 8.f : 0.f) * 2.f;
        int   dir = patrol ? patrol->dir : 1;

        // 몸통 (빨강)
        DrawRect(px, py + bob, owner->w, owner->h, 0.90f, 0.22f, 0.08f);
        // 가슴 줄무늬
        DrawRect(px, py + bob, owner->w * 0.55f, 7.f, 0.95f, 0.55f, 0.10f);
        // 다리
        DrawRect(px - 6.f, py + hh + 5.f + bob, 8.f, 10.f, 0.68f, 0.15f, 0.05f);
        DrawRect(px + 6.f, py + hh + 5.f + bob, 8.f, 10.f, 0.68f, 0.15f, 0.05f);
        // 머리
        float headY = py - hh - 12.f + bob;
        DrawCircle(px, headY, 12.f, 1.0f, 0.45f, 0.10f);
        // 뿔
        DrawRect(px - 8.f, headY - 17.f, 5.f, 12.f, 0.78f, 0.08f, 0.05f);
        DrawRect(px + 8.f, headY - 17.f, 5.f, 12.f, 0.78f, 0.08f, 0.05f);
        // 눈 (노란색 + 검정 동공)
        float eyeX = (float)dir * 4.f + px;
        DrawCircle(eyeX, headY, 3.5f, 1.0f, 0.85f, 0.0f);
        DrawCircle(eyeX, headY, 1.5f, 0.1f, 0.0f,  0.0f);
    }
};

// 물방울 렌더러
class BubbleRenderer : public Component {
    float life = 0.f;
public:
    void Update(float dt) override { life += dt; }
    void Render() override
    {
        float pulse = sinf(life * 4.f) * 0.5f + 0.5f;
        float r = 0.28f + pulse * 0.12f;
        float g = 0.78f + pulse * 0.12f;
        float b = 1.0f;

        // 외곽 글로우 (반투명)
        DrawCircle(owner->x, owner->y, 14.f, r * 0.5f, g * 0.5f, b, 0.25f);
        // 메인 구체
        DrawCircle(owner->x, owner->y, 11.f, r, g, b, 0.88f);
        // 하이라이트 반점
        DrawCircle(owner->x - 3.5f, owner->y - 3.5f, 3.5f, 1.f, 1.f, 1.f, 0.55f);
    }
};

// ================================================================
//  전방 선언
// ================================================================
void SpawnBubble();
void CheckCollisions();
void DrawBackground();
void DrawTerrain();
void DrawHUD();

// ================================================================
//  Init() - D3D11 초기화 + 게임 오브젝트 생성
// ================================================================
void Init(HWND hWnd)
{
    srand((unsigned)time(nullptr));
    g_hwnd = hWnd;

    // ---- D3D11 디바이스 + 스왑 체인 (수업 코드 동일) ----
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount   = 1;
    sd.BufferDesc.Width  = SCR_W;
    sd.BufferDesc.Height = SCR_H;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage   = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow  = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed      = TRUE;
    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION, &sd, &g_swap, &g_device, nullptr, &g_context);

    ID3D11Texture2D* bb = nullptr;
    g_swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);
    g_device->CreateRenderTargetView(bb, nullptr, &g_rtv);
    bb->Release();

    // ---- 셰이더 (수업 코드 동일) ----
    const char* sCode = R"(
        struct VS_IN { float3 p : POSITION; float4 c : COLOR; };
        struct PS_IN { float4 p : SV_POSITION; float4 c : COLOR; };
        PS_IN VS(VS_IN i) { PS_IN o; o.p = float4(i.p, 1.0f); o.c = i.c; return o; }
        float4 PS(PS_IN i) : SV_Target { return i.c; }
    )";
    ID3DBlob* vsB = nullptr, *psB = nullptr, *err = nullptr;
    D3DCompile(sCode, strlen(sCode), 0, 0, 0, "VS", "vs_4_0", 0, 0, &vsB, &err);
    if (err) { OutputDebugStringA((char*)err->GetBufferPointer()); err->Release(); }
    D3DCompile(sCode, strlen(sCode), 0, 0, 0, "PS", "ps_4_0", 0, 0, &psB, &err);
    if (err) { OutputDebugStringA((char*)err->GetBufferPointer()); err->Release(); }
    g_device->CreateVertexShader(vsB->GetBufferPointer(), vsB->GetBufferSize(), 0, &g_vs);
    g_device->CreatePixelShader (psB->GetBufferPointer(), psB->GetBufferSize(), 0, &g_ps);

    D3D11_INPUT_ELEMENT_DESC ied[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    g_device->CreateInputLayout(ied, 2, vsB->GetBufferPointer(), vsB->GetBufferSize(), &g_layout);
    vsB->Release(); psB->Release();

    // ---- 동적 정점 버퍼 (최대 256 정점) ----
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth      = sizeof(Vertex) * 256;
    bd.Usage          = D3D11_USAGE_DYNAMIC;
    bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    g_device->CreateBuffer(&bd, nullptr, &g_vb);

    // ---- 알파 블렌드 (반투명 물방울) ----
    D3D11_BLEND_DESC blend = {};
    blend.RenderTarget[0].BlendEnable    = TRUE;
    blend.RenderTarget[0].SrcBlend       = D3D11_BLEND_SRC_ALPHA;
    blend.RenderTarget[0].DestBlend      = D3D11_BLEND_INV_SRC_ALPHA;
    blend.RenderTarget[0].BlendOp        = D3D11_BLEND_OP_ADD;
    blend.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ONE;
    blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blend.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
    blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    ID3D11BlendState* bs = nullptr;
    g_device->CreateBlendState(&blend, &bs);
    float bf[4] = {};
    g_context->OMSetBlendState(bs, bf, 0xFFFFFFFF);
    bs->Release();

    // ---- 래스터라이저: 2D 이므로 컬링 없음 ----
    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode        = D3D11_FILL_SOLID;
    rd.CullMode        = D3D11_CULL_NONE;
    rd.DepthClipEnable = TRUE;
    ID3D11RasterizerState* rs = nullptr;
    g_device->CreateRasterizerState(&rd, &rs);
    g_context->RSSetState(rs);
    rs->Release();

    // ---- 지형 (플랫폼) 정의 ----
    // 바닥
    g_platforms.push_back({400.f, 575.f, 800.f,  50.f, 0.12f, 0.28f, 0.08f});
    // 좌측 공중 플랫폼
    g_platforms.push_back({130.f, 400.f, 200.f,  18.f, 0.48f, 0.30f, 0.10f});
    // 우측 공중 플랫폼
    g_platforms.push_back({670.f, 400.f, 200.f,  18.f, 0.48f, 0.30f, 0.10f});
    // 상단 중앙 플랫폼
    g_platforms.push_back({400.f, 293.f, 380.f,  18.f, 0.40f, 0.24f, 0.08f});
    // 피라미드 기단
    g_platforms.push_back({400.f, 534.f, 260.f,  32.f, 0.70f, 0.55f, 0.20f});
    // 피라미드 중단
    g_platforms.push_back({400.f, 504.f, 180.f,  28.f, 0.78f, 0.63f, 0.25f});
    // 피라미드 정상 (플레이어 초기 위치: 발 y=470, 플랫폼 top=470)
    g_platforms.push_back({400.f, 480.f, 100.f,  20.f, 0.86f, 0.72f, 0.30f});

    // ---- 플레이어 초기화 (400, 450) ----
    g_player = new GameObject();
    g_player->x = 400.f; g_player->y = 450.f;
    g_player->w = 28.f;  g_player->h = 38.f;
    g_player->Add(new InputComponent());
    g_player->Add(new PhysicsComponent());
    g_player->Add(new PlayerRenderer());

    // ---- 적 초기화 ----
    // 좌측 플랫폼 top=391 -> 적 center y=391-17=374
    auto* e1 = new GameObject();
    e1->x = 100.f; e1->y = 374.f; e1->w = 30.f; e1->h = 34.f;
    auto* p1 = new PatrolComponent(40.f, 220.f, 78.f);
    e1->Add(p1); e1->Add(new EnemyRenderer());
    g_enemies.push_back(e1);

    // 우측 플랫폼 top=391
    auto* e2 = new GameObject();
    e2->x = 700.f; e2->y = 374.f; e2->w = 30.f; e2->h = 34.f;
    auto* p2 = new PatrolComponent(580.f, 760.f, 92.f);
    e2->Add(p2); e2->Add(new EnemyRenderer());
    g_enemies.push_back(e2);

    // 상단 중앙 플랫폼 top=284 -> 적 center y=284-17=267
    auto* e3 = new GameObject();
    e3->x = 350.f; e3->y = 267.f; e3->w = 30.f; e3->h = 34.f;
    auto* p3 = new PatrolComponent(215.f, 585.f, 65.f);
    e3->Add(p3); e3->Add(new EnemyRenderer());
    g_enemies.push_back(e3);

    // ---- 물방울 오브젝트 풀 (30개 사전 할당) ----
    for (int i = 0; i < 30; ++i) {
        auto* b = new GameObject();
        b->w = 22.f; b->h = 22.f;
        b->active = false;
        b->Add(new BubblePhysicsComponent());
        b->Add(new BubbleRenderer());
        g_bubbles.push_back(b);
    }
}

// ================================================================
//  물방울 스폰 (15~20% 추가 버스트 확률)
// ================================================================
void SpawnBubble()
{
    // 비활성 슬롯 탐색
    GameObject* slot = nullptr;
    for (auto* b : g_bubbles)
        if (!b->active) { slot = b; break; }
    if (!slot) return;

    // 스폰 위치: 플레이어 주변 or 화면 하단 랜덤
    float spawnX, spawnY;
    if (rand() % 3 == 0) {
        spawnX = g_player->x + (float)(rand() % 80 - 40);
        spawnY = g_player->y + 24.f;
    } else {
        spawnX = (float)(rand() % 680 + 60);
        spawnY = 560.f;
    }

    // [요구사항] 랜덤 발사 속도 (포물선/위쪽 방향)
    float vx = (float)(rand() % 320 - 160);          // -160 ~ +160
    float vy = -(float)(rand() % 200 + 260);          // -260 ~ -460 (위쪽)

    slot->x  = spawnX; slot->y  = spawnY;
    slot->vx = vx;     slot->vy = vy;
    slot->active = true;

    // BubbleRenderer life 초기화
    for (auto c : slot->comps)
        if (auto* br = dynamic_cast<BubbleRenderer*>(c)) {
            *br = BubbleRenderer(); br->owner = slot; break;
        }
}

// ================================================================
//  충돌 처리: 물방울 <-> 적 (거리 기반)
// ================================================================
void CheckCollisions()
{
    for (auto* b : g_bubbles) {
        if (!b->active) continue;
        for (auto* e : g_enemies) {
            if (!e->active) continue;
            float dx   = b->x - e->x;
            float dy   = b->y - e->y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < 22.f) {           // 물방울 반지름11 + 적 근사 반지름11
                b->active = false;
                e->active = false;
                g_score  += 100;         // [요구사항] 점수 +100
            }
        }
    }
}

// ================================================================
//  Update() (수업 코드 동일 구조)
// ================================================================
void Update(float dt)
{
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) PostQuitMessage(0);

    g_bgScroll = fmodf(g_bgScroll + dt * 12.f, 800.f);

    g_player->Update(dt);
    for (auto* e : g_enemies) e->Update(dt);

    // [요구사항] 무작위 타이밍 스폰 + 15~20% 이중 버스트
    g_bubbleTimer += dt;
    if (g_bubbleTimer >= g_bubbleDelay) {
        g_bubbleTimer = 0.f;
        SpawnBubble();
        if ((float)(rand() % 100) < 18.f) SpawnBubble();  // 18% 추가 스폰
        g_bubbleDelay = 0.35f + (float)(rand() % 55) * 0.01f;
    }

    for (auto* b : g_bubbles) b->Update(dt);
    CheckCollisions();

    // 타이틀에 점수 표시
    static int lastScore = -1;
    if (g_score != lastScore) {
        lastScore = g_score;
        std::wstring t = L"버블 슈터 2D  |  Score: " + std::to_wstring(g_score)
                       + L"  |  방향키 이동 / 위↑ 점프  |  ESC 종료";
        SetWindowText(g_hwnd, t.c_str());
    }
}

// ================================================================
//  배경 그리기
// ================================================================
void DrawBackground()
{
    // 밤하늘 그라디언트
    DrawRect(400.f, 175.f, 800.f, 350.f, 0.03f, 0.05f, 0.18f);
    DrawRect(400.f, 425.f, 800.f, 250.f, 0.06f, 0.10f, 0.15f);

    // 달
    DrawCircle(720.f, 68.f, 28.f, 1.0f, 0.97f, 0.85f);
    DrawCircle(734.f, 60.f, 22.f, 0.03f, 0.05f, 0.18f); // 초승달 컷아웃

    // 시차 별 레이어 1 (빠름)
    static const float sx1[] = {50,120,200,320,450,570,680,760, 30,400,620};
    static const float sy1[] = {25, 65, 40, 15, 50, 30, 75, 15,130, 95,120};
    for (int i = 0; i < 11; ++i) {
        float x = fmodf(sx1[i] + g_bgScroll * 0.5f, 800.f);
        DrawCircle(x, sy1[i], 1.5f, 1.f, 1.f, 1.f, 0.8f);
    }
    // 별 레이어 2 (느림)
    static const float sx2[] = {80,170,280,390,500,610,730, 55,340,700};
    static const float sy2[] = {45,100, 20, 80, 10, 55, 35,155, 70,145};
    for (int i = 0; i < 10; ++i) {
        float x = fmodf(sx2[i] + g_bgScroll * 0.2f, 800.f);
        DrawCircle(x, sy2[i], 1.0f, 0.85f, 0.9f, 1.f, 0.6f);
    }

    // 산 실루엣
    static const float mX[] = {0,100,200,300,400,500,600,700,800};
    static const float mH[] = {90, 60,120, 75,100, 85,110, 65, 90};
    for (int i = 0; i < 8; ++i) {
        float cx2 = (mX[i] + mX[i+1]) * 0.5f;
        float h   = mH[i];
        DrawRect(cx2, 230.f - h * 0.5f, mX[i+1]-mX[i]+2.f, h, 0.08f, 0.09f, 0.14f);
    }
}

// ================================================================
//  지형 그리기 (하이라이트 + 그림자 엣지 포함)
// ================================================================
void DrawTerrain()
{
    for (const auto& p : g_platforms) {
        DrawRect(p.cx, p.cy, p.w, p.h, p.r, p.g, p.b);
        // 상단 밝은 엣지
        float hr = (p.r + 0.28f < 1.f) ? p.r + 0.28f : 1.f;
        float hg = (p.g + 0.28f < 1.f) ? p.g + 0.28f : 1.f;
        float hb = (p.b + 0.28f < 1.f) ? p.b + 0.28f : 1.f;
        DrawRect(p.cx, p.Top()    + 3.f, p.w, 6.f, hr, hg, hb);
        // 하단 그림자 엣지
        DrawRect(p.cx, p.Bottom() - 3.f, p.w, 6.f, p.r*0.5f, p.g*0.5f, p.b*0.5f);
    }
    // 바닥 풀 장식
    for (int i = 0; i < 16; ++i) {
        float gx = 25.f + i * 50.f;
        DrawRect(gx,      552.f, 8.f, 7.f, 0.22f, 0.60f, 0.12f);
        DrawRect(gx+12.f, 554.f, 6.f, 5.f, 0.18f, 0.52f, 0.10f);
    }
}

// ================================================================
//  HUD 그리기
// ================================================================
void DrawHUD()
{
    // 점수 패널 배경
    DrawRect(90.f, 20.f, 164.f, 30.f, 0.f, 0.f, 0.f, 0.5f);
    // 점수 바 (0~3000 범위, 최대 140px)
    float bw = std::min((float)g_score / 3000.f * 140.f, 140.f);
    DrawRect(18.f,  20.f, 162.f, 26.f, 0.05f, 0.08f, 0.25f, 0.75f);
    if (bw > 0.f)
        DrawRect(18.f + bw * 0.5f - 81.f + bw * 0.5f, 20.f, bw, 20.f,
                 1.f, 0.82f, 0.1f);

    // 적 생존 인디케이터 (우측 상단)
    DrawRect(720.f, 20.f, 150.f, 30.f, 0.f, 0.f, 0.f, 0.5f);
    DrawRect(720.f, 20.f, 148.f, 28.f, 0.25f, 0.04f, 0.04f, 0.7f);
    for (int i = 0; i < (int)g_enemies.size(); ++i) {
        float ex = 650.f + i * 24.f;
        bool  a  = g_enemies[i]->active;
        DrawCircle(ex, 20.f, 8.f, a ? 1.f  : 0.3f,
                              a ? 0.25f: 0.3f,
                              a ? 0.1f : 0.3f);
    }
}

// ================================================================
//  Render() (수업 코드 동일 구조)
// ================================================================
void Render()
{
    float clear[4] = {0.03f, 0.05f, 0.18f, 1.f};
    g_context->ClearRenderTargetView(g_rtv, clear);
    g_context->OMSetRenderTargets(1, &g_rtv, nullptr);

    D3D11_VIEWPORT vp = {0, 0, (float)SCR_W, (float)SCR_H, 0.f, 1.f};
    g_context->RSSetViewports(1, &vp);

    UINT stride = sizeof(Vertex), offset = 0;
    g_context->IASetVertexBuffers(0, 1, &g_vb, &stride, &offset);
    g_context->IASetInputLayout(g_layout);
    g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_context->VSSetShader(g_vs, 0, 0);
    g_context->PSSetShader(g_ps, 0, 0);

    // 그리기 순서 (뒤 -> 앞)
    DrawBackground();
    DrawTerrain();
    for (auto* b : g_bubbles) b->Render();  // 물방울
    for (auto* e : g_enemies) e->Render();  // 적
    g_player->Render();                     // 플레이어
    DrawHUD();                              // HUD

    g_swap->Present(1, 0);
}

// ================================================================
//  Release() (수업 코드 동일 구조)
// ================================================================
void Release()
{
    delete g_player;
    for (auto* e : g_enemies) delete e;
    for (auto* b : g_bubbles) delete b;
    g_enemies.clear(); g_bubbles.clear();

    if (g_vb)     g_vb->Release();
    if (g_layout) g_layout->Release();
    if (g_vs)     g_vs->Release();
    if (g_ps)     g_ps->Release();
    if (g_rtv)    g_rtv->Release();
    if (g_swap)  { g_swap->SetFullscreenState(FALSE, nullptr); g_swap->Release(); }
    if (g_context) g_context->Release();
    if (g_device)  g_device->Release();
}

// ================================================================
//  WndProc + WinMain (수업 코드 동일)
// ================================================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow)
{
    WNDCLASS wc = {0, WndProc, 0, 0, hInst, 0, LoadCursor(0, IDC_ARROW), 0, 0, L"BubbleGame"};
    RegisterClass(&wc);

    RECT rc = {0, 0, SCR_W, SCR_H};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hWnd = CreateWindow(L"BubbleGame",
        L"버블 슈터 2D  |  방향키 이동 / 위↑ 점프  |  ESC 종료",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        0, 0, hInst, 0);

    ShowWindow(hWnd, nShow);
    Init(hWnd);   // [Init]

    // [Game Loop] (수업 코드 동일)
    auto prev = std::chrono::high_resolution_clock::now();
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            auto curr = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> elapsed = curr - prev;
            prev = curr;
            float dt = elapsed.count();
            if (dt > 0.05f) dt = 0.05f;

            Update(dt);   // [Update]
            Render();     // [Render]
        }
    }

    Release();    // [Release]
    return (int)msg.wParam;
}
