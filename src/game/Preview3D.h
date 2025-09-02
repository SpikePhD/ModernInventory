#pragma once
#include "PCH.h"

#include <d3d11.h>
#include <wrl/client.h>

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

private:
    void CreateTargets();
    void EnsureScene();     // create scene/camera once
    void ClearToColor(float r, float g, float b, float a = 1.0f);

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
};
