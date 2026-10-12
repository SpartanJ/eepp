# Tab-focus defaults implementation report

Implemented against `a06826ee72208728c18acb563e02e7351215c958` plus the local patch.
Nothing is staged or committed. The implementation follows
`/home/downloads/eepp_tab_focus_defaults_implementation_plan.md` without changing the focus
dispatcher, Scene dependencies, native backends, or accessibility policy.

## Policy and compatibility

`UIWidget` no longer opts into Tab navigation. Interactive constructors opt in; static text,
visuals and structural containers remain semantically accessible without becoming Tab stops.
Non-tabbable containers are traversable. Mouse/programmatic focus is unchanged.

The form regression checks actual dispatcher-delivered Tab and Shift+Tab, including wraparound,
disabled/hidden controls and nested non-tabbable containers. With the former blanket default,
labels, bare widget containers and slider implementation parts were eligible stops. The new
tested form sequence is `TextInput -> CheckBox -> Button -> Slider`, reversed by Shift+Tab.
Reverse traversal now visits a previous sibling's last descendant instead of its first, and
visits a tabbable parent before leaving its children.

Composite decisions:

- Spinbox: embedded input is the sole default entry; Up/Down adjusts the value. Native Focus
  actions target the input and continue projecting to the spinbox's existing semantic identity.
- Combo: embedded dropdown is the entry. Its existing semantic child identity is retained.
  `focusable` properties on spin/combo controls configure the embedded target, not a second host
  stop. Programmatic child policies use `getTextInput()` / `getDropDownList()`.
- Dropdowns: F4/Alt+Down opens the popup without first moving selection; Escape returns to the
  field. Both item and model dropdown handlers recognize the opening keys before forwarding.
  Tab/Shift+Tab closes the popup and lets the dispatcher navigate relative to the restored field,
  rather than the popup's temporary scene-root sibling position; regression-covered in both directions.
- Listbox: root entry, arrow-focused rows remain outside the ordinary Tab ring; Shift+Tab from
  a non-tabbable row exits in one step. Explicitly tabbable rows and `UI_TAB_STOP` retain their
  existing behavior. Rows and popup menus/menu items retain native Focusable state and Focus
  actions independently of sequential eligibility.
- Model views: view entry, no stops for recycled cells or embedded scrollbars. Header buttons
  retain their existing keyboard sorting path pending a view-owned header navigation interface.
- Calendar: root plus four header/Today buttons, no day/month/year-cell stops. Date-picker
  popup button is subordinate; F4/Alt+Down opens and Escape/selection restores editor focus.
- Tabs/radios: individual members remain reachable. Tabs now activate with Enter/keypad Enter
  or Space. Full roving focus remains a follow-up rather than suppressing inactive members.
- Menubar: actual menu buttons are the entries; Enter/keypad Enter/Space/Down opens their menu.
  Popup items use arrows; Escape restores the owner. Moving focus into a popup clears the
  originating button's pressed state because key-up will be delivered elsewhere.
- Scrollbars: no host or implementation stops by default; an intentionally standalone scrollbar
  can opt in its existing keyboard-operable slider. Scroll/document/viewer hosts remain opt-in.
- Links: XML/HTML loading sets the nonempty-href default before author properties. Span Enter
  uses the same navigation interception path as mouse clicks. Runtime href changes preserve
  explicit focus policies; dynamic recomputation and programmatic-link defaults remain explicit.

Keyboard tests also exposed uninitialized existing repeat timers in checkboxes, radios and
sliders. They are initialized, and the first key activation is not delayed by the repeat guard.
No fields, registries, per-frame queries or constructor-time focus changes were added.

The ecode audit confirmed Search and Global Search override departure from their second text
input using `OnTabNavigate`. Those controllers are unchanged; a regression reproduces both
two-field callback loops, including reverse navigation. Their Escape/Enter commands and text
input eligibility are preserved. This is source and callback-pattern validation, not a claim
that every ecode interface received interactive end-to-end QA.

The [keyboard migration guide](../../docs/articles/uiaccessibility.md#keyboard-navigation)
documents custom-widget opt-in and the deliberately deferred policies.

## Validation

- Linux Release: SDL3, Ninja, debug symbols, no ASAN; all release targets including ecode build.
- New `TabFocus.*`: 14/14 passed; full Linux suite: 1695/1695 passed.
- AT-SPI: normal (53 nodes), multi-window (57), close-primary (57), policy-reset (53) passed
  sequentially in isolated D-Bus sessions, with hidden windows on the host display; no Xvfb.
- MinGW Release SDL3: library, unit tests, Windows provider harness and accessibility example build.
- Wine: 14/14 TabFocus tests, 81/81 accessibility tests and all four provider tests passed.
- Native Windows/macOS execution and CI for the uncommitted patch remain unverified.
- Formatting and `git diff --check` pass.

Final performance guards were run last, with the CPU-idle monitor rejecting overlapping work.
The accepted inactive run passed the unchanged 1.01
notification-ratio threshold: 59.674 ms disabled vs 59.940 ms enabled for 100,000 operations
(1.004458x). Wall ratio was 1.012592x and passed its existing guard. This is an enabled/disabled
comparison within the same build, not a regression measurement against develop.

The three-sample dormant allocation guards passed. Counts/bytes match the pre-task measurements
for deep/nested hierarchies, typing, notifications, list scrolling, large deletion and spinbox
construction/destruction. Examples: typing 17,840 allocations / 2,540,540 bytes; large deletion
6 / 20,711,832; spinbox construction 200,004 / 27,671,234; spinbox destruction 17 / 38,552;
list scrolling and post-disconnect model mutations 0 / 0. These counters cover ordinary C++
new/new[] on the measuring thread, not malloc or background work.

## Changed files

Headers:

- `include/eepp/ui/uiwidget.hpp`
- `include/eepp/ui/uicheckbox.hpp`
- `include/eepp/ui/uiradiobutton.hpp`
- `include/eepp/ui/uispinbox.hpp`
- `include/eepp/ui/uilistboxitem.hpp`
- `include/eepp/ui/uimenubar.hpp`
- `include/eepp/ui/uitab.hpp`
- `include/eepp/ui/uitablecell.hpp`
- `include/eepp/ui/uitextview.hpp`
- `include/eepp/ui/uitextspan.hpp`

Implementation:

- `src/eepp/ui/uiwidget.cpp`
- `src/eepp/ui/uipushbutton.cpp`
- `src/eepp/ui/uicheckbox.cpp`
- `src/eepp/ui/uiradiobutton.cpp`
- `src/eepp/ui/uitextinput.cpp`
- `src/eepp/ui/uicodeeditor.cpp`
- `src/eepp/ui/uiconsole.cpp`
- `src/eepp/ui/uislider.cpp`
- `src/eepp/ui/uiscrollbar.cpp`
- `src/eepp/ui/uispinbox.cpp`
- `src/eepp/ui/uicombobox.cpp`
- `src/eepp/ui/uidropdown.cpp`
- `src/eepp/ui/uidropdownlist.cpp`
- `src/eepp/ui/uidropdownmodellist.cpp`
- `src/eepp/ui/uilistbox.cpp`
- `src/eepp/ui/uilistboxitem.cpp`
- `src/eepp/ui/uicalendar.cpp`
- `src/eepp/ui/uidatepicker.cpp`
- `src/eepp/ui/uimenuitem.cpp`
- `src/eepp/ui/uimenubar.cpp`
- `src/eepp/ui/uimenu.cpp`
- `src/eepp/ui/uitab.cpp`
- `src/eepp/ui/uitextview.cpp`
- `src/eepp/ui/uitextspan.cpp`
- `src/eepp/ui/abstract/uiabstractview.cpp`
- `src/eepp/ui/accessibility/accessibilitywidgetresolver.cpp`

Tests/documentation:

- `src/tests/unit_tests/ui_tab_focus_tests.cpp` (new)
- `src/tests/unit_tests/accessibility_tests.cpp` (custom interactive test widget explicitly opts in)
- `docs/articles/uiaccessibility.md`
- `.agents/plans/tab_focus_defaults_implementation_report.md` (this report)
