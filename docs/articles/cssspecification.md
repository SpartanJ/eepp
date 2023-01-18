# CSS Specification

## Introduction

eepp CSS custom implementation is heavily based on the official [CSS standard](https://www.w3.org/TR/CSS/#css).
This document will try to explain the shared features and the current differences with the CSS
specification. Since CSS is widely used and documented, every feature that it's shared with the CSS
standard will be directly linked to the Mozilla CSS documentation. If you are totally new to CSS
please go the [Mozilla CSS portal](https://developer.mozilla.org/en-US/docs/Web/CSS).

## Relevant differences with CSS standard

* Layout properties are not supported (display, float, etc), since eepp layout system differs from
the HTML+CSS specification (design decision).

* [inheritance](https://developer.mozilla.org/en-US/docs/Web/CSS/Inheritance) is not supported,
except for [CSS custom properties](https://developer.mozilla.org/en-US/docs/Web/CSS/--*)
(design decision).

* [Attribute selector](https://developer.mozilla.org/en-US/docs/Web/CSS/Attribute_selectors)
is not supported (probably will not be supported in the near future, since collides with some
implementation decisions).

* CSS relative paths start from the process current working directory instead the relative path from
the CSS file.

* eepp CSS supports alternative path methods besides paths (resource locator to previously loaded
resources by the process).

* eepp CSS supports [Device-independent
pixel](https://en.wikipedia.org/wiki/Device-independent_pixel) unit `dp`.

* CSS files should be always UTF-8 encoded.

## CSS Selectors

CSS selectors define the elements to which a set of CSS rules apply.

---

### Basic selectors

#### Universal Selector

[Universal selector](https://developer.mozilla.org/en-US/docs/Web/CSS/Universal_selectors)
selects all elements. Optionally, it may be restricted to a specific namespace or to all namespaces.

Syntax: `* ns|* *|*`

Example: `*` will match all the elements of the document.

---

#### Type Selector

[Type selector](https://developer.mozilla.org/en-US/docs/Web/CSS/Type_selectors) selects all
elements that have the given node name.

Syntax: `elementname`

Example: `widget` will match any `<widget>` element.

---

#### Class Selector

[Class selector](https://developer.mozilla.org/en-US/docs/Web/CSS/Class_selectors) selects all
elements that have the given class attribute.

Syntax: `.classname`

Example: `.index` will match any element that has a class of "index".

---

#### ID Selector

[ID selector](https://developer.mozilla.org/en-US/docs/Web/CSS/ID_selectors) selects an element
based on the value of its id attribute. There should be only one element with a given ID in a
document.

Syntax: `#idname`

Example: `#toc` will match the element that has the ID "toc".

---

### Grouping selectors

#### Selector List

The [selector list](https://developer.mozilla.org/en-US/docs/Web/CSS/Selector_list) `,` is a
grouping method, it selects all the matching nodes.

Syntax: `A, B`

Example: `widget, image` will match both `<widget>` and `<image>` elements.

---

### Combinators

#### Descendant combinator

The [descendant combinator](https://developer.mozilla.org/en-US/docs/Web/CSS/Descendant_combinator)
" " (space) combinator selects nodes that are descendants of the first element.

Syntax: `A B`

Example: `LinearLayout image` will match all `<image>` elements that are inside a `<LinearLayout>`
element.

#### Child combinator

The [child combinator](https://developer.mozilla.org/en-US/docs/Web/CSS/Child_combinator) `>`
selects nodes that are direct children of the first element.

Syntax: `A > B`

Example: `LinearLayout > listbox` will match all `<listbox>` elements that are nested directly
inside a `<LinearLayout>` element.

#### General sibling combinator

The [general sibling combinator](https://developer.mozilla.org/en-US/docs/Web/CSS/General_sibling_combinator)
`~` selects siblings. This means that the second element follows the first (though not necessarily
immediately), and both share the same parent.

Syntax: `A ~ B`

Example: `TextView ~ image` will match all `<image>` elements that follow a `<TextView>`,
immediately or not.

#### Adjacent sibling combinator

The [adjacent sibling combinator](https://developer.mozilla.org/en-US/docs/Web/CSS/Adjacent_sibling_combinator)
`+` selects adjacent siblings. This means that the second element directly follows the first, and
both share the same parent.

Syntax: `A + B`

Example: `TextView + image` will match all `<image>` elements that directly follow an `<TextView>`.

---

## CSS Properties

eepp CSS properties can be categorized in two big groups: CSS standard properties and eepp CSS
properties.

**Definitions:**

* Property names are *case insensitive*.
* Some properties have aliases.
* All property names that contain dashes (`-`), have aliases without those dashes.
  * For example: `animation-delay` also is `animationDelay`.
  * Other aliases will be clarified in the properties documentation.
* All properties can be used as attributes of the XML node.
  * XML attributes are treated as inline CSS styling.
  * For example:
	* Something like:
	* `LinearLayout {  layout-width: match_parent; layout_height: match_parent; orientation: vertical; }`
	* is similar to (but not equivalent, since specificity of inline CSS is higher):
	* `<LinearLayout layout_width="match_parent" layout_height="match_parent" orientation="vertical"></LinearLayout>`
* CSS standard properties directly linked to the Mozilla docs are fully supported, otherwise a clarification will appear.
* _Applicable to_ will link to the EE::UI::UIWidget that can use that property, and betweeen parenthesis is the CSS element name.

---

### animation-delay

Read [animation-delay](https://developer.mozilla.org/en-US/docs/Web/CSS/animation-delay) docs.

---

### animation-direction

Read [animation-direction](https://developer.mozilla.org/en-US/docs/Web/CSS/animation-direction) documenation.

---

### animation-duration

Read [animation-duration](https://developer.mozilla.org/en-US/docs/Web/CSS/animation-duration) documenation.

---

### animation-fill-mode

Read [animation-fill-mode](https://developer.mozilla.org/en-US/docs/Web/CSS/animation-fill-mode) documentation.

---

### animation-iteration-count

Read [animation-iteration-count](https://developer.mozilla.org/en-US/docs/Web/CSS/animation-iteration-count) documentation.

---

### animation-name

Read [animation-name](https://developer.mozilla.org/en-US/docs/Web/CSS/animation-name) documentation.

### animation-play-state

Read [animation-play-state](https://developer.mozilla.org/en-US/docs/Web/CSS/animation-play-state) documentation.

---

### animation-timing-function

Read [animation-timing-function](https://developer.mozilla.org/en-US/docs/Web/CSS/animation-timing-function) documentation.

Timing function names are custom, and not the same as the standard, but `cubic-bezier` will be added
soon. Current timing functions supported: linear, quadratic-in, quadratic-out, quadratic-in-out,
sine-in, sine-out, sine-in-out, exponential-in, exponential-out, exponential-in-out, quartic-in,
quartic-out, quartic-in-out, circular-in, circular-out, circular-in-out, cubic-in, cubic-out,
cubic-in-out, back-in, back-out, back-in-out, bounce-in, bounce-out, bounce-in-out, elastic-in,
elastic-out, elastic-in-out, none.

---

### allow-editing

Enable or disable editing on input elements.

* Applicable to: EE::UI::UITextEdit (TextEdit), EE::UI::UITextInput (TextInput)
* Data Type: [boolean](#boolean-data-type)
* Default value: `true`

---

### allow-float

On a input that only accept numbers, enables floating point numbers.

* Applicable to: EE::UI::UITextInput (TextInput), EE::UI::UISpinBox (SpinBox::input)
* Data Type: [boolean](#boolean-data-type)
* Default value: `false`

---

### animation-speed

Sets the loader speed animation.

* Applicable to: EE::UI::UILoader (Loader)
* Data Type: [number](#number-data-type)
* Default value: `0.5`

---

### arc-start-angle

The angle value where a loader progress starts rendering.

* Applicable to: EE::UI::UILoader (Loader)
* Data Type: [number](#number-data-type)
* Default value: `0`

---

### background-color

Read [background-color](https://developer.mozilla.org/en-US/docs/Web/CSS/background-color) documentation.

---

### background-expand

Sets if the element skin background should be expanded to the element dimensions.

* Applicable to: EE::UI::UISlider (Slider), EE::UI::UIScrollBar (ScrollBar)
* Data Type: [boolean](#boolean-data-type)
* Default value: `false`

---

### background-image

Read [background-image](https://developer.mozilla.org/en-US/docs/Web/CSS/background-image) documentation.

* Applicable to: Any element
* Data Type: [resource-path](#resource-path-data-type)
* Default value: `none`

---

### background-position-x

Read [background-position-x](https://developer.mozilla.org/en-US/docs/Web/CSS/background-position-x) documentation.

---

### background-position-y

Read [background-position-y](https://developer.mozilla.org/en-US/docs/Web/CSS/background-position-y) documentation.

---

### backtround-repeat

Read [background-repeat](https://developer.mozilla.org/en-US/docs/Web/CSS/background-repeat) documentation.
`space` and `round` values are not supported yet.

* Default value: `no-repeat` (this is different from the CSS standard)

---

### background-tint

Sets the tint color of the background image.

* Applicable to: Any element
* Data Type: [color](#color-data-type)
* Default value: `white`

---

### background-size

Read [background-size](https://developer.mozilla.org/en-US/docs/Web/CSS/background-size) documentation.

---

### blend-mode

Sets the blend mode to the widget.

* Applicable to: Any element
* Data Type: [string-list](#string-list-data-type)
* Value List:
  * `add`
  * `alpha`
  * `multiply`
  * `none`
* Default value: `alpha`

### border-bottom-color

Read [border-bottom-color](https://developer.mozilla.org/en-US/docs/Web/CSS/border-bottom-color) documentation.

---

### border-bottom-left-radius

Read [border-bottom-left-radius](https://developer.mozilla.org/en-US/docs/Web/CSS/border-bottom-left-radius) documentation.

---

### border-bottom-right-radius

Read [border-bottom-right-radius](https://developer.mozilla.org/en-US/docs/Web/CSS/border-bottom-right-radius) documentation.

---

### border-bottom-width

Read [border-bottom-width](https://developer.mozilla.org/en-US/docs/Web/CSS/border-bottom-width) documentation.

---

### border-left-color

Read [border-left-color](https://developer.mozilla.org/en-US/docs/Web/CSS/border-left-color) documentation.

---

### border-left-width

Read [border-left-width](https://developer.mozilla.org/en-US/docs/Web/CSS/border-left-width) documentation.

---

### border-right-color

Read [border-right-color](https://developer.mozilla.org/en-US/docs/Web/CSS/border-right-color) documentation.

---

### border-right-width

Read [border-right-width](https://developer.mozilla.org/en-US/docs/Web/CSS/border-right-width) documentation.

---

### border-top-color

Read [border-top-color](https://developer.mozilla.org/en-US/docs/Web/CSS/border-top-color) documentation.

---

### border-top-left-radius

Read [border-top-left-radius](https://developer.mozilla.org/en-US/docs/Web/CSS/border-top-left-radius) documentation.

---

### border-top-right-radius

Read [border-top-right-radius](https://developer.mozilla.org/en-US/docs/Web/CSS/border-top-right-radius) documentation.

---

### border-top-width

Read [border-top-width](https://developer.mozilla.org/en-US/docs/Web/CSS/border-top-width) documentation.

---

### border-type

Defines where the border box is drawn.

* Applicable to: Any element
* Data Type: [string-list](#string-list-data-type)
* Value List:
  * `inside`: The border will be drawn inside the content box.
  * `outside`: The border will be drawn outside the content box.
  * `outline`: The border will be drawn in the middle point between the content box and the border width.

---

### change-page-percent

Sets the percentage of scroll of a page that triggers a page change in a ViewPager.

* Applicable to: EE::UI::UIViewPager (ViewPager).
* Data Type: [number](#number-data-type) (normalized between `0` and `1`)
* Default value: `0.33`

---

### click-step

For any element that has a stepped value change, this sets the step variation for a click to the
element.

* Applicable to: EE::UI::UIScrollBar (ScrollBar), EE::UI::UISlider (Slider), EE::UI::UISpinBox (SpinBox).
* Data Type: [number](#number-data-type)
* Default value:
  * ScrollBar: `0.1`
  * Slider: `0.1`
  * SpinBox: `1`

---

### clip

Specifies how the clipping is applied to the element if clip is enabled.

* Applicable to: Any element
* Data Type: [string-list](#string-list-data-type)
* Value List:
  * `none`: No clipping is applied.
  * `content-box`: Clips the content box of the element (means the whole box).
  * `padding-box`: Clips the padding box of the element (the whole box minus the padding).
  * `border-box`: Clips the border box of the element (the content box plus the border).
* Default Value: Varies for each widget/element.

---

### color

Read [color](https://developer.mozilla.org/en-US/docs/Web/CSS/color) documentation.

---

### column-mode

Sets the column mode of a grid layout.
The column mode defines how the size of the column of a grid layout is measured.

* Applicable to: EE::UI::UIGridLayout (GridLayout)
* Data Type: [string-list](#string-list-data-type)
* Value List:
  * `size`: The column width of each grid child will be defined with a fixed width based on the value of the [column-width](#column-width) length.
  * `weight`: The column width of each grid child will be calculated as a normalized percentage of the total grid width using the [column-weight](#column-weight) value.
* Default value: `weight`

---

### column-margin

Sets the horizontal separation between each element in the grid layout.

* Applicable to: EE::UI::UIGridLayout (GridLayout)
* Data Type: [length](#length-data-type)
* Default value: `0dp`

---

### column-weight

Sets the percentage width of the child elements of a grid layout.
[column-mode](#column-mode) must be `weight` in order to work.

* Applicable to: EE::UI::UIGridLayout (GridLayout)
* Data Type: [number](#number-data-type) (normalized between `0` and `1`)
* Default value: `0.25`

---

### column-width

Sets the width of the child elements of a grid layout.

* Applicable to: EE::UI::UIGridLayout (GridLayout)
* Data Type: [length](#length-data-type)
* Default value: `0`

---

### cursor

Read [cursor](https://developer.mozilla.org/en-US/docs/Web/CSS/cursor) documentation.

Custom cursors not yet supported (but supported by the engine, only not implemented in the CSS side).

---

### display-percent

Enables/disables displaying the percentage of progress in the progress bar.

* Applicable to: EE::UI::UIProgressBar (ProgressBar)
* Data Type: [boolean](#boolean-data-type)
* Default value: `false`

---

### drag-resistance

Sets the amount of drag that is needed to start dragging an element.

* Applicable to: EE::UI::UIViewPager (ViewPager)
* Data Type: [length](#length-data-type)
* Default value: `8dp`

---

### droppable-hovering-color

Sets the foreground color to highlight the current dragged element that its is hovering
an element that accepts a drop of the dragged element.

* Applicatable to: EE::UI::UITabWidget (TabWidget), EE::UI::UIRoot (:root)
* Data Type: [color](#color-data-type)
* Default value: `#FFFFFF20`

---

### enabled

Sets if the element is enabled

* Applicable to: Any element
* Data Type: [boolean](#boolean-data-type)
* Default value: `true`

---

### font-family

Read [font-family](https://developer.mozilla.org/en-US/docs/Web/CSS/font-family) documentation.

---

### font-size

Read [font-size](https://developer.mozilla.org/en-US/docs/Web/CSS/font-size) documentation.

---

### font-style

Read [font-style](https://developer.mozilla.org/en-US/docs/Web/CSS/font-style) documentation.

---

### fill-color

Sets the fill color of a element.

* Applicable to: EE::UI::UILoader (Loader)
* Data Type: [color](#color-data-type)
* Default value: `white`

---

### flags

Allows to set some very specific flags to the element.
Multiple flags can be set, flags are separated by `|`.

* Applicable to: Any element
* Data Type: [string-list](#string-list-data-type)
* Value List:
  * `autosize`: This flag allows the widget/element to resize itself to a minimum size that allows the widget to be fully visible.
  * `clip`: Enables clipping of the element box.
  * `multiselect`: Enables multiple selection on elements that support selection. EE::UI::UIListBox (ListBox) for the moment.
  * `autopadding`: Enables the element to calculate the padding based on the skin size.
  * `reportsizechangetochilds`: When enabled the element will emmit a event (`OnParentSizeChange`) to its childs reporting the size change of the parent.
* Default value: _No value_

---

### foreground-color

Same as [background-color](#background-color) but for the foreground.

---

### foreground-image

Same as [background-image](#background-image) but for the foreground.

* Applicable to: Any element
* Data Type: [resource-path](#resource-path-data-type)
* Default value: `none`

---

### foreground-position-x

Same as [background-position-x](#background-position-x) but for the foreground.

---

### foreground-position-y

Same as [background-position-y](#background-position-y) but for the foreground.

---

### foreground-repeat

Same as [background-repeat](#background-repeat) but for the foreground.

---

### foreground-size

Same as [background-size](#background-size) but for the foreground.

---

### foreground-tint

Sets the tint color of the foreground image.

* Applicable to: Any element
* Data Type: [color](#color-data-type)
* Default value: `white`

---

### gravity

Sets the horizontal and vertical align of the inner elements/contents of a widget (for example the
text on a TextView). Vertical and horizontal can be set by using `|` as a separator of each rule.

* Applicable to: Any element. This applies at the EE::UI::UIWidget (Widget) level, the base element
  of all widgets. But every widget has it's own behaviour, based on it's own content. Some may not
  do anything.
* Data Type: [string-list](#string-list-data-type)
* Value List:
  * `left`
  * `right`
  * `center_horizontal`
  * `top`
  * `bottom`
  * `center_vertical`
  * `center`: shorthand of `center_horizontal|center_vertical`
* Default value: Varies on each widget implementation.
* Syntax:

```CSS
gravity: center;
gravity: left;
gravity: left|top;
gravity: center_horizontal|bottom;
```

---

### gravity-owner

Widgets elements can inform that own childs position using a special flag on its implementation 
(UI_OWNS_CHILDS_POSITION). Sometimes some layouts may need to override the parent behavior that uses
that flag. This flag will let the layout manage his gravity against its parent in all cases.

* Applicable to: Any layout element.
* Data Type: [boolean](#boolean-data-type)
* Default value: `false`

---

### half-slider

Defines how the slider widget should set its movement limits against the slider.
With `half-slider` enabled the button will start and end from its center (and not its border).

* Applicable to: EE::UI::UISlider (Slider)
* Data Type: [boolean](#boolean-data-type)
* Default value: `false`

---

### hint

Sets the hint text of an input. This is the equivalen of `placeholder` in HTML.
Usually this property will not be set in the CSS file but in the XML as an attribute or from the
code implementation, but it's available as an option.

* Applicable to: EE::UI::UITextInput (TextInput)
* Data Type: [string](#string-data-type)
* Default value: _No value_

---

### hint-color

Sets the hint font color.

* Applicable to: EE::UI::UITextInput (TextInput)
* Data Type: [color](#color-data-type)
* Default value: `white`

---

### hint-font-family

Sets the hint font family.

* Applicable to: EE::UI::UITextInput (TextInput)
* Data Type: [string](#string-data-type)
* Default value: _No value_

---

### hint-font-size

Sets the hint font size.

* Applicable to: EE::UI::UITextInput (TextInput)
* Data Type: [length](#length-data-type)
* Default value: `12px`

---

### hint-font-style

Sets the hint font style.

* Applicable to: EE::UI::UITextInput (TextInput)
* Data Type: [string-list](#string-list-data-type)
* Value List:
  * `underline`
  * `bold`
  * `italic`
  * `strikethrough`
  * `shadow`: Drops a shadow to the text.
* Default value: _No value_

---

### hint-shadow-color

Sets the hint font shadow color.

* Applicable to: EE::UI::UITextInput (TextInput)
* Data Type: [color](#color-data-type)
* Default value: `#323232E6`

---

### hint-stroke-color

Sets the hint font stroke (the outline) color.

* Applicable to: EE::UI::UITextInput (TextInput)
* Data Type: [color](#color-data-type)
* Default value: `black`

---

### hint-shadow-offset

Sets the hint font shadow offset.

* Applicable to: EE::UI::UITextView (TextView) and any element that holds inside text or extends from a
  TextView. EE::UI::UICheckBox (CheckBox), EE::UI::UIRadioButton (RadioButton), EE::UI::UITextInput
  (TextInput), EE::UI::UIListBoxItem (ListBox::item), EE::UI::UIDropDownList (DropDownList),
  EE::UI::UITextInputPassword (TextInputPassword), EE::UI::UIPushButton (PushButton), EE::UI::UIToolti
  (Tooltip)
* Data Type: [vector2-number](#vector2-number-data-type)
* Default offset: `1dp 1dp`

---

### hint-stroke-width

Sets the hint font stroke (the outline) width.

* Applicable to: EE::UI::UITextInput (TextInput)
* Data Type: [length](#length-data-type)
* Default value: `0dp`

---

### hscroll-mode

Sets the horizontal scroll mode to any scrollable element.
The mode defines the visibility of the horizontal scroll.

* Applicable to: EE::UI::DropDownList (DropDownList), EE::UI::UIListBox (ListBox),
  EE::UI::UIScrollView (ScrollView), EE::UI::UIWidgetTable (WidgetTable), EE::UI::UITextEdit (TextEdit).
* Data Type: [string-list](#string-list-data-type)
* Value List:
  * `auto`: The horizontal scrollbar will be visible only when needed.
  * `on`: The horizontal scrollbar will be always visible.
  * `off`: The horizontal scrollbar will be always hidden.
* Default value: `auto`

---

### icon

Sets an icon to an element that support icons.

* Applicable to: EE::UI::UIImage (Image), EE::UI::UIPushButton (PushButton), EE::UI::UITab (Tab)
* Data Type: [resource-path](#resource-path-data-type)
* Default value: _No value_

---

### indeterminate

Enables/disables indeterminate mode for a loader. Indeterminate mode is used when the progress of
the load can't be determined.

* Applicable to: EE::UI::UILoader (Loader)
* Data Type: [boolean](#boolean-data-type)
* Default value: `true`

---

### layout-gravity

The layout gravity defines how the element gravitates againts its parent (when possible). Gravity
specifies how a component should be placed in its group of cells. Must be one or more (separated by
`|`) of the _value list_ values. This behaves as the [android:layout_gravity](https://developer.android.com/reference/android/widget/LinearLayout.LayoutParams#attr_android:layout_gravity)
XML attribute (for those who are familiar with).

* Applicable to: Any element child of a layout element
* Data Type: [string-list](#string-list-data-type)
* Value List:
  * `left`: Push element to the left of its container.
  * `right`: Push element to the right of its container.
  * `center_horizontal`: Push element to the horizontal center of its container.
  * `top`: Push element to the top of its container.
  * `bottom`: Push element to the bottom of its container.
  * `center_vertical`: Place element in the vertical center of its container.
  * `center`: shorthand of `center_horizontal|center_vertical`
* Default value: `left|top`
* Aliases: `layout_gravity`, `lg`

---

### layout-height

Specifies the basic height of the element. This is a required attribute for any element inside of a
containing layout element that doesn't use any fixed size for its childs (ej: EE::UI::UILinearLayout,
EE::UI::UIRelativeLayout). Its value may be a length/dimension (such as `12dp`) for a constant height
or one of the special constants.

* Applicable to: Any element child of a EE::UI::UILinearLayout (LinearLayout) or
  EE::UI::UIRelativeLayout (RelativeLayout)
* Data Type: [length](#length-data-type) or string-list
* Value List: (for the constant values)
  * `match_parent`: The element must be as big as its parent.
  * `wrap_content`: The element should be only big enough to enclose its content (plus padding).
* Default value: `wrap_content`
* Aliases: `layout_height`, `lh`

---

### layout-to-bottom-of

Positions the top edge of this element below the given anchor element ID.
Accommodates top margin of this element and bottom margin of anchor view.

* Applicatable to: Any child of a EE::UI::UIRelativeLayout (RelativeLayout)
* Data Type: [string](#string-data-type) (anchor element ID)
* Default value: _No value_
* Aliases: `layout_to_bottom_of`

---

### layout-to-left-of

Positions the right edge of this element to the left of the given anchor element ID.
Accommodates right margin of this element and left margin of anchor element.

* Applicatable to: Any child of a EE::UI::UIRelativeLayout (RelativeLayout)
* Data Type: [string](#string-data-type) (anchor element ID)
* Default value: _No value_
* Aliases: `layout_to_left_of`

---

### layout-to-right-of

Positions the left edge of this element to the right of the given anchor element ID. Accommodates
left margin of this element and right margin of anchor element.

* Applicatable to: Any child of a EE::UI::UIRelativeLayout (RelativeLayout)
* Data Type: [string](#string-data-type) (anchor element ID)
* Default value: _No value_
* Aliases: `layout_to_right_of`

---

### layout-to-top-of

Positions the bottom edge of this element above the given anchor element ID.
Accommodates bottom margin of this element and top margin of anchor view.

* Applicatable to: Any child of a EE::UI::UIRelativeLayout (RelativeLayout)
* Data Type: [string](#string-data-type) (anchor element ID)
* Default value: _No value_
* Aliases: `layout_to_top_of`

---

### layout-weight

Indicates how much of the extra space in the EE::UI::UILinearLayout (LinearLayout) is allocated to
the element. Specify 0 if the element should not be stretched. Otherwise the extra pixels will be
pro-rated among all views whose weight is greater than 0. The weight sum of all elements must be 1
to fill the whole parent length. The weight is applied to the orientation that the LinearLayout uses.
This property behaves the same as the [android:layout_weight](https://developer.android.com/reference/android/widget/LinearLayout.LayoutParams#attr_android:layout_weight)
XML attribute in Android (for those who are familiar with it).
LinearLayout supports assigning a weight to individual children. This attribute assigns an
"importance" value to a element, and allows it to expand to fill any remaining space in the parent
view. Element/widget default weight is zero.
In order to work properly you must set the [layout-width](#layout-width) or the
[layout-height](#layout-height) to `0` to the element that uses layout weight. If the parent is a
horizontal LinearLayout the [layout-width](#layout-width) must be set to `0`, and if the parent if a
vertical LinearLayout the [layout-height](#layout-height) must be set to `0`, in order to use the
layout weight.

* Applicable to: Any element child of a EE::UI::UILinearLayout (LinearLayout)
* Data Type: [number](#number-data-type) (normalized from `0` to `1`)
* Default value: `0`
* Aliases: `layout_weight`, `lw8`

---

### layout-width

Specifies the basic width of the element. This is a required attribute for any element inside of a
containing layout element that doesn't use any fixed size for its childs (ej: EE::UI::UILinearLayout,
EE::UI::UIRelativeLayout). Its value may be a length/dimension (such as `12dp`) for a constant width
or one of the special constants.

* Applicable to: Any element child of a EE::UI::UILinearLayout (LinearLayout) or
  EE::UI::UIRelativeLayout (RelativeLayout)
* Data Type: [length](#length-data-type) or string-list
* Value List: (for the constant values)
  * `match_parent`: The element must be as big as its parent.
  * `wrap_content`: The element should be only big enough to enclose its content (plus padding).
* Default value: `wrap_content`
* Aliases: `layout_width`, `lw`

---

### line-spacing

Sets a extra line spacing to the line box.

* Applicable to: Any element child of a EE::UI::UICodeEditor (CodeEditor)
* Data Type: [length](#length-data-type)
* Default value: `0dp`

---

### locked

Enable or disable editing on code editor elements.

* Applicable to: EE::UI::UICodeEditor (TextEdit)
* Data Type: [boolean](#boolean-data-type)
* Default value: `false`

---

### margin-bottom

Read [margin-bottom](https://developer.mozilla.org/en-US/docs/Web/CSS/margin-bottom) documentation.

* Aliases: `layout_marginBottom`

---

### margin-left

Read [margin-left](https://developer.mozilla.org/en-US/docs/Web/CSS/margin-left) documentation.

* Aliases: `layout_marginLeft`

---

### margin-right

Read [margin-right](https://developer.mozilla.org/en-US/docs/Web/CSS/margin-right) documentation.

* Aliases: `layout_marginRight`

---

### margin-top

Read [margin-top](https://developer.mozilla.org/en-US/docs/Web/CSS/margin-top) documentation.

* Aliases: `layout_marginTop`

---

### max-height

Read [max-height](https://developer.mozilla.org/en-US/docs/Web/CSS/max-height) documentation.

---

### max-width

Read [max-width](https://developer.mozilla.org/en-US/docs/Web/CSS/max-width) documentation.

---

### max-edge-resistance

On a view pager the max edge resistance allows to scroll outside the edge limits by a percentage of
the element width/height (depending on the view pager orientation) and automatically go back to the
final position when released.

* Applicable to: EE::UI::UIViewPager (ViewPager)
* Data Type: [number](#number-data-type) (normalized between `0` and `1`)
* Default value: `0`

---

### max-length

Sets the maximum length of an input.

* Applicable to: EE::UI::UITextInput (TextInput)
* Data Type: integer
* Default value: _No value_

---

### max-progress

Sets the maximum number of progress that a loader can have.

* Applicable to: EE::UI::UILoader (Loader)
* Data Type: [number](#number-data-type)
* Default value: `100`

---

### max-tab-width

Sets the maximum tab width accepted in a tab widget.

* Applicable to: EE::UI::UITabWidget (TabWidget)
* Data Type: [length](#length-data-type)
* Default value: `300dp`

---

### max-text-length

Sets the maximum text length in a tab.

* Applicable to: EE::UI::UITabWidget (TabWidget)
* Data Type: integer
* Default value: `100`

---

### max-value

Sets the maximum value that a range element will hold.

* Applicable to: EE::UI::UISlider (Slider), EE::UI::UIScrollBar (ScrollBar), EE::UI::UISpinBox (SpinBox)
* Data Type: [number](#number-data-type)
* Default value: `1` for Slider and ScrollBar. Float numeric limits for SpinBox.

---

### max-visible-items

Sets the maximum visible items for the list shown by a drop down list or combo box.

* Applicable to: EE::UI::UIDropDownList (DropDownList)
* Data Type: integer
* Default value: `10`

---

### min-height

Read [min-height](https://developer.mozilla.org/en-US/docs/Web/CSS/min-height) documentation.

---

### min-icon-size

Sets the minimum size of the icon inside a push button (or any widget that extends from it, for
example a menu item or a tab). It's usually required for the EE::UI::UIMenu (Menu) sice the icon
box must have a size if the menu contains items with icons and others without, otherwise the menu
items will be misaligned.

* Applicatable to: EE::UI::UIPushButton (PushButton), EE::UI::UIMenuItem (Menu::item),
  EE::UI::UITab (Tab), EE::UI::UISelectButton (SelectButton), EE::UI::UIMenuCheckBox (Menu::checkbox),
  EE::UI::UIMenuSubMenu (Menu::SubMenu)
* Data Type: [vector2-length](#vector2-length-data-type)
* Default value: _No value_

---

### min-tab-width

Sets the minimum width that a tab can have inside a EE::UI::UITabWidget (TabWidget).

* Applicable to: EE::UI::UITabWidget (TabWidget)
* Data Type: [length](#length-data-type)
* Default value: `32dp`

---

### min-value

Sets the minimum value that a range element will hold.

* Applicable to: EE::UI::UISlider (Slider), EE::UI::UIScrollBar (ScrollBar), EE::UI::UISpinBox (SpinBox)
* Data Type: [number](#number-data-type)
* Default value: `0`

---

### min-width

Read [min-width](https://developer.mozilla.org/en-US/docs/Web/CSS/min-width) documentation.

---

### movement-speed

Sets the movement speed in displaced length per second of the progress bar filler.

* Applicable to: EE::UI::UIProgressBar (ProgressBar)
* Data Type: [vector2-length](#vector2-length-data-type)
* Default value: `0dp 0dp`

---

### numeric

Sets the text input as a numeric input, this means that only numbers will be accepted and
if [allow-float](#allow-float) also is enabled it will allow numeric float numbers.

* Applicable to: EE::UI::UITextInput (TextInput)
* Data Type: [boolean](#boolean-data-type)
* Default value: `false`

---

### opacity

Read [opacity](https://developer.mozilla.org/en-US/docs/Web/CSS/opacity) documentation.

---

### orientation

Sets the orientation to any element that allows vertical and horizontal orientation.

* Applicable to: EE::UI::UILinearLayout (LinearLayout), EE::UI::UISlider (Slider), EE::UI::UIScrollBar
  (ScrollBar), EE::UI::UIViewPager (ViewPager)
* Data Type: [string-list](#string-list-data-type)
* Value List:
  * `horizontal`
  * `vertical`
* Default value: `vertical` for LinearLayout, Slider and ScrollBar, `horizontal` for ViewPager

---

### outline-thickness

Sets the outline thickness of the loader spinner.

* Applicable to: EE::UI::UILoader (Loader)
* Data Type: [length](#length-data-type)
* Default value: `8dp`

---

### owns

Indicates the element ID that the Tab element owns. The ownership indicates that the element ID
is going to be shown when the Tab element is selected, and will be closed if the Tab is closed.
The owned element will be automatically resized to the tab widget container size.

* Applicable to: EE::UI::UITab (Tab)
* Data Type: [string](#string-data-type) (element ID)
* Default value: _No value_

---

### padding-bottom

Read [padding-bottom](https://developer.mozilla.org/en-US/docs/Web/CSS/padding-bottom) documentation.

---

### padding-left

Read [padding-left](https://developer.mozilla.org/en-US/docs/Web/CSS/padding-left) documentation.

---

### padding-right

Read [padding-right](https://developer.mozilla.org/en-US/docs/Web/CSS/padding-right) documentation.

---

### padding-top

Read [padding-top](https://developer.mozilla.org/en-US/docs/Web/CSS/padding-top) documentation.

---

### page-locked

Locks/unlocks the current page in a view pager (locking means that the page cannot be changed and it's
not scrollable).

* Applicable to: EE::UI::UIViewPager (ViewPager)
* Data Type: [boolean](#boolean-data-type)
* Default value: `false`

---

### page-step

Sets the length value that needs the element to scroll a page. Currently the page step it's not used
by any element but it's a value that can be fetched to implement a page up and down in a scrollable
component.

* Applicable to: EE::UI::UISlider (Slider), EE::UI::UIProgressBar (ProgressBar)
* Data Type: [number](#number-data-type)
* Default value: `0`

---

### popup-to-root

Sets if the drop down listbox should popup in the root widget node, otherwise will popup in the
parent window.

* Applicable to: EE::UI::UIDropDownList (DropDownList)
* Data Type: [boolean](#boolean-data-type)
* Default value: `false`

---

### progress

Sets the current progress of any type of progress bar or loader.

* Applicable to: EE::UI::UIProgresBar (ProgressBar), EE::UI::UILoader (Loader)
* Data Type: [number](#number-data-type)
* Default value: `0`

---

### radius

Sets the radius of a loader. This will not change the size of the loader,
only the loader internal size.

* Applicable to: EE::UI::UILoader (Loader)
* Data Type: [length](#length-data-type)
* Default value: `0` (but it will change with the loader layout size)

---

### reverse-draw

Enables/disables the reverse draw order for the element. When enabled the element will draw from
the last child to the first.

* Applicable to: Any element
* Data Type: [boolean](#boolean-data-type)
* Default value: `false`

---

### rotation

Sets the element rotation in degrees.

* Applicable to: Any element
* Data Type: [number](#number-data-type)
* Default value: `0`
* Aliases: `rotate`

---

### rotation-origin-point-x

Sets the x-axis rotation origin point relative to the element expressed as a length or a percentage.

* Applicable to: Any element
* Data Type: [length-percentage](#length-percentage-data-type)
* Default value: `center`
* Aliases: `rotate-origin-point-x`

Syntax:

```CSS
rotation-origin-point-x: 50dp;
rotation-origin-point-x: center;
rotation-origin-point-x: 100%;
```

---

### rotation-origin-point-y

Sets the y-axis rotation origin point relative to the element expressed as a length or a percentage.

* Applicable to: Any element
* Data Type: [length-percentage](#length-percentage-data-type)
* Default value: `center`
* Aliases: `rotate-origin-point-y`

Syntax:

```CSS
rotation-origin-point-y: 50dp;
rotation-origin-point-y: center;
rotation-origin-point-y: 100%;
```

---

### row-height

Sets the row height in any element that contains fixed size rows.

* Applicable to: EE::UI::UIListBox (ListBox), EE::UI::UIDropDownList (DropDownList), EE::UI::UIWidgetTable (WidgetTable), EE::UI::UIGridLayout (GridLayout), EE::UI::TableView (TableView), EE::UI::TreeView (TreeView), EE::UI::ListView (ListView)
* Data Type: [length](#length-data-type)
* Default value: Varies on each case. ListBox and DropDownList will guess the value based on the [font-size](#font-size). Table requires this value to be manually set in order to work. GridLayout
will require the value only if [row-mode](#row-mode) is `size`.

---

### row-mode

Sets the row mode of a grid layout.
The row mode defines how the size of the row of a grid layout is measured.

* Applicable to: EE::UI::UIGridLayout (GridLayout)
* Data Type: [string-list](#string-list-data-type)
* Value List:
  * `size`: The row width of each grid child will be defined with a fixed width based on the value of the [row-width](#row-width) length.
  * `weight`: The row width of each grid child will be calculated as a normalized percentage of the total grid width using the [row-weight](#row-weight) value.
* Default value: `weight`

---

### row-margin

Sets the vertical separation between each element in the grid layout.

* Applicable to: EE::UI::UIGridLayout (GridLayout)
* Data Type: [length](#length-data-type)
* Default value: `0dp`

---

### row-weight

Sets the percentage height of the child elements of a grid layout.
[row-mode](#row-mode) must be `weight` in order to work.

* Applicable to: EE::UI::UIGridLayout (GridLayout)
* Data Type: [number](#number-data-type) (normalized between `0` and `1`)
* Default value: `0.25`

---

### scale

Sets the scale of the element.

* Applicable to: Any element
* Data Type: [vector2-number](#vector2-number-data-type)
* Default value: `1 1`

---

### scale-origin-point-x

Sets the x-axis scale origin point relative to the element expressed as a length or a percentage.

* Applicable to: Any element
* Data Type: [length-percentage](#length-percentage-data-type)
* Default value: `center`

Syntax:

```CSS
scale-origin-point-x: 50dp;
scale-origin-point-x: center;
scale-origin-point-x: 100%;
```

---

### scale-origin-point-y

Sets the y-axis scale origin point relative to the element expressed as a length or a percentage.

* Applicable to: Any element
* Data Type: [length-percentage](#length-percentage-data-type)
* Default value: `center`

Syntax:

```CSS
scale-origin-point-y: 50dp;
scale-origin-point-y: center;
scale-origin-point-y: 100%;
```

---

### scale-type

Sets a method to scale the image inside an element.

* Applicable to: EE::UI::UIImage (Image), EE::UI::UITextureRegion (TextureRegion)
* Data Type: [string-list](#string-list-data-type)
* Value List:
  * `fit-inside`: Fits the image inside the element mantaining its ratio.
  * `expand`: Expands the image to cover the whole element without mantaining its ratio.
  * `none`: Keeps the image as it is.

---

### scrollbar-mode

Defines how the scrollbar should fit inside an scrollable element.

* Applicable to: EE::UI::UIScrollView (ScrollView)
* Data Type: [string-list](#string-list-data-type)
* Value List:
  * `inclusive`: The scrollbar is part of the scrollable element container, this means that it will
	be on top of the content (scrollbars on mobile usually behave like this).
  * `exclusive`: The scrollbar is outside the scrollable element container, this means that it will not
	be on top of the content (scrollbars on desktop usually behave like this).
* Default value: `exclusive`

---

### scrollbar-style

Sets the scrollbar style (for the moment defines if it contains buttons at its edge or it's simply a bar).

* Applicable to: EE::UI::UIScrollBar (ScrollBar), EE::UI::UIListBox (ListBox), EE::UI::UIDropDownList
  (DropDownList), EE::UI::UIWidgetTable (WidgetTable), EE::UI::UIScrollView (ScrollView)
* Data Type: [string-list](#string-list-data-type)
* Value List:
  * `no-buttons`: It's just a scroll bar.
  * `two-buttons`: Adds to buttons at its edge.
* Default value: `two-buttons` for desktop platforms, `no-button` for mobile.

---

### selected

Sets the element state as selected when supported by the specific widget.

* Applicable to: EE::UI::UICheckBox (CheckBox), EE::UI::UIRadioButton (RadioButton), EE::UI::SelectButton (SelectButton)
* Data Type: [boolean](#boolean-data-type)
* Default value: `false`
* Aliases: `active`

---

### selected-index

Selects an item inside an element containing a list of items from the item index in the list
(starting from `0`).

* Applicable to: EE::UI::UIListBox (ListBox), EE::UI::UIDropDownList (DropDownList)
* Data Type: integer
* Default value: _No value_

---

### selectex-text

Selects an item inside an element containing a list of items from its item text.

* Applicable to: EE::UI::UIListBox (ListBox), EE::UI::UIDropDownList (DropDownList)
* Data Type: [string](#string-data-type)
* Default value: _No value_

---

### selection-back-color

Sets the text selection background color on a text element that suports text selection.

* Applicable to: EE::UI::UITextView (TextView) and any element that holds inside or extends from a
  TextView. EE::UI::UICheckBox (CheckBox), EE::UI::UIRadioButton (RadioButton), EE::UI::UITextInput
  (TextInput), EE::UI::UIListBoxItem (ListBox::item), EE::UI::UIDropDownList (DropDownList),
  EE::UI::UITextInputPassword (TextInputPassword)
* Data Type: [color](#color-data-type)
* Default color: `#323232`

---

### selection-color

Sets the text selection color on a text element that suports text selection.

* Applicable to: EE::UI::UITextView (TextView) and any element that holds inside text or extends from a
  TextView. EE::UI::UICheckBox (CheckBox), EE::UI::UIRadioButton (RadioButton), EE::UI::UITextInput
  (TextInput), EE::UI::UIListBoxItem (ListBox::item), EE::UI::UIDropDownList (DropDownList),
  EE::UI::UITextInputPassword (TextInputPassword), EE::UI::UIPushButton (PushButton)
* Data Type: [color](#color-data-type)
* Default color: `white`

---

### select-on-click

Enables the selection state toggle on element click.

* Applicable to: EE::UI::SelectButton (SelectButton)
* Data Type: [boolean](#boolean-data-type)
* Default value: `false`

---

### skin

Sets the skin (a.k.a. decoration) to an element. Element skins are automatically applied if there's a specific skin for the element
loaded in the current theme, but new skins can be used and loaded with this property. Skins are a way
to decorate the widgets, they are loaded from a list of textures or from a texture atlas. Skins are stateful,
this means that a skin can contain all the possible states that a widget can have for decoration, these are: `normal`, `focus`, `selected`, `hover`, `pressed`, `selectedhover`, `selectedpressed`, `disabled`, `checked`.
Current UI skin documentation is lacking.

* Applicable to: Any element
* Data Type: [string](#string-data-type) (skin name or `none` to disable skinning)
* Default value: The skin corresponding to the widget.

---

### skin-color

Sets the [skin](#skin) tint color.

* Applicable to: Any element
* Data Type: [color](#color-data-type)
* Default value: `white` (no tint)

---

### splitter-always-show

Sets if the splitter divisor is always visible. If false it will be shown only if two views are
attached to the splitter.

* Applicable to: EE::UI::UISplitter (Splitter)
* Data Type: [boolean](#boolean-data-type)
* Default value: `true`

---

### splitter-partition

Sets the space ocuppied by the first view contained by the splitter.

* Applicable to: EE::UI::UISplitter (Splitter)
* Data Type: [length-percentage](#length-percentage-data-type)
* Default value: `50%`

---

### src

Sets the source of a resource in an element that supports source.

* Applicable to: EE::UI::UIImage (Image), EE::UI::UISprite (Sprite), EE::UI::UITextureRegion (TextureRegion)
* Data Type: [resource-path](#resource-path-data-type)
* Default value: _No value_

---

### tabbar-hide-on-single-tab

Enables/disables the tab bar hiding when only one tab is contained in the tab widget.

* Applicable to: EE::UI::UITabWidget (TabWidget)
* Data Type: [boolean](#boolean-data-type)
* Default value: `false`

---

### tabbar-allow-rearrange

Enables/disables manually rearraging the tabs in the tab bar.

* Applicable to: EE::UI::UITabWidget (TabWidget)
* Data Type: [boolean](#boolean-data-type)
* Default value: `false`

---

### tabbar-allow-drag-and-drop-tabs

Enables/disables the hability to move any tab from a TabWidget to another.

* Applicable to: EE::UI::UITabWidget (TabWidget)
* Data Type: [boolean](#boolean-data-type)
* Default value: `false`

---

### tabbar-allow-switch-tabs-in-empty-spaces

Enables/disables switching tabs from empty/blank spaces in the tab widget.

* Applicable to: EE::UI::UITabWidget (TabWidget)
* Data Type: [boolean](#boolean-data-type)
* Default value: `false`

---

### tab-closable

Enables/disables tabs to be closable with the middle mouse button click or by clicking on the close
button.

* Applicable to: EE::UI::UITabWidget (TabWidget)
* Data Type: [boolean](#boolean-data-type)
* Default value: `false`

---

### tabs-edges-diff-skin

Enable/disable the tabs edges (most left and right) to use a different skin/decoration from the other tabs.
The most left tab will use the skin: `tab_left` and the most right tab will use `tab_right`. instead of using
the normal `tab` skin. This is to be able to achieve more "closed" tab widgets.

* Applicable to: EE::UI::UITabWidget (TabWidget)
* Data Type: [boolean](#boolean-data-type)
* Default value: `false`

---

### tab-height

Sets the tab widget tabs container height. If not set it will try to get the height from the tab
skin height.

* Applicable to: EE::UI::UITabWidget (TabWidget)
* Data Type: [length](#length-data-type)
* Default value: `0dp`

### tab-separation

Sets the tabs separation/distance between each other. Negative values are accepted.

* Applicable to: EE::UI::UITabWidget (TabWidget)
* Data Type: [length](#length-data-type)
* Default value: `0dp`

---

### text

Sets the text to any element that holds or contains text.
Usually this property will not be set in the CSS file but in the XML as an attribute or from the
code implementation, but it's available as an option.

* Applicable to: EE::UI::UITextView (TextView) and any element that holds inside or extends from a
  TextView. EE::UI::UICheckBox (CheckBox), EE::UI::UIRadioButton (RadioButton), EE::UI::UITextInput
  (TextInput), EE::UI::UIListBoxItem (ListBox::item), EE::UI::UIDropDownList (DropDownList),
  EE::UI::UITextInputPassword (TextInputPassword), EE::UI::UITooltip (Tooltip), EE::UI::UITab (Tab),
  EE::UI::UITextEdit (TextEdit)
* Data Type: [string](#string-data-type)
* Default value: _No value_

---

### text-align

Read [text-align](https://developer.mozilla.org/en-US/docs/Web/CSS/text-align) documentation.

---

### text-as-fallback

When enabled will only display text if the icon is not set/found.

* Applicable to: EE::UI::UIPushButton (PushButton)
* Data Type: [boolean](#boolean-data-type)
* Default value: `false`

---

### text-selection

Enables/disables text selection in any element that contains text.

* Applicable to: EE::UI::UITextView (TextView) and any element that holds inside or extends from a
  TextView. EE::UI::UICheckBox (CheckBox), EE::UI::UIRadioButton (RadioButton), EE::UI::UITextInput
  (TextInput), EE::UI::UIListBoxItem (ListBox::item), EE::UI::UIDropDownList (DropDownList),
  EE::UI::UITextInputPassword (TextInputPassword), EE::UI::UITooltip (Tooltip), EE::UI::UITab (Tab),
  EE::UI::UITextEdit (TextEdit)
* Data Type: [boolean](#boolean-data-type)
* Default value: `true` for TextEdit, TextInput. `false` for any other element.

---

### text-shadow-color

Sets the text shadow color.

* Applicable to: EE::UI::UITextView (TextView) and any element that holds inside text or extends from a
  TextView. EE::UI::UICheckBox (CheckBox), EE::UI::UIRadioButton (RadioButton), EE::UI::UITextInput
  (TextInput), EE::UI::UIListBoxItem (ListBox::item), EE::UI::UIDropDownList (DropDownList),
  EE::UI::UITextInputPassword (TextInputPassword), EE::UI::UIPushButton (PushButton), EE::UI::UIToolti
  (Tooltip)
* Data Type: [color](#color-data-type)
* Default color: `#323232E6`

---

### text-shadow-offset

Sets the text shadow offset.

* Applicable to: EE::UI::UITextView (TextView) and any element that holds inside text or extends from a
  TextView. EE::UI::UICheckBox (CheckBox), EE::UI::UIRadioButton (RadioButton), EE::UI::UITextInput
  (TextInput), EE::UI::UIListBoxItem (ListBox::item), EE::UI::UIDropDownList (DropDownList),
  EE::UI::UITextInputPassword (TextInputPassword), EE::UI::UIPushButton (PushButton), EE::UI::UIToolti
  (Tooltip)
* Data Type: [vector2-number](#vector2-number-data-type)
* Default offset: `1dp 1dp`

---

### text-stroke-color

Sets the text stroke (also known as text outline) color.

* Applicable to: EE::UI::UITextView (TextView) and any element that holds inside or extends from a
  TextView.
* Data Type: [color](#color-data-type)
* Default color: `black`
* Aliases: `fontOutlineColor`

---

### text-stroke-width

Sets the text stroke (also known as text outline) width/thickness.

* Applicable to: EE::UI::UITextView (TextView) and any element that holds inside or extends from a
  TextView.
* Data Type: [length](#length-data-type)
* Default value: `0dp`
* Aliases: `fontOutlineThickness`

---

### text-transform

The text-transform CSS property specifies how to capitalize an element's text. It can be used to 
make text appear in all-uppercase or all-lowercase, or with each word capitalized.

* Applicable to: EE::UI::UITextView (TextView) and any element that holds inside or extends from a
  TextView, EE::UI::UITooltip (Tooltip).
* Data Type: [string-list](#string-list-data-type)
* Value List:
  * `lowercase`
  * `uppercase`
  * `capitalize`
  * `none`
* Default value: `none`

---

### theme

Sets a theme to the element. Changing the theme will not have any effect unless [skin](#skin) is also set.

* Applicable to: Any element
* Data Type: [string](#string-data-type)
* Default value: _No value_

---

### timing-function

Sets the timing function (easing function) of a view pager.

* Applicable to: EE::UI::UIViewPager (ViewPager)
* Data Type: [string](#string-data-type)
* Value List:
  * Current timing functions supported: linear, quadratic-in, quadratic-out, quadratic-in-out,
  sine-in, sine-out, sine-in-out, exponential-in, exponential-out, exponential-in-out, quartic-in,
  quartic-out, quartic-in-out, circular-in, circular-out, circular-in-out, cubic-in, cubic-out,
  cubic-in-out, back-in, back-out, back-in-out, bounce-in, bounce-out, bounce-in-out, elastic-in,
  elastic-out, elastic-in-out, none.
* Default value: `sine-in`

---

### tint

Sets the tint color of an image.

* Applicable to: EE::UI::UIImage (Image), EE::UI::UITextureRegion (TextureRegion)
* Data Type: [color](#color-data-type)
* Default value: `white` (no tint)

---

### tooltip

Sets the tooltip text and enables the tooltip to the element.
Usually this property will not be set in the CSS file but in the XML as an attribute or from the
code implementation, but it's available as an option.

* Applicable to: Any element
* Data Type: [string](#string-data-type)
* Default value: _No value_

---

### total-steps

Sets the total steps required to complete a progress bar (the maximum value a progress bar will hold).

* Applicable to: EE::UI::UIProgressBar (ProgressBar)
* Data Type: [number](#number-data-type)
* Default value: `100`

---

### touch-drag

Enables/disables the element as draggable. When enabled the element will be able to be dragged.

* Applicable to: EE::UI::UITouchDraggable (TouchDraggable), (EE::UI::UIListBox (ListBox),
  EE::UI::UIScrollView (ScrollView), EE::UI::UIWidgetTable (WidgetTable)
* Data Type: [boolean](#boolean-data-type)
* Default value: `false`

---

### touch-drag-deceleration

Sets a acceleration value that defines how much will accelerate the drag when the mouse or touch is released.

* Applicable to: EE::UI::UITouchDraggable (TouchDraggable), (EE::UI::UIListBox (ListBox),
  EE::UI::UIScrollView (ScrollView), EE::UI::UIWidgetTable (WidgetTable)
* Data Type: [vector2-number](#vector2-number-data-type)
* Default value: `5 5`

---

### transition-delay

Read [transition-delay](https://developer.mozilla.org/en-US/docs/Web/CSS/transition-delay) documentation.

---

### transition-duration

Read [transition-duration](https://developer.mozilla.org/en-US/docs/Web/CSS/transition-duration) documentation.

---

### transition-property

Read [transition-property](https://developer.mozilla.org/en-US/docs/Web/CSS/transition-property) documentation.

---

### transition-timing-function

Read [transition-timing-function](https://developer.mozilla.org/en-US/docs/Web/CSS/transition-timing-function) documentation.

Timing function names are custom, and not the same as the standard, but `cubic-bezier` is supported.
Current timing functions supported: linear, quadratic-in, quadratic-out, quadratic-in-out,
sine-in, sine-out, sine-in-out, exponential-in, exponential-out, exponential-in-out, quartic-in,
quartic-out, quartic-in-out, circular-in, circular-out, circular-in-out, cubic-in, cubic-out,
cubic-in-out, back-in, back-out, back-in-out, bounce-in, bounce-out, bounce-in-out, elastic-in,
elastic-out, elastic-in-out, cubic-bezier, none.

### value

Sets the current value to an element that accepts values.

* Applicable to: EE::UI::UIScrollBar (ScrollBar), EE::UI::UISlider (Slider), EE::UI::UISpinBox (SpinBox)
* Data Type: [number](#number-data-type)
* Default value: _Not set_

---

### vertical-expand

In a progress bar enables/disables if the progress bar filler should be expanded vertically to the element
height. Whend disabled the filler height will be defined by the filler skin height.

* Applicable to: EE::UI::UIScrollBar (ScrollBar)
* Data Type: [boolean](#boolean-data-type)
* Default value: `true`

---

### visible

Enables/disables the element visibility.

* Applicable to: Any element
* Data Type: [boolean](#boolean-data-type)
* Default value: `true`

---

### vscroll-mode

Sets the vertical scroll mode to any scrollable element.
The mode defines the visibility of the vertical scroll.

* Applicable to: EE::UI::DropDownList (DropDownList), EE::UI::UIListBox (ListBox),
  EE::UI::UIScrollView (ScrollView), EE::UI::UIWidgetTable (WidgetTable), EE::UI::UITextEdit (TextEdit).
* Data Type: [string-list](#string-list-data-type)
* Value List:
  * `auto`: The vertical scrollbar will be visible only when needed.
  * `on`: The vertical scrollbar will be always visible.
  * `off`: The vertical scrollbar will be always hidden.
* Default value: `auto`

---

### window-border-auto-size

Sets if the window border size should be automatically calculated based on the skin size.

* Applicable to: EE::UI::UIWindow (Window)
* Data Type: [boolean](#boolean-data-type)
* Default value: `true`

---

### window-border-size

Manually sets the size of the window border.

* Applicable to: EE::UI::UIWindow (Window)
* Data Type: [vector2-length](#vector2-length-data-type)
* Default value: _No value_

---

### window-buttons-offset

Sets the window titlebar buttons position offset.

* Applicable to: EE::UI::UIWindow (Window)
* Data Type: [vector2-length](#vector2-length-data-type)
* Default Value: `0dp 0dp`

---

### window-buttons-separation

Sets the separation between the window titlebar buttons.

* Applicable to: EE::UI::UIWindow (Window)
* Data Type: [length](#length-data-type)
* Default value: `4dp`

---

### window-corner-distance

Sets the minimum distance where the hovering cursor detects an edge of the window to enable edge
resizing.

* Applicable to: EE::UI::UIWindow (Window)
* Data Type: [length](#length-data-type)
* Default value: `24dp`

---

### window-flags

Sets the window flags. Window flags activate a set of different boolean options of the window.
Multiple values are separated with a `|` (as a logical or).

* Applicable to: EE::UI::UIWindow (Window)
* Data Type: [string-list](#string-list-data-type)
* Value List:
  * `default`: same as `close|buttonactions|resizeable|shareopacity`
  * `close`: Adds a close button to the titlebar.
  * `maximize`: Adds a maximize button to the titlebar.
  * `minimize`: Adds a minimize button to the titlebar.
  * `draggable`: Makes the window background draggable.
  * `shadow`: The window will project a shadow.
  * `modal`: Makes the window modal (blocks interacting any element ouside the window).
  * `undecorated`: Creates an undecorated window (no borders and titlebar).
  * `resizeable`: Makes the window resizeable.
  * `shareopacity`: Childs will inherit the window opacity (otherwise if you change the opacity of the window will only affect its borders, background and titlebar).
  * `buttonactions`: Implements the window minimize, maximize and close buttons default actions. This means that close button click will close, minimize will hide (but not close) and maximize will maximize.
  * `framebuffer`: Creates the window on its own frame buffer.
  * `colorbuffer`: Enables the color buffer to the frame buffer (if active).
* Default value: `default`

---

### window-min-size

Sets the window minimum size.

* Applicable to: EE::UI::Window (Window)
* Data Type: [vector2-length](#vector2-length-data-type)
* Default value: _No value_

---

### window-opacity

Sets the base opacity of a window, this is the maximum opacity that the window will be able to have.
And the opacity property applied to the window will be multiplied by this value.

* Applicable to: EE::UI::UIWindow (window)
* Data Type: [number](#number-data-type)
* Default value: `1`

---

### window-title

Sets the window title.

* Applicable to: EE::UI::UIWindow (Window)
* Data Type: [string](#string-data-type)
* Default value: _No value_

### window-titlebar-auto-size

Sets if the window titlebar size should be automatically calculated based on the skin size.

* Applicable to: EE::UI::UIWindow (Window)
* Data Type: [boolean](#boolean-data-type)
* Default value: `true`

---

### window-titlebar-size

Manually sets the size of the window titlebar.

* Applicable to: EE::UI::UIWindow (Window)
* Data Type: [vector2-length](#vector2-length-data-type)
* Default value: _No value_.

---

### word-wrap

Enables/disables word-wrap in the text view element.

* Applicable to: EE::UI::UITextVIew (TextView), EE::UI::UITextInput (TextInput),
  EE::UI::UICheckBox (CheckBox), EE::UI::UIRadioButton (RadioButton),
  EE::UI::UIListBoxItem (ListBox::item), EE::UI::UITextInputPassword (TextInputPassword),
  EE::UI::UIDropDownList (DropDownList).
* Data Type: [boolean](#boolean-data-type)
* Default value: `false`

---

## CSS Shorthands

[Shorthand properties](https://developer.mozilla.org/en-US/docs/Web/CSS/Shorthand_properties) are
CSS properties that let you set the values of multiple other CSS properties simultaneously.
Using a shorthand property, you can write more concise (and often more readable) style sheets,
saving time and energy.

---

### animation

Read [animation](https://developer.mozilla.org/en-US/docs/Web/CSS/animation) documentation.

---

### background

Read [background](https://developer.mozilla.org/en-US/docs/Web/CSS/background) documentation.
Only eepp CSS properties supported are valid in the shorthand.

---

### background-position

Read [background-position](https://developer.mozilla.org/en-US/docs/Web/CSS/background-position) documentation.

---

### border

Read [border](https://developer.mozilla.org/en-US/docs/Web/CSS/border) documentation.
`border-style` is not implemented yet.

---

### border-color

Read [border-color](https://developer.mozilla.org/en-US/docs/Web/CSS/border-color) documentation.

---

### border-radius

Read [border-radius](https://developer.mozilla.org/en-US/docs/Web/CSS/border-radius) documentation.

---

### border-width

Read [border-width](https://developer.mozilla.org/en-US/docs/Web/CSS/border-width) documentation.

---

### box-margin

Shorthand for [column-margin](#column-margin) and [row-margin](#row-margin) (in that order).

The syntax consists in two lengths separated by a space.

Syntax:

```CSS
box-margin: 10dp 10dp;
```

---

### foreground

Same as the [background](#background) but for the foreground of the node.
The foreground is rendered on the top of the element.

---

### foreground-position

Same as the [background-position](#background-position) but for the foreground of the node.

---

### foreground-radius

Same as the [border-radius](#border-radius) but for the foreground of the node.

---

### hint-shadow

The hint text shadow, same as text-shadow but for hints.

Read [text-shadow](https://developer.mozilla.org/en-US/docs/Web/CSS/text-shadow) documentation.

blur-radius is not supported.

---

### margin

Read [margin](https://developer.mozilla.org/en-US/docs/Web/CSS/margin) documentation.

---

### min-size

Shorthand for [min-width](#min-width) and [min-height](#min-height)

---

### max-size

Shorthand for [max-width](#max-width) and [max-height](#max-height)

---

### padding

Read [padding](https://developer.mozilla.org/en-US/docs/Web/CSS/padding) documentation.

---

### rotation-origin-point

The origin point of the rotation expressed as a [position](#position-data-type).

Default value: `center center`.
Aliases: `rotate-origin-point`

Syntax:

```CSS
rotation-origin-point: 50dp 10dp;
rotation-origin-point: center top;
rotation-origin-point: 100% 0%;
```

---

### scale-origin-point

The origin point of the node scale expressed as [position](#position-data-type).

Default value: `center center`.

Syntax:

```CSS
scale-origin-point: 50dp 10dp;
scale-origin-point: center top;
scale-origin-point: 100% 0%;
```

---

### transition

Read [transition](https://developer.mozilla.org/en-US/docs/Web/CSS/transition) documentation.

### text-shadow

Read [text-shadow](https://developer.mozilla.org/en-US/docs/Web/CSS/text-shadow) documentation.

blur-radius is not supported.

---

## At-rules

[At-rules](https://developer.mozilla.org/en-US/docs/Web/CSS/At-rule) are CSS statements that
instructs CSS how to behave. They begin with an at sign, '@' (U+0040 COMMERCIAL AT), followed by an
identifier and includes everything up to the next semicolon, ';' (U+003B SEMICOLON), or the next CSS
block, whichever comes first.

The following at-rules are currently supported:

* [@import](https://developer.mozilla.org/en-US/docs/Web/CSS/@import) —
  Tells the CSS engine to include an external style sheet.

* [@media](https://developer.mozilla.org/en-US/docs/Web/CSS/@media) — A conditional group rule that
  will apply its content if the device meets the criteria of the condition defined using a media
  query. Media features currently supported:
  [width](https://developer.mozilla.org/en-US/docs/Web/CSS/@media/width),
  [height](https://developer.mozilla.org/en-US/docs/Web/CSS/@media/height),
  [device-width](https://developer.mozilla.org/en-US/docs/Web/CSS/@media/device-width),
  [device-height](https://developer.mozilla.org/en-US/docs/Web/CSS/@media/device-height),
  [orientation](https://developer.mozilla.org/en-US/docs/Web/CSS/@media/orientation),
  [aspect-ratio](https://developer.mozilla.org/en-US/docs/Web/CSS/@media/aspect-ratio),
  [device-aspect-ratio](https://developer.mozilla.org/en-US/docs/Web/CSS/@media/device-aspect-ratio),
  [color](https://developer.mozilla.org/en-US/docs/Web/CSS/@media/color),
  [color-index](https://developer.mozilla.org/en-US/docs/Web/CSS/@media/color-index),
  [monochrome](https://developer.mozilla.org/en-US/docs/Web/CSS/@media/monochrome),
  [resolution](https://developer.mozilla.org/en-US/docs/Web/CSS/@media/resolution),
  [prefers-color-scheme](https://developer.mozilla.org/en-US/docs/Web/CSS/@media/prefers-color-scheme).
  Note: eepp prefers dark as default.

  non-standard media features:
  *pixel-density*:
  The `pixel-density` feature is specified as a `float` value representing the current pixel density
  used by the application. It is a range feature, meaning that you can also use the prefixed
  min-pixel-density and max-pixel-density variants to query minimum and maximum values, respectively.

  Also, eepp only uses `screen` media type.

* [@font-face](https://developer.mozilla.org/en-US/docs/Web/CSS/@font-face) —
  Describes the aspect of an external font to be downloaded.
  Only [font-family](https://developer.mozilla.org/en-US/docs/Web/CSS/@font-face/font-family) and
  [src](https://developer.mozilla.org/en-US/docs/Web/CSS/@font-face/src) properties are supported at
  the moment.

* [@keyframes](https://developer.mozilla.org/en-US/docs/Web/CSS/@keyframes) — Describes the aspect
  of intermediate steps in a CSS animation sequence.

## Pseudo-class selectors

A [CSS pseudo-class](https://developer.mozilla.org/en-US/docs/Web/CSS/Pseudo-classes) is a keyword
added to a selector that specifies a special state of the selected element(s). For example, :hover
can be used to change a button's color when the user's pointer hovers over it.

Most common pseudo-class selectors are supported. The list is:

* [:hover](https://developer.mozilla.org/en-US/docs/Web/CSS/:hover)

* [:focus](https://developer.mozilla.org/en-US/docs/Web/CSS/:focus)

* [:focus-within](https://developer.mozilla.org/en-US/docs/Web/CSS/:focus-within)

* [:checked](https://developer.mozilla.org/en-US/docs/Web/CSS/:checked)

* [:disabled](https://developer.mozilla.org/en-US/docs/Web/CSS/:disabled)

* [:empty](https://developer.mozilla.org/en-US/docs/Web/CSS/:empty)

* [:enabled](https://developer.mozilla.org/en-US/docs/Web/CSS/:enabled)

* [:active or :pressed](https://developer.mozilla.org/en-US/docs/Web/CSS/:active)

* [:first-child](https://developer.mozilla.org/en-US/docs/Web/CSS/:first-child)

* [:first-of-type](https://developer.mozilla.org/en-US/docs/Web/CSS/:first-of-type)

* [:last-child](https://developer.mozilla.org/en-US/docs/Web/CSS/:last-child)

* [:last-of-type](https://developer.mozilla.org/en-US/docs/Web/CSS/:last-of-type)

* [:not()](https://developer.mozilla.org/en-US/docs/Web/CSS/:not)

* [:nth-child()](https://developer.mozilla.org/en-US/docs/Web/CSS/:nth-child)

* [:nth-last-child()](https://developer.mozilla.org/en-US/docs/Web/CSS/:nth-last-child)

* [:nth-last-of-type()](https://developer.mozilla.org/en-US/docs/Web/CSS/:nth-last-of-type)

* [:nth-of-type()](https://developer.mozilla.org/en-US/docs/Web/CSS/:nth-of-type)

* [:only-child](https://developer.mozilla.org/en-US/docs/Web/CSS/:only-child)

* [:only-of-type](https://developer.mozilla.org/en-US/docs/Web/CSS/:only-of-type)

* [:root](https://developer.mozilla.org/en-US/docs/Web/CSS/:root)

* :selected Matches when elements are selectable and currently selected.

## CSS Data Types

CSS data types define typical values (including keywords and units) accepted by CSS properties and functions.

For more details regarding this topic please read the
[CSS values and units Mozilla docs](https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Types).

**Types:**

### boolean (data-type)

Binary state with two possible values:

* `true`
  * Aliases: `1`, `t`, `T`, `y`, `Y`
* `false`
  * Aliases: `0`, `f`, `F`, `n`, `N`

---

### integer (data-type)

Read [integer](https://developer.mozilla.org/en-US/docs/Web/CSS/integer) documentation.

---

### color (data-type)

Read [color](https://developer.mozilla.org/en-US/docs/Web/CSS/color_value) documentation.

**Differences with documentation:**

* Only CSS Level 1 color keywords supported.
* eepp adds hsv and hsva functions with the same syntax that hsl and hsla.
* System colors not support.
* New global color keywords can be registered to eepp by calling the static function:
  EE::System::Color::registerColor.

---

### length (data-type)

Read [length](https://developer.mozilla.org/en-US/docs/Web/CSS/length) documentation.

**Differences with documentation:**

* Supported lenghts: `em`, `rem`, `pt`, `pc`, `in`, `cm`, `mm`, `vw`, `vh`, `vmin`, `vmax`.

---

### length-percentage (data-type)

Read [length-percentage](https://developer.mozilla.org/en-US/docs/Web/CSS/length-percentage) documentation.

**Differences with documentation:**

* `calc` currently not supported

---

### number (data-type)

Read [number](https://developer.mozilla.org/en-US/docs/Web/CSS/number) documentation.

---

### position (data-type)

Read [position](https://developer.mozilla.org/en-US/docs/Web/CSS/position_value) documentation.

---

### resource-path (data-type)

For EE::UI::UIImage (Image), background-image, foreground-image (all the examples are valid).

For a EE::UI::UITextureRegion (TextureRegion) only the examples with: @textureregion, @drawable, 
drawable_resource_name from a texture region resource are valid.

For a EE::UI::UISprite (Sprite) only the examples with: all the examples are valid except for @9p 
and http/s resources.

Valid resources path:
```CSS
file://assets/icon/ee.png; /** relative path to the current working diretory */
"file://assets/icon/ee.png";
https://raw.githubusercontent.com/SpartanJ/eepp/develop/bin/assets/icon/ee.png;
"https://raw.githubusercontent.com/SpartanJ/eepp/develop/bin/assets/icon/ee.png";
url(file://assets/icon/ee.png);
url(https://raw.githubusercontent.com/SpartanJ/eepp/develop/bin/assets/icon/ee.png);
url("file://assets/icon/ee.png");
url(data:image/format;base64,data);
url(data:image/format,url-encoded-data);
@image/image_name_already_in_texture_factory;
@texture/image_name_already_in_texture_factory;
@textureregion/region_name_already_in_any_texture_atlas;
@sprite/sprite_name_already_in_any_texture_atlas; /* sprite pattern name **/
@drawable/drawable_name_already_in_any_drawable_manager; /* drawable managers are any holder of image resources. This includes: texture atlases, textures, nine patchs. */
@9p/nine_patch_resource_name_already_in_the_nine_path_manager;
drawable_resource_name; /** same as doing: @drawable/drawable_resource_name */
linear-gradient(from_color, to_color);
linear-gradient(direction, from_color, to_color); /** valid directions are (without quotes): "to bottom", "to left", "to right", "to top". */
circle(radius, color, type); /** type (optional) can be (without quotes): "fill" or "solid" (filled), or "line" (lined). */
rectangle(type, color, rotation, radius); /** type (optional) can be (without quotes): "fill" or "solid" (filled), or "line" (lined). rotation (optional) is a number in degress: "0º" (without quotes). radius (optional), must be the last parameter. */
triangle(type, color, "point_1.x point1.y, point_2.x point2.y, point_3.x point3.y") /** type can be (without quotes): "fill" or "solid" (filled), or "line" (lined). */
poly(type, color, "point_1.x point1.y, point_2.x point2.y, ...") /** polygon. type (optional) can be (without quotes): "fill" or "solid" (filled), or "line" (lined). */
icon(name, size) /** icons usually come from font glyphs, size is the font glyph size. For example: icon("quit", 24dp) */
glyph(font_family, font_size, codepoint) /** codepoint can be an integer value, an hex value (hex values start with 0x), or a 1 codepoint string representing glyph character. For example: glyph("monospace", 24dp, "@") */
```

---

### string (data-type)

Read [string](https://developer.mozilla.org/en-US/docs/Web/CSS/string) documentation.

**Differences with documentation:**

* Only supports `"` (double quote)

---

### string-list (data-type)

A set of predefined string value types. Each CSS property will have its own string set specified in
the documentation.

---

### vector2-number (data-type)

A set of two [number](#number-data-type)s separated by a " " (space), usually representing `x` and `y` axis
in that order.

---

### vector2-length (data-type)

A set of two [length](#number-data-type)s separated by a " " (space), usually representing `x` and `y` axis
in that order or also representing `width` and `height`.

---
