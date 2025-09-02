#pragma once

#include <string_view>

namespace MI
{
    namespace Log
    {
        void Init();
        void Info(std::string_view msg);
        void Warn(std::string_view msg);
        void Error(std::string_view msg);
    }

    // Conditionally show a brief on-screen toast (uses ConfigSys::Get().debugToasts)
    void Toast(const char* text);
}

