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
        ImGui::TextUnformatted("Heavy Ship");
        ImGui::TextWrapped("Adds inertia to vertical ship movement. The value is an inertia factor, not a direct game gravity value; larger values make velocity changes more gradual.");
        ImGui::Spacing();
        ImGuiWidgetConfig::HackCheckbox("Enable Heavy Ship", "fps.heavy_ship", false);

        float strength = config.get<float>("fps.heavy_ship::strength", 4.f);
        if (!std::isfinite(strength) || strength < 0.f) strength = 4.f;

        // Slider for quick adjustment, plus a numeric text field for precise values.
        // The slider covers the common 0..90 range; the input field accepts values above 90 too.
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.58f);
        if (ImGui::SliderFloat("##ship_heaviness_slider", &strength, 0.f, 90.f, "Heaviness: %.2f")) {
            config.set<float>("fps.heavy_ship::strength", strength);
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-1.f);
        if (ImGui::InputFloat("##ship_heaviness_input", &strength, 1.f, 10.f, "%.3f")) {
            if (!std::isfinite(strength) || strength < 0.f) strength = 0.f;
            config.set<float>("fps.heavy_ship::strength", strength);
        }
        ImGui::TextDisabled("Type a value in the right field and press Enter. Values above 90 are allowed; changes are saved.");
        if (ImGuiH::Button("Reset ship heaviness", ImVec2(ImGui::GetContentRegionAvail().x, 0.f))) {
            config.set<float>("fps.heavy_ship::strength", 4.f);
        }

        ImGuiH::SpaceSeparator();
        ImGui::TextWrapped("Wave buffering does not unlock 90 FPS or change the screen refresh rate. Test both options in practice mode first; excessive ship inertia or wave buffering can affect control precision.");
    });
}
#endif
