#include "PCH.h"
#include "game/Preview3D.h"

// Choose one of these = 1. Leave the other = 0.
// Default to UI3D scene manager path.
#ifndef MI_USE_UI3D_SCENE_MANAGER
#define MI_USE_UI3D_SCENE_MANAGER 1
#endif
#ifndef MI_USE_BSGRAPHICS_RENDERER
#define MI_USE_BSGRAPHICS_RENDERER 0
#endif

#if MI_USE_UI3D_SCENE_MANAGER
#  if __has_include(<RE/U/UI3DSceneManager.h>)
#    include <RE/U/UI3DSceneManager.h>
#    define MI_HAS_UI3D 1
#  elif __has_include(<RE/UI3DSceneManager.h>)
#    include <RE/UI3DSceneManager.h>
#    define MI_HAS_UI3D 1
#  else
#    define MI_HAS_UI3D 0
#  endif
#endif

#if MI_USE_BSGRAPHICS_RENDERER
#  if __has_include(<RE/R/Renderer.h>)
#    include <RE/R/Renderer.h>
#    define MI_HAS_RENDERER 1
#  elif __has_include(<RE/Renderer.h>)
#    include <RE/Renderer.h>
#    define MI_HAS_RENDERER 1
#  else
#    define MI_HAS_RENDERER 0
#  endif
#endif
#include <cmath>

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
    needsCameraUpdate_ = true;
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

// Simple yaw(Z) + pitch(X) orbit camera around origin; computes camera position only.
static inline RE::NiPoint3 ComputeOrbitPos(float yaw, float pitch, float distance)
{
    const float cp = std::cos(pitch);
    const float sp = std::sin(pitch);
    const float sy = std::sin(yaw);
    const float cy = std::cos(yaw);

    // Forward vector given yaw/pitch (Y-forward convention)
    const RE::NiPoint3 forward{ sy * cp, cy * cp, sp };
    return RE::NiPoint3{ -forward.x * distance, -forward.y * distance, -forward.z * distance };
}

void Preview3D::UpdateCamera()
{
    if (!camera_ || !needsCameraUpdate_) {
        return;
    }

    const RE::NiPoint3 pos = ComputeOrbitPos(yaw_, pitch_, distance_);
    camera_->world.translate = pos;
    // Keep identity rotation for now; the engine UI renderer can compute view from camera node
    // or we will extend this in the next patch to build a full basis.
    // camera_->world.rotate = RE::NiMatrix3();

    camera_->UpdateWorldBound();
    needsCameraUpdate_ = false;
}

void Preview3D::Render()
{
    if (!initialized_ || !rtv_) return;

    // If we don't have a scene/clone yet, show purple as a fallback
    if (!sceneReady_ || !cloneRoot_) {
        ClearToColor(1.f, 0.f, 1.f, 1.f); // purple fallback
        return;
    }

    // Try engine/UI path first; if unavailable, fall back to slate clear.
    if (TryRenderEngineScene()) {
        return;
    }
    // Placeholder clear to dark slate (indicates scene path is running):
    ClearToColor(0.10f, 0.12f, 0.18f, 1.0f);
    UpdateCamera();

    // TODO (next step):
    //  - Create/borrow a UI 3D scene and attach sceneRoot_
    //  - Use the game's BSSceneRenderer to render that scene to our RTV
    //  - Or setup a tiny offscreen pass using the engine's draw calls
    //
    // For stability, we keep the purple fallback when clone isn't ready.
}

bool Preview3D::TryRenderEngineScene()
{
    if (!device_ || !context_ || !rtv_ || !sceneRoot_ || !camera_) {
        return false;
    }

    // Bind our RTV + viewport for offscreen pass
    D3D11_VIEWPORT vp{};
    vp.TopLeftX = 0.0f; vp.TopLeftY = 0.0f;
    vp.Width    = static_cast<float>(width_);
    vp.Height   = static_cast<float>(height_);
    vp.MinDepth = 0.0f; vp.MaxDepth = 1.0f;
    ID3D11RenderTargetView* rt = rtv_.Get();
    context_->OMSetRenderTargets(1, &rt, nullptr);
    context_->RSSetViewports(1, &vp);

    // Clear to transparent (lets ENB/compositors behave)
    const float preClear[4] = { 0.f, 0.f, 0.f, 0.f };
    context_->ClearRenderTargetView(rtv_.Get(), preClear);

    // Ensure transforms are current (skipped: Update requires NiUpdateData variant)
    // UpdateCamera applies latest orbit state.
    UpdateCamera();

    // Attach camera temporarily to our scene root (if not already)
    bool cameraAttached = false;
    if (auto* root = sceneRoot_.get()) {
        // If your SDK exposes a parent pointer, you can check it here.
        // We just attach unconditionally for now.
        root->AttachChild(camera_.get(), true);
        cameraAttached = true;
    }

    bool rendered = false;

#if MI_USE_UI3D_SCENE_MANAGER && MI_HAS_UI3D
    // ---------- PATH A: UI3DSceneManager ----------
    if (!rendered) {
        if (auto* mgr = RE::UI3DSceneManager::GetSingleton()) {
            auto tryCall = [&](auto* M) {
                bool ok = false;
                (void)M;
                if constexpr (requires { M->Render(sceneRoot_.get(), camera_.get()); }) {
                    M->Render(sceneRoot_.get(), camera_.get());
                    ok = true;
                } else if constexpr (requires { M->RenderNode(sceneRoot_.get(), camera_.get()); }) {
                    M->RenderNode(sceneRoot_.get(), camera_.get());
                    ok = true;
                } else if constexpr (requires { M->DrawScene(sceneRoot_, camera_); }) {
                    M->DrawScene(sceneRoot_, camera_);
                    ok = true;
                }
                return ok;
            };
            rendered = tryCall(mgr);
        }
    }
#endif

#if MI_USE_BSGRAPHICS_RENDERER && MI_HAS_RENDERER
    // ---------- PATH B: BSGraphics::Renderer ----------
    if (!rendered) {
        if (auto* r = RE::BSGraphics::Renderer::GetSingleton()) {
            auto tryCall = [&](auto* R) {
                bool ok = false;
                (void)R;
                if constexpr (requires { R->DrawNiScene(sceneRoot_.get(), camera_.get()); }) {
                    R->DrawNiScene(sceneRoot_.get(), camera_.get());
                    ok = true;
                } else if constexpr (requires { R->DrawScene(camera_.get(), sceneRoot_.get()); }) {
                    R->DrawScene(camera_.get(), sceneRoot_.get());
                    ok = true;
                } else if constexpr (requires { R->RenderNode(sceneRoot_->AsNode(), camera_.get()); }) {
                    R->RenderNode(sceneRoot_->AsNode(), camera_.get());
                    ok = true;
                }
                return ok;
            };
            rendered = tryCall(r);
        }
    }
#endif

    // Detach camera if we attached it
    if (cameraAttached) {
        if (auto* root = sceneRoot_.get()) {
            root->DetachChild(camera_.get());
        }
    }

    return rendered;
}
