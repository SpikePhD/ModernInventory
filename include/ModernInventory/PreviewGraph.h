#pragma once

#include <RE/N/NiSmartPointer.h>

namespace RE
{
    class NiAVObject;
    class NiNode;
}

namespace MI::PreviewGraph
{
    // Clone the player’s 3D into a standalone root node for preview rendering.
    // Returns nullptr if cloning is not possible at this time.
    RE::NiPointer<RE::NiAVObject> BuildFromPlayer();

    // Basic sanitation for previewing (disable app cull, ensure visible).
    void Sanitize(RE::NiAVObject* root);
}

