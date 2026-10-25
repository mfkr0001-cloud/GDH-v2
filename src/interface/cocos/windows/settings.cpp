#include <Geode/Geode.hpp>
#include "../hacks_tab.hpp"
#include "../../../core/gui.hpp"
#include "../../../core/config.hpp"
#include "../../../core/replayEngine.hpp"
#include "../overlay_button.hpp"
#include "../hacks_layer.hpp"

$execute {
    auto& gui = GDH::Gui::get();
    auto& config = Config::get();
    auto& window = gui.getWindow("Settings");

    window.setCustomWindowCocos([&config, &gui](cocos2d::CCNode* node) {
        auto tab = static_cast<HacksTab*>(node);
        tab->addConfigFloatInput("Floating Button Size (0.1 - 5)", "ui_icon.scale", 0.5f, 0.1f, 5.f, [](float val) {
            OverlayButton::get()->setSizeScale(val);
        });
        tab->addConfigFloatInput("Button Transparency (0 - 1)", "ui_icon.minOpacity", 0.2f, 0.f, 1.f, [](float val) {
            OverlayButton::get()->setMinOpacity(val);
        });
        
        tab->addSeparator();
        tab->prepareNewRow();

        tab->addConfigToggle("Hide icon on game", "ui_icon.hide_on_game", true);
        tab->addConfigToggle("Hide icon on editor", "ui_icon.hide_on_editor", false);

        tab->addSeparator();
        tab->prepareNewRow();

        tab->addConfigToggle("Notify about updates", "ui.notify_updates", true, [](bool enabled) {
            if (!enabled) {
                GDH::MaterialLayer(FLAlertLayer::create("Important", "Even if you turn off update popup, please keep GDH updated\n\nIt's still in beta, so updating helps avoid bugs and issues from older versions :(", "OK"))->show();
            }
        });

        tab->addSeparator();

        tab->addConfigButton("Disable cheating hacks", []() {
            GDH::MaterialLayer(
                geode::createQuickPopup("Confimation", "Are you sure you want to turn off cheating hacks?",
                "No", "Yes",
                [](auto, bool btn2) {
                    if (btn2) {
                        GDH::Gui::get().disableCheats();
                        if (HacksLayer::isOpened()) {
                            HacksLayer::get()->onClose(nullptr);
                        }
                    }
                })
            );
        });

        tab->addConfigButton("Show Cheat Hack List", []() {
            auto& activeCheats = GDH::Gui::get().getActiveCheats();
            bool replay_engine = (GDH::ReplayEngine::get().mode != state::disable);
            
            std::string text = "";
            int index = 0;

            for (const auto& hackID : activeCheats) {
                if (!text.empty()) text += ", ";
                
                std::string color = (index % 2 == 0) ? "<cr>" : "<cy>";
                text += color + hackID + "</c>";
                index++;
            }

            if (replay_engine) {
                if (!text.empty()) text += ", ";
                
                std::string color = (index % 2 == 0) ? "<cr>" : "<cy>";
                text += color + "Replay Engine</c>";
            }

            if (text.empty())
                text = "<cg>No cheat hacks enabled</c>";

            GDH::MaterialLayer(FLAlertLayer::create("Cheat Hack Lists", text.c_str(), "OK"))->show();
        });
    });
}
