#if defined(GEODE_IS_DESKTOP) || defined(GEODE_IS_ANDROID)
#include "imgui.h"
#include <Geode/Geode.hpp>
#include "../../../core/gui.hpp"
#include "../../../core/config.hpp"
#include "../../../core/utils.hpp"
#include "../widget_helper.hpp"
#include "../layout.hpp"
#include "../widgetH.hpp"
#include <imgui-cocos.hpp>

$execute {
    auto& gui = GDH::Gui::get();
    auto& config = Config::get();
    auto& layout = GDH::Layout::Manager::get();
    auto& window = gui.getWindow("FPS");

    window.setCustomWindowImGui([&config, &gui, &layout] {
        ImGui::TextUnformatted("FPS limit");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - layout.multipleScale(52.f));
        if (ImGuiWidgetConfig::InputFloat("##FPS_CustomValue", "invisible.fps::value", 120.f)) {
            float value = config.get<float>("invisible.fps::value", 120.f);
            if (value < 1.f) {
                value = 1.f;
                config.set<float>("invisible.fps::value", value);
            }
            auto& hack = gui.getWindow("Invisible").findHackByName("FPS");
            if (hack.getEnabled()) {
                hack.disable();
                hack.enable();
            }
        }
        ImGui::SameLine();
        ImGuiWidgetConfig::HackCheckbox("##FPS_CustomEnabled", "invisible.fps", false);
        ImGui::TextDisabled("Enter a value, then use the checkbox to enable/disable FPS.");
    });
}
#endif
