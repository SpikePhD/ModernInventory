#pragma once

namespace RE
{
    class NiAVObject;
}

namespace MI::Player3D
{
    // Returns the player character's third-person 3D root if available; otherwise nullptr.
    RE::NiAVObject* Get();

    // Convenience: is any player 3D available for previewing right now?
    bool IsAvailable();
}

