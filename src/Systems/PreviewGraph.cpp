#include "PCH.h"
#include "ModernInventory/PreviewGraph.h"
#include "ModernInventory/Player3D.h"

namespace MI::PreviewGraph
{
    void Sanitize(RE::NiAVObject* root)
    {
        if (!root) {
            return;
        }
        // Make sure it draws even if the source graph had AppCulled set
        root->SetAppCulled(false);
        // Optional: further sanitation can be done here (strip projectiles, decals, etc.)
    }

    RE::NiPointer<RE::NiAVObject> BuildFromPlayer()
    {
        auto* src = MI::Player3D::Get();
        if (!src) {
            return nullptr;
        }
        // Clone the full graph; NiAVObject::Clone() returns a deep copy where supported
        RE::NiAVObject* cloned = src->Clone();
        if (!cloned) {
            return nullptr;
        }
        Sanitize(cloned);
        return RE::NiPointer<RE::NiAVObject>{ cloned };
    }
}

