#pragma once

#include <cstdint>

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace MI
{
    class OffscreenRT;

    class PreviewRenderer
    {
    public:
        void Init(ID3D11Device* dev, ID3D11DeviceContext* ctx);
        void OnResize();
        void RenderTo(OffscreenRT& rt);

    private:
        ID3D11Device*        m_dev{ nullptr };
        ID3D11DeviceContext* m_ctx{ nullptr };
    };
}

