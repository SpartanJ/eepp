# Keyboard shortcuts editor

`EE::UI::Tools::UIKeybindingsEditor` is an embeddable native widget for application shortcuts.
Include `<eepp/ui/tools/uikeybindingseditor.hpp>`. Most applications keep their shortcuts in a
`keybindings.cfg` file and only need `NewForFile()`:

```cpp
static constexpr std::array<KeybindingsConfig::Context, 1> contexts{ { { "viewer" } } };

UIWidget* App::createKeybindingsEditor( UIWidget* parent ) {
    return UIKeybindingsEditor::NewForFile(
        parent,
        { .path = keybindingsPath,
          .contexts = contexts,
          .catalog = [this]( KeybindingsConfig::Catalog& catalog ) {
              // Defaults, plus assignable commands that have no default shortcut.
              catalog.add( "viewer", getDefaultKeybindings(), getCommandList(),
                           i18n( "viewer", "Viewer" ) );
          },
          .apply = [this] { return reloadKeybindings() ? String() : String( "Reload failed" ); },
          .manualEdit = [this] { openKeybindingsFile(); } } );
}
```

`NewForFile()` reads and strictly validates the file. When the file can't be edited (unreadable,
invalid shortcuts, or invalid modifiers), it returns an error view with the manual-edit button
instead of an editor. Each save re-reads the file, refuses to overwrite it if it changed since it
was read, runs the optional `validate` callback, writes atomically, and then calls `apply` to
install the new bindings. An error returned by `validate` or `apply` is shown and keeps the editor
unchanged. `note` shows optional text below the editor. Context names must outlive the widget;
static arrays are the usual choice.

The widget can also be returned from `UISettingsPanel::addCustomWidget()`. Pass `true` as the third
argument to that method for a dedicated category whose editor fills the content viewport. The panel
supplies the category heading. Normal settings and aggregate search results retain their scrolling
layout.

For shortcuts that don't live in a keybindings file, `New()` takes the command list and a save
callback directly:

```cpp
KeybindingsConfigData configuration;
configuration.modifier = KeyMod::getDefaultModifierString();
configuration.secondaryModifier = KeyMod::getDefaultSecondaryModifierString();
configuration.commands.push_back( { "editor", "save", "Save", { "mod+s" }, { "mod+s" } } );

auto* editor = UIKeybindingsEditor::New( parent, std::move( configuration ),
    [app]( const KeybindingsConfigData& proposal ) -> String {
        // Persist the complete proposal, then update the application's command executers.
        return app->applyKeyboardShortcuts( proposal ); // Empty on success; error text otherwise.
    } );
```

The command catalog belongs to the editor after construction. Binding order defines Main,
Alternative, and additional shortcuts. `KeybindingsCommand::unbound` records an explicit clearing,
distinct from a command that has no default shortcut. Assignment and reset clear this flag. Include commands with empty bindings to make them assignable.
The application supplies complete effective contexts, including inherited shortcuts. Independent
contexts may reuse the same shortcut; a conflicting assignment inside a context requires Reassign
or Cancel. Reassign removes only the conflicting shortcut from the previous command.

Changes apply immediately through the callback. Failed saves retain the previous configuration. Store
expressions such as `mod+s` rather than only resolved key codes: symbolic modifiers follow modifier
changes while literal `ctrl+s` stays literal. `KeybindingsConfigData::resolve()` resolves expressions
without changing the global modifier configuration.

Contexts are expandable tree headings with Command, Main, and Alternative columns. Filtering
retains matching commands under their context headings. Select a command to record or clear its Main/Alternative shortcuts, restore its defaults, or expand
additional shortcuts. While recording, the active select button stays selected and displays “Recording… Press a shortcut”.
Escape cancels capture; the explicit Escape button assigns Escape. Labels are translated through
the scene's translator (`UISceneNode::i18n()`), using the `kb_*` keys. Context headings show the
capitalized context name unless `KeybindingsConfigData::contextNames` provides one; the catalog's
`add()` sets it from its last argument. The widget owns its controls and outstanding confirmation
dialog; callbacks must remain valid until the widget is destroyed.

In ecode, Keyboard Shortcuts opens this editor in user settings. The `keybindings-file` command opens
the underlying configuration. Binding state lives in `keybindings.cfg`, so copying that file preserves
intentional unbindings and Main/Alternative order. Existing mouse shortcuts remain separate and are
preserved by the keyboard editor. Plugin shortcuts remain managed by their plugins. Reloading core
shortcuts retains plugin/widget overrides; a new core assignment shadowed by such an override is
rejected with the owning command so it can be changed in its own settings.

To offer manual editing, call `editor->setManualEditCallback(callback)`. The button is hidden and
disabled by default; supplying a callback shows it, and supplying an empty callback hides it again.
The consumer handles opening a file or editor and closing any surrounding modal window. No path or
file-opening policy is stored by the reusable widget. ecode uses this action to open `keybindings.cfg`.

## Shared configuration helpers

`<eepp/ui/tools/keybindingsconfigdata.hpp>` supplies the command catalog and symbolic modifier resolver
without including the editor widget. `<eepp/ui/tools/keybindingsconfig.hpp>` supplies section
parsing, serialization, validation, optional default recovery, and replacement of a managed binding
layer while preserving externally installed widget/plugin overrides.

Applications supply their own context names and input types:

```cpp
using namespace EE::UI::Tools;

const std::array<KeybindingsConfig::Context, 2> contexts{{
    { "viewer" },
    { "viewer_gestures", KeybindingsConfig::InputType::Mouse }
}};
std::string error;
if ( !KeybindingsConfig::validate( ini, contexts, error, input ) )
    return error; // Keep the last working runtime configuration.

KeybindingsConfig::Section section;
if ( !KeybindingsConfig::readSection( ini, "viewer", section, &error ) )
    return error;
```

`readModifiers()` reads `modifier.mod` and `modifier.mod2` into two strings without changing
`KeyMod`. Applications explicitly apply validated preferences and choose when to install runtime
bindings. Parsing a file never restores defaults automatically.

For loading at startup or on reload, `loadFile()` combines these steps:

```cpp
KeybindingsConfig::LoadedFile file;
KeybindingsConfig::Section section;
std::string error;
if ( !KeybindingsConfig::loadFile( path, contexts, input, file, error ) ||
     !KeybindingsConfig::readSection( file.ini, "viewer", section, &error ) )
    return error; // Keep the last working runtime configuration.
KeybindingsConfig::mergeDefaults( section, getDefaultKeybindings(), input );
KeybindingsConfig::writeSection( file.ini, "viewer", section );
file.save(); // Persists recovered defaults unless entries were skipped.
section.apply( runtimeBindings );
```

It reads the file tolerantly (see `prepareForLoad()` below) and logs each skipped entry, then
applies the modifiers to `KeyMod` and fills in missing `modifier` entries. Nothing changes when it
fails. An empty path loads compiled defaults only. `LoadedFile::save()` writes only when the
configuration changed, never for a defaults-only load, and never when entries were skipped, so the
original file stays available for manual repair. `canSave()` reports whether a save is possible.

For tolerant loading, call `prepareForLoad()` instead of strict `validate()`. It removes unknown
keyboard names and conflicting aliases from the in-memory INI, returning a warning for every
skipped entry. The lexically first expression wins a physical collision. Metadata and modifiers
must still be valid. If warnings are returned, retain the original file on disk for manual repair;
both consumers follow this policy. Editor saves use strict validation.

Call `mergeDefaults()` only when the application's recovery policy requires it. It preserves existing
assignments, explicit unbindings, and occupied physical shortcuts. The default restores one shortcut
per missing command; `restoreAllAliases=true` restores every supplied alias for missing commands,
which is useful on the first run. String-map defaults use lexical expression order; typed keyboard
defaults use `KeyBindings::getOrderedShortcuts()` order. Mouse recovery uses an explicit
`InputType::Mouse` or the typed mouse overload, independently of the section name. Symbolic default
expressions use the current `KeyMod` preferences; apply the selected preferences before merging.
Pass the application's input object to protect physical keyboard aliases. A null input in the
string-map overload checks expression occupancy only; the typed keyboard overload requires input.

`writeSection()` updates only the supplied INI object. `writeConfiguration()` writes an editor
proposal: the modifiers and every keyboard context that has commands in it, keeping the stored order
of unchanged commands. `NewForFile()` uses it; applications using `New()` can too. Persist the
result with `IniFile::writeFileAtomic()`, then install its runtime maps with `Section::apply()` and
the application's command routing. `KeybindingsConfig::Catalog` builds the editor's command list
from a parsed file. Typed defaults keep `getOrderedShortcuts()` order for Main and Alternative;
expression-map defaults are ordered lexically. `replaceBindings()` preserves the externally
installed layer; `validateOverrides()` reports new assignments shadowed by that layer.
`KeybindingsConfigData::shortcuts()` resolves one context of a proposal for that check.

Both ecode and eterm use `loadFile()` and `NewForFile()`. ecode keeps its version migrations and
command routing, and adds a `validate` callback that rejects assignments shadowed by plugin or
widget shortcuts. eterm keeps its first-run alias and missing-command recovery policies.

## Recovery and manual editing in ecode

Use **Edit → Key Bindings** with the mouse to open the editor even when application shortcuts
are missing. **Restore All Shortcuts…** is always available without selecting a command. It asks
for confirmation, restores the supplied defaults for all keyboard contexts, and retains the chosen
modifiers. Per-command reset is available separately. Mouse mappings are not changed by these UI
actions.

The existing automatic recovery of missing defaults remains enabled for manually edited files:
missing commands can regain their default shortcuts when those shortcuts are free. Explicit
assignments are preserved. To intentionally leave a command unbound, clear it through the editor;
that intention is stored in the context's optional `unbound` list. To re-enable it manually,
remove its name from that list and assign a shortcut. The UI handles both changes automatically.
Saving the bindings file in ecode reloads it, including through a symlink or canonicalized
profile path. External changes are loaded on restart or an explicit reload. If the file changes
while the settings editor is open, close and reopen the settings window before applying another
change; the editor refuses to overwrite a newer file.

## Keyboard shortcuts in eterm

Open **Settings → General → Keyboard Shortcuts**, or choose **Keybindings** from a terminal's
context menu. On macOS it is also available from the File menu. Changes apply to all open terminals
and are used by new terminals. The editor includes terminal and tab commands with no default
shortcut, so they can be assigned without editing the file.

**Edit keybindings manually…** closes settings and opens `keybindings.cfg` in an editor tab.
Saving that file reloads the bindings. The `open-keybindings-file` command opens the same tab,
while `open-keybindings` opens the graphical settings page. A file changed outside the graphical
editor must be reopened before further GUI edits; stale edits cannot overwrite it.

macOS defaults use the default modifier, normally Meta, for copy/paste, find, new/close tab,
numbered-tab selection, and split navigation. Splitting uses Meta+Shift with I/J/K/L. Other
platforms retain the Ctrl/Shift combinations that avoid common terminal-input shortcuts. Existing
configured assignments are retained; use **Restore All Shortcuts…** to adopt the new defaults.

## Configuration format

Shortcut mappings always use plain command names. Optional state lives in a separate section in
**the same `keybindings.cfg` file**:

```ini
[editor]
alt+up=add-cursor-above
mod+w=close-tab
mod+f4=close-tab

[editor.state]
order=["mod+w","mod+f4"]
unbound=["close-app","some-other-command"]
```

`order` lists shortcuts in the desired Main, Alternative, and additional shortcut order for each
command. Listed shortcuts precede unlisted shortcuts; unlisted ones use lexical order. Entries
for missing shortcuts are ignored, so deleting a mapping by hand does not invalidate the file.
The editor only saves ordering when lexical order would produce a different result.

A command that has no default shortcut can remain unassigned without an `unbound` entry.
`unbound` lists commands that must remain disabled, even when a mapping for them exists in the
shortcut section. Clearing the last shortcut adds its command to this list. Assigning a shortcut
or restoring defaults through the UI removes it. When editing manually, remove the command from
`unbound` as well as assigning its shortcut. Commands absent from this list still benefit from
automatic restoration of missing defaults. Empty state arrays and unused state sections are omitted;
comments and unknown state entries are retained.

Older ecode versions can still read all assigned shortcuts because the original binding sections
contain only plain commands. They ignore the optional state sections, so they cannot honor explicit
unbindings or the chosen primary shortcut. An older version may restore disabled defaults, but the
new version continues honoring `unbound` when reopening the file. Keeping that list authoritative
preserves the user's intention across versions.

Arrays must fit on one INI line and contain nonempty strings. Malformed state is a configuration
error: a failed reload keeps the last working runtime bindings and leaves the file untouched.
Invalid modifier preferences also reject the load. Unknown keyboard names and conflicting physical
shortcuts are skipped individually, with every issue reported; the lexically first expression owns
a conflicting physical shortcut. Other custom shortcuts remain active. When entries are skipped,
the original file stays untouched for manual repair. The settings editor requires a valid file and
rejects conflicting or invalid shortcuts before saving.
If loading fails at startup, ecode uses compiled defaults in memory so typing, navigation, saving,
and the mouse route to settings remain available. Writes from the settings editor
and default recovery use a temporary sibling file and replace the destination only after the full
contents have been written successfully. The settings page offers manual editing to fix the error.

`editor_mouse_bindings` supports the same plain mappings and optional `editor_mouse_bindings.state`
section. Mouse capture UI is not provided yet.
