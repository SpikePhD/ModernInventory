#include "PCH.h"
#include "ModernInventory/Player3D.h"
#include "ModernInventory/Log.h"

namespace MI::Player3D
{
    RE::NiAVObject* Get()
    {
        const auto* pc = RE::PlayerCharacter::GetSingleton();
        if (!pc) {
            return nullptr;
        }

        // Prefer third-person graph for a full-body preview
        auto* obj = pc->Get3D();
        if (!obj) {
            // Some setups may rebuild 3D late; tolerate nullptr and let caller fallback
            return nullptr;
        }
        return obj;
    }

    bool IsAvailable()
    {
        return Get() != nullptr;
    }
}

