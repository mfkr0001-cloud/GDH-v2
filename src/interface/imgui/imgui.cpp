#if defined(GEODE_IS_DESKTOP) || defined(GEODE_IS_ANDROID)
#include "imgui.h"
#include <Geode/Geode.hpp>
#include <imgui-cocos.hpp>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include "layout.hpp"
#ifdef GEODE_IS_ANDROID
#include "../cocos/overlay_button.hpp"
#endif

#include "widgetH.hpp"
#include "widget_helper.hpp"
#include "font.hpp"

#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/CCEGLView.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <unordered_map>

using namespace geode::prelude;

#include "../../core/gui.hpp"
#include "../../core/config.hpp"
#include "../../core/utils.hpp"
#include "../../core/labels.hpp"
#include "../../core/keybinds.hpp"
#include "../../core/replayEngine.hpp"
#include "../../core/recorder/recorder.hpp"

#include "windows/replayEngine.hpp"

static bool g_show = false;
#ifdef GEODE_IS_ANDROID
// Scale for Android: can be adjusted from the persistent floating control.
// Phone-friendly defaults: compact controls and panels to reduce clipping on Android.
static float g_androidUiScale = 0.72f;
static float g_androidButtonScale = 0.78f;
static float g_androidPanelScale = 0.68f;
#endif

static bool g_showThemeEditor = false;
static bool g_reopenAfterTheme = false;
static bool g_showUpdatePopup = false;

static bool g_resetLayoutCalled = false;
static bool g_hardRecalculation = false;
static bool g_reopenAfterRecalc = false;

static bool g_isAnimating = false;
static bool g_isFadingIn = false;
static float g_animTime = 0.0f;
static float g_animStartAlpha = 1.0f;

static bool g_inited = false;
static std::string g_search_text = "";

static std::vector<std::vector<std::string>> g_layout = {
    {"Core", "Bypass", "Variables"},
    {"Cosmetic"},
    {"Level"},
    {"Creator", "Labels"},
    {"Replay Engine", "Keybinds"},
    {"Framerate"},
    {"Shortcuts"},
    {"GDH Settings"},
    {"Menu Size"}
};

static std::vector<GDH::Layout::WindowInfo> g_fixedWindowSizes = {
    {"Cosmetic", 0.f, 1100.f},
    {"Level", 0.f, 1100.f},
    // {"Labels", 240.f, 380.f},
    {"Replay Engine", 300.f, 225.f},
    {"Keybinds", 0.f, 180.f},
    {"Level", 0.f, 1080.f},
    {"Shortcuts", 260.f, 0.f},
    {"GDH Settings", 225.f, 0.f},
    {"Menu Size", 390.f, 0.f},
};

void onOpen() {
    GDH::Utils::updateCursorState(g_show);

    auto& config = Config::get();
    if (config.get<bool>("ui.need_update", false) && config.get<bool>("ui.notify_updates", true)) {
        g_showUpdatePopup = true;
        config.set<bool>("ui.need_update", false);
    }
}

void onClose() {
    GDH::Utils::updateCursorState(g_show);
    Config::get().save(getFileDataPath());
    GDH::Labels::Manager::get().save();
    GDH::Keybinds::get().save();
    ImGuiH::SaveTheme();
    g_search_text = "";
}

void ToggleUI()
{
    // Reverse an in-progress transition instead of ignoring a repeated tap.
    ImGuiStyle& style = ImGui::GetStyle();
    const bool wasAnimating = g_isAnimating;
    const bool nextFadeIn = wasAnimating ? !g_isFadingIn : !g_show;

    g_animStartAlpha = style.Alpha;
    g_isFadingIn = nextFadeIn;
    g_isAnimating = true;
    g_animTime = 0.0f;

    if (g_isFadingIn) {
        const bool wasClosed = !g_show;
        g_show = true;
        if (wasClosed || wasAnimating) onOpen();
    }
}

void animateAlpha()
{
    if (!g_isAnimating)
        return;

    ImGuiStyle& style = ImGui::GetStyle();
    float deltaTime = std::min(ImGui::GetIO().DeltaTime, 0.033f);

    float duration = std::max(0.001f, Config::get().get<int>("gui::anim_durr", 100) / 1000.0f);
    g_animTime += deltaTime;

    float t = std::clamp(g_animTime / duration, 0.0f, 1.0f);
    const float targetAlpha = g_isFadingIn ? 1.0f : 0.0f;
    if (t >= 1.0f)
    {
        style.Alpha = targetAlpha;
        g_isAnimating = false;

        if (!g_isFadingIn)
        {
            g_show = false;
            onClose();

            if (g_reopenAfterRecalc || g_reopenAfterTheme) {
                if (g_reopenAfterRecalc) {
                    g_reopenAfterRecalc = false;
                    g_hardRecalculation = true;
                }
                if (g_reopenAfterTheme) {
                    g_reopenAfterTheme = false;
                    g_showThemeEditor = !g_showThemeEditor;
                }
                ToggleUI();
            }
        }

        return;
    }

    style.Alpha = ImLerp(g_animStartAlpha, targetAlpha, t);
}

void PushAnimateFoundColor(const std::string& hackName) {
    static std::unordered_map<std::string, float> anim;

    std::string search_name = hackName;
    std::string search_item = g_search_text;

    std::transform(search_item.begin(), search_item.end(), search_item.begin(), ::tolower);
    std::transform(search_name.begin(), search_name.end(), search_name.begin(), ::tolower);

    bool founded = search_item.empty() ? true : (search_name.find(search_item) != std::string::npos);

    float& t = anim[hackName];
    
    float dt = std::min(ImGui::GetIO().DeltaTime, 0.033f);
    float speed = 16.0f;
    float target = founded ? 1.0f : 0.20f;

    float factor = std::clamp(dt * speed, 0.f, 1.f);
    t = ImLerp(t, target, factor);

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * t);
}

void SettingsRender() {
    auto& layoutManager = GDH::Layout::Manager::get();
    auto& config = Config::get();

    std::string windowName = "GDH Settings";
    layoutManager.applyWindowTransform(windowName);

    if (g_showThemeEditor) {
        ImGui::SetNextWindowPos({layoutManager.multipleScale(10.f), layoutManager.multipleScale(10.f)});
    }

    ImGui::Begin(windowName.c_str());

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputTextWithHint("##Search", "Search:", &g_search_text);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    
    int duration = config.get<int>("gui::anim_durr", 100);
    if (ImGuiH::DragInt("##gui_anim_durr", &duration, 50.f, 0, 500, "Duration Anim: %dms")) {
        config.set<int>("gui::anim_durr", duration);
    }

    bool horizontal_center = config.get<bool>("gui::horizontal_center", false);
    if (ImGuiH::Checkbox("Horizontal Center", &horizontal_center)) {
        config.set<bool>("gui::horizontal_center", horizontal_center);
        g_reopenAfterRecalc = true;
        ToggleUI();
    }

    bool notify_updates = config.get<bool>("ui.notify_updates", true);
    if (ImGuiH::Checkbox("Notify about updates", &notify_updates)) {
        config.set<bool>("ui.notify_updates", notify_updates);
    }

    if (ImGuiH::Button("Show Cheat Hack List", {ImGui::GetContentRegionAvail().x, 0})) {
        auto& activeCheats = GDH::Gui::get().getActiveCheats();
        bool replay_engine = (GDH::ReplayEngine::get().mode != state::disable);
        
        std::string text = "";
        for (const auto& hackID : activeCheats)
            text += hackID + "\n";

        if (replay_engine)
            text += "Replay Engine\n";

        if (text.empty())
            text += "No cheat hacks enabled";

        ImGuiH::AddPopup(text);
    }

    if (ImGuiH::Button("Disable cheating hacks", {ImGui::GetContentRegionAvail().x, 0})) {
        ImGui::OpenPopup("Disable cheating hacks##Confirm");
    }

    if (ImGui::BeginPopupModal("Disable cheating hacks##Confirm", 0, ImGuiWindowFlags_AlwaysAutoResize)) {
        auto& layout = GDH::Layout::Manager::get();
        auto glow_in = ImGui::ColorConvertFloat4ToU32(ImGuiH::colorTable[ImGuiH::Glow_Popup_Warning_In]);
        auto glow_out = ImGui::ColorConvertFloat4ToU32(ImGuiH::colorTable[ImGuiH::Glow_Popup_Warning_Out]);

        ImGuiH::GlowWindow(glow_in, glow_out, layout.multipleScale(500.f));
        ImGui::Text("Are you sure you want to turn off cheating hacks?");

        if (ImGuiH::Button("Cancel", {ImGui::GetContentRegionAvail().x, 0})) {
            ImGui::CloseCurrentPopup();
        }

        if (ImGuiH::Button("OK", {ImGui::GetContentRegionAvail().x, 0})) {
            GDH::Gui::get().disableCheats();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGuiH::SpaceSeparator();

    if (ImGuiH::Button("Refresh Layout", {ImGui::GetContentRegionAvail().x, 0})) {
        g_resetLayoutCalled = true;
    }

    if (ImGuiH::Button(g_showThemeEditor ? "Disable Theme Editor" : "Theme Editor", {ImGui::GetContentRegionAvail().x, 0})) {
        g_reopenAfterTheme = true;
        if (g_showThemeEditor) g_reopenAfterRecalc = true;
        ToggleUI();
    }

    if (layoutManager.isCollecting()) {
        auto size = ImGui::GetWindowSize();
        layoutManager.addWindowInfo(windowName, size.x, size.y);
    }

    ImGui::End();
}

#ifdef GEODE_IS_ANDROID
static void MenuSizeRender() {
    auto& layoutManager = GDH::Layout::Manager::get();
    layoutManager.applyWindowTransform("Menu Size");
    const bool visible = ImGuiH::BeginSmoothScroll("Menu Size");
    if (visible && !ImGui::IsWindowCollapsed()) {
        ImGui::TextWrapped("Adjust the PC-style GDH menu and its controls.");
        ImGui::Separator();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::SliderFloat("Menu scale", &g_androidUiScale, 0.45f, 2.00f, "%.2fx");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::SliderFloat("Panel size", &g_androidPanelScale, 0.35f, 1.75f, "%.2fx")) {
            layoutManager.setPanelScale(g_androidPanelScale);
        }
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::SliderFloat("Button spacing", &g_androidButtonScale, 0.45f, 1.50f, "%.2fx");

        ImGui::Separator();
        ImGui::TextUnformatted("Floating GDH button");
        auto& uiConfig = Config::get();
        float floatingButtonSize = uiConfig.get<float>("ui_icon.scale", 0.5f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::InputFloat("Button size (0.1–5)", &floatingButtonSize, 0.1f, 0.5f, "%.2f")) {
            floatingButtonSize = std::clamp(floatingButtonSize, 0.1f, 5.0f);
            uiConfig.set<float>("ui_icon.scale", floatingButtonSize);
            OverlayButton::get()->setSizeScale(floatingButtonSize);
        }
        float floatingButtonOpacity = uiConfig.get<float>("ui_icon.minOpacity", 0.2f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::SliderFloat("Button transparency (0 = invisible)", &floatingButtonOpacity, 0.0f, 1.0f, "%.2f")) {
            floatingButtonOpacity = std::clamp(floatingButtonOpacity, 0.0f, 1.0f);
            uiConfig.set<float>("ui_icon.minOpacity", floatingButtonOpacity);
            OverlayButton::get()->setMinOpacity(floatingButtonOpacity);
        }
        ImGui::TextWrapped("Menu scale changes overall UI/text size. Panel size changes menu panels. Button spacing changes padding and spacing.");
        if (ImGuiH::Button("Reset sizes", {ImGui::GetContentRegionAvail().x, 0.0f})) {
            g_androidUiScale = 0.72f;
            g_androidPanelScale = 0.68f;
            g_androidButtonScale = 0.78f;
            layoutManager.setPanelScale(g_androidPanelScale);
            uiConfig.set<float>("ui_icon.scale", 0.5f);
            uiConfig.set<float>("ui_icon.minOpacity", 0.2f);
            OverlayButton::get()->setSizeScale(0.5f);
            OverlayButton::get()->setMinOpacity(0.2f);
        }
    }
    if (layoutManager.isCollecting()) {
        const auto size = ImGui::GetWindowSize();
        layoutManager.addWindowInfo("Menu Size", size.x, size.y);
    }
    ImGuiH::EndSmoothScroll();
}
#endif

void KeybindsRender() {
    auto& layoutManager = GDH::Layout::Manager::get();
    auto& config = Config::get();
    auto& kb = GDH::Keybinds::get();

    std::string windowName = "Keybinds";
    layoutManager.applyWindowTransform(windowName);

    ImGui::Begin(windowName.c_str());

    ImGuiH::Checkbox("Keybinds Mode", &kb.m_isKeybindsMode);
    
    ImGuiWidgetConfig::Checkbox("Shortcuts", "shortcuts::enable", false);
    ImGui::SameLine();
    if (ImGuiH::ArrowButton("##Shortcuts Keys", ImGuiDir_Right)) {
        ImGui::OpenPopup("Shortcuts Keys");
    }

    if (ImGui::BeginPopup("Shortcuts Keys")) {
        ImGuiWidgetConfig::DrawCustomKeybindButton("shortcut::options", "Options");
        ImGuiWidgetConfig::DrawCustomKeybindButton("shortcut::graphics", "Graphics Settings");
        ImGuiWidgetConfig::DrawCustomKeybindButton("shortcut::quests", "Quests Page");
        ImGuiWidgetConfig::DrawCustomKeybindButton("shortcut::mobile_ui", "Open Mobile UI");
        ImGuiWidgetConfig::DrawCustomKeybindButton("shortcut::reset_level", "Reset Level");
        ImGuiWidgetConfig::DrawCustomKeybindButton("shortcut::practice_mode", "Practice Mode");
        ImGuiWidgetConfig::DrawCustomKeybindButton("shortcut::reset_volume", "Reset Volume");
        ImGuiWidgetConfig::DrawCustomKeybindButton("shortcut::uncomplete_level", "Uncomplete Level");
        ImGuiWidgetConfig::DrawCustomKeybindButton("shortcut::resources", "Resources");
        ImGuiWidgetConfig::DrawCustomKeybindButton("shortcut::appdata", "AppData");
        ImGuiWidgetConfig::DrawCustomKeybindButton("shortcut::gdh_appdata", "GDH AppData");
        ImGuiWidgetConfig::DrawCustomKeybindButton("shortcut::crash_game", "Crash Game");
        ImGuiWidgetConfig::DrawCustomKeybindButton("shortcut::restart_game", "Restart Game");
        ImGuiWidgetConfig::DrawCustomKeybindButton("shortcut::disable_cheat_hacks", "Disable cheat hacks");
        ImGui::EndPopup();
    }

    ImGuiWidgetConfig::Checkbox("Framerate", "framerate::enable", false);
    ImGui::SameLine();
    if (ImGuiH::ArrowButton("##Framerate Keys", ImGuiDir_Right)) {
        ImGui::OpenPopup("Framerate Keys");
    }

    if (ImGui::BeginPopup("Framerate Keys")) {
        ImGui::Text("Speedhack:");
        ImGuiWidgetConfig::DrawCustomKeybindButton("keybinds::speedhack", "Speedhack");
        ImGuiWidgetConfig::DrawCustomKeybindButton("keybinds::speedhack_inc", "Speedhack +0.05x");
        ImGuiWidgetConfig::DrawCustomKeybindButton("keybinds::speedhack_dec", "Speedhack -0.05x");
        ImGui::EndPopup();
    }

    ImGuiWidgetConfig::Checkbox("Replay Engine", "keybinds::engine", false);
    ImGui::SameLine();
    if (ImGuiH::ArrowButton("##Engine Keys", ImGuiDir_Right)) {
        ImGui::OpenPopup("Engine Keys");
    }

    if (ImGui::BeginPopup("Engine Keys")) {
        ImGuiWidgetConfig::DrawCustomKeybindButton("re::record", "Disable/Record");
        ImGuiWidgetConfig::DrawCustomKeybindButton("re::playback", "Disable/Playback");
        ImGuiWidgetConfig::DrawCustomKeybindButton("re::save_macro_by_name", "Save Macro by Current Level Name");
        ImGuiWidgetConfig::DrawCustomKeybindButton("re::load_macro_by_name", "Load Macro by Current Level Name");
        ImGui::EndPopup();
    }

    ImGuiWidgetConfig::DrawCustomKeybindButton("gui::toggle_ui", "Toggle UI");

    if (layoutManager.isCollecting()) {
        auto size = ImGui::GetWindowSize();
        layoutManager.addWindowInfo(windowName, size.x, size.y);
    }

    ImGui::End();
}

void RenderRecordingBadge() {
    ImGuiIO& io = ImGui::GetIO();
    auto& layout = GDH::Layout::Manager::get();
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    ImFont* font = ImGui::GetFont();
    
    float fontSize = layout.multipleScale(32.0f); 

    const char* recText = "Recording";
    ImVec2 recSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, recText);

    float dotRadius = layout.multipleScale(8.0f);
    float dotPadding = layout.multipleScale(10.0f);
    float sidePadding = layout.multipleScale(12.0f);
    
    float totalWidth = sidePadding + dotRadius * 2.0f + dotPadding + recSize.x + sidePadding;
    float pillHeight = recSize.y + layout.multipleScale(10.0f);
    float totalHeight = pillHeight + layout.multipleScale(16.0f);

    float screenPadding = layout.multipleScale(10.0f);
    ImVec2 badgePosMin = ImVec2(screenPadding, io.DisplaySize.y - totalHeight - screenPadding);
    ImVec2 badgePosMax = ImVec2(screenPadding + totalWidth, io.DisplaySize.y - screenPadding);
    
    drawList->AddRectFilled(badgePosMin, badgePosMax, IM_COL32(0, 0, 0, 125), 999.0f);
    
    ImVec2 dotCenter = ImVec2(
        badgePosMin.x + sidePadding + dotRadius,
        badgePosMin.y + (totalHeight / 2.0f)
    );
    drawList->AddCircleFilled(dotCenter, dotRadius, IM_COL32(255, 0, 0, 255));

    ImVec2 recTextPos = ImVec2(dotCenter.x + dotRadius + dotPadding, badgePosMin.y + layout.multipleScale(13.0f));
    drawList->AddText(font, fontSize, recTextPos, IM_COL32(255, 255, 255, 255), recText);
}

void RenderVersionBadge() {
    ImGuiIO& io = ImGui::GetIO();
    auto& layout = GDH::Layout::Manager::get();
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    ImFont* font = ImGui::GetFont();
    
    float fontSize = layout.multipleScale(32); 
    std::string versionText = geode::Mod::get()->getVersion().toVString();

    ImVec2 labelSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, "GDH");
    ImVec2 versionSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, versionText.c_str());

    float pillWidth = layout.multipleScale(24.0f) + labelSize.x;
    float pillHeight = labelSize.y + layout.multipleScale(10.0f);
    float sidePadding = layout.multipleScale(12.0f); 
    float totalWidth = sidePadding + pillWidth + layout.multipleScale(12.0f) + versionSize.x + sidePadding;
    float totalHeight = pillHeight + layout.multipleScale(16.0f);

    float screenPadding = layout.multipleScale(10.0f);
    ImVec2 badgePosMin = ImVec2(io.DisplaySize.x - totalWidth - screenPadding, io.DisplaySize.y - totalHeight - screenPadding);
    ImVec2 badgePosMax = ImVec2(io.DisplaySize.x - screenPadding, io.DisplaySize.y - screenPadding);
    
    ImVec2 pillPosMin = ImVec2(badgePosMin.x + sidePadding, badgePosMin.y + layout.multipleScale(8.0f));
    ImVec2 pillPosMax = ImVec2(pillPosMin.x + pillWidth, pillPosMin.y + pillHeight);
    drawList->AddRectFilled(badgePosMin, badgePosMax, ImGui::GetColorU32(IM_COL32(0, 0, 0, 125)), 999.f);
    drawList->AddRectFilled(pillPosMin, pillPosMax, ImGui::GetColorU32(IM_COL32(213, 196, 255, 225)), 999.f);
    
    ImVec2 labelTextPos = ImVec2(pillPosMin.x + layout.multipleScale(12.0f), pillPosMin.y + layout.multipleScale(5.0f));
    ImVec2 versionTextPos = ImVec2(pillPosMax.x + layout.multipleScale(12.0f), badgePosMin.y + layout.multipleScale(13.0f));
    drawList->AddText(font, fontSize, labelTextPos, ImGui::GetColorU32(IM_COL32(33, 33, 78, 225)), "GDH");
    drawList->AddText(font, fontSize, versionTextPos, ImGui::GetColorU32(IM_COL32(213, 196, 255, 225)), versionText.c_str());
}

void notifyUpdate() {
    if (g_showUpdatePopup) {
        ImGui::OpenPopup("Update Available##Popup");
        g_showUpdatePopup = false;
    }

    if (ImGui::BeginPopupModal("Update Available##Popup", 0, ImGuiWindowFlags_AlwaysAutoResize)) {
        auto& layout = GDH::Layout::Manager::get();
        auto& config = Config::get();
        bool dont_show_again = !config.get<bool>("ui.notify_updates", true);
        
        auto glow_in = ImGui::ColorConvertFloat4ToU32(ImGuiH::colorTable[ImGuiH::Glow_Popup_Update_In]);
        auto glow_out = ImGui::ColorConvertFloat4ToU32(ImGuiH::colorTable[ImGuiH::Glow_Popup_Update_Out]);
        ImGuiH::GlowWindow(glow_in, glow_out, layout.multipleScale(500.f));
        
        float oldScale = ImGui::GetFont()->Scale;
        ImGui::GetFont()->Scale = layout.multipleScale(32.0f) / ImGui::GetFontSize();
        ImGui::PushFont(ImGui::GetFont());

        ImGui::Text("A new update is available!\nPlease open the Geode menu and download the latest GDH update to get new features and bug fixes");
        if (dont_show_again) {
            ImGui::TextColored(ImColor(255, 128, 128).Value, "\nEven if you turn off this popup, please keep GDH updated\nIt's still in beta, so updating helps avoid bugs and issues from older versions :(");
        }

        ImGui::GetFont()->Scale = oldScale;
        ImGui::PopFont();
        
        if (ImGuiH::Checkbox("Don't show again", &dont_show_again)) {
            config.set<bool>("ui.notify_updates", !dont_show_again);
        }

        if (ImGuiH::Button("OK", {ImGui::GetContentRegionAvail().x, 0})) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

#ifdef GEODE_IS_ANDROID
// Single-tap the title text to collapse/expand a panel. The standard ImGui
// collapse arrow remains available; taps on the arrow are deliberately ignored
// here so the state is not toggled twice.
static bool HandleAndroidTitleTap() {
    static std::unordered_map<ImGuiID, ImVec2> pressPositions;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window) return false;

    const ImGuiID id = window->ID;
    const ImVec2 pos = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();
    const ImGuiStyle& style = ImGui::GetStyle();
    const float titleHeight = ImGui::GetFrameHeight();
    const float arrowWidth = titleHeight + style.FramePadding.x;
    const ImVec2 mouse = ImGui::GetIO().MousePos;
    const bool inTitle = mouse.x >= pos.x && mouse.x <= pos.x + size.x &&
                         mouse.y >= pos.y && mouse.y <= pos.y + titleHeight;
    const bool onArrow = mouse.x >= pos.x + size.x - arrowWidth;

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && inTitle && !onArrow) {
        pressPositions[id] = mouse;
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        auto it = pressPositions.find(id);
        if (it != pressPositions.end()) {
            const ImVec2 start = it->second;
            pressPositions.erase(it);
            const float dx = mouse.x - start.x;
            const float dy = mouse.y - start.y;
            const bool smallMove = (dx * dx + dy * dy) <= 36.0f;
            const bool stillOnTitle = mouse.x >= pos.x && mouse.x <= pos.x + size.x &&
                                      mouse.y >= pos.y && mouse.y <= pos.y + titleHeight;
            if (smallMove && stillOnTitle) {
                ImGui::SetWindowCollapsed(!ImGui::IsWindowCollapsed());
                return true;
            }
        }
    }
    return false;
}
#endif

void RenderMain() {
    auto& gui = GDH::Gui::get();
    auto& windows = gui.getWindows();
    auto& config = Config::get();
    auto& layoutManager = GDH::Layout::Manager::get();
    auto& kb = GDH::Keybinds::get();
    ImGuiIO &io = ImGui::GetIO();

#ifdef GEODE_IS_ANDROID
    // Keep a pristine style baseline; never scale sizes that were scaled last frame.
    static ImGuiStyle androidBaseStyle;
    static bool androidBaseStyleCaptured = false;
    if (!androidBaseStyleCaptured) {
        androidBaseStyle = ImGui::GetStyle();
        androidBaseStyleCaptured = true;
    }
    ImGui::GetStyle() = androidBaseStyle;
    ImGui::GetStyle().ScaleAllSizes(g_androidUiScale);
    // Button size is independent of the overall menu/font scale.
    ImGui::GetStyle().FramePadding.x *= g_androidButtonScale;
    ImGui::GetStyle().FramePadding.y *= g_androidButtonScale;
    ImGui::GetStyle().ItemSpacing.x *= g_androidButtonScale;
    ImGui::GetStyle().ItemSpacing.y *= g_androidButtonScale;
    ImGui::GetStyle().ItemInnerSpacing.x *= g_androidButtonScale;
    ImGui::GetStyle().ItemInnerSpacing.y *= g_androidButtonScale;
    io.FontGlobalScale = g_androidUiScale;
#endif
    
    animateAlpha();

#ifdef GEODE_IS_DESKTOP
    if (config.get<bool>("re::show_editor", false)) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.f);

        ImVec2 window_size = ImVec2(layoutManager.multipleScale(700.0f), layoutManager.multipleScale(500.0f));
        ImVec2 display_size = ImGui::GetIO().DisplaySize;

        ImVec2 window_pos = ImVec2(
            display_size.x - window_size.x,
            display_size.y - window_size.y
        );

        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Once);
        ImGui::SetNextWindowSize(window_size, ImGuiCond_Once);

        if (ImGui::Begin("Macro Editor##Window")) {
            GDH::UI::drawMacroEditor();
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }
#endif

    ImGuiH::RenderPopups();
    if (GDH::Recorder::get().isRecording()) RenderRecordingBadge();


    if (!g_show) return;

    static bool lastRightPressed = false;
    bool isRightPressed = GDH::Keybinds::get().isMouseButtonDown(geode::MouseInputData::Button::Right);

    if (isRightPressed != lastRightPressed) {
        io.AddMouseButtonEvent(1, isRightPressed);
        lastRightPressed = isRightPressed;
    }

    RenderVersionBadge();
    SettingsRender();
#ifdef GEODE_IS_ANDROID
    MenuSizeRender();
#endif
    notifyUpdate();

    if (g_showThemeEditor) {
        ImGuiH::DrawColorEditor();
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.f);
    }

    KeybindsRender();

#ifdef GEODE_IS_ANDROID
    // One draggable handle translates every GDH panel together.
    {
        const float offsetX = layoutManager.getLayoutOffsetX();
        const float offsetY = layoutManager.getLayoutOffsetY();
        ImGui::SetNextWindowPos(ImVec2(12.0f + offsetX, 12.0f + offsetY), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.88f);
        if (ImGui::Begin("Move all tabs", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
            ImGui::TextUnformatted("Hold and drag to move all panels");
            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) && ImGui::IsMouseDragging(0, 1.0f)) {
                const ImVec2 delta = io.MouseDelta;
                layoutManager.setLayoutOffset(offsetX + delta.x, offsetY + delta.y);
            }
        }
        ImGui::End();
    }
#endif

    for (auto& window : windows) {
        std::string windowName = window.getName();
        if (windowName == "Invisible" || windowName == "Settings" || windowName == "Search") continue;
        
        const bool isReplayEngineWindow = (windowName == "Replay Engine");
        // Keep Replay Engine on the layout manager's original position and size.
        // Do not force a larger Android-only window; that can cover the bot UI
        // and ignores the user's saved layout.
        layoutManager.applyWindowTransform(windowName);
        // Only Replay Engine keeps direct Begin for its custom playback controls.
        // Every other panel uses the touch content scroller so lower controls remain reachable.
        const bool beginVisible = isReplayEngineWindow
            ? ImGui::Begin(windowName.c_str())
            : ImGuiH::BeginSmoothScroll(windowName.c_str());
#ifdef GEODE_IS_ANDROID
        const bool titleTapToggled = HandleAndroidTitleTap();
        layoutManager.setWindowCollapsed(windowName, ImGui::IsWindowCollapsed());
#else
        const bool titleTapToggled = false;
#endif
        const bool drawWindowContents = beginVisible && !ImGui::IsWindowCollapsed() && !titleTapToggled;
        if (drawWindowContents) {
        if (window.avaibleCustomWindowImGui()) window.callCustomWindowImGui();

        for (auto& hack : window.getHacks()) {
            if (kb.m_isKeybindsMode) {
                ImGuiWidgetConfig::DrawKeybindButton(windowName, hack);
                continue;
            }

            std::string id = hack.getID();
            std::string hackName = hack.getName();

            bool state = config.get(id, false);

            PushAnimateFoundColor(hackName);
            if (hack.isCheating()) ImGui::PushStyleColor(ImGuiCol_Text, ImColor(255, 128, 128).Value);

            ImGui::BeginDisabled(hack.getDisabled());
            if (ImGuiH::Checkbox(hackName.c_str(), &state)) {
                hack.toggle();
            }
            ImGui::EndDisabled();

            if (hack.isCheating()) ImGui::PopStyleColor();

            ImGuiH::Tooltip(hack.getDesc().c_str(), !hack.getDesc().empty() && ImGui::IsItemHovered());

            if (hack.avaibleCustomWindowImGui()) {
                ImGui::SameLine();
                if (ImGuiH::ArrowButton(fmt::format("{} Settings", hackName).c_str(), ImGuiDir_Right)) {
                    ImGui::OpenPopup(fmt::format("{} Settings##Popup", hackName).c_str());
                }

                if (ImGui::BeginPopup(fmt::format("{} Settings##Popup", hackName).c_str(), 0)) {
                    hack.callCustomWindowImGui();
                    ImGui::EndPopup();
                }
            }
            ImGui::PopStyleVar();
        }

        if (layoutManager.isCollecting()) {
            auto size = ImGui::GetWindowSize();
            layoutManager.addWindowInfo(windowName, size.x, size.y);
        }
        }

        // BeginSmoothScroll maintains an internal stack even when it returns false;
        // its matching End is required for every window opened through that helper.
        ImGuiH::EndSmoothScroll();
    }

    if (g_showThemeEditor) ImGui::PopStyleVar();

    if (layoutManager.isCollecting()) layoutManager.finishCollecting();
    else if (layoutManager.isApplying()) layoutManager.finishApplying();
    
    static bool g_inited = false;
    
    if (g_inited) {
        static ImVec2 lastSize = ImVec2(0, 0);
        ImVec2 currentSize = io.DisplaySize;
    
        if (g_hardRecalculation || currentSize.x != lastSize.x || currentSize.y != lastSize.y) {
            g_hardRecalculation = false;
            layoutManager.startCollecting(); 
            lastSize = currentSize;
        }
    
        if (g_resetLayoutCalled) {
            g_resetLayoutCalled = false;
            layoutManager.startApplying();
        }
    }
    else {
        layoutManager.setLayout(g_layout);
        layoutManager.setFixedWindowSizeInfo(g_fixedWindowSizes);

        layoutManager.startCollecting();
        g_inited = true;
    }
}

class $modify(ImGuiInitMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        
		static bool inited = false;
        if (!inited) {
            GDH::Keybinds::get().addCallback("gui::toggle_ui", geode::Keybind(cocos2d::KEY_Tab, geode::KeyboardModifier::None),
                [](bool repeat) {
                    ToggleUI();
                }
            );

            auto mod = geode::Mod::get();
            
            #ifndef GEODE_IS_MACOS
            ImGuiCocos::get().setForceLegacy(mod->getSettingValue<bool>("legacy-render"));
            #else
            ImGuiCocos::get().setForceLegacy(true);
            #endif

            ImGuiCocos::get().setup([] {
                auto& config = Config::get();
                ImGuiIO &io = ImGui::GetIO();
                io.IniFilename = 0;

                // backuping and applying original theme
                ImGuiH::ProccessOriginalTheme(true);
                ImGuiH::ApplyNativeGuiColors();
                ImGuiH::ApplyStyle(1.f);
                
                // applying custom theme (if it exists)
                ImGuiH::LoadTheme();

                io.Fonts->AddFontFromMemoryCompressedTTF(roboto_font_data, roboto_font_size, 18.f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
            }).draw([] {
                RenderMain();
            });
            inited = true;
        }

		return true;
    }
};

#endif
