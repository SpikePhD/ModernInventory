#pragma once
#include "PCH.h"

#include <d3d11.h>
#include <wrl/client.h>
#include <algorithm>

class Preview3D {
public:
    static Preview3D& Get() {
        static Preview3D inst;
        return inst;
    }

    // Called once (after your D3D hook initializes ImGui and you have device/context)
    void Init(ID3D11Device* device, ID3D11DeviceContext* context);

    // Resize the offscreen texture (call every frame with current pane size)
    void EnsureSize(UINT width, UINT height);

    // Build a static "paperdoll" from current player 3D (call on Inventory open, or equip change)
    void BuildFromPlayer();

    // Render scene into our RTV (safe fallback to purple)
    void Render();

    // ImGui uses SRV as texture id (DX11 backend)
    ID3D11ShaderResourceView* GetSRV() const { return srv_.Get(); }

    // Cleanup
    void Shutdown();

    // NEW: call when inventory opens (or right before first frame)
    void RebuildNow() { BuildFromPlayer(); }

    // NEW: camera controls (can be bound to hotkeys later)
    void SetYaw(float radians)   { yaw_ = radians; needsCameraUpdate_ = true; }
    void SetPitch(float radians) { pitch_ = std::clamp(radians, -1.2f, 1.2f); needsCameraUpdate_ = true; }
    void SetZoom(float dist)     { distance_ = std::clamp(dist, 60.0f, 220.0f); needsCameraUpdate_ = true; }

private:
    void CreateTargets();
    void EnsureScene();     // create scene/camera once
    void ClearToColor(float r, float g, float b, float a = 1.0f);
    void UpdateCamera();          // NEW: position/orient camera from yaw/pitch/distance

private:
    // D3D
    bool initialized_ = false;
    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* context_ = nullptr;

    UINT width_ = 0, height_ = 0;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> tex_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv_;

    // UI 3D scene objects
    RE::NiPointer<RE::NiNode>     sceneRoot_;   // our root for the preview scene
    RE::NiPointer<RE::NiCamera>   camera_;      // preview camera
    RE::NiPointer<RE::NiAVObject> cloneRoot_;   // deep-cloned player tree

    bool sceneReady_ = false;

    // NEW: simple orbit camera state
    float yaw_   = 0.0f;     // left/right rotate
    float pitch_ = 0.1f;     // up/down tilt
    float distance_ = 140.0f; // zoom distance from target
    bool needsCameraUpdate_ = true;
};
