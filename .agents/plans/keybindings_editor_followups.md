# Keybindings editor follow-ups

Status: deferred improvements, recorded 2026-10-28. This note does not authorize implementation.

The reusable editor and configuration helpers are implemented. Plugin shortcut editing remains
outside the initial scope; existing mouse mappings are preserved, but the UI records keyboard
shortcuts only. Consumer documentation: [Keyboard shortcuts editor](../../docs/articles/ui_keybindings_editor.md).

## Unified plugin shortcut editing

- [ ] Add an optional shortcut interface on ecode's `Plugin`, rather than only `PluginBase`.
  Several major plugins do not inherit from `PluginBase`. Expose command names, defaults, current
  bindings, and callbacks to persist and apply changes.
- [ ] Show plugin commands under expandable headings on the same Keyboard Shortcuts page.
  Keep the eepp editor independent of ecode and its plugin system.
- [ ] Separate ownership/display grouping from execution scope. Plugin editor commands must
  participate in core editor conflicts; completion-popup navigation and other contextual bindings
  must not produce false conflicts with ordinary editor navigation.
- [ ] Detect conflicts from declared plugin bindings, not just effective widget maps. The current
  save check protects new core assignments against plugin/widget bindings installed on the main
  layout and open editors. It does not inventory disabled plugins, shadowed assignments, or custom
  event handlers such as autocomplete's shortcuts. Decide how unavailable plugin catalogs are
  represented without activating disabled plugins.
- [ ] Extend plugin configuration/loading to support Main, Alternative, and additional shortcuts.
  Most plugins currently store command -> one shortcut. Preserve existing configurations and let
  plugins retain responsibility for persistence initially; do not create competing sources of truth.
- [ ] Exercise reassignment in both directions, plugin reload/disable, modifier changes, and
  context-specific shortcuts. Reassignment must update the losing owner as well as the new owner.

## Mouse binding UI

- [ ] Add editing/capture for mouse actions, buttons, modifier combinations, and wheel bindings.
  Reuse the existing mouse codec and optional state sections. Define capture/cancellation and
  conflict behavior before extending the keyboard recorder; keyboard edits must preserve mouse
  mappings.

## Constraints to preserve

Keep manual editing, readable plain shortcut -> command mappings, optional state in the same file,
explicit unbindings, and automatic recovery of missing defaults. Tolerant loading must retain usable
custom shortcuts and leave the source untouched when entries are skipped; editor saves stay strict.
The intermediate formats developed during this feature never shipped, and their migration code was
removed deliberately. Development-profile cleanup, if ever needed, is a separate operation.
