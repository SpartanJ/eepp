# Plan: UIDiffView Implementation

## 1. Overview and Objectives
The goal of this project is to implement `UIDiffView`, a new widget within the `eepp` UI framework that provides a rich visual representation of code differences. 

### Key Objectives:
- **Phase 1: Unified View (Single Panel):** Display a unified diff with custom line background colors (red for removed, green for added, specific styling for headers).
- **Phase 2: Proper Syntax Highlighting:** Use language-specific syntax highlighting (e.g., C++, Python) for the text, rather than treating the entire file as a `.diff` text.
- **Phase 3: Diff Generation:** Integrate `dtl.h` (or similar) to generate diff information dynamically when comparing two text documents or strings, in addition to parsing existing `.patch` or `.diff` files.
- **Phase 4: Split View (Two Panel):** Support an optional side-by-side split view.

## 2. Architecture & Design

### 2.1 Widget Hierarchy
- **`UIDiffView`** will be a custom composite widget, *not* a direct subclass of `UICodeEditor`. 
- By composing `UICodeEditor`(s) internally, `UIDiffView` can seamlessly transition between a "Unified" single-editor mode and a "Side-by-Side" two-editor mode in the future.
- `UIDiffView` will manage an internal instance of `UICodeEditor` (for unified view) and attach a custom `UICodeEditorPlugin` to handle custom background drawing.

### 2.2 Data Model & Diff Processing
We need a representation of diff data that abstracts away whether the diff was loaded from a file or generated on the fly.
- Create a struct `DiffLine`:
  ```cpp
  enum class DiffLineType { Added, Removed, Context, Header };
  struct DiffLine {
      DiffLineType type;
      String text;
      // Original line numbers (for the gutter later)
      Int64 oldLineNum;
      Int64 newLineNum; 
  };
  ```
- **Parsing:** If loaded from a `.patch`/`.diff` file, a parser will extract `DiffLine`s and determine the underlying file extension (e.g., from `+++ b/src/main.cpp` -> `.cpp`).
- **Generation (dtl.h):** If provided two strings or `TextDocument`s (Old vs New), we will use `dtl.h` to compute the differences and generate a unified list of `DiffLine`s.

### 2.3 Syntax Highlighting Challenge
A standard syntax highlighter will fail if lines start with `+` or `-` because it breaks language grammar.
**Solution:**
- The internal `TextDocument` of the `UICodeEditor` will hold the *clean* text (without the leading `+` or `-`).
- The syntax highlighter will run normally, initialized with the detected base language (e.g., C++).
- The `+` and `-` indicators will be drawn visually in the gutter or injected via rendering hooks, rather than being part of the raw `TextDocument` string. Alternatively, if we keep `+` and `-` in the string, we might need a composite `SyntaxHighlighter` that delegates to the underlying language while skipping the first character. *Recommendation: Strip `+`/`-` from the document text, and draw them manually during rendering to preserve perfect syntax highlighting.*

### 2.4 Custom Rendering (Backgrounds & Indicators)
We will leverage `UICodeEditorPlugin` to draw custom line backgrounds without modifying the core `UICodeEditor` drawing routine.
- Create `UIDiffEditorPlugin : public UICodeEditorPlugin`.
- Override `drawBeforeLineText`:
  - Check the line index against the list of `DiffLine`s.
  - If `DiffLineType::Added`, draw a greenish `Primitives::drawRectangle` across the editor's width.
  - If `DiffLineType::Removed`, draw a reddish rectangle.
  - If `DiffLineType::Header`, draw a bluish/gray rectangle.
- Override `drawGutter` (or similar) if we want to display dual line numbers (Old and New) or custom `+`/`-` icons.

## 3. Step-by-Step Execution Plan

### Step 1: Core Diff Parsing & Generation
1. Integrate `dtl.h` into `src/eepp/thirdparty/dtl/` (if not already present).
2. Create `DiffDocument` (or `DiffData`) utility class capable of:
   - Parsing a unified diff string into a structured format.
   - Generating a unified diff structure from two source strings using `dtl.h`.
   - Identifying the target language extension from diff headers.

### Step 2: Custom Rendering Plugin
1. Implement `UIDiffEditorPlugin` inheriting from `UICodeEditorPlugin`.
2. Implement background drawing in `drawBeforeLineText` based on line states provided by `DiffDocument`.
3. Test drawing performance. Ensure no memory/object allocations happen during the render loop (Negen mandate).

### Step 3: `UIDiffView` Implementation (Unified View)
1. Create `UIDiffView` widget (`include/eepp/ui/tools/uidiffview.hpp` and `src/eepp/ui/tools/uidiffview.cpp`).
2. Instantiate a read-only `UICodeEditor` internally.
3. Apply the `UIDiffEditorPlugin` to the editor.
4. Implement `loadFromPatch(const std::string& patchText)` and `loadFromStrings(const std::string& oldText, const std::string& newText)`.
5. Set the syntax definition of the internal `TextDocument` based on the detected file extension.

### Step 4: Gutter and Line Numbers (Refinement)
1. Hide the default line number gutter of `UICodeEditor` or override it.
2. Draw custom line numbers representing both Old and New file line numbers.

### Step 5: Integration into ecode
1. Map the `.diff` and `.patch` extensions to open in `UIDiffView` instead of standard `UICodeEditor` inside `ecode`.
2. Add a command/shortcut to "Compare against saved version" or "Compare against Git HEAD" which generates a diff dynamically using the newly integrated `dtl.h` logic.

## 4. Performance & Memory Considerations (Negen's Directives)
- The mapping of line index to `DiffLineType` must be fast (e.g., an `std::vector<DiffLineType>` indexed directly by `lineIndex`).
- Do not allocate strings or complex objects inside the `drawBeforeLineText` or `drawGutter` render loops.
- Avoid modifying the `UICodeEditor` document layout excessively on the fly. Build the unified document cleanly once.