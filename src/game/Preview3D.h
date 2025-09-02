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

    // Call once after you have a D3D11 device/context (e.g., when ImGui initializes)
    void Init(ID3D11Device* device, ID3D11DeviceContext* context) {
        if (initialized_) return;
        device_  = device;
        context_ = context;
        initialized_ = (device_ && context_);
    }

    // Ensure the off-screen texture matches the panel size (recreates if needed)
    void EnsureSize(UINT width, UINT height) {
        if (!initialized_) return;
        if (width == 0 || height == 0) return;
        if (width_ == width && height_ == height && rtv_) return;

        width_  = width;
        height_ = height;
        CreateTargets();
    }

    // For step 1: just clear to a visible color so you know the SRV is being shown
    void RenderTestPattern() {
        if (!initialized_ || !rtv_) return;

        // Set viewport to the texture size
        D3D11_VIEWPORT vp{};
        vp.TopLeftX = 0.0f; vp.TopLeftY = 0.0f;
        vp.Width    = static_cast<float>(width_);
        vp.Height   = static_cast<float>(height_);
        vp.MinDepth = 0.0f; vp.MaxDepth = 1.0f;
        context_->RSSetViewports(1, &vp);

        // Bind and clear
        const float color[4] = { 0.12f, 0.12f, 0.14f, 1.0f }; // dark-ish gray
        ID3D11RenderTargetView* rtvs[] = { rtv_.Get() };
        context_->OMSetRenderTargets(1, rtvs, nullptr);
        context_->ClearRenderTargetView(rtv_.Get(), color);

        // (Nothing else for step 1)
    }

    // ImGui (DX11 backend) accepts SRV pointer as ImTextureID
    ID3D11ShaderResourceView* GetSRV() const { return srv_.Get(); }

    void Shutdown() {
        srv_.Reset();
        rtv_.Reset();
        tex_.Reset();
        device_ = nullptr;
        context_ = nullptr;
        initialized_ = false;
        width_ = height_ = 0;
    }

private:
    void CreateTargets() {
        srv_.Reset();
        rtv_.Reset();
        tex_.Reset();

        D3D11_TEXTURE2D_DESC td{};
        td.Width = width_;
        td.Height = height_;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
        if (FAILED(device_->CreateTexture2D(&td, nullptr, &tex))) return;

        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
        rtvDesc.Format = td.Format;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        if (FAILED(device_->CreateRenderTargetView(tex.Get(), &rtvDesc, &rtv_))) return;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = td.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        if (FAILED(device_->CreateShaderResourceView(tex.Get(), &srvDesc, &srv_))) return;

        tex_ = tex; // keep the texture alive
    }

    // State
    bool initialized_ = false;
    UINT width_ = 0, height_ = 0;

    // D3D11
    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* context_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> tex_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv_;
};
