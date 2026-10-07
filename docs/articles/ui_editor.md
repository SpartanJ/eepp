# UI editor

`eepp-UIEditor` previews native eepp XML layouts and CSS while you edit them. Open a layout,
a stylesheet, or a project from the menus or command line:

```sh
bin/eepp-UIEditor --xml /path/to/layout.xml --css /path/to/style.css
bin/eepp-UIEditor --project /path/to/project.xml
bin/eepp-UIEditor --xml /path/to/layout.xml --disable-app-theme
```

The preview uses the bundled Breeze stylesheet by default and opens it in an editor tab.
Use `--disable-app-theme` to start without it, or toggle **View → Use app theme (Breeze)** at runtime.
The toggle refreshes the preview with or without Breeze, preserves project and embedded styles,
and leaves the editor shell's theme unchanged. Use
`--pixel-density 2` to check HiDPI behavior and `--prefers-color-scheme light`, `dark`, or `system`
to select the preview's color preference.

## Live editing

XML and CSS changes in the built-in editor, including undo and redo, rebuild the preview after
350 ms of inactivity. Unsaved documents are parsed directly from memory, preserving their original
file URI for relative assets, CSS imports, and author font faces. Incomplete XML keeps the last
valid preview and logs a parse error to the console.

The selected XML, project CSS, and enabled base CSS are also watched for external changes,
including replacement files written by editors that save through a rename. Dirty editor buffers
take precedence over disk changes. New layouts have temporary backing files; Save prompts for a
permanent destination, as does Save All for edited new layouts. Save As preserves an existing source
file.

## Projects

Project paths are relative to the project file's directory, or to an optional `basepath` resolved
against that directory. Opening a project does not change the application's working directory.

```xml
<uiproject>
    <basepath>assets</basepath>
    <font><path>fonts</path></font>
    <drawable><path>images</path></drawable>
    <widget>
        <customWidget name="GameScreen" replacement="RelativeLayout" />
    </widget>
    <layout width="1920" height="1080">
        <path>layouts</path>
    </layout>
    <stylesheet path="style.css" />
</uiproject>
```

Font, drawable, and layout paths can name individual files or directories. `uitheme` elements can
name texture-atlas theme files. Custom widget replacements allow layouts using application-specific
widgets to be previewed with known native controls. The Layouts menu selects between project layouts.

Projects with positive layout width and height automatically use a fixed viewport at those pixel
dimensions, scaled down and centered when necessary to fit the preview split. Layout, viewport
units, and media queries use the project dimensions. Disable **View → Use project viewport** to
use the split's available size without scaling. Projects without valid dimensions and standalone
layouts use the split's size by default. F1 can resize the native window toward the project dimensions.

## Shortcuts

| Shortcut | Action |
| --- | --- |
| F1 | Resize the native window to fit the project viewport, within the usable display area |
| F3 or backslash | Toggle console |
| F6 | Toggle focus and hover highlighting in the preview and its nested scenes |
| F7 | Toggle debug boxes |
| F8 | Toggle debug data |
| F9 | Toggle editor panel |
| F11 | Open the widget inspector for the preview |
| F12 | Reapply preview style states |
| Ctrl+Escape | Request application close |

The editor also supports its usual document save, Save As, Save All, and New commands. The File menu
retains recent layouts and projects; Resources loads additional fonts, images, and stylesheets.
