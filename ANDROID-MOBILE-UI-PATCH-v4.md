# GDH Android mobile UI patch v4

Base: the supplied GDH-main.zip. Target: Geometry Dash Android 2.2081 / Geode SDK 5.10.1 as declared in mod.json.

## Changed files

- `src/interface/cocos/hacks_layer.cpp`: stopped hiding the standard Geode Popup close button. Runtime appearance is not yet tested.
- `src/interface/cocos/hacks_tab.cpp`: reduced the mobile content viewport from 500x250 to 340x250 to fit the 460x260 popup; changed hack/settings rows to one column and resized row/separator/button widths to avoid horizontal clipping in the mobile window.

## Important limitations

- This is a source patch, not a compiled or tested mod.
- No claim is made that all desktop-only features work on Android. Features that rely on `GEODE_IS_DESKTOP`, keyboard input, or ImGui still require individual mobile implementations.
- No Android CI run has been performed here. Upload the project contents to a GitHub repository and run the existing `Build Geode Mod` workflow; share the full failing log if it fails.
- Check the upstream GDH license and third-party notices before redistributing modified builds.
