#include "PCH.h"
#include "ModernInventory/PreviewRenderer.h"
#include "ModernInventory/OffscreenRT.h"

namespace MI
{
    void PreviewRenderer::Init(ID3D11Device* dev, ID3D11DeviceContext* ctx)
    {
        m_dev = dev;
        m_ctx = ctx;
    }

    void PreviewRenderer::OnResize()
    {
        // no-op for now
    }

    void PreviewRenderer::RenderTo(OffscreenRT& rt)
    {
        // Placeholder: clear to a different tint to indicate this path is called.
        // Next pass: render the player-only scene graph into this RT.
        rt.Clear(0.10f, 0.12f, 0.18f, 1.0f);

        // TODO(next):
        // - Acquire RE::PlayerCharacter::GetSingleton()->Get3D()
        // - Build a preview scene/camera
        // - Bind rt.GetRTV()/rt.GetDSV() and viewport
        // - Issue draw for the player graph only using the game renderer
    }
}

