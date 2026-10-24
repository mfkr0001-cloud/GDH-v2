--- a/src/interface/imgui/imgui.cpp
+++ b/src/interface/imgui/imgui.cpp
@@ -27,5 +27,7 @@
 #ifdef GEODE_IS_ANDROID
 // Scale for Android: can be adjusted from the persistent floating control.
 static float g_androidUiScale = 1.0f;
-static bool g_androidControlOpen = false;
+static bool g_androidStyleScaleInitialized = false;
+static float g_androidAppliedScale = -1.0f;
+static ImGuiStyle g_androidBaseStyle;
 #endif
@@ -414,3 +414,18 @@ void RenderMain() {
     ImGuiIO &io = ImGui::GetIO();
 
     animateAlpha();
+
+#ifdef GEODE_IS_ANDROID
+    // Apply a full UI scale from an immutable baseline. Never scale the already-scaled
+    // style again: doing so every frame causes sizes to grow without bound.
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
@@
 #ifdef GEODE_IS_ANDROID
-    // Persistent, draggable launcher: it is drawn even while the main menu is closed.
-    // Move it by dragging the title bar. The scale slider is intentionally independent
-    // of the launcher visibility so the menu can always be reopened.
+    // Persistent launcher: it is always drawn, even when the main menu is closed.
+    // The window's title bar is the drag handle; keep its position between frames.
     ImGui::SetNextWindowPos(ImVec2(18.0f, 90.0f), ImGuiCond_FirstUseEver);
-    ImGui::SetNextWindowSize(ImVec2(190.0f, 112.0f), ImGuiCond_FirstUseEver);
+    ImGui::SetNextWindowSize(ImVec2(205.0f, 126.0f), ImGuiCond_FirstUseEver);
     ImGui::SetNextWindowBgAlpha(0.88f);
     if (ImGui::Begin("GDH launcher", nullptr,
-        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
+        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
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
*** End Patch
