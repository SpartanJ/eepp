# UI Layout Phase 8: Form Action and Navigation Plan

This document outlines the architectural plan for implementing HTML `<form>` submissions, input value extraction, and expanding `UISceneNode` to support interceptable navigation requests (GET/POST).

**AGENT DIRECTIVE (CRITICAL):** You MUST compile and run the unit tests (`bin/unit_tests/eepp-unit_tests-debug`) after EVERY step. Do NOT proceed to the next step if there is even a 1-pixel difference in visual layout tests. Take a git stash snapshot (`git stash push -m "Phase 8.X passed" && git stash apply`) upon passing a step to keep a checkpoint while continuing to work. **If you need to restore a stash, use `git stash apply` instead of `git stash pop` so the stable snapshot is never lost.**

---

## Phase 8: Form and Navigation implementation

**Step 8.1: Extend Navigation System**
- The current `UISceneNode::openURL(URI)` and `setURLInterceptorCb` only handle simple URIs, which cannot represent `POST` requests or request bodies.
- In `include/eepp/ui/uiscenenode.hpp`, create:
  ```cpp
  struct NavigationRequest {
      URI uri;
      std::string method{ "GET" };
      std::string body;
      std::map<std::string, std::string> extraHeaders;
  };
  ```
- Add `void navigate( const NavigationRequest& request );` to `UISceneNode`.
- Add `void setNavigationInterceptorCb( std::function<bool( const NavigationRequest& request )> cb );`.
- Update `openURL(URI)` to wrap `navigate(NavigationRequest{uri})` for backward compatibility.
- In `navigate()`, if `mNavigationInterceptorCb` returns `true`, return early. Else if `mURLInterceptorCb` returns `true`, return early. Else, fallback to `Engine::instance()->openURI()`.
- **Validation:** Compile and run all tests. (Snapshot)

**Step 8.2: Retrieve Values from Form Elements**
- Form submission requires querying the value of input elements.
- Add `virtual String getValue() const { return String(); }` to `UIWidget`.
- Override `getValue()` in the appropriate classes:
  - `HTMLInput`: return `getText()` for text, or `"on"`/`""` for checkboxes/radio buttons based on `isChecked()`.
  - `HTMLTextArea` (and `UITextEdit`): return `getText()`.
  - `UIDropDownList` (and `UIComboBox`): return the selected item's text.
- Add `virtual String getName() const { return getAttribute("name"); }` or rely on `getAttribute("name")` to get the field identifier.
- **Validation:** Write unit tests to verify `getValue()` for text, checkbox, and dropdowns. Compile and run all tests. (Snapshot)

**Step 8.3: Implement UIHTMLForm**
- Create `UIHTMLForm` class inheriting from `UIHTMLWidget` (or `UIRichText` if treating as a block container).
- Add members: `mAction` (URI), `mMethod` (String, default "GET"), `mEnctype` (String).
- Override `applyProperty` to capture `action`, `method`, and `enctype`.
- Implement `void submit()`.
  - `submit()` iterates over all child widgets recursively.
  - If a widget has a non-empty `name` attribute (using `getAttribute("name")`), it collects its `getValue()`.
  - It URL-encodes the keys and values.
  - If `mMethod == "GET"`, it appends the URL-encoded query string to `mAction` and calls `navigate()`.
  - If `mMethod == "POST"`, it puts the URL-encoded string into the `body` of `NavigationRequest`, sets `method = "POST"`, and calls `navigate()`.
- In `uiwidgetcreator.cpp`, update `registeredWidget["form"]` to instantiate `UIHTMLForm::New`.
- **Validation:** Compile and run all tests. (Snapshot)

**Step 8.4: Form Submission Triggers & Testing**
- In `UIHTMLForm`, listen for `Event::OnMouseClick` on any child widget. If the target is a submit button (e.g., `HTMLInput` with `type="submit"`, or a `UIPushButton` with `type="submit"`), prevent the default action and call `submit()`.
- Listen to `Event::OnPressEnter` inside text inputs within the form to trigger `submit()`.
- Write a unit test simulating a form with inputs and a submit button. Attach a `NavigationInterceptorCb` to the scene node, simulate a click on the submit button, and verify the intercepted `NavigationRequest` contains the correct URI and encoded body.
- **Validation:** Compile and run all tests. Must pass. (Snapshot)
