#include "PCH.h"
#include "ModernInventory/PreviewCamera.h"
#include <algorithm>
#include <cmath>

namespace MI::Camera
{
    PreviewCamera ComputeFullBody(const RE::NiBound& bound,
                                  unsigned rtWidth,
                                  unsigned rtHeight,
                                  float fovYDeg,
                                  float fitMargin,
                                  float yawDeg,
                                  float pitchDeg)
    {
        PreviewCamera cam{};
        const float fovY = std::clamp(fovYDeg, 20.0f, 90.0f) * (3.1415926535f / 180.0f);
        const float yaw   = yawDeg   * (3.1415926535f / 180.0f);
        const float pitch = pitchDeg * (3.1415926535f / 180.0f);
        cam.fovYRad  = fovY;
        cam.yawRad   = yaw;
        cam.pitchRad = pitch;

        // Fit sphere of radius R inside vertical FOV: distance = (R * fitMargin) / sin(FOV/2)
        const float R = std::max(bound.radius, 0.01f) * fitMargin;
        const float halfF = std::max(fovY * 0.5f, 0.1f);
        const float dist = R / std::sin(halfF);

        // Also consider aspect ratio; ensure horizontal fit if RT is very wide.
        const float aspect = (rtHeight > 0) ? (static_cast<float>(rtWidth) / static_cast<float>(rtHeight)) : 1.777f;
        const float fovX = 2.0f * std::atan(std::tan(halfF) * aspect);
        const float distX = R / std::sin(std::max(fovX * 0.5f, 0.1f));

        cam.distance = std::max(dist, distX);
        return cam;
    }
}

