#include "PCH.h"
#include "game/Preview3D.h"

using Microsoft::WRL::ComPtr;

void Preview3D::Init(ID3D11Device* device, ID3D11DeviceContext* context)
{
    if (initialized_) return;
    device_  = device;
    context_ = context;
    initialized_ = (device_ && context_);
}

void Preview3D::Shutdown()
{
    srv_.Reset();
    rtv_.Reset();
    tex_.Reset();

    cloneRoot_ = nullptr;
    camera_    = nullptr;
    sceneRoot_ = nullptr;
    sceneReady_ = false;

    device_ = nullptr;
    context_ = nullptr;
    initialized_ = false;
    width_ = height_ = 0;
}

void Preview3D::EnsureSize(UINT w, UINT h)
{
    if (!initialized_) return;
    if (w == 0 || h == 0)   return;
    if (width_ == w && height_ == h && rtv_) return;

    width_ = w; height_ = h;
    CreateTargets();
}

void Preview3D::CreateTargets()
{
    srv_.Reset();
    rtv_.Reset();
    tex_.Reset();

    D3D11_TEXTURE2D_DESC td{};
    td.Width  = width_;
    td.Height = height_;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    ComPtr<ID3D11Texture2D> tex;
    if (FAILED(device_->CreateTexture2D(&td, nullptr, &tex))) {
        return;
    }

    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = td.Format;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    if (FAILED(device_->CreateRenderTargetView(tex.Get(), &rtvDesc, &rtv_))) {
        return;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = td.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    if (FAILED(device_->CreateShaderResourceView(tex.Get(), &srvDesc, &srv_))) {
        return;
    }

    tex_ = tex;
}

void Preview3D::EnsureScene()
{
    if (sceneReady_) return;

    // Root node for our preview
    sceneRoot_ = RE::NiPointer<RE::NiNode>{ RE::NiNode::Create() };

    // Camera
#if 0
    camera_ = RE::NiCamera::Create();
    if (camera_) {
        // A simple perspective camera setup—tweak later for framing
        camera_->frustum.left   = -0.8f;
        camera_->frustum.right  =  0.8f;
        camera_->frustum.top    =  1.0f;
        camera_->frustum.bottom = -1.0f;
        camera_->frustum.near   =  5.0f;   // push near plane to avoid clipping
        camera_->frustum.far    = 5000.0f;

        // Position the camera in front of the actor (Z forward in Skyrim is usually "into" screen)
        camera_->world.translate = RE::NiPoint3{ 0.0f, -150.0f, 10.0f }; // y back, z up
        camera_->world.rotate = RE::NiMatrix3(); // identity look forward
    }
#endif

    sceneReady_ = (sceneRoot_ != nullptr);
}

void Preview3D::BuildFromPlayer()
{
    EnsureScene();
    if (!sceneReady_) return;

    // Clear any previous clone
    if (cloneRoot_) {
        // Detach previous clone from our scene
        if (auto* node = sceneRoot_.get()) {
            node->DetachChild(cloneRoot_.get());
        }
        cloneRoot_ = nullptr;
    }

    const auto* pc = RE::PlayerCharacter::GetSingleton();
    if (!pc) return;

    // false => don't compute if not ready; we only need current 3D if present
    auto* player3D = pc->Get3D(false);
    if (!player3D) {
        // Player 3D not ready yet; we'll stay purple this frame
        return;
    }

    // Deep clone the full NiAVObject tree
    auto* clone = player3D->Clone(); // deep copy
    if (!clone) return;

    // Attach clone under our scene root
    if (auto* root = sceneRoot_.get()) {
        root->AttachChild(clone, true);
    }

    cloneRoot_ = RE::NiPointer<RE::NiAVObject>{ clone };
}

void Preview3D::ClearToColor(float r, float g, float b, float a)
{
    D3D11_VIEWPORT vp{};
    vp.TopLeftX = 0.0f; vp.TopLeftY = 0.0f;
    vp.Width  = static_cast<float>(width_);
    vp.Height = static_cast<float>(height_);
    vp.MinDepth = 0.0f; vp.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &vp);

    ID3D11RenderTargetView* rtvs[] = { rtv_.Get() };
    context_->OMSetRenderTargets(1, rtvs, nullptr);
    const float col[4] = { r, g, b, a };
    context_->ClearRenderTargetView(rtv_.Get(), col);
}

void Preview3D::Render()
{
    if (!initialized_ || !rtv_) return;

    // If we don't have a scene/clone yet, show purple as a fallback
    if (!sceneReady_ || !cloneRoot_) {
        ClearToColor(1.f, 0.f, 1.f, 1.f); // purple fallback
        return;
    }

    // --- Minimal, engine-friendly "render" handoff ---
    //
    // We rely on the game's UI 3D rendering path to draw NiTrees.
    // The simplest safe stand-in (for now) is to ensure transforms are current
    // and let the engine's UI compositor draw as part of the normal frame.
    //
    // Because we already draw this texture *before* ImGui, the naive version
    // keeps purple unless we explicitly render. For a first visible step,
    // we "fake" a render by clearing to a different color so you can see we
    // reached this path; next patch will hook the engine UI scene renderer
    // (UI3DSceneManager) to actually draw `sceneRoot_` to our RTV.

    // Placeholder clear to dark slate (indicates scene path is running):
    ClearToColor(0.10f, 0.12f, 0.18f, 1.0f);

    // TODO (next step):
    //  - Create/borrow a UI 3D scene and attach sceneRoot_
    //  - Use the game's BSSceneRenderer to render that scene to our RTV
    //  - Or setup a tiny offscreen pass using the engine's draw calls
    //
    // For stability, we keep the purple fallback when clone isn't ready.
}
