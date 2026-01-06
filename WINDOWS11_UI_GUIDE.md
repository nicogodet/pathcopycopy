# Windows 11 Modern Context Menu Integration - Settings UI Update Guide

## Overview
This document describes the changes needed to the PathCopyCopy Settings application UI to support Windows 11 modern context menu configuration.

## Required UI Changes

### MainForm Updates

The following UI controls need to be added to the MainForm (PathCopyCopySettings/UI/Forms/MainForm.Designer.cs):

#### 1. Windows 11 Integration Tab or Group Box

Add a new tab page or group box labeled "Windows 11 Integration" to the main settings interface.

#### 2. Windows 11 Menu Enabled Checkbox
- **Control Type**: CheckBox
- **Name**: `Windows11MenuEnabledChk`
- **Text**: "Enable Windows 11 modern context menu integration"
- **Description**: When checked, Path Copy Copy commands will appear in the Windows 11 modern context menu without requiring "Show more options"
- **Property Binding**: Binds to `UserSettings.Windows11MenuEnabled`

#### 3. Quick Access Plugin Combo Box
- **Control Type**: ComboBox
- **Name**: `Windows11QuickPluginCbo`
- **Text/Label**: "Quick access plugin (optional):"
- **Description**: Allows user to select one plugin that will execute directly without showing a submenu
- **Data Source**: All available plugins (same as other plugin dropdowns)
- **Property Binding**: Binds to `UserSettings.Windows11QuickPlugin`
- **Special Behavior**: 
  - Includes "(None)" option at the top
  - When set, clicking the main menu item will execute this plugin directly
  - Submenu is still available through a submenu arrow

#### 4. Submenu Plugins CheckedListBox
- **Control Type**: CheckedListBox or similar multi-select control
- **Name**: `Windows11SubmenuPluginsChkList`
- **Text/Label**: "Plugins to show in submenu:"
- **Description**: List of plugins that will appear in the Windows 11 submenu
- **Data Source**: All available plugins
- **Property Binding**: Binds to `UserSettings.Windows11SubmenuPlugins`
- **Special Behavior**: 
  - If empty, shows all plugins (same as current submenu behavior)
  - Allows reordering of plugins

## Implementation Notes

### Code-Behind Changes (MainForm.cs)

1. **Add Windows 11 settings load/save methods**:
```csharp
private void LoadWindows11Settings()
{
    // Load Windows 11 menu enabled setting
    Windows11MenuEnabledChk.Checked = UserSettings.Windows11MenuEnabled;
    
    // Load quick plugin setting
    if (UserSettings.Windows11QuickPlugin.HasValue)
    {
        // Find and select the plugin in combo box
        var quickPluginId = UserSettings.Windows11QuickPlugin.Value;
        // TODO: Set Windows11QuickPluginCbo.SelectedItem
    }
    
    // Load submenu plugins setting
    var submenuPlugins = UserSettings.Windows11SubmenuPlugins;
    if (submenuPlugins != null)
    {
        // TODO: Set checked items in Windows11SubmenuPluginsChkList
    }
}

private void SaveWindows11Settings()
{
    // Save Windows 11 menu enabled setting
    UserSettings.Windows11MenuEnabled = Windows11MenuEnabledChk.Checked;
    
    // Save quick plugin setting
    // TODO: Get selected item from Windows11QuickPluginCbo
    
    // Save submenu plugins setting
    // TODO: Get checked items from Windows11SubmenuPluginsChkList
}
```

2. **Call load/save methods in appropriate places**:
   - Call `LoadWindows11Settings()` in form load event
   - Call `SaveWindows11Settings()` in ApplySettings or OK button click handler

3. **Add event handlers**:
```csharp
private void Windows11MenuEnabledChk_CheckedChanged(object sender, EventArgs e)
{
    // Enable/disable Windows 11 sub-controls based on checkbox state
    Windows11QuickPluginCbo.Enabled = Windows11MenuEnabledChk.Checked;
    Windows11SubmenuPluginsChkList.Enabled = Windows11MenuEnabledChk.Checked;
}
```

### User Experience Considerations

1. **Default Behavior**: 
   - Windows 11 integration disabled by default (for compatibility)
   - When first enabled, uses all plugins in submenu (same as classic menu)

2. **Visual Feedback**:
   - Show a note that Windows 11 integration requires Windows 11
   - Indicate that both classic and modern menus will coexist
   - Explain that the classic menu is available via "Show more options"

3. **Validation**:
   - Warn if quick access plugin is selected but submenu is empty
   - Suggest selecting at least one plugin for submenu

## Testing Checklist

- [ ] Windows 11 settings load correctly from registry
- [ ] Changes to Windows 11 settings save correctly to registry
- [ ] Quick access plugin combo box populates with all plugins
- [ ] Submenu plugins list allows selection/deselection
- [ ] Enabling/disabling Windows 11 integration works correctly
- [ ] Settings persist across application restarts
- [ ] No regression in existing settings functionality

## Registry Values Created

The following registry values will be created under `HKEY_CURRENT_USER\Software\clechasseur\PathCopyCopy`:

- `Windows11MenuEnabled` (DWORD): 0 or 1
- `Windows11QuickPlugin` (String): Plugin GUID or empty
- `Windows11SubmenuPlugins` (String): Comma-separated list of plugin GUIDs

## Future Enhancements

Potential future improvements to consider:
1. Drag-and-drop reordering of submenu plugins
2. Preview of what the menu will look like
3. Import/export of Windows 11 settings
4. Per-plugin icon customization for Windows 11 menu
