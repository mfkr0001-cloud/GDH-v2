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
    auto& window = gui.getWindow("FPS");

    window.setCustomWindowImGui([&config, &gui] {
        ImGui::TextUnformatted("Wave settings");
        ImGui::TextWrapped("Short taps can be missed or feel different at 60 Hz. This option keeps a very short wave tap active until a game update can process it.");
        ImGui::Spacing();

        ImGuiWidgetConfig::HackCheckbox("Enable Wave Tap Buffer", "fps.wave_response", false);

        float bufferMs = config.get<float>("fps.wave_response::buffer_ms", 8.f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::SliderFloat("Tap buffer", &bufferMs, 0.f, 20.f, "%.1f ms")) {
            bufferMs = std::clamp(bufferMs, 0.f, 20.f);
            config.set<float>("fps.wave_response::buffer_ms", bufferMs);
        }
        ImGui::TextDisabled("0 ms = off; larger values preserve shorter taps but may make release feel delayed.");

        if (ImGuiH::Button("11.1 ms preset (90 FPS frame time)", ImVec2(ImGui::GetContentRegionAvail().x, 0.f))) {
            config.set<float>("fps.wave_response::buffer_ms", 11.1f);
        }
        if (ImGuiH::Button("Reset to default (8 ms)", ImVec2(ImGui::GetContentRegionAvail().x, 0.f))) {
            config.set<float>("fps.wave_response::buffer_ms", 8.f);
        }

        ImGuiH::SpaceSeparator();
        ImGui::TextWrapped("This does not unlock 90 FPS or change the screen refresh rate. Test in practice mode first: a large buffer can make wave releases feel late.");
    });
}
#endif
