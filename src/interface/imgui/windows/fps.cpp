#if defined(GEODE_IS_DESKTOP) || defined(GEODE_IS_ANDROID)
#include "imgui.h"
#include <Geode/Geode.hpp>
#include "../../../core/gui.hpp"
#include "../../../core/config.hpp"
#include "../widget_helper.hpp"
#include "../layout.hpp"
#include "../widgetH.hpp"

$execute {
    auto& gui = GDH::Gui::get();
    auto& config = Config::get();
    auto& layout = GDH::Layout::Manager::get();
    auto& window = gui.getWindow("FPS");

    window.setCustomWindowImGui([&config, &layout] {
        ImGui::TextUnformatted("Heavy Ship");
        ImGui::TextWrapped("Makes the ship respond more slowly to direction changes, helping with straight fly. It does not unlock FPS or change the screen refresh rate.");
        ImGui::Spacing();

        float strength = config.get<float>("fps.heavy_ship::strength", 55.f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::SliderFloat("Ship heaviness", &strength, 0.f, 90.f, "%.0f%%")) {
            strength = std::clamp(strength, 0.f, 90.f);
            config.set<float>("fps.heavy_ship::strength", strength);
        }
        ImGui::TextDisabled("0%% = normal ship, 90%% = very heavy response");
        ImGui::Spacing();
        ImGuiH::SpaceSeparator();
        ImGuiWidgetConfig::HackCheckbox("Enable Heavy Ship", "fps.heavy_ship", false);
        ImGui::Spacing();
        ImGui::TextWrapped("Only affects ship mode. Try 45–65%% first. If the ship feels too sluggish, lower the slider.");
    });
}
#endif
