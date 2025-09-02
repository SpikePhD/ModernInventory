#pragma once

#include <cstdint>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Texture2D;

namespace MI
{
    class OffscreenRT;

    class PreviewRenderer
    {
    public:
        void Init(ID3D11Device* dev, ID3D11DeviceContext* ctx);
        void OnResize();
        // Temporary step: copy from backbuffer as a dynamic preview.
        // Next step will render the player model into rt directly.
        void RenderTo(OffscreenRT& rt, ID3D11Texture2D* backBufferTex);

    private:
        ID3D11Device*        m_dev{ nullptr };
        ID3D11DeviceContext* m_ctx{ nullptr };
    };
}
