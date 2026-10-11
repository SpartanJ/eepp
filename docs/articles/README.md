# eepp articles

Use this page to find the guide for the task you are doing. The articles explain eepp's
supported behavior; class headers and current source code remain the reference for API details.

| Task | Start here | Follow up with |
| --- | --- | --- |
| Build a first UI with XML and CSS | [UI introduction](ui_introduction.md) | [Application UI authoring](ui_authoring.md) |
| Create or change an application screen | [Application UI authoring](ui_authoring.md) | [Native layout reference](ui_layout_reference.md), [CSS for application UI](ui_css_for_applications.md) |
| Look up a CSS property, selector, or value | [CSS specification](css_specification.md) | [CSS for application UI](ui_css_for_applications.md) when styling native widgets |
| Render HTML or Markdown content | [HTML compatibility layer](ui_html_compatibility.md) | [CSS specification](css_specification.md) for supported properties and values |
| Preview and edit native XML/CSS layouts | [UI editor](ui_editor.md) | [Application UI authoring](ui_authoring.md), [Runtime UI inspector](ui_inspector.md) |
| Inspect or automate a running UI | [Runtime UI inspector](ui_inspector.md) | [Application UI authoring](ui_authoring.md) for a layout debugging workflow |
| Bind application data or commands to widgets | [UI data binding](ui_databinding.md) | [Application UI authoring](ui_authoring.md) for screen structure |
| Add an interactive chart | [UI charts](ui_charts.md) | [Application UI authoring](ui_authoring.md) for placement and sizing |
| Work on `eterm` Kitty support | [Keyboard protocol](eterm_kitty_keyboard.md), [graphics protocol](eterm_kitty_graphics.md) | The corresponding `src/modules/eterm/` implementation and tests |

Native application widgets and HTML documents have different layout behavior. Start with
[application UI authoring](ui_authoring.md) for toolbars, dialogs, forms, and editor windows;
start with [HTML compatibility](ui_html_compatibility.md) for `UIWebView` and `UIMarkdownView`.
