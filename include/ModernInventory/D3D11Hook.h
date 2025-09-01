#pragma once

#include <cstdint>

namespace MI
{
    // Installs a Present() hook and initializes Dear ImGui on first Present.
    void InstallD3D11Hook();

    // Control overlay visibility from gameplay/menu events
    void SetInventoryOpen(bool open);
    bool IsInventoryOpen();
}

