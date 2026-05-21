#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// 16-byte aligned, 3 constant registers (48 bytes)
struct __declspec(align(16)) CBData {
    float pos[2];     // c0.xy  center in screen-space
    float sz[2];      // c0.zw  width, height
    float color[4];   // c1     RGBA
    float screen[2];  // c2.xy  800, 600
    float isCircle;   // c2.z   0=rect  1=circle
    float pad;        // c2.w
};

class Shader {
public:
    Shader()  = default;
    ~Shader() = default;

    bool Init(ID3D11Device* dev, ID3D11DeviceContext* ctx);

    void DrawRect  (ID3D11DeviceContext* ctx,
                    XMFLOAT2 pos, XMFLOAT2 sz,    XMFLOAT4 color) const;
    void DrawCircle(ID3D11DeviceContext* ctx,
                    XMFLOAT2 pos, float radius, XMFLOAT4 color) const;

private:
    void UpdateCB(ID3D11DeviceContext* ctx,
                  XMFLOAT2 pos, XMFLOAT2 sz,
                  XMFLOAT4 color, float isCircle) const;

    ComPtr<ID3D11VertexShader>   m_vs;
    ComPtr<ID3D11PixelShader>    m_ps;
    ComPtr<ID3D11InputLayout>    m_layout;
    ComPtr<ID3D11Buffer>         m_vb;
    ComPtr<ID3D11Buffer>         m_ib;
    ComPtr<ID3D11Buffer>         m_cb;
    ComPtr<ID3D11BlendState>     m_blend;
    ComPtr<ID3D11RasterizerState> m_rs;
};
