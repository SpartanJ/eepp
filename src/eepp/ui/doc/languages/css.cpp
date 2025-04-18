#include <eepp/ui/doc/languages/css.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addCSS() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "CSS",
		  { "%.css$" },
		  {
			  { { "\\." }, "normal" },
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "#%x%x%x%x?%x?%x?%x?%x?%f[%W]" }, "string" },
			  { { "#[%a%_%-][%w%_%-]*" }, "keyword2" },
			  { { "%-%-%a[%w%-%_]*" }, "keyword2" },
			  { { "-?%d+[%d%.]*p[xt]" }, "number" },
			  { { "-?%d+[%d%.]*dp" }, "number" },
			  { { "-?%d+[%d%.]*deg" }, "number" },
			  { { "-?%d+[%d%.]*" }, "number" },
			  { { "@[%a][%w%_%-]*" }, "keyword2" },
			  { { "%.[%a%_%-][%w%_%-]*" }, "keyword2" },
			  { { "(:)(hover)" }, { "normal", "operator", "literal" } },
			  { { "(:)(focus%-within)" }, { "normal", "operator", "literal" } },
			  { { "(:)(focus)" }, { "normal", "operator", "literal" } },
			  { { "(:)(selected)" }, { "normal", "operator", "literal" } },
			  { { "(:)(pressed)" }, { "normal", "operator", "literal" } },
			  { { "(:)(disabled)" }, { "normal", "operator", "literal" } },
			  { { "(:)(checked)" }, { "normal", "operator", "literal" } },
			  { { "(:)(root)" }, { "normal", "operator", "link" } },
			  { { "([%a%-_]+)(%()" }, { "normal", "function", "normal" } },
			  { { "[%a][%w-]*%s*%f[:]" }, "keyword" },
			  { { "[{}:>~!]" }, "operator" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "orange", "literal" },
			  { "tabwidget", "keyword" },
			  { "olivedrab", "literal" },
			  { "olive", "literal" },
			  { "navajowhite", "literal" },
			  { "mistyrose", "literal" },
			  { "midnightblue", "literal" },
			  { "darkseagreen", "literal" },
			  { "mediumseagreen", "literal" },
			  { "lavenderblush", "literal" },
			  { "mediumaquamarine", "literal" },
			  { "limegreen", "literal" },
			  { "mediumspringgreen", "literal" },
			  { "lightgreen", "literal" },
			  { "lime", "literal" },
			  { "mediumorchid", "literal" },
			  { "gray", "literal" },
			  { "black", "literal" },
			  { "lightyellow", "literal" },
			  { "mediumblue", "literal" },
			  { "lightslategray", "literal" },
			  { "lightseagreen", "literal" },
			  { "tableview", "keyword" },
			  { "input", "keyword" },
			  { "lightpink", "literal" },
			  { "lightslategrey", "literal" },
			  { "lightgrey", "literal" },
			  { "lightgoldenrodyellow", "literal" },
			  { "lightblue", "literal" },
			  { "lemonchiffon", "literal" },
			  { "lawngreen", "literal" },
			  { "mintcream", "literal" },
			  { "pushbutton", "keyword" },
			  { "khaki", "literal" },
			  { "ivory", "literal" },
			  { "moccasin", "literal" },
			  { "indigo", "literal" },
			  { "transparent", "literal" },
			  { "oldlace", "literal" },
			  { "indianred", "literal" },
			  { "hotpink", "literal" },
			  { "sizenwse", "literal" },
			  { "honeydew", "literal" },
			  { "false", "literal" },
			  { "grey", "literal" },
			  { "green", "literal" },
			  { "cornflowerblue", "literal" },
			  { "steelblue", "literal" },
			  { "mediumturquoise", "literal" },
			  { "lightgray", "literal" },
			  { "goldenrod", "literal" },
			  { "greenyellow", "literal" },
			  { "saddlebrown", "literal" },
			  { "lightcyan", "literal" },
			  { "rlay", "keyword" },
			  { "gainsboro", "literal" },
			  { "hslider", "keyword" },
			  { "fuchsia", "literal" },
			  { "forestgreen", "literal" },
			  { "palegoldenrod", "literal" },
			  { "maroon", "literal" },
			  { "floralwhite", "literal" },
			  { "dodgerblue", "literal" },
			  { "mediumvioletred", "literal" },
			  { "dimgray", "literal" },
			  { "peru", "literal" },
			  { "deepskyblue", "literal" },
			  { "textureregion", "keyword" },
			  { "blue", "literal" },
			  { "bisque", "literal" },
			  { "slategray", "literal" },
			  { "treeview", "keyword" },
			  { "scrollbar", "keyword" },
			  { "sizewe", "literal" },
			  { "none", "literal" },
			  { "image", "keyword" },
			  { "checkbox", "keyword" },
			  { "lavender", "literal" },
			  { "cornsilk", "literal" },
			  { "textinput", "keyword" },
			  { "blueviolet", "literal" },
			  { "skyblue", "literal" },
			  { "darkslateblue", "literal" },
			  { "lightsalmon", "literal" },
			  { "a", "keyword" },
			  { "widgettable", "keyword" },
			  { "beige", "literal" },
			  { "darkgray", "literal" },
			  { "crimson", "literal" },
			  { "menucheckbox", "keyword" },
			  { "hscrollbar", "keyword" },
			  { "dimgrey", "literal" },
			  { "sizeall", "literal" },
			  { "aliceblue", "literal" },
			  { "progressbar", "keyword" },
			  { "lightskyblue", "literal" },
			  { "blanchedalmond", "literal" },
			  { "darkslategray", "literal" },
			  { "mediumpurple", "literal" },
			  { "tomato", "literal" },
			  { "paleturquoise", "literal" },
			  { "darkturquoise", "literal" },
			  { "powderblue", "literal" },
			  { "codeeditor", "keyword" },
			  { "wheat", "literal" },
			  { "seashell", "literal" },
			  { "hbox", "keyword" },
			  { "sienna", "literal" },
			  { "salmon", "literal" },
			  { "menubar", "keyword" },
			  { "silver", "literal" },
			  { "tan", "literal" },
			  { "thistle", "literal" },
			  { "sandybrown", "literal" },
			  { "turquoise", "literal" },
			  { "azure", "literal" },
			  { "plum", "literal" },
			  { "vslider", "keyword" },
			  { "waitarrow", "literal" },
			  { "textview", "keyword" },
			  { "springgreen", "literal" },
			  { "widgettablerow", "keyword" },
			  { "vscrollbar", "keyword" },
			  { "wait", "literal" },
			  { "relativelayout", "keyword" },
			  { "palevioletred", "literal" },
			  { "spinbox", "keyword" },
			  { "darkblue", "literal" },
			  { "linearlayout", "keyword" },
			  { "chartreuse", "literal" },
			  { "teal", "literal" },
			  { "darkorchid", "literal" },
			  { "seagreen", "literal" },
			  { "listview", "keyword" },
			  { "red", "literal" },
			  { "selectbutton", "keyword" },
			  { "viewpagerhorizontal", "keyword" },
			  { "vbox", "keyword" },
			  { "slategrey", "literal" },
			  { "gridlayout", "keyword" },
			  { "listbox", "keyword" },
			  { "peachpuff", "literal" },
			  { "coral", "literal" },
			  { "stacklayout", "keyword" },
			  { "center", "literal" },
			  { "lightcoral", "literal" },
			  { "rosybrown", "literal" },
			  { "viewpager", "keyword" },
			  { "tab", "keyword" },
			  { "inputpassword", "keyword" },
			  { "window", "keyword" },
			  { "tooltip", "keyword" },
			  { "scrollview", "keyword" },
			  { "stackwidget", "keyword" },
			  { "textedit", "keyword" },
			  { "loader", "keyword" },
			  { "combobox", "keyword" },
			  { "radiobutton", "keyword" },
			  { "dropdownlist", "keyword" },
			  { "splitter", "keyword" },
			  { "snow", "literal" },
			  { "palegreen", "literal" },
			  { "anchor", "keyword" },
			  { "royalblue", "literal" },
			  { "slider", "keyword" },
			  { "violet", "literal" },
			  { "whitesmoke", "literal" },
			  { "white", "literal" },
			  { "darksalmon", "literal" },
			  { "pink", "literal" },
			  { "slateblue", "literal" },
			  { "darkred", "literal" },
			  { "lightsteelblue", "literal" },
			  { "darkolivegreen", "literal" },
			  { "darkviolet", "literal" },
			  { "yellowgreen", "literal" },
			  { "widget", "keyword" },
			  { "linen", "literal" },
			  { "darkgrey", "literal" },
			  { "menuradiobutton", "keyword" },
			  { "crosshair", "literal" },
			  { "darkcyan", "literal" },
			  { "sprite", "keyword" },
			  { "navy", "literal" },
			  { "darkmagenta", "literal" },
			  { "burlywood", "literal" },
			  { "menuseparator", "keyword" },
			  { "firebrick", "literal" },
			  { "orchid", "literal" },
			  { "chocolate", "literal" },
			  { "darkgreen", "literal" },
			  { "aquamarine", "literal" },
			  { "ghostwhite", "literal" },
			  { "arrow", "literal" },
			  { "antiquewhite", "literal" },
			  { "deeppink", "literal" },
			  { "yellow", "literal" },
			  { "console", "keyword" },
			  { "cadetblue", "literal" },
			  { "touchdraggable", "keyword" },
			  { "gold", "literal" },
			  { "darkkhaki", "literal" },
			  { "viewpagervertical", "keyword" },
			  { "mediumslateblue", "literal" },
			  { "aqua", "literal" },
			  { "brown", "literal" },
			  { "cyan", "literal" },
			  { "darkgoldenrod", "literal" },
			  { "papayawhip", "literal" },
			  { "hand", "literal" },
			  { "purple", "literal" },
			  { "tv", "keyword" },
			  { "sizenesw", "literal" },
			  { "darkslategrey", "literal" },
			  { "button", "keyword" },
			  { "nocursor", "literal" },
			  { "orangered", "literal" },
			  { "ibeam", "literal" },
			  { "darkorange", "literal" },
			  { "true", "literal" },
			  { "sizens", "literal" },
			  { "magenta", "literal" },
			  { "textinputpassword", "keyword" },
			  { "important", "literal" },
			  { "a", "keyword" },
			  { "abbr", "keyword" },
			  { "address", "keyword" },
			  { "area", "keyword" },
			  { "article", "keyword" },
			  { "aside", "keyword" },
			  { "audio", "keyword" },
			  { "b", "keyword" },
			  { "base", "keyword" },
			  { "bdi", "keyword" },
			  { "bdo", "keyword" },
			  { "blockquote", "keyword" },
			  { "body", "keyword" },
			  { "br", "keyword" },
			  { "button", "keyword" },
			  { "canvas", "keyword" },
			  { "caption", "keyword" },
			  { "cite", "keyword" },
			  { "code", "keyword" },
			  { "col", "keyword" },
			  { "colgroup", "keyword" },
			  { "data", "keyword" },
			  { "datalist", "keyword" },
			  { "dd", "keyword" },
			  { "del", "keyword" },
			  { "details", "keyword" },
			  { "dfn", "keyword" },
			  { "dialog", "keyword" },
			  { "div", "keyword" },
			  { "dl", "keyword" },
			  { "dt", "keyword" },
			  { "em", "keyword" },
			  { "embed", "keyword" },
			  { "fieldset", "keyword" },
			  { "figcaption", "keyword" },
			  { "figure", "keyword" },
			  { "footer", "keyword" },
			  { "form", "keyword" },
			  { "h1", "keyword" },
			  { "h2", "keyword" },
			  { "h3", "keyword" },
			  { "h4", "keyword" },
			  { "h5", "keyword" },
			  { "h6", "keyword" },
			  { "head", "keyword" },
			  { "header", "keyword" },
			  { "hr", "keyword" },
			  { "html", "keyword" },
			  { "i", "keyword" },
			  { "iframe", "keyword" },
			  { "img", "keyword" },
			  { "input", "keyword" },
			  { "ins", "keyword" },
			  { "kbd", "keyword" },
			  { "label", "keyword" },
			  { "legend", "keyword" },
			  { "li", "keyword" },
			  { "link", "keyword" },
			  { "main", "keyword" },
			  { "map", "keyword" },
			  { "mark", "keyword" },
			  { "menu", "keyword" },
			  { "meta", "keyword" },
			  { "meter", "keyword" },
			  { "nav", "keyword" },
			  { "noscript", "keyword" },
			  { "object", "keyword" },
			  { "ol", "keyword" },
			  { "optgroup", "keyword" },
			  { "option", "keyword" },
			  { "output", "keyword" },
			  { "p", "keyword" },
			  { "param", "keyword" },
			  { "picture", "keyword" },
			  { "pre", "keyword" },
			  { "progress", "keyword" },
			  { "q", "keyword" },
			  { "rp", "keyword" },
			  { "rt", "keyword" },
			  { "ruby", "keyword" },
			  { "s", "keyword" },
			  { "samp", "keyword" },
			  { "script", "keyword" },
			  { "section", "keyword" },
			  { "select", "keyword" },
			  { "slot", "keyword" },
			  { "small", "keyword" },
			  { "source", "keyword" },
			  { "span", "keyword" },
			  { "strong", "keyword" },
			  { "style", "keyword" },
			  { "sub", "keyword" },
			  { "summary", "keyword" },
			  { "sup", "keyword" },
			  { "table", "keyword" },
			  { "tbody", "keyword" },
			  { "td", "keyword" },
			  { "template", "keyword" },
			  { "textarea", "keyword" },
			  { "tfoot", "keyword" },
			  { "th", "keyword" },
			  { "thead", "keyword" },
			  { "time", "keyword" },
			  { "title", "keyword" },
			  { "tr", "keyword" },
			  { "track", "keyword" },
			  { "u", "keyword" },
			  { "ul", "keyword" },
			  { "var", "keyword" },
			  { "video", "keyword" },
			  { "wbr", "keyword" },

			  { "Widget", "keyword" },
			  { "LinearLayout", "keyword" },
			  { "RelativeLayout", "keyword" },
			  { "TextView", "keyword" },
			  { "PushButton", "keyword" },
			  { "CheckBox", "keyword" },
			  { "RadioButton", "keyword" },
			  { "ComboBox", "keyword" },
			  { "DropDownList", "keyword" },
			  { "Image", "keyword" },
			  { "ListBox", "keyword" },
			  { "MenuBar", "keyword" },
			  { "ProgressBar", "keyword" },
			  { "ScrollBar", "keyword" },
			  { "Slider", "keyword" },
			  { "SpinBox", "keyword" },
			  { "Sprite", "keyword" },
			  { "Tab", "keyword" },
			  { "WidgetTable", "keyword" },
			  { "WidgetTableRow", "keyword" },
			  { "TabWidget", "keyword" },
			  { "TextEdit", "keyword" },
			  { "TextInput", "keyword" },
			  { "TextInputPassword", "keyword" },
			  { "Loader", "keyword" },
			  { "SelectButton", "keyword" },
			  { "Window", "keyword" },
			  { "ScrollView", "keyword" },
			  { "TextureRegion", "keyword" },
			  { "TouchDraggable", "keyword" },
			  { "GridLayout", "keyword" },
			  { "StackLayout", "keyword" },
			  { "ViewPager", "keyword" },
			  { "CodeEditor", "keyword" },
			  { "Splitter", "keyword" },
			  { "TreeView", "keyword" },
			  { "TableView", "keyword" },
			  { "ListView", "keyword" },
			  { "StackWidget", "keyword" },
			  { "Console", "keyword" },
			  { "MenuCheckBox", "keyword" },
			  { "MenuRadioButton", "keyword" },
			  { "MenuSeparator", "keyword" },
			  { "Anchor", "keyword" },
			  { "TextureViewer", "keyword" },
			  { "Tooltip", "keyword" },
			  { "Menu", "keyword" },
			  { "PopUpMenu", "keyword" },

		  },
		  "",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
}

}}}} // namespace EE::UI::Doc::Language
