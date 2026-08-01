# eTerm: Kitty Keyboard Protocol Progressive Enhancement — Detailed Implementation Plan

## Document status

- **Target repository:** `SpartanJ/eepp`
- **Target branch:** `develop`
- **Primary module:** `src/modules/eterm`
- **Primary goal:** Implement Kitty keyboard protocol progressive enhancement in eTerm so modern terminal applications can distinguish key combinations that legacy terminal encoding collapses, especially `Ctrl+Enter`, `Shift+Enter`, modified Escape, modified Tab, modified Backspace, and modified printable keys.
- **Initial motivating application:** OpenAI Codex CLI, which uses Crossterm keyboard enhancement flags.
- **Expected result for the motivating case:** When Codex enables the Kitty keyboard protocol and the user presses `Ctrl+Enter`, eTerm must send `CSI 13;5u` (`ESC [ 1 3 ; 5 u`) instead of the same carriage-return byte sent for plain Enter.
- **Compatibility requirement:** When no application has enabled the protocol, eTerm must preserve its existing legacy key behavior byte-for-byte.

---

# 1. Executive summary

eTerm currently receives keyboard modifiers correctly from EEPP, but its terminal key encoder deliberately maps many variants of Enter to the same legacy byte:

```cpp
{ KEY_RETURN, KEYMOD_CTRL_SHIFT_ALT_META, "\r", 0, 0 },
```

The key map treats `KEYMOD_CTRL_SHIFT_ALT_META` as a wildcard. Consequently, plain Enter, Ctrl+Enter, Shift+Enter, and Ctrl+Shift+Enter all produce `\r` unless a more specific entry matched first. A terminal application therefore cannot distinguish those events.

The proper fix is not an unconditional special case for Codex. eTerm must implement the Kitty keyboard protocol’s **progressive enhancement** mechanism:

1. Parse requests from the child application to query, push, set, or pop keyboard enhancement flags.
2. Store enhancement state per terminal screen, with stack behavior.
3. Reply to support queries.
4. Encode keyboard events according to the active flags.
5. Continue using existing legacy mappings when the protocol is inactive.
6. Support press, repeat, and release events when requested.
7. Avoid duplicate input from EEPP key and text-input event paths.
8. Add deterministic unit tests for parser state, state stacks, and output bytes.
9. Add integration tests against a PTY and Crossterm/Codex-like sequences.

The implementation should be split into four logical pieces:

- **Protocol/state model** in `TerminalEmulator`
- **CSI command parsing and responses** in `TerminalEmulator::csihandle()`
- **Key event encoding** in a focused encoder class/helper used by `TerminalDisplay`
- **Input event plumbing** in `UITerminal` / `TerminalDisplay`, including key-up and repeat support

Do not mix the new protocol rules into the existing large `keys[]` table. Keep the legacy table intact as a fallback and add a separate enhanced encoder that runs before it when active.

---

# 2. Scope

## 2.1 In scope

The implementation must support the complete progressive-enhancement control surface and enough event encoding to be considered a robust Kitty keyboard protocol implementation:

### Protocol control

- Query current flags: `CSI ? u`
- Push flags: `CSI > flags u`
- Pop flags: `CSI < number u`
- Set flags: `CSI = flags ; mode u`
- Correct response to query: `CSI ? flags u`
- Independent state for primary and alternate screen
- Bounded push/pop stacks
- Reset behavior on terminal reset and process replacement

### Enhancement flags

Support these standardized bits:

```text
1   DISAMBIGUATE_ESCAPE_CODES
2   REPORT_EVENT_TYPES
4   REPORT_ALTERNATE_KEYS
8   REPORT_ALL_KEYS_AS_ESCAPE_CODES
16  REPORT_ASSOCIATED_TEXT
```

Even if some EEPP backends cannot provide all data for flags 4 or 16, the implementation must:

- parse and store the bits,
- advertise only what eTerm actually supports,
- emit the best correct representation available,
- never emit malformed approximations.

### Keyboard event encoding

Support:

- Printable Unicode keys
- Enter
- Tab
- Backspace
- Escape
- Arrow keys
- Home / End
- Insert / Delete
- Page Up / Page Down
- Function keys
- Keypad keys
- Modifier keys, when identifiable
- Lock keys, when identifiable
- Menu / Print Screen / Pause, when identifiable
- Press events
- Repeat events
- Release events
- Shift, Alt, Ctrl, Super/Meta, Hyper where EEPP exposes them
- Caps Lock and Num Lock modifier state where EEPP exposes it

### Compatibility

- Existing behavior unchanged while enhancement flags are zero
- Existing terminal shortcuts remain higher priority than forwarding
- Existing application keypad/cursor modes remain functional in legacy mode
- No unsolicited Kitty sequences
- No protocol activation based on `$TERM`, executable name, or heuristic

### Testing and documentation

- Unit tests for every control sequence
- Unit tests for state stack semantics
- Table-driven encoder tests
- PTY integration tests
- Manual test procedure for Codex CLI
- Debug logging option suitable for protocol diagnosis
- Documentation of supported flags and known backend limitations

## 2.2 Explicitly out of scope for the first merge

These may be follow-up work unless trivial:

- Modifying tmux, screen, SSH, or other intermediaries
- Implementing terminal-side key remapping configuration
- Inventing eTerm-specific keyboard escape sequences
- Changing Codex keybindings
- Supporting raw physical-key identifiers beyond the Kitty specification
- Perfect reporting of alternate keys on platforms where EEPP does not expose the necessary layout data
- Perfect associated-text reporting for dead-key/IME composition before EEPP exposes committed text and source key correlation

However, the architecture must not block these features.

---

# 3. Authoritative protocol references

The implementing agent must consult the current official Kitty specification while coding:

- Kitty keyboard protocol:
  `https://sw.kovidgoyal.net/kitty/keyboard-protocol/`
- Progressive enhancement section:
  `https://sw.kovidgoyal.net/kitty/keyboard-protocol/#progressive-enhancement`
- Key codes:
  `https://sw.kovidgoyal.net/kitty/keyboard-protocol/#key-codes`
- Modifiers:
  `https://sw.kovidgoyal.net/kitty/keyboard-protocol/#modifiers`
- Event types:
  `https://sw.kovidgoyal.net/kitty/keyboard-protocol/#event-types`

Also use Crossterm as the immediate compatibility target:

- `KeyboardEnhancementFlags`
- `PushKeyboardEnhancementFlags`
- `PopKeyboardEnhancementFlags`

Crossterm currently defines:

```text
DISAMBIGUATE_ESCAPE_CODES        = 1
REPORT_EVENT_TYPES               = 2
REPORT_ALTERNATE_KEYS            = 4
REPORT_ALL_KEYS_AS_ESCAPE_CODES  = 8
REPORT_ASSOCIATED_TEXT           = 16
```

Codex has used a combination including disambiguation, event types, and alternate keys. eTerm must be able to process combinations, not only flag `1`.

---

# 4. Current eTerm architecture and exact integration points

## 4.1 Event flow

The current flow is approximately:

```text
EEPP Window/Input backend
    -> UI event dispatcher
        -> UITerminal::onKeyDown(KeyEvent)
            -> TerminalDisplay::onKeyDown(keyCode, char, mod, scancode)
                -> terminal shortcuts
                -> Ctrl-letter handling
                -> legacy TerminalKeyMap
                -> PTY ttywrite()
```

Text input follows a separate path:

```text
EEPP text input event
    -> UITerminal::onTextInput(TextInputEvent)
        -> TerminalDisplay::onTextInput(codepoint)
            -> UTF-8 conversion
            -> PTY ttywrite()
```

This split is important. Printable characters are normally emitted by `onTextInput()`, while special/control keys are emitted by `onKeyDown()`. Kitty’s `REPORT_ALL_KEYS_AS_ESCAPE_CODES` changes this assumption: printable keys may need to be emitted from the key-event path as CSI-u events, and the later text-input event must then be suppressed to prevent duplicate characters.

## 4.2 Existing legacy keyboard map

`src/modules/eterm/src/eterm/terminal/terminaldisplay.cpp` contains:

- `TerminalKeyMap`
- `shortcuts[]`
- `keys[]`
- `platformKeys[]`
- `TerminalDisplay::onKeyDown()`

The current key table includes:

```cpp
{ KEY_RETURN, KEYMOD_LALT, "\033\r", 0, 0 },
{ KEY_RETURN, KEYMOD_CTRL_SHIFT_ALT_META, "\r", 0, 0 },
```

and:

```cpp
{ SCANCODE_RETURN, KEYMOD_CTRL_SHIFT_ALT_META, "\r", 0, 0 },
```

The wildcard comparison is:

```cpp
if ( k.mask == KEYMOD_CTRL_SHIFT_ALT_META || k.mask == smod )
```

This is why modified Enter collapses to carriage return.

## 4.3 Existing parser support placeholder

`TerminalEmulator::csihandle()` already has a branch similar to:

```cpp
case '=': /* Progressive enhancement sequences */
    /* Keyboard protocol ESC[=Nu */
    /* Do nothing for the moment */
    break;
```

This is a strong signal that progressive enhancement belongs in `TerminalEmulator`, not in the UI widget.

The CSI parser stores:

```cpp
struct CSIEscape {
    char buf[ESC_BUF_SIZ];
    size_t len;
    char priv;
    int arg[ESC_ARG_SIZ];
    int narg;
    char mode[2];
};
```

Before implementation, verify exactly how these sequences are represented after `csiparse()`:

```text
ESC [ ? u
ESC [ > 7 u
ESC [ < u
ESC [ < 2 u
ESC [ = 7 u
ESC [ = 7 ; 1 u
ESC [ = 7 ; 2 u
ESC [ = 7 ; 3 u
```

Do not assume that `?`, `>`, `<`, and `=` always land in the same field. The current `csihandle()` switch structure suggests some private/intermediate bytes may be placed into `mode[0]` or `priv`. Add parser tests first and adjust the handler according to observed representation.

## 4.4 Screen swapping

`TerminalEmulator` owns primary and alternate screen state and implements:

```cpp
void tswapscreen();
int tisaltscr();
```

Kitty keyboard enhancement stacks must be independent for primary and alternate screens. The cleanest implementation is either:

- store keyboard state inside each screen-owned structure, or
- store two explicit state objects indexed by active screen.

Do not use one global stack shared across both screens.

---

# 5. Proposed code organization

Create a focused implementation rather than expanding `terminaldisplay.cpp` indefinitely.

Recommended files:

```text
src/modules/eterm/include/eterm/terminal/kittykeyboardprotocol.hpp
src/modules/eterm/src/eterm/terminal/kittykeyboardprotocol.cpp
```

Possible test file:

```text
src/modules/eterm/tests/kittykeyboardprotocol_test.cpp
```

Adapt to the repository’s actual test layout and build system.

## 5.1 `kittykeyboardprotocol.hpp`

This header should contain:

- Flag enum
- Supported-mask constant
- Event-type enum
- Key event data structure
- Keyboard state structure
- Encoder result
- Encoder class/free functions
- Modifier conversion helpers
- Functional key constants or lookup declarations

Suggested API:

```cpp
#ifndef ETERM_KITTYKEYBOARDPROTOCOL_HPP
#define ETERM_KITTYKEYBOARDPROTOCOL_HPP

#include <eepp/window/keycodes.hpp>
#include <eepp/window/scancode.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace eterm::Terminal {

enum class KittyKeyboardFlag : std::uint32_t {
    DisambiguateEscapeCodes       = 1u << 0,
    ReportEventTypes              = 1u << 1,
    ReportAlternateKeys           = 1u << 2,
    ReportAllKeysAsEscapeCodes    = 1u << 3,
    ReportAssociatedText          = 1u << 4,
};

constexpr std::uint32_t kittyFlag( KittyKeyboardFlag flag ) {
    return static_cast<std::uint32_t>( flag );
}

constexpr std::uint32_t KITTY_KEYBOARD_KNOWN_FLAGS =
    kittyFlag( KittyKeyboardFlag::DisambiguateEscapeCodes ) |
    kittyFlag( KittyKeyboardFlag::ReportEventTypes ) |
    kittyFlag( KittyKeyboardFlag::ReportAlternateKeys ) |
    kittyFlag( KittyKeyboardFlag::ReportAllKeysAsEscapeCodes ) |
    kittyFlag( KittyKeyboardFlag::ReportAssociatedText );

enum class KittyKeyEventType : std::uint8_t {
    Press = 1,
    Repeat = 2,
    Release = 3,
};

struct KittyKeyboardState {
    std::uint32_t flags{ 0 };
    std::vector<std::uint32_t> stack;

    void reset();
    void push( std::uint32_t requestedFlags );
    void pop( std::size_t count = 1 );
    void set( std::uint32_t requestedFlags, unsigned int mode );
};

struct KittyKeyEvent {
    EE::Window::Keycode keycode;
    EE::Window::Scancode scancode;
    std::uint32_t character{ 0 };
    std::uint32_t modifiers{ 0 };
    KittyKeyEventType type{ KittyKeyEventType::Press };

    // Optional alternate/shifted/base-layout codepoints.
    std::optional<std::uint32_t> shiftedCodepoint;
    std::optional<std::uint32_t> baseLayoutCodepoint;

    // Optional committed associated text, represented as Unicode codepoints.
    std::vector<std::uint32_t> associatedText;
};

struct KittyEncodedKey {
    bool handled{ false };
    bool suppressTextInput{ false };
    std::string bytes;
};

class KittyKeyboardEncoder {
  public:
    static KittyEncodedKey encode(
        const KittyKeyEvent& event,
        std::uint32_t activeFlags
    );

    static std::uint32_t encodeModifiers( std::uint32_t eeppModifiers );
};

} // namespace eterm::Terminal

#endif
```

Names may be adjusted to project style, but maintain clear separation between:

- protocol state,
- incoming EEPP event,
- encoding decision,
- output bytes,
- duplicate-text suppression.

## 5.2 Keep protocol state owned by `TerminalEmulator`

`TerminalEmulator` receives application output and therefore owns negotiation state.

Add:

```cpp
KittyKeyboardState mPrimaryKeyboardState;
KittyKeyboardState mAlternateKeyboardState;
```

Add public read-only accessors and event-state operations:

```cpp
std::uint32_t getKeyboardEnhancementFlags() const;
bool hasKeyboardEnhancementFlag( KittyKeyboardFlag flag ) const;
```

Potentially add:

```cpp
const KittyKeyboardState& getKeyboardState() const;
KittyKeyboardState& getKeyboardState();
```

Keep mutating access private if possible.

`TerminalDisplay` should only query active flags. It must not parse application control sequences or mutate protocol stacks directly.

## 5.3 Consider moving `sanitizeMod()`

The current `sanitizeMod()` is a local static function in `terminaldisplay.cpp`. The Kitty encoder also needs normalized modifiers. Either:

- expose a shared helper, or
- keep a Kitty-specific normalization function.

Do not change legacy normalization semantics accidentally. For Kitty encoding, preserve every modifier that the protocol supports and EEPP exposes.

---

# 6. Protocol state model

## 6.1 Known, supported, and requested flags

Distinguish these concepts:

```cpp
constexpr std::uint32_t KITTY_KEYBOARD_KNOWN_FLAGS = ...;
constexpr std::uint32_t KITTY_KEYBOARD_SUPPORTED_FLAGS = ...;
```

`KNOWN_FLAGS` means the parser recognizes the bit.

`SUPPORTED_FLAGS` means eTerm can correctly honor it.

At initial implementation, the supported mask should likely be:

```cpp
DisambiguateEscapeCodes
| ReportEventTypes
| ReportAllKeysAsEscapeCodes
```

Add `ReportAlternateKeys` only if EEPP can reliably supply shifted/base-layout key values. Add `ReportAssociatedText` only when the implementation can correlate committed text with the originating key event without duplication or loss.

However, for Codex compatibility, determine whether Crossterm requires the terminal to echo all requested bits or merely accepts the subset returned by the query. The preferred behavior is standards-compliant subset support, not falsely claiming unsupported features.

A safe first complete target is to implement all five flags to the degree required by the spec, with documented backend limitations.

## 6.2 Query semantics

When the child sends:

```text
CSI ? u
```

eTerm must respond through the PTY input channel with:

```text
CSI ? <flags> u
```

For example:

```text
ESC [ ? 31 u
```

if all five flags are supported.

Use `ttywrite()` or the appropriate low-level response method with echo disabled. Follow existing device-status response patterns.

Suggested helper:

```cpp
void TerminalEmulator::replyKeyboardEnhancementFlags() {
    char buf[32];
    const int len = std::snprintf(
        buf, sizeof( buf ), "\033[?%uu",
        KITTY_KEYBOARD_SUPPORTED_FLAGS
    );
    ttywrite( buf, static_cast<std::size_t>( len ), 0 );
}
```

Important: verify from the official spec whether the query response reports **supported flags** or **currently active flags**. Implement exactly the current spec. Do not infer from naming. Add a test matching Kitty and Crossterm behavior.

## 6.3 Push semantics

For:

```text
CSI > flags u
```

- Push the current active flags onto the current screen’s stack.
- Set active flags to `flags & KITTY_KEYBOARD_SUPPORTED_FLAGS`.
- Preserve unknown requested bits nowhere unless the spec explicitly requires round-tripping.
- A missing argument is interpreted according to the official spec, normally zero.
- Bound the stack depth.

Suggested implementation:

```cpp
void KittyKeyboardState::push( std::uint32_t requestedFlags ) {
    constexpr std::size_t MAX_DEPTH = 64;

    if ( stack.size() == MAX_DEPTH ) {
        // Drop oldest or ignore push; choose behavior matching Kitty.
        stack.erase( stack.begin() );
    }

    stack.push_back( flags );
    flags = requestedFlags & KITTY_KEYBOARD_SUPPORTED_FLAGS;
}
```

Before choosing overflow behavior, inspect Kitty’s reference implementation. Prefer matching it exactly. A fixed maximum avoids unbounded memory growth from hostile terminal output.

## 6.4 Pop semantics

For:

```text
CSI < number u
```

- Default number should be `1`.
- Pop up to `number` levels from the current screen’s stack.
- If the stack underflows, restore zero/default flags and remain stable.
- Never crash on zero, negative, huge, or malformed values.
- Clamp unreasonable counts before looping.

Suggested:

```cpp
void KittyKeyboardState::pop( std::size_t count ) {
    if ( count == 0 )
        count = 1;

    while ( count-- > 0 ) {
        if ( stack.empty() ) {
            flags = 0;
            return;
        }
        flags = stack.back();
        stack.pop_back();
    }
}
```

Confirm exact zero semantics from the official specification.

## 6.5 Set semantics

The protocol includes:

```text
CSI = flags ; mode u
```

The `mode` controls how `flags` affects current state. Implement all standardized modes exactly.

Expected conceptual operations are generally:

- replace/set absolute flags,
- set/add bits,
- clear/remove bits.

Do not hardcode these assumptions without checking the current official table.

Implement through one function:

```cpp
void KittyKeyboardState::set(
    std::uint32_t requestedFlags,
    unsigned int mode
) {
    const auto supported =
        requestedFlags & KITTY_KEYBOARD_SUPPORTED_FLAGS;

    switch ( mode ) {
        case /* set */:
            flags = supported;
            break;
        case /* OR */:
            flags |= supported;
            break;
        case /* clear */:
            flags &= ~supported;
            break;
        default:
            // Ignore unsupported mode.
            break;
    }
}
```

Tests must cover every mode and unknown modes.

## 6.6 Primary versus alternate screen

Maintain:

```cpp
KittyKeyboardState& TerminalEmulator::activeKeyboardState() {
    return tisaltscr()
        ? mAlternateKeyboardState
        : mPrimaryKeyboardState;
}
```

But be careful: `tisaltscr()` may depend on state during a screen transition. Ensure a push sent immediately before/after `DECSET 1049` lands on the expected screen.

Required scenario:

1. Primary flags = 1
2. Enter alternate screen
3. Alternate flags initially = 0
4. Push flags = 3 in alternate
5. Return primary
6. Primary flags still = 1
7. Re-enter alternate
8. Determine whether alternate state persists or resets according to Kitty behavior

The official protocol says flag stacks are maintained separately for the main and alternate screens. Match Kitty’s persistence semantics exactly.

## 6.7 Reset semantics

Reset both states when:

- terminal emulator is constructed,
- full terminal reset (`RIS`, `ESC c`) occurs,
- process/PTY is replaced for terminal restart,
- terminal object is destroyed.

Decide, based on the spec/reference terminal, whether a soft reset clears keyboard flags. Test this separately.

Do not leave enhancement flags active when a new shell is spawned in the same terminal display after a previous enhanced application exits unexpectedly.

---

# 7. CSI parser implementation

## 7.1 Add parser characterization tests first

Before modifying `csihandle()`, create tests that feed these exact byte strings into `TerminalEmulator`:

```cpp
"\033[?u"
"\033[>1u"
"\033[>7u"
"\033[<u"
"\033[<2u"
"\033[=1u"
"\033[=1;1u"
"\033[=1;2u"
"\033[=1;3u"
```

Expose or instrument parsed `CSIEscape` only in tests if needed. Capture:

- `priv`
- `arg[]`
- `narg`
- `mode[0]`
- `mode[1]`
- raw buffer

This prevents implementing against an incorrect mental model of the inherited `st` parser.

## 7.2 Add a dedicated handler

Avoid putting all semantics directly into `csihandle()`.

Add:

```cpp
bool TerminalEmulator::handleKittyKeyboardProtocol();
```

or:

```cpp
bool TerminalEmulator::handleKeyboardEnhancementCSI(
    char prefix,
    const int* args,
    int narg,
    char final
);
```

`csihandle()` should dispatch and return if handled.

Pseudo-structure:

```cpp
void TerminalEmulator::csihandle() {
    // Existing parser setup...

    if ( isKittyKeyboardSequence( mCsiescseq ) ) {
        handleKittyKeyboardSequence( mCsiescseq );
        return;
    }

    switch ( ... ) {
        // Existing behavior...
    }
}
```

This is preferable to abusing the existing `case 'u'` restore-cursor handling.

## 7.3 Preserve `CSI u` restore-cursor behavior

Legacy:

```text
CSI u
```

means restore cursor.

Kitty controls have one of the private prefixes:

```text
CSI ? u
CSI > ... u
CSI < ... u
CSI = ... u
```

Therefore dispatch only when a prefix is present. Bare `CSI u` must continue restoring the cursor exactly as before.

## 7.4 Validation rules

For each sequence:

- Reject extra unsupported intermediate bytes.
- Clamp integer arguments before conversion to unsigned.
- Treat absent arguments according to spec.
- Ignore malformed sequences silently in release builds.
- Optionally log malformed sequences in debug mode.
- Never treat a negative parsed argument as a huge unsigned number.
- Never allocate based on untrusted argument magnitude.
- Never loop billions of times for `CSI < 2147483647 u`.

Suggested clamp:

```cpp
constexpr unsigned int MAX_POP_COUNT = 64;
```

## 7.5 Query response path

The control sequence is application output; the response must be terminal input.

Use the same response path as device status reports. Ensure:

- no local echo,
- no screen rendering,
- no data callback confusion,
- PTY write is immediate enough for Crossterm’s capability probe.

## 7.6 Debug tracing

Add optional protocol tracing controlled by a compile-time flag or existing logger level:

```text
eterm kitty-kbd: query -> response flags=31
eterm kitty-kbd: push requested=7 effective=7 depth=1 screen=primary
eterm kitty-kbd: pop count=1 effective=0 depth=0 screen=primary
eterm kitty-kbd: encode key=RETURN mods=CTRL event=press -> ESC[13;5u
```

Do not log every key at normal log levels.

---

# 8. Key event model and EEPP plumbing

## 8.1 Add key-up forwarding

Current `UITerminal::onKeyUp()` returns `1` without forwarding:

```cpp
Uint32 UITerminal::onKeyUp( const KeyEvent& ) {
    return 1;
}
```

To implement release reporting:

```cpp
Uint32 UITerminal::onKeyUp( const KeyEvent& event ) {
    if ( mTerm ) {
        mTerm->onKeyUp(
            event.getKeyCode(),
            event.getChar(),
            event.getMod(),
            event.getScancode()
        );
    }
    return 1;
}
```

Add `TerminalDisplay::onKeyUp()`.

Only emit a release sequence when:

```cpp
activeFlags & REPORT_EVENT_TYPES
```

and the key is representable.

When event types are disabled, preserve current behavior and send nothing on key-up.

## 8.2 Determine repeat information

Inspect `KeyEvent` and backend APIs for:

- explicit repeat property,
- repeated keydown indicator,
- OS repeat events represented as repeated keydown calls,
- key state table.

If available, map:

```text
initial keydown -> event type 1
autorepeat keydown -> event type 2
keyup -> event type 3
```

If EEPP does not expose repeat status:

1. Add a repeat boolean/event kind to `KeyEvent`, populated by SDL and other backends.
2. Avoid heuristic timers if possible.
3. As a temporary fallback, report every keydown as press rather than falsely reporting repeat.
4. Document `REPORT_EVENT_TYPES` as partial until repeat is available.

Do not maintain a simple “currently pressed keys” set and classify any second keydown as repeat without considering missed key-up, focus loss, and backend behavior, unless tests establish this is reliable.

## 8.3 Focus loss cleanup

If maintaining key state:

- Clear pressed-key tracking on terminal focus loss.
- Clear on window focus loss.
- Clear on full reset/process restart.
- Do not synthesize release events on focus loss unless the protocol/reference behavior requires it.

## 8.4 Preserve terminal shortcuts

Current eTerm intercepts shortcuts such as copy, paste, scrolling, and font size.

Priority should remain:

1. eTerm/UI command binding
2. Terminal-local shortcut action
3. Kitty protocol forwarding
4. Legacy control-letter conversion
5. Legacy key map
6. Text-input fallback

A shortcut consumed by the terminal itself must not also be emitted to the child.

## 8.5 Exclusive mode

`UITerminal` has `mExclusiveMode`. Maintain its behavior. The enhanced encoder should run only after UI command handling has decided the event belongs to the child.

---

# 9. Modifier encoding

Kitty modifier values use a one-based encoded mask:

```text
modifier parameter = 1 + bitmask
```

The bitmask contains:

```text
1   Shift
2   Alt
4   Ctrl
8   Super
16  Hyper
32  Meta
64  Caps Lock
128 Num Lock
```

Implement using named constants:

```cpp
enum KittyModifierBit : std::uint32_t {
    KittyShift    = 1u << 0,
    KittyAlt      = 1u << 1,
    KittyCtrl     = 1u << 2,
    KittySuper    = 1u << 3,
    KittyHyper    = 1u << 4,
    KittyMeta     = 1u << 5,
    KittyCapsLock = 1u << 6,
    KittyNumLock  = 1u << 7,
};

std::uint32_t KittyKeyboardEncoder::encodeModifiers(
    std::uint32_t mod
) {
    std::uint32_t bits = 0;

    if ( mod & KEYMOD_SHIFT )
        bits |= KittyShift;

    if ( mod & ( KEYMOD_LALT | KEYMOD_RALT ) )
        bits |= KittyAlt;

    if ( mod & KEYMOD_CTRL )
        bits |= KittyCtrl;

    // Map EEPP's exact semantics carefully:
    if ( mod & KEYMOD_SUPER )
        bits |= KittySuper;

    if ( mod & KEYMOD_HYPER )
        bits |= KittyHyper;

    if ( mod & KEYMOD_META )
        bits |= KittyMeta;

    if ( mod & KEYMOD_CAPS )
        bits |= KittyCapsLock;

    if ( mod & KEYMOD_NUM )
        bits |= KittyNumLock;

    return bits + 1;
}
```

The actual EEPP constant names must be inspected. Do not alias Super and Meta blindly. On macOS, Command may appear as GUI/Super/Meta depending on SDL. Document and test per platform.

## 9.1 AltGr

Right Alt may represent AltGr. Kitty can report alternate layout information. Determine current EEPP behavior:

- Does `KEYMOD_RALT` arrive alone?
- Does SDL report Ctrl+Alt for AltGr?
- Is a text-input event generated with the composed character?

Do not convert AltGr-generated printable text into an accidental Ctrl+Alt command sequence. For `REPORT_ALL_KEYS_AS_ESCAPE_CODES`, prefer the committed Unicode key/text data when available.

---

# 10. Key-code mapping

## 10.1 Printable keys

For a printable key, the primary key number is normally its Unicode codepoint.

Examples:

```text
a          -> 97
A          -> base/shifted representation depending flags and event data
é          -> 233
Enter      -> 13
Tab        -> 9
Escape     -> 27
Backspace  -> 127
```

Do not use UTF-8 bytes as key numbers. Use Unicode scalar values.

Validate:

```cpp
codepoint <= 0x10FFFF
codepoint is not surrogate range 0xD800..0xDFFF
```

## 10.2 Functional key constants

Kitty assigns private-use codepoints to non-text functional keys. Create a table based exactly on the official specification.

Example structure:

```cpp
struct FunctionalKeyMapping {
    Keycode keycode;
    Scancode scancode;
    std::uint32_t kittyCode;
};
```

Prefer scancode for physical/function keys when keycode is ambiguous, but document precedence.

Required mapping categories:

- Escape
- Enter
- Tab
- Backspace
- Insert
- Delete
- Left / Right / Up / Down
- Page Up / Page Down
- Home / End
- Caps Lock
- Scroll Lock
- Num Lock
- Print Screen
- Pause
- Menu
- F1 through the highest EEPP-supported function key
- Keypad digits
- Keypad decimal
- Keypad divide/multiply/subtract/add
- Keypad Enter
- Keypad equal/separator where supported
- Left/right Shift
- Left/right Ctrl
- Left/right Alt
- Left/right Super
- Left/right Hyper
- Left/right Meta
- ISO level shifts where available
- Media keys where EEPP exposes them

Do not invent code values. Copy them from the official key-code table and add a comment with the reference URL.

## 10.3 Enter special case

With `DISAMBIGUATE_ESCAPE_CODES` enabled:

- Plain Enter may retain legacy `\r` unless another active flag requires CSI-u.
- Modified Enter must be encoded as CSI-u.
- Ctrl+Enter must become:

```text
CSI 13 ; 5 u
```

because Ctrl’s modifier mask is 4 and the encoded modifier parameter is `4 + 1 = 5`.

Shift+Enter:

```text
CSI 13 ; 2 u
```

Alt+Enter:

```text
CSI 13 ; 3 u
```

Ctrl+Shift+Enter:

```text
CSI 13 ; 6 u
```

When `REPORT_ALL_KEYS_AS_ESCAPE_CODES` is active, plain Enter should also use the protocol form required by the spec.

## 10.4 Legacy cursor/function-key forms under enhancement

Kitty preserves certain traditional CSI forms for named keys while adding modifier/event parameters. Follow the exact encoding rules:

- Some keys use `CSI 1;mod A/B/C/D/...`
- Some use `CSI number;mod ~`
- CSI-u is used for Unicode and functional-key codepoints in specified cases

Do not simplistically encode every key as `CSI code;mod u` unless the specification explicitly allows/requires that representation under the active flag combination.

Implement the official algorithm, not a Codex-only subset.

---

# 11. Sequence serialization

## 11.1 General CSI-u shape

The full shape is conceptually:

```text
CSI key[:alternate...] ; modifiers[:event-type] ; text... u
```

Fields are conditionally omitted according to flags and defaults.

Create a serializer with explicit components rather than `snprintf()` branches spread across key handling:

```cpp
struct KittySequence {
    std::uint32_t key;
    std::optional<std::uint32_t> shiftedKey;
    std::optional<std::uint32_t> baseLayoutKey;
    std::uint32_t modifiers{ 1 };
    std::optional<KittyKeyEventType> eventType;
    std::vector<std::uint32_t> associatedText;
};

std::string serializeKittySequence( const KittySequence& seq );
```

Serializer requirements:

- No empty illegal subfields
- No trailing separators
- Correct omission of default modifier/event fields
- Correct colon versus semicolon separators
- Bounded output length
- No locale-sensitive number formatting
- No dynamic formatting vulnerabilities

## 11.2 Event type

When `REPORT_EVENT_TYPES` is active, encode:

```text
1 press
2 repeat
3 release
```

in the event-type subfield as specified.

When inactive:

- Never emit repeat/release events.
- Press encoding must use the shorter compatible form.

## 11.3 Alternate keys

When `REPORT_ALTERNATE_KEYS` is active and data is available:

- primary codepoint
- shifted codepoint
- base-layout codepoint

must be serialized in the exact colon-separated form.

Use absent optional values correctly; do not substitute zero unless the protocol says zero means unavailable.

## 11.4 Associated text

When `REPORT_ASSOCIATED_TEXT` is active:

- include Unicode codepoints of committed associated text,
- include multiple codepoints for composed text,
- never include uncommitted IME preedit text,
- never emit text twice through `onTextInput()`.

This feature should be implemented only after input-event correlation is sound.

---

# 12. Progressive enhancement decision algorithm

Implement one deterministic function that decides enhanced versus legacy output.

Pseudo-code:

```cpp
KittyEncodedKey KittyKeyboardEncoder::encode(
    const KittyKeyEvent& event,
    std::uint32_t flags
) {
    KittyEncodedKey result;

    if ( flags == 0 )
        return result; // not handled; use legacy path

    const bool reportAll =
        flags & REPORT_ALL_KEYS_AS_ESCAPE_CODES;
    const bool disambiguate =
        flags & DISAMBIGUATE_ESCAPE_CODES;
    const bool reportEvents =
        flags & REPORT_EVENT_TYPES;

    if ( event.type != Press && !reportEvents )
        return result;

    auto mapped = mapEventToKittyKey( event );
    if ( !mapped )
        return result;

    const bool hasNonLockModifier =
        hasShiftAltCtrlSuperHyperMeta( event.modifiers );

    const bool legacyAmbiguous =
        isLegacyAmbiguousKey( *mapped, event.modifiers );

    const bool mustEncode =
        reportAll ||
        event.type != Press ||
        ( disambiguate && ( hasNonLockModifier || legacyAmbiguous ) );

    if ( !mustEncode )
        return result;

    result.bytes = serialize...
    result.handled = true;
    result.suppressTextInput = eventMayAlsoGenerateTextInput( event );
    return result;
}
```

The exact `mustEncode` rules must match the spec. The above is architecture, not a substitute for the official rules.

---

# 13. Avoiding duplicate printable input

This is one of the most important implementation hazards.

## 13.1 Problem

For a printable key, EEPP/SDL may deliver:

```text
KeyDown(A)
TextInput("a")
```

If Kitty `REPORT_ALL_KEYS_AS_ESCAPE_CODES` causes `KeyDown(A)` to emit a CSI-u sequence and `TextInput("a")` still emits `a`, the child receives the key twice.

## 13.2 Preferred solution: correlated suppression queue

Add a small queue of expected text-input codepoints after an enhanced keydown consumed the event:

```cpp
struct SuppressedTextInput {
    std::vector<std::uint32_t> codepoints;
    Clock createdAt;
};

std::deque<SuppressedTextInput> mSuppressedTextInputs;
```

When an enhanced key event includes or represents committed text:

- enqueue the expected text,
- on `onTextInput()`, match and consume the queue entry,
- suppress only the matching event,
- expire stale entries quickly,
- clear on focus loss/reset.

However, keydown may not provide the final composed Unicode text. Therefore this approach is reliable only for ordinary keys where keycode/character data is trustworthy.

## 13.3 Alternative: defer printable key emission until text input

For printable keys under `REPORT_ALL_KEYS_AS_ESCAPE_CODES`:

- Record the pending keydown metadata.
- Wait for the corresponding text-input event.
- Build the Kitty event using committed text.
- Emit it once.
- If no text event arrives within the event loop for a non-text functional key, emit immediately.

This is more correct for keyboard layouts and IME but requires careful event ordering and latency handling.

Recommended architecture:

```cpp
std::optional<PendingKittyTextKey> mPendingTextKey;
```

Flow:

1. Printable `onKeyDown` arrives.
2. Store keycode/scancode/modifiers/event type.
3. Do not write yet if committed text is required.
4. `onTextInput` arrives.
5. Construct Unicode/alternate/associated fields.
6. Emit one Kitty sequence.
7. Clear pending state.

For repeat events, verify backend ordering.

## 13.4 IME handling

Current `TerminalDisplay::onKeyDown()` exits while IME is editing:

```cpp
if ( mWindow->getIME().isEditing() )
    return;
```

Preserve IME correctness:

- Do not report raw component keystrokes as committed text.
- Preedit updates remain rendering-only.
- Commit event produces associated text if supported.
- Escape/navigation used to control IME may be consumed by the platform and never forwarded; that is acceptable.

## 13.5 First implementation recommendation

For the first merge:

- Fully support enhanced special/control/function keys.
- Fully support printable keys when `REPORT_ALL_KEYS_AS_ESCAPE_CODES` using a deferred text-input path.
- Add tests for US-layout ASCII and multibyte Unicode.
- Document remaining layout limitations for alternate keys.

Do not ship an implementation that duplicates printable characters.

---

# 14. Changes to `TerminalDisplay`

## 14.1 Add active flag accessor

Use:

```cpp
const auto flags =
    mTerminal->getKeyboardEnhancementFlags();
```

Do not cache flags in `TerminalDisplay`; application output may change them at any moment.

## 14.2 Refactor `onKeyDown()`

Recommended structure:

```cpp
void TerminalDisplay::onKeyDown(
    const Keycode& keyCode,
    const Uint32& chr,
    const Uint32& mod,
    const Scancode& scancode,
    bool isRepeat
) {
    if ( mWindow->getIME().isEditing() )
        return;

    const Uint32 smod = sanitizeMod( mod );

    if ( handleTerminalShortcut( keyCode, smod ) )
        return;

    const KittyKeyEvent event{
        keyCode,
        scancode,
        chr,
        mod,
        isRepeat
            ? KittyKeyEventType::Repeat
            : KittyKeyEventType::Press
    };

    const auto enhanced = KittyKeyboardEncoder::encode(
        event,
        mTerminal->getKeyboardEnhancementFlags()
    );

    if ( enhanced.handled ) {
        mTerminal->ttywrite(
            enhanced.bytes.data(),
            enhanced.bytes.size(),
            1
        );

        if ( enhanced.suppressTextInput )
            registerTextInputSuppression( event );

        return;
    }

    handleLegacyKeyDown( keyCode, chr, mod, scancode );
}
```

Extract current shortcut and legacy map logic into helpers to make tests possible:

```cpp
bool handleTerminalShortcut(...);
void handleLegacyKeyDown(...);
```

## 14.3 Add `onKeyUp()`

```cpp
void TerminalDisplay::onKeyUp(
    const Keycode& keyCode,
    const Uint32& chr,
    const Uint32& mod,
    const Scancode& scancode
) {
    const auto flags =
        mTerminal->getKeyboardEnhancementFlags();

    if ( !( flags & REPORT_EVENT_TYPES ) )
        return;

    KittyKeyEvent event{
        keyCode,
        scancode,
        chr,
        mod,
        KittyKeyEventType::Release
    };

    const auto encoded =
        KittyKeyboardEncoder::encode( event, flags );

    if ( encoded.handled )
        mTerminal->ttywrite(
            encoded.bytes.data(),
            encoded.bytes.size(),
            1
        );
}
```

Terminal shortcuts consumed on keydown should generally not emit release events to the child. Track consumed physical keys until key-up if necessary:

```cpp
std::unordered_set<Scancode> mLocallyConsumedKeys;
```

## 14.4 `onTextInput()`

Refactor:

```cpp
void TerminalDisplay::onTextInput( const Uint32& chr ) {
    if ( !mTerminal )
        return;

    if ( handlePendingKittyTextInput( chr ) )
        return;

    // Existing UTF-8 legacy behavior.
}
```

Ensure a multi-codepoint text-input event is handled correctly. Current API appears to pass one `Uint32`; inspect whether EEPP emits one event per codepoint.

---

# 15. Changes to `UITerminal`

Required changes:

- Forward key-up.
- Forward repeat status if `KeyEvent` exposes it.
- Preserve focus checks.
- Preserve custom keybinding command interception.
- Clear pending Kitty input state on focus loss if there is an existing focus callback.

Possible signature changes:

```cpp
void TerminalDisplay::onKeyDown(
    const Keycode&,
    const Uint32&,
    const Uint32&,
    const Scancode&,
    bool isRepeat = false
);

void TerminalDisplay::onKeyUp(
    const Keycode&,
    const Uint32&,
    const Uint32&,
    const Scancode&
);
```

Update interfaces and all callers.

---

# 16. Legacy behavior rules

When active flags are zero:

- `TerminalDisplay::onKeyDown()` must execute the old code path.
- `onTextInput()` must behave exactly as before.
- `onKeyUp()` must emit nothing.
- Existing `Ctrl+A` through control-code conversion must remain unchanged.
- Alt+Enter must retain `ESC CR`.
- Existing application keypad and cursor mode handling must remain unchanged.
- Existing shortcuts must remain unchanged.
- No query response is emitted unless queried.

Add regression tests comparing old expected output bytes.

---

# 17. Handling Ctrl-letter logic

Current code manually maps Ctrl plus certain scancodes to bytes `0x01` etc. This block must occur **after** enhanced protocol encoding.

Reason:

- Under legacy mode, Ctrl+A should remain byte `0x01`.
- Under `DISAMBIGUATE_ESCAPE_CODES`, modified keys may need CSI-u to remove ambiguity.
- Under `REPORT_ALL_KEYS_AS_ESCAPE_CODES`, Ctrl+A must be CSI-u, not byte `0x01`.

Therefore:

```text
enhanced encoder first
legacy Ctrl mapping second
```

Do not leave the Ctrl block before the enhanced encoder.

---

# 18. Response and input channel correctness

Distinguish:

- Child output is parsed by `TerminalEmulator`.
- Terminal responses and user input are written to the PTY.
- Renderer output must never contain protocol responses.
- `ttywrite(..., may_echo=0)` is likely correct for terminal-generated responses.
- User key input should maintain current `may_echo` semantics, likely `1`.

Audit current uses:

```cpp
ttywrite( buf, len, 0 ); // terminal response
ttywrite( key, len, 1 ); // user input
```

Match established conventions.

---

# 19. Capability advertisement

Do not change `$TERM` immediately unless required.

Kitty progressive enhancement is runtime-negotiated, so applications can query `CSI ? u`.

Potential optional follow-up:

- Add an eTerm-specific terminfo capability/version marker.
- Set an environment variable indicating protocol support only if a convention exists.

Do not claim `TERM=xterm-kitty` merely because the keyboard protocol is supported. That would imply many unrelated capabilities.

---

# 20. Tests

## 20.1 State unit tests

Required table:

### Initial state

```text
primary flags = 0
alternate flags = 0
both stacks empty
```

### Push

```text
push 1 -> flags 1, depth 1, saved 0
push 7 -> flags 7, depth 2, saved 1
```

### Pop

```text
pop 1 -> flags 1, depth 1
pop 1 -> flags 0, depth 0
pop empty -> flags 0
pop huge -> flags 0, no hang
```

### Set operations

Cover every official mode:

```text
initial 3
set/replace 4 -> expected
add 4 -> expected
remove 1 -> expected
unknown mode -> unchanged
unknown flag bits -> masked
```

### Screen independence

Verify primary and alternate stacks never contaminate each other.

### Reset

Verify both state objects return to zero.

## 20.2 Parser tests

Feed exact byte sequences and assert state/response:

```text
CSI ? u
CSI > 1 u
CSI > 7 u
CSI < u
CSI < 2 u
CSI = 3 ; mode u
```

Malformed:

```text
CSI > -1 u
CSI > 999999999999999999999 u
CSI < -1 u
CSI < 0 u
CSI < 999999999 u
CSI = u
CSI = ; u
CSI = 1 ; 999 u
CSI ? 1 u
CSI > 1 ; 2 u
```

Expected behavior should be explicit and non-crashing.

## 20.3 Modifier tests

For each modifier combination, assert parameter:

```text
none             -> 1
shift            -> 2
alt              -> 3
shift+alt        -> 4
ctrl             -> 5
shift+ctrl       -> 6
alt+ctrl         -> 7
shift+alt+ctrl   -> 8
super            -> 9
```

Continue for supported lock/meta bits.

## 20.4 Motivating Enter tests

With no flags:

```text
Enter       -> 0d
Ctrl+Enter  -> 0d  // current legacy behavior retained
```

With disambiguation:

```text
Enter             -> legacy CR, unless spec requires encoded form
Ctrl+Enter        -> 1b 5b 31 33 3b 35 75
Shift+Enter       -> 1b 5b 31 33 3b 32 75
Alt+Enter         -> protocol form required by spec
Ctrl+Shift+Enter  -> protocol form with modifier 6
```

With report-all:

```text
Enter -> enhanced form
```

With event types:

```text
Ctrl+Enter press   -> event type 1
Ctrl+Enter repeat  -> event type 2
Ctrl+Enter release -> event type 3
```

## 20.5 Printable tests

- `a`
- `A`
- Ctrl+A
- Alt+A
- Ctrl+Shift+A
- `é`
- CJK character
- emoji outside BMP
- composed character
- dead-key input if test harness can simulate it

Check no duplicate raw UTF-8 follows enhanced sequence.

## 20.6 Function key tests

Table-driven tests for every mapped special key and every modifier class.

## 20.7 Legacy regression tests

Snapshot bytes from current implementation before refactor for:

- arrows in normal/app cursor modes
- keypad in numeric/app keypad modes
- Home/End
- Insert/Delete
- Page Up/Down
- F1–F12
- Ctrl-letter control bytes
- Alt+Enter
- Shift+Tab
- terminal copy/paste shortcuts not forwarded

Run tests with flags zero and assert identical output.

## 20.8 PTY integration test

Create a test child program or shell command that:

1. Writes `CSI ? u`.
2. Reads terminal response from stdin.
3. Writes `CSI > 7 u`.
4. Waits for key events.
5. Prints received bytes as hex.
6. Writes `CSI < u`.
7. Exits.

Use a mocked display/input or actual pseudo-terminal according to existing test infrastructure.

## 20.9 Crossterm integration fixture

A tiny Rust helper is ideal:

```rust
execute!(
    stdout(),
    PushKeyboardEnhancementFlags(
        KeyboardEnhancementFlags::DISAMBIGUATE_ESCAPE_CODES
            | KeyboardEnhancementFlags::REPORT_EVENT_TYPES
            | KeyboardEnhancementFlags::REPORT_ALTERNATE_KEYS
    )
)?;
```

Then print decoded events.

This validates eTerm against the actual parser used by Codex.

Do not make Rust a mandatory build dependency for normal C++ tests; keep it as an optional integration tool or prebuilt test script.

---

# 21. Manual verification checklist

## 21.1 Raw byte test

Run inside eTerm:

```bash
od -An -t x1
```

Before enhancement activation, verify:

```text
Enter       -> 0d
Ctrl+Enter  -> 0d
```

Use a helper to activate flags:

```bash
printf '\033[>1u'
```

Then press Ctrl+Enter and verify:

```text
1b 5b 31 33 3b 35 75
```

Finally:

```bash
printf '\033[<u'
```

and verify legacy behavior returns.

Note that a shell command printing the push sequence may not be sufficient if shell line discipline or program lifecycle immediately pops/resets state. Prefer the dedicated test helper.

## 21.2 Query

Use a small raw-mode helper to send:

```text
ESC [ ? u
```

and read:

```text
ESC [ ? <supported-mask> u
```

## 21.3 Codex CLI

1. Build and run eTerm from the modified `develop` branch.
2. Start current Codex CLI.
3. Configure Codex:
   - Enter inserts newline.
   - Ctrl+Enter submits.
4. Type a multiline prompt.
5. Press Enter: newline appears.
6. Press Ctrl+Enter: message submits.
7. Hold/repeat keys to ensure no duplicate or stuck events.
8. Exit Codex normally.
9. Verify shell input returns to legacy behavior.
10. Kill Codex abruptly and verify a restarted shell/process does not inherit stale flags.

## 21.4 Other TUIs

Test at least:

- a Crossterm key-event dumper,
- Neovim,
- Vim,
- less,
- bash/readline,
- fish,
- tmux if used,
- ecode embedded terminal.

No regressions should appear when the applications do not enable the protocol.

---

# 22. Fuzzing and robustness

The CSI parser is exposed to arbitrary child output. Add fuzz coverage if the project supports it.

Targets:

- `csiparse()` plus Kitty handler
- serializer
- key mapping with arbitrary keycode/scancode/modifier values

Properties:

- no crash,
- no unbounded allocation,
- no very long loops,
- output length bounded,
- flags always subset of supported mask,
- stack depth bounded,
- pop cannot underflow,
- Unicode validation holds.

Suggested maximums:

```text
stack depth: 64
pop count processed: 64
associated text codepoints per event: conservative bounded count
sequence output: < 1 KiB
```

Use constants and comments.

---

# 23. Threading and lifetime

Determine which thread:

- parses PTY output,
- handles UI input,
- reads active flags.

If parser updates and UI reads can occur on different threads, `flags` access is a data race.

Options:

1. Confirm all terminal update/input operations run on the main thread.
2. If not, use:
   - atomic active flags,
   - mutex around stack mutation,
   - or message passing.

Stacks are modified rarely; a mutex is acceptable. Do not add atomics without protecting the vectors.

Document the threading guarantee in code.

---

# 24. API and ABI considerations

`eterm` may be built as a shared module.

Minimize public ABI exposure:

- Keep new state internals private.
- Add accessors rather than exposing vectors.
- Avoid changing existing virtual interfaces unless necessary.
- `ITerminalDisplay` probably does not need protocol methods; `TerminalDisplay` has direct access to `TerminalEmulator`.
- If key-up is added to a public class but not a virtual interface, ABI impact is limited.

Follow project versioning policy.

---

# 25. Build-system changes

Add the new source file to every build path used by eepp:

- Premake
- CMake generation, if explicit lists exist
- Ninja/export generators if relevant
- Module-specific static/shared builds
- Tests

Search for existing glob behavior. Do not assume a new `.cpp` is automatically compiled.

Run at least:

```text
Linux GCC debug
Linux GCC release
Linux Clang
Windows/MSVC if CI supports it
macOS if CI supports it
```

Enable warnings as errors if that is project policy.

---

# 26. Documentation changes

Add a module document such as:

```text
src/modules/eterm/README.md
```

or update existing docs with:

- Kitty keyboard protocol support
- Supported flags
- Backend limitations
- Runtime negotiation behavior
- No need to set `TERM=xterm-kitty`
- Manual diagnostic commands
- Known tmux/screen caveats
- Codex Ctrl+Enter validation

Add a changelog entry:

```text
eTerm: implement Kitty keyboard protocol progressive enhancement,
allowing modern TUIs to distinguish modified Enter and other keys.
```

---

# 27. Suggested implementation phases

## Phase 0: Characterize current behavior

Deliverables:

- Parser tests for private CSI prefixes
- Legacy byte snapshots
- Confirm keydown/textinput ordering
- Confirm repeat exposure
- Confirm modifier constants
- Confirm thread model

No functional change.

## Phase 1: Negotiation and state

Deliverables:

- Flag enum
- State per screen
- Query response
- Push/pop/set
- Reset behavior
- Unit tests

Still no enhanced key emission.

## Phase 2: Minimal disambiguation

Deliverables:

- Encoder framework
- Enter, Tab, Escape, Backspace
- Modified key CSI-u emission
- Ctrl+Enter works in Crossterm/Codex
- Legacy fallback intact

This phase can be merged independently if desired.

## Phase 3: Special/function keys

Deliverables:

- Complete functional key table
- Arrow/navigation/function/keypad encoding
- Full modifier handling
- Tests

## Phase 4: Event types

Deliverables:

- key-up forwarding
- repeat detection/plumbing
- press/repeat/release serialization
- consumed-shortcut key tracking
- focus cleanup

## Phase 5: Printable keys and report-all

Deliverables:

- pending text event correlation
- Unicode keys
- no duplication
- layout/IME tests
- `REPORT_ALL_KEYS_AS_ESCAPE_CODES`

## Phase 6: Alternate keys and associated text

Deliverables:

- shifted/base-layout values
- associated committed text
- platform limitations documented
- all supported bits advertised accurately

## Phase 7: Integration and hardening

Deliverables:

- Crossterm helper
- Codex manual test
- fuzz tests
- CI matrix
- documentation

---

# 28. Acceptance criteria

The implementation is complete only when all of the following are true.

## Protocol

- [ ] `CSI ? u` gets a valid response.
- [ ] `CSI > flags u` pushes and activates flags.
- [ ] `CSI < u` pops one level.
- [ ] `CSI < n u` pops `n` levels safely.
- [ ] `CSI = flags ; mode u` implements all official modes.
- [ ] Main and alternate screens have independent state.
- [ ] Reset/process restart clears state appropriately.
- [ ] Unknown/malformed sequences do not crash or corrupt state.

## Encoding

- [ ] Plain legacy behavior is unchanged with flags zero.
- [ ] Ctrl+Enter is `CSI 13;5u` when disambiguation is active.
- [ ] Shift+Enter is distinguishable.
- [ ] Modified Escape, Tab, and Backspace are distinguishable.
- [ ] Special/function keys follow official Kitty encoding.
- [ ] Printable Unicode keys work in report-all mode.
- [ ] No duplicate text-input events occur.
- [ ] Press/repeat/release work when requested.
- [ ] Unsupported features are not falsely advertised.

## Integration

- [ ] Crossterm decodes the events with correct code/modifier/kind.
- [ ] Codex can bind Ctrl+Enter separately from Enter.
- [ ] Shell, Vim/Neovim, and existing terminal use remain unaffected when protocol is inactive.
- [ ] Abrupt child termination does not leave stale protocol state for a replacement process.

## Quality

- [ ] New logic has table-driven tests.
- [ ] Stack and output sizes are bounded.
- [ ] No data races.
- [ ] Code follows EEPP formatting/style.
- [ ] Build files and documentation are updated.

---

# 29. Concrete patch outline by file

## `src/modules/eterm/include/eterm/terminal/kittykeyboardprotocol.hpp`

Add:

- enums/constants,
- state structure,
- event structure,
- encoded-result structure,
- encoder declarations,
- functional-key mapping interface.

## `src/modules/eterm/src/eterm/terminal/kittykeyboardprotocol.cpp`

Implement:

- flag masking,
- push/pop/set,
- modifier conversion,
- key mapping,
- CSI-u serialization,
- legacy-versus-enhanced decision,
- event-type handling,
- Unicode validation.

## `src/modules/eterm/include/eterm/terminal/terminalemulator.hpp`

Add:

```cpp
#include <eterm/terminal/kittykeyboardprotocol.hpp>
```

Private:

```cpp
KittyKeyboardState mPrimaryKeyboardState;
KittyKeyboardState mAlternateKeyboardState;

KittyKeyboardState& activeKeyboardState();
const KittyKeyboardState& activeKeyboardState() const;

bool handleKittyKeyboardProtocol();
void replyKittyKeyboardProtocolSupport();
void resetKittyKeyboardProtocol();
```

Public:

```cpp
std::uint32_t getKeyboardEnhancementFlags() const;
```

## `src/modules/eterm/src/eterm/terminal/terminalemulator.cpp`

Modify:

- CSI dispatch
- query/push/pop/set handling
- screen-swap state selection if needed
- reset logic
- process replacement logic
- terminal-generated query response
- optional debug tracing

Remove the current no-op placeholder only after replacing it with real handling.

## `src/modules/eterm/include/eterm/terminal/terminaldisplay.hpp`

Add:

- `onKeyUp`
- repeat parameter or event kind
- pending/suppressed text state
- locally consumed key tracking
- helper declarations for enhanced and legacy paths

## `src/modules/eterm/src/eterm/terminal/terminaldisplay.cpp`

Modify:

- place enhanced encoding before Ctrl-letter legacy handling
- retain terminal shortcut priority
- add key-up behavior
- add repeat behavior
- add text-input correlation
- preserve legacy map unchanged as fallback

## `src/modules/eterm/src/eterm/ui/uiterminal.cpp`

Modify:

- forward key-up
- pass repeat status
- clear pending state on focus transitions if appropriate

## Tests/build/docs

Add all files described above.

---

# 30. Code-review checklist for the implementing agent

Before submitting:

1. Search for every path that resets or replaces `TerminalEmulator`.
2. Search for every screen swap path.
3. Verify parser representation for `?`, `>`, `<`, `=`.
4. Verify no conflict with legacy `CSI u` restore cursor.
5. Verify no unconditional Kitty output.
6. Verify active flags are read from the correct screen.
7. Verify text input cannot be emitted twice.
8. Verify release events for locally consumed shortcuts are suppressed.
9. Verify stack depth and pop count are bounded.
10. Verify unsupported bits are masked.
11. Verify query response mask matches actual implementation.
12. Verify Unicode scalar validation.
13. Verify AltGr/IME behavior.
14. Verify legacy snapshots with flags zero.
15. Verify Crossterm key dump.
16. Verify Codex Ctrl+Enter.
17. Verify process restart clears stale state.
18. Run formatter and full test suite.

---

# 31. Minimum viable implementation versus complete implementation

A very small patch can solve only the immediate issue:

```cpp
if ( flags & DISAMBIGUATE_ESCAPE_CODES &&
     keyCode == KEY_RETURN &&
     mod & KEYMOD_CTRL ) {
    ttywrite( "\033[13;5u", 7, 1 );
    return;
}
```

This is **not** the requested final implementation. It may be useful as a temporary proof of concept, but it is incomplete because it omits:

- capability query,
- push/pop stack,
- set modes,
- alternate screen independence,
- other modifiers,
- event types,
- report-all,
- printable keys,
- release/repeat,
- robust testing.

The production implementation must follow the stateful progressive-enhancement design in this document.

---

# 32. Final expected behavior example

Application sends:

```text
ESC [ ? u
```

eTerm replies:

```text
ESC [ ? 31 u
```

Application sends:

```text
ESC [ > 7 u
```

eTerm stores prior flags on the current screen’s stack and activates:

```text
DISAMBIGUATE_ESCAPE_CODES
REPORT_EVENT_TYPES
REPORT_ALTERNATE_KEYS
```

User presses Ctrl+Enter.

eTerm receives approximately:

```text
keycode   = KEY_RETURN
scancode  = SCANCODE_RETURN
modifiers = KEYMOD_CTRL
event     = press
```

eTerm writes to the child:

```text
ESC [ 1 3 ; 5 : 1 u
```

or the exact shorter equivalent required by the current Kitty specification when press event type may be omitted. For plain Ctrl+Enter without an explicit event-type field, the canonical motivating sequence is:

```text
ESC [ 1 3 ; 5 u
```

Crossterm decodes:

```text
KeyCode::Enter
KeyModifiers::CONTROL
KeyEventKind::Press
```

Codex matches its Ctrl+Enter submit binding.

Application exits cleanly and sends:

```text
ESC [ < u
```

eTerm restores the previous flags. The shell again receives normal legacy Enter as `0x0d`.

---

# 33. Notes for the AI implementation agent

- Read the official protocol before writing sequence serialization.
- Do not rely only on examples from this document for obscure keys.
- Treat this document as an architecture and completeness contract.
- Reuse existing eTerm parser and PTY response machinery.
- Preserve the legacy key tables.
- Add tests before changing event encoding.
- Make the smallest coherent commits:
  1. tests/parser characterization,
  2. state/negotiation,
  3. encoder/minimal keys,
  4. event types,
  5. printable/alternate/associated text,
  6. integration/docs.
- When an EEPP backend lacks required information, implement a clean capability boundary rather than fabricating data.
- The key outcome is standards compatibility, not a one-off Codex workaround.
