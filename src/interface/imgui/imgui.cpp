diff --git a/src/interface/imgui/imgui.cpp b/src/interface/imgui/imgui.cpp
--- a/src/interface/imgui/imgui.cpp
+++ b/src/interface/imgui/imgui.cpp
@@ -26,6 +26,10 @@
 static bool g_show = false;
 #ifdef GEODE_IS_ANDROID
-// Scale for Android: can be adjusted from the persistent floating control.
+// Android launcher state persists independently from the main menu.
 static float g_androidUiScale = 1.0f;
-static bool g_androidControlOpen = false;
+static bool g_androidLauncherInitialized = false;
+static ImVec2 g_androidLauncherPos = ImVec2(18.0f, 90.0f);
+static bool g_androidStyleScaleInitialized = false;
+static float g_androidAppliedScale = -1.0f;
+static ImGuiStyle g_androidBaseStyle;
 #endif
@@ -85,14 +89,14 @@
 void ToggleUI()
 {
-    if (g_isAnimating)
-        return;
-
-    g_isFadingIn = !g_show;
+    // Allow a tap during an animation to reverse the transition.
+    const bool shouldOpen = g_isAnimating
+        ? !g_isFadingIn
+        : !g_show;
+    g_isFadingIn = shouldOpen;
     g_isAnimating = true;
     g_animTime = 0.0f;

-    if (g_isFadingIn) {
+    if (shouldOpen && !g_show) {
         g_show = true;
         onOpen();
     }
@@ -412,6 +416,18 @@
     ImGuiIO &io = ImGui::GetIO();

     animateAlpha();
+#ifdef GEODE_IS_ANDROID
+    // Scale from a stable baseline only when the requested scale changes.
+    // This prevents cumulative growth caused by scaling an already-scaled style.
+    if (!g_androidStyleScaleInitialized) {
+        g_androidBaseStyle = ImGui::GetStyle();
+        g_androidStyleScaleInitialized = true;
+    }
+    const float requestedScale = std::clamp(g_androidUiScale, 0.60f, 3.00f);
+    if (std::abs(requestedScale - g_androidAppliedScale) > 0.001f) {
+        ImGui::GetStyle() = g_androidBaseStyle;
+        ImGui::GetStyle().ScaleAllSizes(requestedScale);
+        io.FontGlobalScale = requestedScale;
+        g_androidAppliedScale = requestedScale;
+    }
+#endif
 #ifdef GEODE_IS_DESKTOP
@@ -441,18 +457,22 @@
 #ifdef GEODE_IS_ANDROID
-    // Persistent, draggable launcher: it is drawn even while the main menu is closed.
-    // Move it by dragging the title bar. The scale slider is intentionally independent
-    // of the launcher visibility so the menu can always be reopened.
-    ImGui::SetNextWindowPos(ImVec2(18.0f, 90.0f), ImGuiCond_FirstUseEver);
-    ImGui::SetNextWindowSize(ImVec2(190.0f, 112.0f), ImGuiCond_FirstUseEver);
+    // Always draw the launcher before `if (!g_show) return`.
+    // Store the moved position so the title bar remains a usable drag handle.
+    ImGui::SetNextWindowPos(
+        g_androidLauncherPos,
+        g_androidLauncherInitialized ? ImGuiCond_Always : ImGuiCond_FirstUseEver
+    );
+    ImGui::SetNextWindowSize(ImVec2(205.0f, 126.0f), ImGuiCond_FirstUseEver);
     ImGui::SetNextWindowBgAlpha(0.88f);
     if (ImGui::Begin("GDH launcher", nullptr,
-        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
+        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
+        g_androidLauncherPos = ImGui::GetWindowPos();
+        g_androidLauncherInitialized = true;
         if (ImGui::Button(g_show ? "Close GDH" : "Open GDH", ImVec2(150.0f, 48.0f))) {
             ToggleUI();
         }
         ImGui::SetNextItemWidth(150.0f);
-        ImGui::SliderFloat("Menu scale", &g_androidUiScale, 0.75f, 2.5f, "%.2fx");
-        io.FontGlobalScale = g_androidUiScale;
+        ImGui::SliderFloat("Menu scale", &g_androidUiScale, 0.60f, 3.00f, "%.2fx");
     }
     ImGui::End();
 #endif
     if (!g_show) return;
