#pragma once

#include <d3d11.h>

namespace MI
{
    class OffscreenRT
    {
    public:
        void Init(ID3D11Device* device, ID3D11DeviceContext* ctx)
        {
            m_device = device;
            m_context = ctx;
        }

        void Ensure(UINT width, UINT height)
        {
            if (!m_device || !m_context)
                return;
            if (width == 0 || height == 0)
                return;
            if (width == m_w && height == m_h && m_srv && m_rtv)
                return;
            Release();

            D3D11_TEXTURE2D_DESC td{};
            td.Width = width;
            td.Height = height;
            td.MipLevels = 1;
            td.ArraySize = 1;
            td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            td.SampleDesc.Count = 1;
            td.Usage = D3D11_USAGE_DEFAULT;
            td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

            if (FAILED(m_device->CreateTexture2D(&td, nullptr, &m_tex))) {
                return;
            }
            if (FAILED(m_device->CreateRenderTargetView(m_tex, nullptr, &m_rtv))) {
                Release();
                return;
            }
            if (FAILED(m_device->CreateShaderResourceView(m_tex, nullptr, &m_srv))) {
                Release();
                return;
            }

            // Depth-stencil for 3D
            D3D11_TEXTURE2D_DESC dd{};
            dd.Width = width;
            dd.Height = height;
            dd.MipLevels = 1;
            dd.ArraySize = 1;
            dd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            dd.SampleDesc.Count = 1;
            dd.Usage = D3D11_USAGE_DEFAULT;
            dd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
            if (SUCCEEDED(m_device->CreateTexture2D(&dd, nullptr, &m_depth))) {
                D3D11_DEPTH_STENCIL_VIEW_DESC dsvd{};
                dsvd.Format = dd.Format;
                dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                dsvd.Texture2D.MipSlice = 0;
                m_device->CreateDepthStencilView(m_depth, &dsvd, &m_dsv);
            }

            m_w = width;
            m_h = height;
        }

        void Clear(float r, float g, float b, float a)
        {
            if (!m_rtv)
                return;
            const float col[4] = { r, g, b, a };
            m_context->OMSetRenderTargets(1, &m_rtv, m_dsv);
            m_context->ClearRenderTargetView(m_rtv, col);
            if (m_dsv) m_context->ClearDepthStencilView(m_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        }

        ID3D11ShaderResourceView* GetSRV() const { return m_srv; }
        ID3D11RenderTargetView*   GetRTV() const { return m_rtv; }
        ID3D11DepthStencilView*   GetDSV() const { return m_dsv; }
        UINT Width() const { return m_w; }
        UINT Height() const { return m_h; }

        void Release()
        {
            if (m_srv) { m_srv->Release(); m_srv = nullptr; }
            if (m_rtv) { m_rtv->Release(); m_rtv = nullptr; }
            if (m_tex) { m_tex->Release(); m_tex = nullptr; }
            if (m_dsv) { m_dsv->Release(); m_dsv = nullptr; }
            if (m_depth) { m_depth->Release(); m_depth = nullptr; }
            m_w = m_h = 0;
        }

    private:
        ID3D11Device*              m_device{ nullptr };
        ID3D11DeviceContext*       m_context{ nullptr };
        ID3D11Texture2D*           m_tex{ nullptr };
        ID3D11RenderTargetView*    m_rtv{ nullptr };
        ID3D11ShaderResourceView*  m_srv{ nullptr };
        ID3D11Texture2D*           m_depth{ nullptr };
        ID3D11DepthStencilView*    m_dsv{ nullptr };
        UINT                       m_w{ 0 }, m_h{ 0 };
    };
}
