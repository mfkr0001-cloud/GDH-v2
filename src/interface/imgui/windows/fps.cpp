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

    window.setCustomWindowImGui([&config, &gui, &layout] {
        static constexpr int physicsRates[] = {30, 60, 90, 120, 240, 480};
        float currentRate = config.get<float>("invisible.tps::value", 240.f);

        ImGui::TextUnformatted("Physics update rate");
        ImGui::TextDisabled("Select how many physics updates run per second.");
        ImGui::Spacing();

        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        const float buttonWidth = (ImGui::GetContentRegionAvail().x - spacing * 2.f) / 3.f;
        for (int i = 0; i < 6; ++i) {
            const int rate = physicsRates[i];
            const bool selected = (static_cast<int>(currentRate + 0.5f) == rate);
            if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            char label[24];
            snprintf(label, sizeof(label), "%d TPS", rate);
            if (ImGui::Button(label, ImVec2(buttonWidth, layout.multipleScale(48.f)))) {
                config.set<float>("invisible.tps::value", static_cast<float>(rate));
                if (config.get<bool>("invisible.lock_delta::sync_tps", true))
                    config.set<float>("invisible.lock_delta::value", static_cast<float>(rate));

                auto& tpsHack = gui.getWindow("Invisible").findHackByName("TPS");
                if (tpsHack.getEnabled()) {
                    tpsHack.disable();
                    tpsHack.enable();
                }
            }
            if (selected) ImGui::PopStyleColor();
            if ((i % 3) != 2) ImGui::SameLine();
        }

        ImGui::Spacing();
        ImGuiH::SpaceSeparator();
        ImGui::Text("Selected rate: %.0f physics updates/sec", config.get<float>("invisible.tps::value", 240.f));
        ImGui::Spacing();
        ImGuiWidgetConfig::HackCheckbox("Enable physics rate override", "invisible.tps", false);
        ImGui::TextWrapped("This changes the existing TPS physics hook, not the screen refresh/FPS limit. Lower rates may change collisions, timing, or level behavior. 480 TPS may be unstable or unsupported by some hooks.");
    });
}
#endif
