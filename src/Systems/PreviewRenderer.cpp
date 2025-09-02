#include "PCH.h"
#include "ModernInventory/PreviewRenderer.h"
#include "ModernInventory/OffscreenRT.h"
#include <algorithm>
#include "ModernInventory/Player3D.h"
#include "ModernInventory/PreviewGraph.h"
#include "ModernInventory/PreviewCamera.h"
#include "ModernInventory/Config.h"
#include "ModernInventory/Log.h"

namespace MI {

void PreviewRenderer::Init(ID3D11Device* dev, ID3D11DeviceContext* ctx)
{
    m_dev = dev;
    m_ctx = ctx;
}

void PreviewRenderer::OnResize()
{
    // no-op for now
}

void PreviewRenderer::RenderTo(OffscreenRT& rt, ID3D11Texture2D* backBufferTex)
{
    if (!m_ctx || !m_dev) {
        return;
    }

    // Clear first (binds our RTV/DSV)
    rt.Clear(0.06f, 0.07f, 0.09f, 1.0f);
    // Probe player graph + compute camera (Sprint 4/5a)
    if (auto* p3d = MI::Player3D::Get()) {
        auto preview = MI::PreviewGraph::BuildFromPlayer();
        if (preview) {
            const auto& b = preview->worldBound;
            const auto& cfg = MI::ConfigSys::Get();
            auto cam = MI::Camera::ComputeFullBody(b, rt.Width(), rt.Height(), cfg.previewFovDeg, cfg.previewFitMargin, cfg.previewYawDeg, cfg.previewPitchDeg);
            MI::Log::Info(std::string("Preview cam fov=") + std::to_string(cfg.previewFovDeg) +
                          std::string(" dist=") + std::to_string(cam.distance) +
                          std::string(" yaw=") + std::to_string(cfg.previewYawDeg) +
                          std::string(" pitch=") + std::to_string(cfg.previewPitchDeg));
        } else {
            MI::Log::Warn("PreviewGraph clone failed; falling back.");
        }
    } else {
        MI::Log::Warn("Player3D not available; falling back.");
    }


    bool didEngineRender = false;
    // Engine probe: let the game's inventory preview draw into our RT
    if (auto* inv3d = RE::Inventory3DManager::GetSingleton()) {
        ID3D11RenderTargetView* oldRTV = nullptr;
        ID3D11DepthStencilView* oldDSV = nullptr;
        m_ctx->OMGetRenderTargets(1, &oldRTV, &oldDSV);

        D3D11_VIEWPORT oldVP{};
        UINT vpCount = 1;
        m_ctx->RSGetViewports(&vpCount, &oldVP);

        D3D11_VIEWPORT vp{};
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        vp.Width    = static_cast<float>(rt.Width());
        vp.Height   = static_cast<float>(rt.Height());
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        m_ctx->RSSetViewports(1, &vp);

        auto* rtv = rt.GetRTV();
        auto* dsv = rt.GetDSV();
        m_ctx->OMSetRenderTargets(1, &rtv, dsv);

        const std::uint32_t rendered = inv3d->Render();
        didEngineRender = (rendered > 0);

        // Restore state
        m_ctx->OMSetRenderTargets(1, &oldRTV, oldDSV);
        if (oldRTV) oldRTV->Release();
        if (oldDSV) oldDSV->Release();
        if (vpCount == 1) {
            m_ctx->RSSetViewports(1, &oldVP);
        }
    }

    // Fallback: copy a centered region from backbuffer so panel isn't blank
    if (!didEngineRender && backBufferTex) {
        D3D11_TEXTURE2D_DESC bdesc{};
        backBufferTex->GetDesc(&bdesc);

        const UINT copyW = (std::min)(rt.Width(),  bdesc.Width);
        const UINT copyH = (std::min)(rt.Height(), bdesc.Height);
        const UINT left  = (bdesc.Width  > copyW) ? (bdesc.Width  - copyW) / 2u : 0u;
        const UINT top   = (bdesc.Height > copyH) ? (bdesc.Height - copyH) / 2u : 0u;

        D3D11_BOX srcBox{};
        srcBox.left   = left;
        srcBox.top    = top;
        srcBox.front  = 0;
        srcBox.right  = left + copyW;
        srcBox.bottom = top + copyH;
        srcBox.back   = 1;

        ID3D11Resource* dst = nullptr;
        rt.GetRTV()->GetResource(&dst);
        if (dst) {
            m_ctx->CopySubresourceRegion(dst, 0, 0, 0, 0, backBufferTex, 0, &srcBox);
            dst->Release();
        }
    }
}

} // namespace MI
