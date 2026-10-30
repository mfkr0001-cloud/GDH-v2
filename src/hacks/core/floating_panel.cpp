#include <Geode/Geode.hpp>
#include "../../core/gui.hpp"
#include "../../core/config.hpp"
#include "../../interface/cocos/overlay_button.hpp"

GUI_HACK_CREATE("Core", "Floating Control Panel", "Shows the in-level Autoclicker and Pause Menu buttons", true);

$execute {
    auto& hack = GDH::Gui::get().getWindow("Core").findHackByName("Floating Control Panel");
    hack.setHandler([](bool enabled) {
        Config::get().set<bool>("ui_icon.floating_panel_enabled", enabled);
        auto* button = OverlayButton::get();
        if (button) button->setVisible(enabled);
    });
}
