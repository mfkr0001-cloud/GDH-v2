#if defined(GEODE_IS_DESKTOP) || defined(GEODE_IS_ANDROID)
#include "imgui.h"
#include <Geode/Geode.hpp>
#include "../../../core/gui.hpp"
#include "../../../core/config.hpp"
#include "../widget_helper.hpp"
#include "../layout.hpp"
#include "../widgetH.hpp"
#include <algorithm>

$execute {
    auto& gui = GDH::Gui::get();
    auto& config = Config::get();
    auto& layout = GDH::Layout::Manager::get();
    auto& window = gui.getWindow("FPS");

    window.setCustomWindowImGui([&config, &layout] {
        ImGui::TextUnformatted("Heavy Ship");
        ImGui::TextWrapped("Adds inertia to the ship's vertical movement to make straight fly easier. It does not change your screen refresh rate.");
        ImGui::Spacing();

        float strength = config.get<float>("fps.heavy_ship::strength", 55.f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::SliderFloat("Ship heaviness", &strength, 0.f, 90.f, "%.0f%%")) {
            strength = std::clamp(strength, 0.f, 90.f);
            config.set<float>("fps.heavy_ship::strength", strength);
        }
        ImGui::TextDisabled("0%% = normal response; 90%% = strongest inertia");
        ImGui::Spacing();
        ImGuiH::SpaceSeparator();
        ImGuiWidgetConfig::HackCheckbox("Enable Heavy Ship", "fps.heavy_ship", false);
        ImGui::Spacing();
        ImGui::TextWrapped("Try 45-65%% first. If the ship feels too sluggish, lower the value.");
    });
}
#endif
