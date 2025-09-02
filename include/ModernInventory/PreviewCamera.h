#pragma once

#include <RE/N/NiBound.h>

namespace MI
{
    struct PreviewCamera
    {
        float fovYRad{};   // vertical FOV in radians
        float yawRad{};     // rotation around up
        float pitchRad{};   // rotation around right
        float distance{};   // distance from target center
    };

    namespace Camera
    {
        // Compute simple camera parameters to fit a full body in the given RT size.
        PreviewCamera ComputeFullBody(const RE::NiBound& bound,
                                      unsigned rtWidth,
                                      unsigned rtHeight,
                                      float fovYDeg,
                                      float fitMargin,
                                      float yawDeg,
                                      float pitchDeg);
    }
}

