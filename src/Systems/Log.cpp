#include "PCH.h"
#include "ModernInventory/Log.h"
#include "ModernInventory/Config.h"

#include <Windows.h>
#include <ShlObj.h>
#include <KnownFolders.h>
#include <filesystem>
#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace MI
{
    namespace
    {
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

        std::filesystem::path GetSkseDocumentsFolder()
        {
            PWSTR docs = nullptr;
            std::filesystem::path out;
            if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docs)) && docs) {
                out = std::filesystem::path(docs) / L"My Games" / L"Skyrim Special Edition" / L"SKSE";
            }
            if (docs) {
                CoTaskMemFree(docs);
            }
            return out;
        }
    }

    void Log::Init()
    {
        try {
            // Prefer SKSE documents folder; fallback to module folder if not available
            std::filesystem::path folder = GetSkseDocumentsFolder();
            if (folder.empty()) {
                folder = GetModuleFolder();
            }
            std::error_code ec;
            std::filesystem::create_directories(folder, ec);
            auto logPath = folder / L"ModernInventory.log";
            auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath.string(), true);
            auto logger = std::make_shared<spdlog::logger>("ModernInventory", sink);
            spdlog::set_default_logger(logger);
            spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
            spdlog::info("Logger initialized at {}", logPath.string());
        } catch (...) {
            // ignore
        }
    }

    void Log::Info(std::string_view msg)  { spdlog::info("{}", msg); }
    void Log::Warn(std::string_view msg)  { spdlog::warn("{}", msg); }
    void Log::Error(std::string_view msg) { spdlog::error("{}", msg); }

    void Toast(const char* text)
    {
        if (ConfigSys::Get().debugToasts) {
            RE::DebugNotification(text);
        }
        Log::Info(text);
    }
}
