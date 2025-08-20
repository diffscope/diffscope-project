# Accessibility Conventions

This document outlines the accessibility design principles for the DiffScope project, covering both screen reader compatibility and internationalization support.

## Overview

Accessibility in DiffScope encompasses two primary areas:
1. **Assistive Technology Support**: Ensuring compatibility with screen readers and other accessibility tools
2. **Internationalization (i18n)**: Supporting multiple languages and locales, including right-to-left layouts

## Assistive Technology Support

### Focus Management

All interactive controls must support proper focus behavior:

```qml
// Ensure controls can receive keyboard focus
Item {
    focusPolicy: Qt.StrongFocus  // or Qt.TabFocus for tab-only navigation
    activeFocusOnTab: true       // Enable tab navigation
}

// For complex custom controls that need strong focus
ColorPalette {
    focus: true
    focusPolicy: Qt.TabFocus
    activeFocusOnTab: true
}
```

**Focus Policy Guidelines:**
- `Qt.StrongFocus`: Controls that accept both keyboard and wheel focus (e.g., text inputs, sliders)
- `Qt.TabFocus`: Controls that only accept tab focus (e.g., buttons, checkboxes)
- `Qt.WheelFocus`: Controls that accept wheel events for navigation

### Accessible Properties for Custom Controls

For custom QML Quick controls, always set appropriate accessible properties. Qt Quick Controls (Button, Label, etc.) have built-in accessibility support and don't require manual configuration.

```qml
// Custom control accessibility
Frame {
    focusPolicy: Qt.StrongFocus
    Accessible.name: qsTr("Lyric Pronunciation Editor")
    Accessible.role: Accessible.Table  // or appropriate role
    Accessible.description: qsTr("Edit pronunciation for lyrics")
}

// Cell-based controls
Item {
    Accessible.role: Accessible.Cell
    Accessible.name: lyricText
    Accessible.description: qsTr("Current pronunciation is %1. Candidate pronunciations are %2. Activate to modify pronunciation.").arg(pronunciation).arg(candidatePronunciations.join(" "))
    Accessible.checkable: isSelectable
    Accessible.checked: isSelected
    
    // Action handlers for screen readers
    Accessible.onPressAction: performPrimaryAction()
    Accessible.onToggleAction: toggleSelection()
}
```

**Common Accessible Roles:**
- `Accessible.Button`: Clickable buttons
- `Accessible.Cell`: Table/grid cells
- `Accessible.Table`: Tables and grids
- `Accessible.TitleBar`: Window title bars
- `Accessible.Row`: Table rows
- `Accessible.ComboBox`: Dropdown controls

### Descriptive Information

Use the `DescriptiveText` component from `SVSCraft.UIComponents` to provide additional context:

```qml
import SVSCraft.UIComponents

Button {
    icon.source: "icon.svg"
    DescriptiveText.activated: hovered
    DescriptiveText.toolTip: qsTr("Icon only button description")
    DescriptiveText.statusTip: qsTr("Additional status information")
    
    // Automatically binds to Accessible.description when enabled
    DescriptiveText.bindAccessibleDescription: true
}
```

**DescriptiveText Properties:**
- `toolTip`: Brief description shown on hover
- `statusTip`: Detailed status information
- `contextHelpTip`: Extended help text
- `bindAccessibleDescription`: Automatically sets `Accessible.description`

### Explicit Accessible Description

For direct control over accessibility descriptions:

```qml
SelectableLabel {
    text: plugin.version
    DescriptiveText.toolTip: qsTr("Version")
    Accessible.description: DescriptiveText.toolTip
    DescriptiveText.activated: hovered
}
```

### Ignored Elements

Mark decorative or redundant elements as ignored:

```qml
Label {
    id: decorativeLabel
    Accessible.ignored: true  // Screen readers will skip this element
}
```

## Internationalization Support

### Right-to-Left (RTL) Layout Support

Follow Qt Quick's mirroring conventions to support RTL languages:

```qml
// Use mirrored property for layout adjustments
RadioButton {
    leftPadding: control.indicator && !control.mirrored ? control.indicator.width + control.spacing : 0
    rightPadding: control.indicator && control.mirrored ? control.indicator.width + control.spacing : 0
}

// Position elements based on mirroring
MenuItem {
    leftPadding: !control.mirrored ? indicatorPadding : arrowPadding
    rightPadding: control.mirrored ? indicatorPadding : arrowPadding
}

// Use mirrored positioning for icons and indicators
Switch {
    x: control.text ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding) : control.leftPadding + (control.availableWidth - width) / 2
}
```

**RTL Layout Guidelines:**
- Avoid using explicit `x`, `y` coordinates for positioning
- Use `leftPadding`/`rightPadding` instead of fixed padding
- Check `control.mirrored` property for conditional positioning
- Use `Layout` properties for automatic mirroring support

**Enable RTL Testing:**
```qml
ApplicationWindow {
    LayoutMirroring.enabled: mirroringCheckBox.checked
    LayoutMirroring.childrenInherit: true
}
```

### Text Localization

All user-visible text must use `qsTr()` for translation:

```qml
// Correct - translatable text
Button {
    text: qsTr("&Apply")
}

Label {
    text: qsTr("You may need to restart %1 for font changes to take full effect.").arg(Application.name)
}

// Incorrect - hardcoded text
Button {
    text: "Apply"  // ❌ Not translatable
}
```

**Translation Patterns:**
```qml
// Parameter substitution
text: qsTr("Current pronunciation is %1").arg(pronunciation)

// Multiple parameters
text: qsTr("%1 (Left Top)").arg(layoutName)

// Use qsTr consistently for all UI text
Accessible.description: qsTr("Current pronunciation is %1. Candidate pronunciations are %2. Activate to modify pronunciation.")
    .arg(pronunciation)
    .arg(candidatePronunciations.join(" "))
```

### Locale-Aware Formatting

#### Number and Currency Formatting
```qml
// For display text - use locale-aware formatting
SpinBox {
    validator: IntValidator {
        locale: control.locale.name  // Use current locale
        bottom: Math.min(control.from, control.to)
        top: Math.max(control.from, control.to)
    }
    
    // Use locale for number parsing
    valueFromText: function(text, locale) {
        return parseInt(text.replace("%", ""))  // Consider locale-specific separators
    }
}
```

#### Time and Date Formatting
```qml
// For internal/technical data - use fixed formatting
LongTime {
    // Internal time representation - no locale needed
    toString(minuteWidth, secondWidth, msecWidth)
}

// For display - consider locale formatting needs
Label {
    text: timeValue.toLocaleString()  // Use when displaying to users
}
```

**Formatting Guidelines:**
- **Display Text**: Use locale-aware formatting for numbers, dates, currencies
- **Internal Data**: Use consistent format for logs, config files, technical output
- **File Paths**: Always use system-native path separators
- **API Data**: Use standardized formats (ISO dates, etc.)

### Input Method Support

Support international input methods:

```qml
TextInput {
    font: control.font
    inputMethodHints: control.inputMethodHints  // Support IME
    
    // Handle pre-edit text for complex input methods
    visible: !control.length && !control.preeditText && (!control.activeFocus || control.horizontalAlignment !== Qt.AlignHCenter)
}
```

## Testing Accessibility

### Screen Reader Testing
1. Test with Windows Narrator or NVDA
2. Verify all interactive elements are announced correctly
3. Check that custom controls provide meaningful descriptions
4. Ensure logical tab order and focus navigation

### RTL Layout Testing
```qml
// Add debugging controls to test applications
CheckBox {
    id: mirroringCheckBox
    text: "Enable RTL Layout"
}

ApplicationWindow {
    LayoutMirroring.enabled: mirroringCheckBox.checked
    LayoutMirroring.childrenInherit: true
}
```

### Translation Testing
1. Test with longer translations (German, Russian)
2. Verify text doesn't overflow containers
3. Check parameter substitution works correctly
4. Test special characters and Unicode support

## Common Patterns

### Accessible Custom Components
```qml
// Table cell with full accessibility support
component LyricCellDelegate: ColumnLayout {
    property string pronunciation: ""
    property string lyric: ""
    property list<string> candidatePronunciations: []
    
    Accessible.role: Accessible.Cell
    Accessible.name: lyric
    Accessible.description: qsTr("Current pronunciation is %1. Candidate pronunciations are %2. Activate to modify pronunciation.")
        .arg(pronunciation)
        .arg(candidatePronunciations.join(" "))
    Accessible.checkable: true
    Accessible.checked: highlighted
    
    Accessible.onPressAction: openContextMenu()
    Accessible.onToggleAction: openContextMenu()
}
```

### Icon-Only Controls
```qml
Button {
    display: AbstractButton.IconOnly
    icon.source: "icon.svg"
    
    // Always provide text for accessibility
    text: qsTr("Grid view")
    
    // Add tooltip for sighted users
    DescriptiveText.activated: hovered
    DescriptiveText.toolTip: text
}
```

### Multi-language File Handling
```qml
// File paths and technical data
TextField {
    // File paths - no locale formatting
    text: fileInfo.absolutePath  
}

// User-visible content
Label {
    // Translated user content
    text: qsTr("Last modified: %1").arg(file.lastModified.toLocaleString())
}
```

By following these conventions, DiffScope ensures accessibility for users with disabilities and provides excellent internationalization support for global users.
