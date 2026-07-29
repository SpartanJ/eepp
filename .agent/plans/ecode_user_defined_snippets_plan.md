# ecode User-Defined Snippets Implementation Plan

Status: **Phase 1 implemented and manually validated. Phase 2 not started. Phase 3 optional.**

Last status review: 2026-07-29.

Related issue: [SpartanJ/ecode#111](https://github.com/SpartanJ/ecode/issues/111)

## 0. Current status and remaining decisions

The core feature is working and has been tested interactively with project-local C/C++ snippets.
The implementation currently supports loading, matching, inserting, navigating, and hot-reloading
VS Code-compatible user and project snippets. It also preserves the existing LSP snippet behavior.

Completed:

- Phase 1 definition model, JSON/JSONC parser, immutable store, indexes, and focused tests;
- user snippets from `<ecode-config>/snippets`;
- project snippets from `.vscode/*.code-snippets` and `.ecode/*.code-snippets`;
- asynchronous initial loading and incremental create/modify/move/delete reloads;
- workspace switching, generation-based stale-job rejection, and shutdown synchronization;
- last-known-good definitions after an invalid edit and content-hash suppression of unchanged
  valid files;
- global and language scopes, string/array prefixes, string/array bodies, and descriptions;
- autocomplete integration before and after asynchronous LSP completion responses;
- same-prefix snippet identity, punctuation-aware replacement, empty/short-prefix matching, and
  source precedence only between equally matching user snippets;
- contextual multiline indentation, variables, linked placeholders, choices, tab navigation, and
  multi-cursor insertion through the shared snippet runtime;
- allocation-conscious filesystem filtering and cached watched-directory paths;
- `.code-snippets` recognition as JSON by the syntax-definition system;
- a reusable JSONC trailing-comma helper in `jsonhelper.hpp` / `jsonhelper.cpp`;
- a project-local `.ecode/c-cpp.code-snippets` manual fixture.

Phase 1 hardening that remains useful but is not blocking current use:

- broaden loader/index tests for UTF-8, punctuation prefixes, empty-pattern ordering, duplicate
  names across files, and unsupported fields;
- add automated editor-level coverage for replacement ranges, indentation, multi-cursor
  insertion, and filesystem reloads where practical;
- debounce/coalesce repeated invalid-file save events so the same malformed content is not parsed
  and logged repeatedly;
- keep the new user-facing `docs/snippets.md` guide current as Phase 2 capabilities are added;
- run the full manual matrix from Section 17.4 against more real-world snippet collections.

Everything in Phase 2 is now a product choice rather than a prerequisite for useful snippet
support. The most independently useful candidates are:

1. an **Insert Snippet** searchable command;
2. a **Configure Snippets** command that creates/opens the current language file;
3. optional exact-prefix Tab expansion when no popup or active snippet session exists;
4. `include` / `exclude` file-pattern scopes;
5. additional workspace, clipboard, cursor, date/time, random, UUID, and comment variables.

Phase 3 remains explicitly optional and should be driven only by observed compatibility needs.

## 1. Goal

Add user-defined snippets to ecode using the VS Code JSON/JSONC snippet file format while reusing
the snippet parser, insertion, multi-cursor, placeholder, choice, and tab-navigation behavior that
already exists in the autocomplete plugin.

The implementation should provide:

- user-level, language-specific snippets;
- user-level global and multi-language snippets;
- project snippets from existing `.vscode/*.code-snippets` files;
- native project snippets from `.ecode/*.code-snippets` files;
- snippet discovery through the normal autocomplete popup;
- explicit prefix replacement, including prefixes that are not ordinary programming-language
  words;
- hot reload when snippet files are created, modified, moved, or deleted;
- predictable behavior with multiple cursors;
- loading and matching that do not block the UI thread.

This plan intentionally distinguishes **file-format compatibility** from **complete VS Code runtime
parity**. Phase 1 and Phase 2 are the intended implementation. Phase 3 is optional and should only
be attempted when concrete snippet packs or users need the additional behavior.

## 2. Non-goals

The initial implementation must not:

- execute TextMate interpolated shell commands or backtick expressions;
- load Sublime `.sublime-snippet` XML files;
- load TextMate `.tmSnippet` plist/XML files;
- introduce a second plugin that competes with the autocomplete plugin for completion UI or key
  handling;
- automatically discover every possible VS Code, VSCodium, portable, or profile-specific user
  configuration directory;
- claim complete VS Code semantic compatibility;
- implement automatic expansion on Space;
- require file-template snippets, placeholder transforms, or every VS Code variable before the
  first useful release.

Unsupported recognized properties should be ignored safely and, where useful, reported once in
the log. Unknown properties should remain forward-compatible and must not invalidate an otherwise
valid snippet.

## 3. Architectural decision

Keep user-defined snippets inside `AutoCompletePlugin`, but separate storage concerns into a new
class next to the existing parser:

```text
AutoCompletePlugin
|- UserSnippetStore      file discovery, JSONC parsing, scope/index management, reload
|- SnippetParser         snippet body expansion into text and tab-stop metadata
`- snippet session       editor insertion, selections, mirrors, choices, navigation
```

Suggested files:

- `src/tools/ecode/plugins/autocomplete/usersnippetstore.hpp`
- `src/tools/ecode/plugins/autocomplete/usersnippetstore.cpp`
- `src/tests/unit_tests/usersnippetstore_tests.cpp`

Do not make `UserSnippetStore` a plugin. `AutoCompletePlugin` should remain the owner of:

- the completion popup;
- completion-list merging and ranking;
- editor commands and keybindings;
- snippet insertion and navigation sessions;
- rendering and choice UI.

`UserSnippetStore` should not know about `UICodeEditor`, draw UI, intercept keys, edit documents,
or send LSP requests. This boundary allows later extraction if ecode eventually introduces a
general completion-provider interface, without paying that architectural cost now.

## 4. Current implementation premises

The existing implementation already provides the expensive editor-side mechanics:

- `SnippetParser` parses tab stops, placeholders, nested placeholders, linked occurrences,
  choices, variables, and variable transforms.
- `AutoCompletePlugin::pickSuggestion()` parses snippet text separately for every selection and
  inserts it at every cursor.
- `SnippetSession` tracks tab-stop groups across one or more snippet insertions.
- `SnippetDocumentClient` translates snippet ranges as the document changes.
- snippet choices use the existing autocomplete popup.
- LSP completion requests remain on the worker path through `runUpdateSuggestions()`.

The following existing assumptions must be corrected as part of user-snippet integration:

1. `Suggestion` uses `text` for display, matching, and equality. `operator==` compares only
   `text`, so two user snippets with the same prefix would collapse into one.
2. `fuzzyMatchSymbols()` accepts every suggestion whose kind is `Snippet` without testing the
   current pattern. That works for server-filtered LSP results, but it would expose every entry in
   a large user snippet pack on every query.
3. Normal suggestion insertion can fall back to `delete-to-previous-word`. User snippet prefixes
   may contain punctuation and cannot rely on the document's word definition.
4. Local symbols and LSP completions are composed in more than one code path. User snippets must
   be included consistently before and after an asynchronous LSP response.
5. The base plugin filesystem handler watches only `autocomplete.json` semantically and reloads
   the complete plugin. Snippet directories need their own incremental event handling.

## 5. Supported format

### 5.1 File types

Support:

- `<language-id>.json`: all definitions in the file apply to that language;
- `*.code-snippets`: definitions are global unless their `scope` restricts them.

Parse JSON with comments enabled, consistent with the existing `json::parse(..., true)` usage in
ecode.

### 5.2 Initial definition fields

Support these VS Code fields in Phase 1:

- the root property name as the snippet's name;
- `prefix` as a string or array of strings;
- `body` as a string or array of strings;
- `description` as an optional string;
- `scope` as an optional comma-separated string in `.code-snippets` files.

Normalize a body array by joining its entries with `\n`. Do not parse the body with
`SnippetParser` while loading. Parsing depends on the selection, document, cursor index, workspace,
and other runtime variables, so it must remain deferred until insertion.

Reject only the invalid definition, not the complete file, when:

- `prefix` is absent, empty, or neither a string nor an array of strings;
- `body` is absent or neither a string nor an array of strings;
- a prefix or body array contains a non-string value;
- `scope` is present with an unsupported type.

Log the file path and snippet name for skipped definitions. Do not log full snippet bodies, since
user snippets can contain private data.

### 5.3 Recognized but deferred fields

Parse or recognize these fields so their presence does not produce confusing behavior:

- `isFileTemplate`;
- `include`;
- `exclude`.

`include` and `exclude` are planned for Phase 2. `isFileTemplate` belongs to optional Phase 3.
Until supported, do not expose `isFileTemplate` snippets through a file-template command that does
not exist. They may still appear as ordinary insertion snippets unless testing shows VS Code hides
them from normal completion; document the chosen behavior.

### 5.4 Body compatibility boundary

Phase 1 supports whatever the existing `SnippetParser` handles correctly. Explicitly document
these known limitations:

- placeholder transforms such as `${1/(.*)/${1:/upcase}/}` are not supported;
- transform case modifiers are initially limited to the modifiers already implemented;
- ecode regular expressions are used instead of JavaScript regular expressions, so obscure regex
  constructs may differ;
- not every VS Code variable is initially defined;
- interpolated shell execution is deliberately unsupported.

Unknown variables must retain the existing parser behavior: an unknown bare variable becomes an
editable synthetic placeholder, while a variable with a fallback uses its fallback.

## 6. Source discovery and precedence

### 6.1 Default locations

Load snippets from:

1. `<ecode-config>/snippets/<language-id>.json`
2. `<ecode-config>/snippets/*.code-snippets`
3. `<workspace>/.vscode/*.code-snippets`
4. `<workspace>/.ecode/*.code-snippets`

The exact config root must come from the existing application/plugin configuration APIs. Do not
reconstruct platform-specific config paths inside `UserSnippetStore`.

Only files directly inside these directories are required initially. Recursive snippet-directory
scans are unnecessary unless real snippet packs require them.

### 6.2 Source ranking

Use source priority for tie-breaking, not destructive prefix deduplication:

1. native project `.ecode` snippets;
2. compatible project `.vscode` snippets;
3. user snippets.

Multiple snippets may intentionally share a prefix and all must remain selectable. The popup
should use name, description, and optionally source detail to distinguish them. A source reload
replaces definitions originating from that source file, but snippets from other files must not be
removed merely because their names or prefixes match.

Use canonical normalized paths plus the root snippet property name as the stable source identity.
Do not use prefix alone as identity.

### 6.3 Language identifiers

Treat `SyntaxDefinition::getLSPName()` as the canonical language identifier because it is generally
closest to the identifiers used by VS Code snippet files.

For resilience, the lookup layer may also accept:

- a lowercase `getLanguageName()` value;
- explicit aliases only where ecode and VS Code identifiers are known to differ.

Keep alias resolution in one helper. Do not distribute special cases through the autocomplete
pipeline.

An absent `scope` in a `.code-snippets` file means global. A present scope is split on commas,
trimmed, normalized, and indexed under every listed language.

## 7. Data model

Use eepp containers and namespace conventions:

- `using namespace EE;`
- `UnorderedMap` / `UnorderedSet` instead of their `std::` equivalents;
- `SmallVector` for fields normally containing one or a few entries, such as prefixes and scopes.

Suggested logical model:

```cpp
enum class UserSnippetSource {
	User,
	VSCodeProject,
	EcodeProject,
};

struct UserSnippetDefinition {
	std::string name;
	SmallVector<std::string, 2> prefixes;
	std::string body;
	std::string description;
	SmallVector<std::string, 2> scopes;
	std::string sourcePath;
	UserSnippetSource source;
};
```

The exact inline capacities should be confirmed with the available `SmallVector` implementation
and typical object size. Do not put the body into every prefix index entry.

The store should publish an immutable snapshot containing:

- the owning definition vector;
- global definition indices;
- language-to-definition-index mappings;
- any lightweight prefix index proven useful by profiling.

Start with a per-language vector scan on a worker thread. Typical personal collections are small,
and even large snippet packs contain only thousands of entries. Do not introduce a trie before a
measured need exists. Avoid copying snippet bodies while scanning; return matching definition
indices and materialize only the limited set of suggestions that can be displayed or ranked.

If a shared immutable snapshot is used to guarantee background-query lifetime, its heap ownership
is justified by concurrent reloads. Keep snapshot swaps coarse and cheap.

## 8. File loading and reload lifecycle

### 8.1 Initial loading

`AutoCompletePlugin::load()` should construct/configure the store and schedule initial user snippet
loading on `mThreadPool`. Project sources are added once the current workspace is known.

No directory scan, file read, JSON parse, or snippet-pack indexing may run on the main UI thread.
Publishing a completed snapshot and invalidating an editor are appropriate main-thread work.

### 8.2 Workspace changes

Add one normalized `setWorkspaceFolder()` path in `AutoCompletePlugin` or `UserSnippetStore` and
call it from the relevant project lifecycle hooks:

- `onLoadProject()` for a project already open when the plugin subscribes;
- `WorkspaceFolderChanged` for later project changes.

The setter must be idempotent because both paths may report the same workspace. When the workspace
changes:

- remove definitions from the old project's `.vscode` and `.ecode` sources;
- scan the new project sources asynchronously;
- preserve user snippets;
- invalidate stale jobs with a generation counter or equivalent lifetime token;
- clear project definitions when the workspace becomes empty.

### 8.3 Filesystem events

Override `onFileSystemEvent()` in `AutoCompletePlugin` and first preserve the base behavior for
`autocomplete.json`.

For recognized snippet locations, handle all applicable events:

- created: parse and add the file;
- modified: parse and replace that file's definitions;
- deleted: remove that file's definitions;
- moved/renamed: remove the old identity and load the new path when exposed by the event API.

Debounce or coalesce bursts from editors that save through temporary files and renames. Schedule
file reads and parsing on the worker pool.

Keep a per-file record with a content hash or equivalent so unchanged notifications do not rebuild
the snapshot. On a malformed modification:

- retain the last known valid definitions for that file;
- log one useful diagnostic;
- replace the old definitions only after a complete valid parse succeeds.

On initial load, an invalid file contributes nothing.

## 9. Suggestion model changes

Refactor `AutoCompletePlugin::Suggestion` before adding user definitions. It needs distinct
concepts for:

- display label;
- filtering text or matched prefix;
- insertion text;
- stable identity/source;
- explicit replacement range;
- whether an LSP server already filtered the item.

Avoid changing LSP behavior unintentionally. One possible extension is:

```cpp
enum class SuggestionSource {
	LocalSymbol,
	LSP,
	UserSnippet,
	SnippetChoice,
};
```

Add a stable identity only where needed. LSP items can keep their existing semantics, while user
snippet identity should include the source path/name or a snapshot-local ID plus source generation.

Replace text-only duplicate detection. Local symbols can still deduplicate by text, but two user
snippets with the same prefix must coexist. Do not make one global `operator==` silently encode
different source-specific rules; use an explicit deduplication helper or comparison key.

Update fuzzy matching so:

- local symbols are fuzzy matched as today;
- user snippets are matched against their prefixes;
- LSP snippets can retain server-filtered behavior when necessary;
- user snippet kind alone never bypasses pattern matching;
- source priority is a tie-breaker, not a substitute for match score.

VS Code documents substring-style prefix matching, including abbreviated matches such as `fc` for
`for-const`. Reuse `String::fuzzyMatchSimple()` if its behavior produces the expected ordering, and
add focused tests before inventing another matcher.

## 10. Completion pipeline integration

Introduce a shared helper that gathers non-LSP completion sources for an editor and pattern. It
should compose:

- document or language symbol cache entries;
- matching global user snippets;
- matching current-language user snippets.

Use the same helper from:

- `runUpdateSuggestions()` before or while requesting LSP completion;
- `processCodeCompletion()` when an asynchronous LSP response arrives.

This prevents user snippets from disappearing when the LSP response replaces the popup contents.

Requirements:

- user snippets work even if no LSP server/capability exists;
- user snippets work even if local symbol caches are empty;
- Mod+Space can show current-language/global snippets when the partial symbol is empty;
- typing a short snippet prefix is not blocked by the current three-character fallback threshold;
- LSP requests continue to run away from the main UI thread;
- stale completion responses must not bind to a destroyed editor or an obsolete snippet snapshot.

For an empty Mod+Space pattern, show snippets ordered by source priority and name, capped according
to the existing popup behavior. Phase 2's dedicated picker will provide exhaustive browsing.

## 11. Prefix replacement and insertion

### 11.1 Explicit replacement ranges

When a user snippet matches a prefix, compute a replacement range ending at the cursor and
covering that exact prefix in UTF-32 document coordinates. Store that range or enough activation
metadata in the suggestion.

Do not call `delete-to-previous-word` for user snippets. This is required for prefixes such as:

- `for-const`;
- `log!`;
- punctuation-heavy markup triggers;
- prefixes whose boundaries differ from `mSymbolPattern`.

Keep the existing LSP-provided `textEdit.range` behavior unchanged.

### 11.2 Multiple cursors

The primary cursor drives matching and popup selection. On insertion, evaluate the selected prefix
at every cursor:

- if the same prefix is immediately before that cursor, replace it;
- if the cursor has a selection, replace the selection and expose it through `TM_SELECTED_TEXT`;
- if neither applies, insert at the cursor without deleting unrelated text.

Parse and prepare the body separately for every cursor so variables, indentation, selected text,
and cursor index can differ. Continue using the existing snippet session to group equivalent tab
stops across all insertions.

### 11.3 Shared insertion helper

Extract the snippet branch of `pickSuggestion()` into a reusable helper, for example:

```cpp
void insertSnippet( UICodeEditor*, std::string_view body,
					const SnippetActivation& activation );
```

Both LSP snippets and user snippets should call this helper. It should own:

- collecting selections;
- constructing per-selection variables;
- preparing contextual indentation;
- parsing bodies;
- deleting/replacing activation ranges;
- inserting text;
- starting the snippet session;
- showing first-stop choices.

Keep plain-text completion insertion separate. Preserve LSP range behavior and existing
multi-cursor behavior through regression tests/manual validation.

## 12. Contextual indentation

VS Code snippet bodies commonly use tabs for relative indentation. Add a body-preparation step
before `SnippetParser::parse()` for each insertion:

1. Normalize body-array joins to `\n` at load time.
2. Determine the insertion line's leading indentation.
3. Prefix each body line after the first with that base indentation.
4. Translate leading snippet indentation according to the document's configured indentation style
   and width.
5. Preserve non-leading tabs and spaces as literal snippet content.
6. Parse the prepared body afterward so tab-stop codepoint offsets match the actual inserted text.

Use existing `TextDocument` indentation helpers where available. Do not reimplement tab/space
policy locally if the editor already exposes it.

Test insertion:

- at column zero;
- inside an indented block;
- with tabs configured;
- with spaces configured;
- at multiple cursors with different base indentation;
- with placeholders spanning multiple lines.

Exact byte-for-byte VS Code whitespace parity is not required, but common code snippets must insert
with structurally correct indentation.

## 13. Variables

Keep `snippetVariables()` as the shared variable provider for LSP and user snippets. Extend it
incrementally rather than creating a user-only provider.

Phase 1 must preserve the currently supported variables:

- `TM_SELECTED_TEXT`
- `TM_CURRENT_LINE`
- `TM_CURRENT_WORD`
- `TM_LINE_INDEX`
- `TM_LINE_NUMBER`
- `TM_FILENAME`
- `TM_FILENAME_BASE`
- `TM_DIRECTORY`
- `TM_FILEPATH`

Phase 2 should add the high-value, straightforward variables:

- `RELATIVE_FILEPATH`
- `WORKSPACE_NAME`
- `WORKSPACE_FOLDER`
- `CLIPBOARD`
- `CURSOR_INDEX`
- `CURSOR_NUMBER`
- current date/time variables;
- `RANDOM`, `RANDOM_HEX`, and `UUID` if suitable engine utilities already exist;
- `LINE_COMMENT`, `BLOCK_COMMENT_START`, and `BLOCK_COMMENT_END` when syntax definitions expose
  reliable comment delimiters.

Do not create new time, random, UUID, clipboard, or syntax-comment infrastructure solely for
snippets. Use existing services or defer the variable.

## 14. Phase 1 - Core user-defined snippets

Phase 1 is the minimum release intended to satisfy the central request in issue #111.

**Phase status: implemented.** The unchecked items below identify hardening or exact architectural
follow-ups, not blockers for the currently working feature.

### 14.1 Store and loader

- [x] Add `UserSnippetDefinition` and `UserSnippetStore`.
- [x] Parse JSONC core fields, comments, and trailing commas.
- [x] Load user language/global sources.
- [x] Load `.vscode` and `.ecode` project sources.
- [x] Build immutable language/global indexes.
- [x] Preserve last-known-good per-file data on reload errors.
- [x] Add source-aware diagnostics without logging bodies.

### 14.2 Autocomplete integration

- [x] Refactor suggestion identity/filtering.
- [x] Prevent user snippet kind from bypassing prefix matching.
- [x] Preserve same-prefix snippets as distinct candidates.
- [x] Merge user snippets into local and LSP completion paths.
- [x] Support short prefixes and empty-pattern Mod+Space invocation.
- [x] Show snippet name/description and a snippet icon/kind using the existing rendering path.
- [x] Apply project/user source priority only between equally matching user snippets, without
  promoting them above normal completions.

### 14.3 Insertion

- [x] Reuse the shared snippet insertion/session path for LSP and user snippets.
- [x] Add exact matched-prefix replacement independent of word boundaries.
- [x] Adapt multiline indentation per cursor.
- [x] Reuse existing variables and sessions.
- [x] Preserve multi-cursor insertion and choice behavior.

### 14.4 Lifecycle

- [x] Load on the worker pool.
- [x] Handle current and changed workspaces.
- [x] Watch create/modify/delete/move events.
- [x] Ignore stale background work during reload, workspace change, and shutdown.
- [x] Avoid allocation and worker scheduling for unrelated filesystem events.
- [ ] Optionally debounce repeated malformed-file notifications; unchanged valid files are already
  suppressed by content hash.

### 14.5 Phase 1 acceptance criteria

- [x] A copied VS Code language snippet file works from ecode's user snippets directory.
- [x] An existing `.vscode/*.code-snippets` file works without modification.
- [x] A `.ecode/*.code-snippets` project file works.
- [x] String and array forms of `prefix` and `body` work.
- [x] Global and language scopes work.
- [x] Snippets appear while typing and with Mod+Space, with or without an LSP server.
- [x] Two snippets with the same prefix remain independently selectable.
- [x] Punctuation-containing prefixes replace exactly the matched prefix.
- [x] Tab stops, placeholders, mirrors, choices, variables, and multiple cursors continue to work.
- [x] Multiline snippets respect the current indentation style.
- [x] Editing, creating, renaming, and deleting a snippet file updates suggestions without
  restarting ecode.
- [x] Invalid in-progress JSON does not destroy the last valid loaded definitions.
- [x] No filesystem or JSON work occurs on the main UI thread.

## 15. Phase 2 - Editor usability and broader practical compatibility

Phase 2 is part of the intended feature, but should be implemented after Phase 1 is usable and has
been tested with real snippet collections.

**Phase status: not started and no longer required for the initial release.** Each subsection can
be accepted or rejected independently based on whether the workflow is valuable to ecode users.

### 15.1 Dedicated Insert Snippet command

Add an `insert-snippet` editor/application command that opens a searchable list of snippets valid
for the current language and file.

The list should show:

- snippet name;
- prefixes;
- description;
- source when needed to disambiguate duplicates.

Picking an entry inserts it without requiring a typed prefix. Reuse existing list/model helpers or
the Universal Locator where that produces a natural ecode interaction. Do not build a second
autocomplete popup implementation.

Add a configurable keybinding entry, but no default binding is required if the command is readily
available from the command palette.

### 15.2 Optional exact-prefix Tab completion

Add an autocomplete setting such as:

```json
"snippets": {
  "enabled": true,
  "tab_completion": false
}
```

When enabled, key handling order must be:

1. active snippet session: navigate to the next tab stop;
2. visible completion popup: accept the selected suggestion;
3. no popup and exact snippet prefix before the cursor: expand it;
4. otherwise: allow the editor's normal Tab command.

If multiple definitions have the same exact prefix, open a choice list instead of selecting one
arbitrarily. Shift+Tab must never start a new snippet expansion.

Use command/keybinding resolution rather than hard-coded key codes.

### 15.3 Configure Snippets command

Add a command that opens or creates the appropriate user snippet file for the current language.
Creation should use a small commented starter document valid as JSONC. Do not overwrite an existing
file.

Project snippet creation can be a separate command or later follow-up; user-language configuration
is the priority.

### 15.4 File pattern scopes

Implement `include` and `exclude` using the engine's existing glob/path matching facilities.

- Filename-only patterns match the filename.
- Path patterns match the normalized full or workspace-relative path, matching the documented VS
  Code behavior as closely as practical.
- `exclude` wins when both include and exclude match.
- Pattern matching occurs before suggestion materialization.

Cache compiled pattern data in the immutable snapshot. Do not compile glob/regex patterns on every
keystroke.

### 15.5 Common variables and modifiers

- Add variables listed in Section 13 when supported by existing ecode/eepp services.
- Add `camelcase`, `pascalcase`, `snakecase`, and `kebabcase` transform format modifiers.
- Add tests for Unicode behavior where the chosen string helpers define it clearly.

### 15.6 Phase 2 acceptance criteria

- Users can browse and insert snippets without typing a prefix.
- Optional Tab completion does not break indentation or active snippet navigation.
- Duplicate exact prefixes prompt for a choice.
- `include` and `exclude` filter snippets predictably.
- Common workspace, clipboard, cursor, time, and casing transformations behave as documented where
  supported.
- Users can locate/create their language snippet file from ecode without learning platform paths.

## 16. Phase 3 - Optional compatibility work

Phase 3 is explicitly optional. Do not block issue #111 or the initial user-defined snippet release
on this work.

**Phase status: not started; defer unless a concrete compatibility issue requires it.**

Only implement an item after identifying a real snippet pack, user workflow, or compatibility bug
that needs it.

Possible work:

- placeholder transforms that reevaluate transformed mirrors after editing a tab stop;
- `isFileTemplate` and a dedicated Fill File with Snippet workflow;
- closer JavaScript-regex compatibility where `EE::System::RegEx` differs materially;
- additional VS Code variables introduced after the initial implementation;
- configurable external snippet directories;
- explicit VS Code/VSCodium profile import;
- importing Sublime or TextMate container formats into the internal definition model;
- snippet extension/package manifests;
- per-snippet keybindings or context expressions.

Placeholder transforms are the largest runtime item. They require storing transform metadata on
occurrences, identifying the authoritative first placeholder occurrence, and updating transformed
mirrors when the source placeholder changes or when navigation leaves it. This must be designed as
session behavior, not faked as a one-time parse transform.

TextMate shell interpolation remains out of scope even in Phase 3 unless a separate security design
is explicitly approved.

## 17. Tests

**Current status:** all existing `SnippetParser` tests and the focused `UserSnippetStore` suite
pass. Core parsing, scope matching, duplicate triggers, last-known-good replacement, and source
removal are covered. The lists below remain the desired broader coverage rather than a claim that
every item is automated today.

### 17.1 Loader unit tests

Add focused tests for:

- JSONC comments and trailing comments;
- language `.json` default scope;
- global `.code-snippets` scope;
- comma-separated scope trimming;
- string and array prefixes;
- string and array bodies;
- optional description;
- invalid root JSON;
- invalid individual definitions without rejecting valid siblings;
- empty prefixes;
- duplicate prefixes and names from different sources;
- UTF-8 names, descriptions, prefixes, and bodies;
- body arrays joined exactly once with `\n`;
- unsupported recognized fields remaining non-fatal;
- no snippet-body text in diagnostics, where diagnostics can be inspected.

Keep JSON-to-definition parsing isolated enough to test without constructing a `PluginManager` or
GUI. If necessary, expose a small pure parser function in the same module and keep filesystem/store
coordination around it.

### 17.2 Matching/index tests

Test:

- global plus current-language results;
- no snippets from unrelated languages;
- prefix abbreviation/fuzzy ranking;
- source priority tie-breaking;
- same-prefix definitions remaining distinct;
- empty-pattern results;
- punctuation prefixes;
- file-pattern filtering when Phase 2 is implemented;
- snapshot replacement without dangling indices.

### 17.3 Parser regression tests

Keep all existing `SnippetParser` tests. Add tests only when body syntax or transform modifiers are
changed; do not duplicate loader tests in the parser suite.

### 17.4 Integration/manual tests

Because editor/plugin UI integration is difficult to instantiate in unit tests, maintain a small
manual fixture covering:

- one user language file;
- one global scoped file;
- one `.vscode` project file;
- duplicate and punctuation prefixes;
- multiline indentation;
- choices and mirrors;
- two cursors at different indentation levels;
- invalid JSON followed by recovery;
- workspace switching;
- no LSP, an LSP returning no completions, and an LSP returning snippets/completions.

Verify that Mod+Space and configured shortcuts use `KeyBindings` matching helpers rather than raw
shortcut equality or hard-coded keys.

## 18. Build and validation workflow

When implementation begins:

1. Add new source files to both `premake4.lua` and `premake5.lua` where the unit-test target needs
   them. Confirm the ecode target's normal file glob includes them.
2. Add unit-test sources under `src/tests/unit_tests/`.
3. Run `clang-format` on every modified C/C++ file.
4. Regenerate project files using the repository's required debug/ASan premake invocation.
5. Build ecode and the unit-test target.
6. Run focused snippet/user-snippet tests through `projects/scripts/xvfb-run-eepp`.
7. Run the complete unit-test suite before handoff.
8. Run `git diff --check`.
9. Perform the manual editor matrix from Section 17.4.

## 19. Performance and allocation audit

Before completing each phase, explicitly review:

- whether file I/O or JSON parsing can reach the UI thread;
- whether every keystroke copies all snippet bodies;
- whether prefix arrays duplicate bodies in storage;
- whether immutable snapshots have clear ownership during asynchronous matching;
- whether reload jobs capture large vectors or strings by value unnecessarily;
- whether worker lambdas use move captures for owned buffers;
- whether stale jobs can publish after workspace change or plugin shutdown;
- whether pattern compilation happens at load time rather than query time;
- whether popup result caps are applied before expensive suggestion materialization;
- whether repeated same-file events rebuild an unchanged snapshot;
- whether logging accidentally copies or exposes snippet bodies.

Expected justified heap allocations include loaded definition strings, per-language indexes,
immutable snapshots needed across worker jobs, and the small limited set of materialized popup
suggestions. Avoid per-frame or draw-time snippet work entirely.

## 20. Documentation

**Status: complete for Phase 1.** The ecode documentation repository contains `docs/snippets.md`,
linked from `docs/autocomplete.md`, with locations, examples, supported syntax and variables,
runtime behavior, troubleshooting, and a detailed VS Code compatibility comparison. Update that
document alongside each Phase 2 compatibility change.

Ship a concise user-facing document or configuration section containing:

- supported file locations;
- a minimal language-specific example;
- a global scoped example;
- the supported fields;
- how to invoke suggestions and the Insert Snippet command;
- whether Tab completion is enabled;
- supported variables;
- known compatibility limitations;
- the fact that shell interpolation is intentionally unsupported.

Use “VS Code JSON/JSONC snippet file compatible” until the optional compatibility gaps have been
closed. Avoid the unqualified statement “supports all VS Code snippets.”

## 21. Recommended execution order

Implement in this order to keep each change reviewable and testable:

1. Introduce the pure definition model and JSONC file parser with unit tests.
2. Add immutable storage, language/global indexes, source identity, and query tests.
3. Refactor `Suggestion` identity/filtering without changing visible existing behavior.
4. Add user snippet matching to the non-LSP completion pipeline.
5. Merge the same source into LSP response processing through a shared helper.
6. Extract shared snippet insertion and add exact prefix replacement.
7. Add per-cursor indentation preparation and multi-cursor tests/manual validation.
8. Add user/project discovery and initial asynchronous loading.
9. Add workspace changes, filesystem reload, last-known-good behavior, and stale-job protection.
10. Complete Phase 1 acceptance testing and release it for real-world snippet-pack testing.
11. Add the Insert Snippet and Configure Snippets commands.
12. Add optional exact-prefix Tab completion.
13. Add file-pattern filters and high-value variables/modifiers.
14. Reassess optional Phase 3 only from observed incompatibilities.

Items 1-10 are complete for the current Phase 1 implementation, subject to the hardening notes in
Section 0. Items 11-13 are uncommitted product choices from Phase 2. Item 14 remains optional.

The implementation can stop here if the current autocomplete-driven workflow is sufficient. Phase
2 should be selected feature-by-feature from actual feedback. Phase 3 is not part of the completion
definition for issue #111.
