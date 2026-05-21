#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <string>
#include "GameManager.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

static constexpr int SCR_W = 800;
static constexpr int SCR_H = 600;

static ComPtr<ID3D11Device>           g_device;
static ComPtr<ID3D11DeviceContext>    g_ctx;
static ComPtr<IDXGISwapChain>         g_swap;
static ComPtr<ID3D11RenderTargetView> g_rtv;
static HWND g_hwnd = nullptr;

static GameManager g_game;

// ============================================================
static bool InitD3D11()
{
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount          = 1;
    scd.BufferDesc.Width     = SCR_W;
    scd.BufferDesc.Height    = SCR_H;
    scd.BufferDesc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate = { 60, 1 };
    scd.BufferUsage          = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow         = g_hwnd;
    scd.SampleDesc           = { 1, 0 };
    scd.Windowed             = TRUE;

    D3D_FEATURE_LEVEL fl;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        0, nullptr, 0, D3D11_SDK_VERSION,
        &scd, &g_swap, &g_device, &fl, &g_ctx);
    if (FAILED(hr)) return false;

    ComPtr<ID3D11Texture2D> back;
    g_swap->GetBuffer(0, __uuidof(ID3D11Texture2D),
                      reinterpret_cast<void**>(back.GetAddressOf()));
    hr = g_device->CreateRenderTargetView(back.Get(), nullptr, &g_rtv);
    if (FAILED(hr)) return false;

    D3D11_VIEWPORT vp = {};
    vp.Width    = static_cast<float>(SCR_W);
    vp.Height   = static_cast<float>(SCR_H);
    vp.MaxDepth = 1.f;
    g_ctx->RSSetViewports(1, &vp);
    g_ctx->OMSetRenderTargets(1, g_rtv.GetAddressOf(), nullptr);

    return true;
}

// ============================================================
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_DESTROY: PostQuitMessage(0);  return 0;
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) { PostQuitMessage(0); return 0; }
        break;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

// ============================================================
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    WNDCLASSEX wc   = {};
    wc.cbSize       = sizeof(wc);
    wc.lpfnWndProc  = WndProc;
    wc.hInstance    = hInst;
    wc.hCursor      = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground= reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"BubbleShooterWnd";
    RegisterClassEx(&wc);

    // Fixed-size window
    RECT r = { 0, 0, SCR_W, SCR_H };
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&r, style, FALSE);

    g_hwnd = CreateWindowEx(
        0, L"BubbleShooterWnd",
        L"Bubble Shooter 2D  |  Arrow Keys: Move / Jump  |  ESC: Quit",
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        r.right - r.left, r.bottom - r.top,
        nullptr, nullptr, hInst, nullptr);
    if (!g_hwnd) return -1;

    if (!InitD3D11())           return -1;
    if (!g_game.Init(g_device.Get(), g_ctx.Get())) return -1;

    ShowWindow(g_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(g_hwnd);

    LARGE_INTEGER freq, prev, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&prev);

    int  lastScore = -1;
    MSG  msg       = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            QueryPerformanceCounter(&now);
            float dt = static_cast<float>(now.QuadPart - prev.QuadPart)
                     / static_cast<float>(freq.QuadPart);
            prev = now;
            if (dt > 0.05f) dt = 0.05f; // cap to 20 fps minimum

            g_game.Update(dt);

            // Refresh title bar with current score
            int sc = g_game.GetScore();
            if (sc != lastScore) {
                lastScore = sc;
                std::wstring title =
                    L"Bubble Shooter 2D  |  Score: " + std::to_wstring(sc) +
                    L"  |  Arrow Keys: Move/Jump  |  ESC: Quit";
                SetWindowText(g_hwnd, title.c_str());
            }

            // Render
            const float sky[4] = { 0.03f, 0.05f, 0.18f, 1.f };
            g_ctx->ClearRenderTargetView(g_rtv.Get(), sky);

            g_game.Draw(g_ctx.Get());

            g_swap->Present(1, 0); // VSync on
        }
    }
    return 0;
}
