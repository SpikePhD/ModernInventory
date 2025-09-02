#include "PCH.h"
#include "ModernInventory/ModernInventory.h"
#include "ModernInventory/Config.h"
#include "ModernInventory/Log.h"
#include "ModernInventory/D3D11Hook.h"
#include "game/Preview3D.h"

// -------------------- Input sink (detect 'I' key) --------------------
class MI_InputSink final : public RE::BSTEventSink<RE::InputEvent*>
{
public:
    static MI_InputSink* GetSingleton()
    {
        static MI_InputSink inst;
        return std::addressof(inst);
    }

    // Keyboard scancode for 'I' (DirectInput)
    static constexpr std::uint32_t kScan_I = 0x17;

    RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_events,
                                          RE::BSTEventSource<RE::InputEvent*>*) override
    {
        if (!a_events) {
            return RE::BSEventNotifyControl::kContinue;
        }

        for (auto e = *a_events; e; e = e->next) {
            if (e->eventType != RE::INPUT_EVENT_TYPE::kButton) {
                continue;
            }
            const auto* be = e->AsButtonEvent();
            if (!be || be->GetDevice() != RE::INPUT_DEVICE::kKeyboard) {
                continue;
            }

            // fire on press
            if (be->idCode == kScan_I && be->IsDown()) {
                RE::DebugNotification("ModernInventory: I key detected!");
                if (auto* con = RE::ConsoleLog::GetSingleton()) {
                    con->Print("ModernInventory: I key detected!");
                }
            }
        }
        return RE::BSEventNotifyControl::kContinue;
    }
};

// -------------------- Menu sink (Inventory open/close) --------------------
class MI_MenuSink final : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
{
public:
    static MI_MenuSink* GetSingleton()
    {
        static MI_MenuSink inst;
        return std::addressof(inst);
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event,
                                          RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
    {
        if (!a_event) {
            return RE::BSEventNotifyControl::kContinue;
        }

            if (a_event->menuName == RE::InventoryMenu::MENU_NAME) {
                if (a_event->opening) {
                    RE::DebugNotification("ModernInventory: Inventory opened");
                    if (auto* con = RE::ConsoleLog::GetSingleton()) {
                        con->Print("ModernInventory: Inventory opened");
                    }
                    MI::SetInventoryOpen(true);
                    if (auto* inv3d = RE::Inventory3DManager::GetSingleton()) {
                        inv3d->Clear3D(); // hide vanilla 3D preview under our panel
                    }
                    // Kick off our paperdoll build; if player 3D isn't ready yet
                    // we'll briefly show purple until it becomes available.
                    Preview3D::Get().BuildFromPlayer();
                } else {
                    RE::DebugNotification("ModernInventory: Inventory closed");
                    if (auto* con = RE::ConsoleLog::GetSingleton()) {
                        con->Print("ModernInventory: Inventory closed");
                    }
                    MI::SetInventoryOpen(false);
                    // no need to restore Inventory3D; game rebuilds on next open
                    // TODO(next): hide ImGui right pane
                }
            }

        return RE::BSEventNotifyControl::kContinue;
    }
};

// -------------------- Helpers --------------------
namespace
{
    void RegisterSinks()
    {
        if (auto* inputMgr = RE::BSInputDeviceManager::GetSingleton()) {
            inputMgr->AddEventSink(MI_InputSink::GetSingleton());
        }
        if (auto* ui = RE::UI::GetSingleton()) {
            ui->AddEventSink(MI_MenuSink::GetSingleton());
        }
    }

    void OnSKSEMessage(SKSE::MessagingInterface::Message* m)
    {
        using M = SKSE::MessagingInterface;
        switch (m->type) {
        case M::kDataLoaded:
        case M::kNewGame:
        case M::kPostLoadGame:
            RegisterSinks();
            break;
        default:
            break;
        }
    }
}

// -------------------- SKSE entry --------------------
extern "C" __declspec(dllexport) bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* skse)
{
    SKSE::Init(skse);

    if (auto* msg = SKSE::GetMessagingInterface()) {
        msg->RegisterListener(OnSKSEMessage);
    }

    // Early sinks (harmless to repeat on DataLoaded)
    RegisterSinks();

    // Install D3D11 Present hook + Dear ImGui bootstrap
    MI::InstallD3D11Hook();

        MI::ConfigSys::Load();
        MI::Log::Init();

        MI::Toast("ModernInventory loaded");
    if (auto* con = RE::ConsoleLog::GetSingleton()) {
        con->Print("ModernInventory %s loaded", MI::kVersion);
    }
    return true;
}
