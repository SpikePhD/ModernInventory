#pragma once

#include <string>

namespace MI
{
    struct Config
    {
        // Default panel width: ~75% of previous size (0.75 of 0.75 screen => ~0.56)
        float panelWidthRatio = 0.56f; // 0..1 of screen width
        int   panelMinWidth   = 520;   // px
        bool  debugToasts     = true;  // show one-time debug notifications

        // Preview camera defaults (full-body framing)
        float previewFovDeg     = 50.0f;  // vertical FOV in degrees
        float previewYawDeg     = 180.0f; // face camera
        float previewPitchDeg   = 0.0f;   // slight tilt optional
        float previewFitMargin  = 1.10f;  // expand bound to ensure full body fits
    };

    namespace ConfigSys
    {
        // Load from ModernInventory.ini next to the DLL (if present)
        void Load();
        const Config& Get();
    }
}
