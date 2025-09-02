#include "PCH.h"
#include "ModernInventory/Config.h"

#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <charconv>

namespace MI
{
    namespace
    {
        Config g_cfg{};

        std::filesystem::path GetModuleFolder()
        {
            HMODULE hm = nullptr;
            wchar_t path[MAX_PATH] = {};
            if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                    reinterpret_cast<LPCWSTR>(&GetModuleFolder), &hm)) {
                GetModuleFileNameW(hm, path, MAX_PATH);
                std::filesystem::path p = path;
                return p.parent_path();
            }
            return {};
        }

        inline std::string trim(std::string_view sv)
        {
            size_t b = 0, e = sv.size();
            while (b < e && isspace(static_cast<unsigned char>(sv[b]))) b++;
            while (e > b && isspace(static_cast<unsigned char>(sv[e - 1]))) e--;
            return std::string{ sv.substr(b, e - b) };
        }
    }

    void ConfigSys::Load()
    {
        auto folder = GetModuleFolder();
        auto ini = folder / L"ModernInventory.ini";
        if (!std::filesystem::exists(ini)) {
            return; // defaults
        }

        std::ifstream f(ini);
        std::string line;
        while (std::getline(f, line)) {
            // strip comments (# or ;) and trim
            auto posc = line.find_first_of("#;");
            if (posc != std::string::npos) line.erase(posc);
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            auto k = trim(std::string_view{ line }.substr(0, eq));
            auto v = trim(std::string_view{ line }.substr(eq + 1));
            if (k.empty()) continue;

            if (_stricmp(k.c_str(), "PanelWidthRatio") == 0) {
                try {
                    g_cfg.panelWidthRatio = std::clamp(std::stof(v), 0.2f, 0.98f);
                } catch (...) {}
            } else if (_stricmp(k.c_str(), "PanelMinWidth") == 0) {
                try {
                    g_cfg.panelMinWidth = (std::max)(320, std::stoi(v));
                } catch (...) {}
            } else if (_stricmp(k.c_str(), "DebugToasts") == 0) {
                g_cfg.debugToasts = (v == "1" || _stricmp(v.c_str(), "true") == 0 || _stricmp(v.c_str(), "yes") == 0);
            } else if (_stricmp(k.c_str(), "PreviewFovDeg") == 0) {
                try { g_cfg.previewFovDeg = std::clamp(std::stof(v), 20.0f, 90.0f); } catch (...) {}
            } else if (_stricmp(k.c_str(), "PreviewYawDeg") == 0) {
                try { g_cfg.previewYawDeg = std::stof(v); } catch (...) {}
            } else if (_stricmp(k.c_str(), "PreviewPitchDeg") == 0) {
                try { g_cfg.previewPitchDeg = std::clamp(std::stof(v), -45.0f, 45.0f); } catch (...) {}
            } else if (_stricmp(k.c_str(), "PreviewFitMargin") == 0) {
                try { g_cfg.previewFitMargin = std::clamp(std::stof(v), 1.0f, 1.5f); } catch (...) {}
            }
        }
    }

    const Config& ConfigSys::Get()
    {
        return g_cfg;
    }
}
