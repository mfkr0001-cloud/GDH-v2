#if defined(GEODE_IS_DESKTOP) || defined(GEODE_IS_ANDROID)
#include "../imgui.h"
#include <Geode/Geode.hpp>
#include "../../core/gui.hpp"
#include "../../core/config.hpp"
#include "../widget_helper.hpp"
#include "../layout.hpp"

using namespace geode::prelude;

$execute {
    auto& gui = GDH::Gui::get();
    auto& config = Config::get();
    auto& layout = GDH::Layout::Manager::get();
    auto& window = gui.getWindow("FPS");

    window.setCustomWindowImGui([&config, &gui, &layout] {
        ImGui::TextWrapped("Set the visual frame-rate target independently of the existing Framerate panel.");
        ImGui::Spacing();

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - layout.multipleScale(52.f));
        if (ImGuiWidgetConfig::DragFloat(
            "##FPS_Tab_Value", "invisible.fps::value", 1.f, 1.f, 10000.f, 120.f, "Target: %.0f FPS"
        )) {
            auto& hack = gui.getWindow("Invisible").findHackByName("FPS");
            if (hack.getEnabled()) {
                hack.disable();
                hack.enable();
            }
        }
        ImGui::SameLine();
        ImGuiWidgetConfig::HackCheckbox("##FPS_Tab_Enabled", "invisible.fps", false);

        ImGui::Spacing();
        ImGui::TextWrapped("Enable the checkbox to activate the FPS hack. Actual frame rate depends on the display, device and game engine.");
    });
}
#endif
