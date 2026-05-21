#include "Shader.h"
#include <cstring>

// ============================================================
//  HLSL Vertex Shader
// ============================================================
static const char* g_vsSource = R"HLSL(
cbuffer CB : register(b0)
{
    float2 gPos    : packoffset(c0.x);
    float2 gSize   : packoffset(c0.z);
    float4 gColor  : packoffset(c1.x);
    float2 gScreen : packoffset(c2.x);
    float  gCircle : packoffset(c2.z);
    float  gPad    : packoffset(c2.w);
};

struct VSIn  { float2 lp : POSITION; float2 uv : TEXCOORD; };
struct PSIn  { float4 pos : SV_POSITION; float4 col : COLOR0;
               float2 uv : TEXCOORD0; float circle : TEXCOORD1; };

PSIn VS(VSIn i)
{
    PSIn o;
    float2 wp = gPos + i.lp * gSize;
    o.pos.x  = (wp.x / gScreen.x) * 2.0 - 1.0;
    o.pos.y  = 1.0 - (wp.y / gScreen.y) * 2.0;
    o.pos.z  = 0.0;
    o.pos.w  = 1.0;
    o.col    = gColor;
    o.uv     = i.uv;
    o.circle = gCircle;
    return o;
}
)HLSL";

// ============================================================
//  HLSL Pixel Shader  (bubbles get a glossy sphere look)
// ============================================================
static const char* g_psSource = R"HLSL(
struct PSIn { float4 pos : SV_POSITION; float4 col : COLOR0;
              float2 uv : TEXCOORD0; float circle : TEXCOORD1; };

float4 PS(PSIn i) : SV_TARGET
{
    if (i.circle > 0.5)
    {
        float d = length(i.uv);
        if (d > 0.5) discard;

        // glossy highlight offset toward top-left
        float2 hiOff  = i.uv - float2(-0.15, -0.15);
        float  hi     = saturate(1.0 - length(hiOff) * 3.5);
        float  rim    = saturate((0.5 - d) * 4.0);

        float3 base   = i.col.rgb;
        float3 hiCol  = float3(1.0, 1.0, 1.0);
        float3 rimCol = lerp(base, float3(0.5, 0.9, 1.0), 0.5);

        float3 finalRGB = lerp(base, rimCol, rim * 0.4);
        finalRGB = lerp(finalRGB, hiCol, hi * 0.7);

        float alpha = i.col.a * saturate(1.0 - d * 1.8);
        return float4(finalRGB, alpha);
    }
    return i.col;
}
)HLSL";

// ============================================================
struct QuadVert { float lp[2]; float uv[2]; };

bool Shader::Init(ID3D11Device* dev, ID3D11DeviceContext* ctx)
{
    ComPtr<ID3DBlob> blob, err;

    // --- Vertex shader ---
    HRESULT hr = D3DCompile(g_vsSource, strlen(g_vsSource),
        "VS", nullptr, nullptr, "VS", "vs_4_0", 0, 0, &blob, &err);
    if (FAILED(hr)) {
        if (err) OutputDebugStringA((char*)err->GetBufferPointer());
        return false;
    }
    hr = dev->CreateVertexShader(blob->GetBufferPointer(),
                                 blob->GetBufferSize(), nullptr, &m_vs);
    if (FAILED(hr)) return false;

    // --- Input layout ---
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0,  0,
          D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,  8,
          D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = dev->CreateInputLayout(layout, 2,
        blob->GetBufferPointer(), blob->GetBufferSize(), &m_layout);
    if (FAILED(hr)) return false;

    // --- Pixel shader ---
    hr = D3DCompile(g_psSource, strlen(g_psSource),
        "PS", nullptr, nullptr, "PS", "ps_4_0", 0, 0, &blob, &err);
    if (FAILED(hr)) {
        if (err) OutputDebugStringA((char*)err->GetBufferPointer());
        return false;
    }
    hr = dev->CreatePixelShader(blob->GetBufferPointer(),
                                blob->GetBufferSize(), nullptr, &m_ps);
    if (FAILED(hr)) return false;

    // --- Vertex buffer (unit quad, centered at origin) ---
    // Clockwise winding in NDC (D3D11 default front-face)
    QuadVert verts[] = {
        { {-0.5f,-0.5f}, {-0.5f,-0.5f} },  // 0 TL
        {  {0.5f,-0.5f},  {0.5f,-0.5f} },  // 1 TR
        {  {0.5f, 0.5f},  {0.5f, 0.5f} },  // 2 BR
        { {-0.5f, 0.5f}, {-0.5f, 0.5f} },  // 3 BL
    };
    D3D11_BUFFER_DESC vbd = {};
    vbd.ByteWidth = sizeof(verts);
    vbd.Usage     = D3D11_USAGE_IMMUTABLE;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vsd = { verts };
    hr = dev->CreateBuffer(&vbd, &vsd, &m_vb);
    if (FAILED(hr)) return false;

    // --- Index buffer (two CW triangles) ---
    WORD idx[] = { 0,1,2,  0,2,3 };
    D3D11_BUFFER_DESC ibd = {};
    ibd.ByteWidth = sizeof(idx);
    ibd.Usage     = D3D11_USAGE_IMMUTABLE;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA isd = { idx };
    hr = dev->CreateBuffer(&ibd, &isd, &m_ib);
    if (FAILED(hr)) return false;

    // --- Constant buffer ---
    D3D11_BUFFER_DESC cbd = {};
    cbd.ByteWidth      = sizeof(CBData);
    cbd.Usage          = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = dev->CreateBuffer(&cbd, nullptr, &m_cb);
    if (FAILED(hr)) return false;

    // --- Alpha blend state ---
    D3D11_BLEND_DESC bd = {};
    bd.RenderTarget[0].BlendEnable    = TRUE;
    bd.RenderTarget[0].SrcBlend       = D3D11_BLEND_SRC_ALPHA;
    bd.RenderTarget[0].DestBlend      = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOp        = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    bd.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    hr = dev->CreateBlendState(&bd, &m_blend);
    if (FAILED(hr)) return false;

    // --- Rasterizer (no culling for 2D) ---
    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode        = D3D11_FILL_SOLID;
    rd.CullMode        = D3D11_CULL_NONE;
    rd.DepthClipEnable = TRUE;
    hr = dev->CreateRasterizerState(&rd, &m_rs);
    if (FAILED(hr)) return false;

    // --- Bind persistent state ---
    float bf[4] = {};
    ctx->OMSetBlendState(m_blend.Get(), bf, 0xFFFFFFFF);
    ctx->RSSetState(m_rs.Get());

    UINT stride = sizeof(QuadVert), offset = 0;
    ctx->IASetVertexBuffers(0, 1, m_vb.GetAddressOf(), &stride, &offset);
    ctx->IASetIndexBuffer(m_ib.Get(), DXGI_FORMAT_R16_UINT, 0);
    ctx->IASetInputLayout(m_layout.Get());
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(m_vs.Get(), nullptr, 0);
    ctx->PSSetShader(m_ps.Get(), nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, m_cb.GetAddressOf());
    ctx->PSSetConstantBuffers(0, 1, m_cb.GetAddressOf());

    return true;
}

void Shader::UpdateCB(ID3D11DeviceContext* ctx,
                      XMFLOAT2 pos, XMFLOAT2 sz,
                      XMFLOAT4 color, float isCircle) const
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    ctx->Map(m_cb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    auto* cb    = static_cast<CBData*>(mapped.pData);
    cb->pos[0]  = pos.x;   cb->pos[1]  = pos.y;
    cb->sz[0]   = sz.x;    cb->sz[1]   = sz.y;
    cb->color[0] = color.x; cb->color[1] = color.y;
    cb->color[2] = color.z; cb->color[3] = color.w;
    cb->screen[0] = 800.f; cb->screen[1] = 600.f;
    cb->isCircle  = isCircle;
    cb->pad       = 0.f;
    ctx->Unmap(m_cb.Get(), 0);
}

void Shader::DrawRect(ID3D11DeviceContext* ctx,
                      XMFLOAT2 pos, XMFLOAT2 sz, XMFLOAT4 color) const
{
    UpdateCB(ctx, pos, sz, color, 0.f);
    ctx->DrawIndexed(6, 0, 0);
}

void Shader::DrawCircle(ID3D11DeviceContext* ctx,
                        XMFLOAT2 pos, float radius, XMFLOAT4 color) const
{
    XMFLOAT2 sz = { radius * 2.f, radius * 2.f };
    UpdateCB(ctx, pos, sz, color, 1.f);
    ctx->DrawIndexed(6, 0, 0);
}
