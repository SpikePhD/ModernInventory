#include "PCH.h"
#include "ModernInventory/D3D11Hook.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <algorithm>

#include <MinHook.h>

#include <imgui.h>

// Support multiple include layouts used by different imgui packages
#if __has_include(<imgui/backends/imgui_impl_win32.h>)
#  include <imgui/backends/imgui_impl_win32.h>
#elif __has_include(<backends/imgui_impl_win32.h>)
#  include <backends/imgui_impl_win32.h>
#elif __has_include(<imgui_impl_win32.h>)
#  include <imgui_impl_win32.h>
#else
#  error "Could not find imgui_impl_win32.h — check include paths or vcpkg features"
#endif

#if __has_include(<imgui/backends/imgui_impl_dx11.h>)
#  include <imgui/backends/imgui_impl_dx11.h>
#elif __has_include(<backends/imgui_impl_dx11.h>)
#  include <backends/imgui_impl_dx11.h>
#elif __has_include(<imgui_impl_dx11.h>)
#  include <imgui_impl_dx11.h>
#else
#  error "Could not find imgui_impl_dx11.h — check include paths or vcpkg features"
#endif

#include "ModernInventory/OffscreenRT.h"
#include "ModernInventory/PreviewRenderer.h"

namespace MI
{
    namespace
    {
        using Present_t = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);
        Present_t g_OrigPresent = nullptr;
        using CreateDevSwap_t = HRESULT (WINAPI*)(
            IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT,
            const D3D_FEATURE_LEVEL*, UINT, UINT,
            const DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**, ID3D11Device**,
            D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);
        CreateDevSwap_t g_OrigCreateDevSwap = nullptr;

        HWND                 g_hWnd = nullptr;
        ID3D11Device*        g_Device = nullptr;
        ID3D11DeviceContext* g_Context = nullptr;
        ID3D11RenderTargetView* g_MainRTV = nullptr;
        DXGI_FORMAT          g_BackBufferFormat = DXGI_FORMAT_UNKNOWN;
        UINT                 g_Width = 0, g_Height = 0;
        bool                 g_ImGuiInitialized = false;
        bool                 g_InventoryOpen = false;
        bool                 g_HookEnabledNotified = false;
        bool                 g_FirstPresentNotified = false;
        bool                 g_ImGuiInitNotified = false;
        OffscreenRT          g_Offscreen;
        PreviewRenderer      g_Preview;

        void CreateRenderTarget(IDXGISwapChain* swap)
        {
            ID3D11Texture2D* backBuffer = nullptr;
            if (SUCCEEDED(swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer))) {
                g_Device->CreateRenderTargetView(backBuffer, nullptr, &g_MainRTV);
                backBuffer->Release();
            }
        }

        void DestroyRenderTarget()
        {
            if (g_MainRTV) {
                g_MainRTV->Release();
                g_MainRTV = nullptr;
            }
        }

        void EnsureImGuiInit(IDXGISwapChain* swap)
        {
            if (g_ImGuiInitialized) {
                return;
            }

            if (!g_Device || !g_Context) {
                // Acquire device/context from game swapchain
                if (FAILED(swap->GetDevice(__uuidof(ID3D11Device), (void**)&g_Device))) {
                    return;
                }
                g_Device->GetImmediateContext(&g_Context);
            }

            DXGI_SWAP_CHAIN_DESC desc{};
            swap->GetDesc(&desc);
            g_hWnd = desc.OutputWindow;
            g_Width = desc.BufferDesc.Width;
            g_Height = desc.BufferDesc.Height;

            CreateRenderTarget(swap);

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.IniFilename = nullptr; // avoid imgui.ini on disk
            io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

            ImGui::StyleColorsDark();
            ImGui_ImplWin32_Init(g_hWnd);
            ImGui_ImplDX11_Init(g_Device, g_Context);
            g_Offscreen.Init(g_Device, g_Context);
            g_Preview.Init(g_Device, g_Context);

            g_ImGuiInitialized = true;
            if (!g_ImGuiInitNotified) {
                g_ImGuiInitNotified = true;
                RE::DebugNotification("MI: ImGui initialized");
            }
        }

        void OnResize(IDXGISwapChain* swap)
        {
            DXGI_SWAP_CHAIN_DESC desc{};
            swap->GetDesc(&desc);
            if (desc.BufferDesc.Width != 0 && desc.BufferDesc.Height != 0 &&
                (desc.BufferDesc.Width != g_Width || desc.BufferDesc.Height != g_Height)) {
                g_Width = desc.BufferDesc.Width;
                g_Height = desc.BufferDesc.Height;
                DestroyRenderTarget();
                CreateRenderTarget(swap);
            }
        }

        HRESULT __stdcall Present_Hook(IDXGISwapChain* swap, UINT syncInterval, UINT flags)
        {
            if (!g_FirstPresentNotified) {
                g_FirstPresentNotified = true;
                RE::DebugNotification("MI: Present hook called");
            }

            EnsureImGuiInit(swap);
            if (g_ImGuiInitialized) {
                OnResize(swap);

                ImGui_ImplDX11_NewFrame();
                ImGui_ImplWin32_NewFrame();
                ImGui::NewFrame();

                if (g_InventoryOpen) {
                    // Right-side panel only (leave SkyUI left side visible)
                    const float ratio = 0.75f; // take 75% of screen width by default
                    const float minWidth = 520.0f;
                    const float maxWidth = static_cast<float>(g_Width) - 40.0f; // keep a small left margin
                    const float target = (std::max)(minWidth, static_cast<float>(g_Width) * ratio);
                    const float panelWidth = (std::min)(target, maxWidth);
                    const ImVec2 panelPos = ImVec2(static_cast<float>(g_Width) - panelWidth, 0.0f);
                    const ImVec2 panelSize = ImVec2(panelWidth, static_cast<float>(g_Height));

                    ImGui::SetNextWindowPos(panelPos);
                    ImGui::SetNextWindowSize(panelSize);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                    ImGui::Begin("MI_RightPanelRoot", nullptr,
                        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
                        ImGuiWindowFlags_NoCollapse);

                    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(20, 20, 20, 230));
                    ImGui::BeginChild("MI_RightPanelBg", ImVec2(-1, -1), false,
                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

                    ImGui::Text("ModernInventory");
                    ImGui::Separator();
                    ImGui::TextWrapped("Right-side preview area (offscreen RT).");

                    // Ensure and clear offscreen target that will host the character preview
                    const UINT rtW = static_cast<UINT>((std::max)(1.0f, panelSize.x - 24.0f));
                    const UINT rtH = static_cast<UINT>((std::max)(1.0f, panelSize.y - 64.0f));
                    g_Offscreen.Ensure(rtW, rtH);
                    // Render the player preview into the RT (placeholder clears for now)
                    g_Preview.RenderTo(g_Offscreen);

                    // Display the RT as an image
                    if (auto* srv = g_Offscreen.GetSRV()) {
                        ImGui::Dummy(ImVec2(0, 8));
                        ImGui::Image(reinterpret_cast<ImTextureID>(srv), ImVec2(static_cast<float>(rtW), static_cast<float>(rtH)));
                    }
                    
                    ImGui::EndChild();
                    ImGui::PopStyleColor();
                    ImGui::End();
                    ImGui::PopStyleVar();
                }

                ImGui::Render();
                if (g_MainRTV) {
                    g_Context->OMSetRenderTargets(1, &g_MainRTV, nullptr);
                }
                ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            }

            return g_OrigPresent(swap, syncInterval, flags);
        }

        // Hook D3D11CreateDeviceAndSwapChain; then hook Present on the game's swapchain.
        HRESULT WINAPI CreateDeviceAndSwapChain_Hook(
            IDXGIAdapter* pAdapter,
            D3D_DRIVER_TYPE DriverType,
            HMODULE Software,
            UINT Flags,
            const D3D_FEATURE_LEVEL* pFeatureLevels,
            UINT FeatureLevels,
            UINT SDKVersion,
            const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
            IDXGISwapChain** ppSwapChain,
            ID3D11Device** ppDevice,
            D3D_FEATURE_LEVEL* pFeatureLevel,
            ID3D11DeviceContext** ppImmediateContext)
        {
            auto hr = g_OrigCreateDevSwap(
                pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion,
                pSwapChainDesc, ppSwapChain, ppDevice, pFeatureLevel, ppImmediateContext);

            if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain && !g_OrigPresent) {
                auto** vtbl = *reinterpret_cast<void***>(*ppSwapChain);
                auto presentAddr = reinterpret_cast<Present_t>(vtbl[8]);
                if (MH_CreateHook(reinterpret_cast<LPVOID>(presentAddr), reinterpret_cast<LPVOID>(&Present_Hook),
                                  reinterpret_cast<LPVOID*>(&g_OrigPresent)) == MH_OK) {
                    if (MH_EnableHook(reinterpret_cast<LPVOID>(presentAddr)) == MH_OK) {
                        if (!g_HookEnabledNotified) {
                            g_HookEnabledNotified = true;
                            RE::DebugNotification("MI: Present hook enabled (CreateDevice hook)");
                        }
                    }
                }
            }
            return hr;
        }

        void InstallPresentHook()
        {
            if (MH_Initialize() != MH_OK) {
                return;
            }

            if (!g_OrigCreateDevSwap) {
                HMODULE d3d11 = GetModuleHandleW(L"d3d11.dll");
                if (!d3d11) {
                    d3d11 = LoadLibraryW(L"d3d11.dll");
                }
                if (d3d11) {
                    auto addr = GetProcAddress(d3d11, "D3D11CreateDeviceAndSwapChain");
                    if (addr && MH_CreateHook(addr, &CreateDeviceAndSwapChain_Hook,
                                              reinterpret_cast<LPVOID*>(&g_OrigCreateDevSwap)) == MH_OK) {
                        MH_EnableHook(addr);
                        RE::DebugNotification("MI: Hooked D3D11CreateDeviceAndSwapChain");
                        return;
                    }
                }
            }

            // Fallback: create a dummy swapchain to get the Present address and hook globally
            WNDCLASSEXW wc{ sizeof(WNDCLASSEXW) };
            wc.lpfnWndProc = DefWindowProcW;
            wc.hInstance = GetModuleHandleW(nullptr);
            wc.lpszClassName = L"MI_D3D11_WND";
            RegisterClassExW(&wc);
            HWND hwnd = CreateWindowW(wc.lpszClassName, L"MI_D3D11", WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

            DXGI_SWAP_CHAIN_DESC sd{};
            sd.BufferCount = 2;
            sd.BufferDesc.Width = 100;
            sd.BufferDesc.Height = 100;
            sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            sd.OutputWindow = hwnd;
            sd.SampleDesc.Count = 1;
            sd.Windowed = TRUE;
            sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

            ID3D11Device* dev = nullptr;
            ID3D11DeviceContext* ctx = nullptr;
            IDXGISwapChain* sc = nullptr;

            D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
            const UINT flags = 0;
            if (SUCCEEDED(D3D11CreateDeviceAndSwapChain(
                    nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, &featureLevel, 1,
                    D3D11_SDK_VERSION, &sd, &sc, &dev, nullptr, &ctx))) {
                auto** vtbl = *reinterpret_cast<void***>(sc);
                auto presentAddr = reinterpret_cast<Present_t>(vtbl[8]);
                if (MH_CreateHook(reinterpret_cast<LPVOID>(presentAddr), reinterpret_cast<LPVOID>(&Present_Hook),
                                  reinterpret_cast<LPVOID*>(&g_OrigPresent)) == MH_OK) {
                    if (MH_EnableHook(reinterpret_cast<LPVOID>(presentAddr)) == MH_OK) {
                        if (!g_HookEnabledNotified) {
                            g_HookEnabledNotified = true;
                            RE::DebugNotification("MI: Present hook enabled (fallback)");
                        }
                    }
                }
            }

            if (sc)
                sc->Release();
            if (ctx)
                ctx->Release();
            if (dev)
                dev->Release();
            if (hwnd)
                DestroyWindow(hwnd);
            UnregisterClassW(wc.lpszClassName, wc.hInstance);
        }
    } // namespace

    void InstallD3D11Hook()
    {
        InstallPresentHook();
    }

    void SetInventoryOpen(bool open)
    {
        g_InventoryOpen = open;
    }
    bool IsInventoryOpen()
    {
        return g_InventoryOpen;
    }
}

