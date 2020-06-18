
# TODO - Short and mid term plans.

## UIIconTheme and UIIconThemeManager

Implement icon themes separated from the `UITheme` and customizable from a CSS file.

## UICodeEditor

* Add show white spaces.

* Add new CSS properties related to the widget.

## TextDocument

* Add indentation type auto-detection.

* Add multi-line search and replace.
 
* Add auto-close brackets.
 
* Add XML tags auto-close.
 
* Add command to comment selected lines.
 
* On Save: trim white spaces.
 
* On Save: Ensure new line at end of file option.
 
* On Save: Allow to select line endings type.

## UITreeView

Implement a simple tree view widget, to at least cover the most common use cases.

## Code Editor

Keep improving it:

* Add more menues ( Edit, Window ).

* Save user configuration.

* Support single instance (when a new file is opened while a previous instance exists, open it in the first instance).

* Allow to open a document in any number of tabs.

## UI Editor

* Integrate the `UICodeEditor` into the editor in order to be able to edit the layouts and CSS in app.

* Once `UITreeView` is finished add a tree view inspecto of the node tree.

# StyleSheetParser

Detect errors and log them.

# UISceneNode and UIWindow

Redesign the shortcut API to use the new keybinding class.
