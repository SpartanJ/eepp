# CSS Specification

## WIP

This document is a work in progress. You'll find that many features are currently not documented.

## Introduction

eepp CSS custom implementation is heavily based on the official [CSS standard](https://www.w3.org/TR/CSS/#css). This document will try to explain the shared features and the current differences with the
CSS specification. Since CSS is widely used and documented, every feature that it's shared with the CSS standard will be directly linked to the Mozilla CSS documentation. If you are totally new to CSS please go the
[Mozilla CSS portal](https://developer.mozilla.org/en-US/docs/Web/CSS).

## Relevant differences with CSS standard

* Layout properties are not supported (display, float, etc), since eepp layout system differs from the HTML+CSS specification (design decision).

* [inheritance](https://developer.mozilla.org/en-US/docs/Web/CSS/Inheritance) is not supported, except for [CSS custom properties](https://developer.mozilla.org/en-US/docs/Web/CSS/--*) (design decision).

* [Attribute selector](https://developer.mozilla.org/en-US/docs/Web/CSS/Attribute_selectors) is not supported (probably will not be supported in the near future, since collides with some implementation decisions).

* CSS relative paths start from the process current working directory instead the relative path from the CSS file.

* eepp CSS supports alternative path methods besides paths (resource locator to previously loaded resources by the process).

* eepp CSS supports [Device-independent
pixel](https://en.wikipedia.org/wiki/Device-independent_pixel) unit `dp`.

* CSS files should be always UTF-8 encoded.

## CSS Selectors

CSS selectors define the elements to which a set of CSS rules apply.

---

### Basic selectors

#### Universal Selector

[Universal selector](https://developer.mozilla.org/en-US/docs/Web/CSS/Universal_selectors) selects all elements. Optionally, it may be restricted to a specific namespace or to all namespaces.

Syntax: `* ns|* *|*`

Example: `*` will match all the elements of the document.

#### Type Selector

[Type selector](https://developer.mozilla.org/en-US/docs/Web/CSS/Type_selectors) selects all elements that have the given node name.

Syntax: `elementname`

Example: `widget` will match any `<widget>` element.

#### Class Selector

[Class selector](https://developer.mozilla.org/en-US/docs/Web/CSS/Class_selectors) selects all elements that have the given class attribute.

Syntax: `.classname`

Example: `.index` will match any element that has a class of "index".

#### ID Selector

[ID selector](https://developer.mozilla.org/en-US/docs/Web/CSS/ID_selectors) selects an element based on the value of its id attribute. There should be only one element with a given ID in a document.

Syntax: `#idname`

Example: `#toc` will match the element that has the ID "toc".

---

### Grouping selectors

#### Selector List

The [selector list](https://developer.mozilla.org/en-US/docs/Web/CSS/Selector_list) `,` is a grouping method, it selects all the matching nodes.

Syntax: `A, B`

Example: `widget, image` will match both `<span>` and `<image>` elements.

---

### Combinators

#### Descendant combinator

The [descendant combinator](https://developer.mozilla.org/en-US/docs/Web/CSS/Descendant_combinator) " " (space) combinator selects nodes that are descendants of the first element.

Syntax: `A B`

Example: `LinearLayout image` will match all `<image>` elements that are inside a `<LinearLayout>` element.

#### Child combinator

The [child combinator](https://developer.mozilla.org/en-US/docs/Web/CSS/Child_combinator) `>` selects nodes that are direct children of the first element.

Syntax: `A > B`

Example: `LinearLayout > listbox` will match all `<listbox>` elements that are nested directly inside a `<LinearLayout>` element.

#### General sibling combinator

The [general sibling combinator](https://developer.mozilla.org/en-US/docs/Web/CSS/General_sibling_combinator) `~` selects siblings. This means that the second element follows the first (though not necessarily immediately), and both share the same parent.

Syntax: `A ~ B`

Example: `TextView ~ image` will match all `<image>` elements that follow a `<TextView>`, immediately or not.

#### Adjacent sibling combinator

The [adjacent sibling combinator](https://developer.mozilla.org/en-US/docs/Web/CSS/Adjacent_sibling_combinator) `+` selects adjacent siblings. This means that the second element directly follows the first, and both share the same parent.

Syntax: `A + B`

Example: `TextView + image` will match all `<image>` elements that directly follow an `<TextView>`.

---

## CSS Properties

eepp CSS properties can be categorized in two big groups: CSS standard properties and eepp CSS properties.

### Standard Properties

Properties directly linked to the Mozilla docs are fully supported, otherwise a clarification will appear.

* [animation-delay](https://developer.mozilla.org/en-US/docs/Web/CSS/animation-delay)

* [animation-direction](https://developer.mozilla.org/en-US/docs/Web/CSS/animation-direction)

* [animation-duration](https://developer.mozilla.org/en-US/docs/Web/CSS/animation-duration)

* [animation-fill-mode](https://developer.mozilla.org/en-US/docs/Web/CSS/animation-fill-mode)

* [animation-iteration-count](https://developer.mozilla.org/en-US/docs/Web/CSS/animation-iteration-count)

* [animation-name](https://developer.mozilla.org/en-US/docs/Web/CSS/animation-name)

* [animation-play-state](https://developer.mozilla.org/en-US/docs/Web/CSS/animation-play-state)

* [animation-timing-function](https://developer.mozilla.org/en-US/docs/Web/CSS/animation-timing-function):
Timing function names are custom, and not the same as the standard, but `cubic-bezier` will be added soon. Current timing functions supported: linear, quadratic-in, quadratic-out, quadratic-in-out, sine-in, sine-out, sine-in-out, exponential-in, exponential-out, exponential-in-out, quartic-in, quartic-out, quartic-in-out, circular-in, circular-out, circular-in-out, cubic-in, cubic-out, cubic-in-out, back-in, back-out, back-in-out, bounce-in, bounce-out, bounce-in-out, elastic-in, elastic-out, elastic-in-out, none.

* [background-color](https://developer.mozilla.org/en-US/docs/Web/CSS/background-color)

* [background-image](https://developer.mozilla.org/en-US/docs/Web/CSS/background-image)

* [background-position-x](https://developer.mozilla.org/en-US/docs/Web/CSS/background-position-x)

* [background-position-y](https://developer.mozilla.org/en-US/docs/Web/CSS/background-position-y)

* [background-repeat](https://developer.mozilla.org/en-US/docs/Web/CSS/background-repeat)

* [background-size](https://developer.mozilla.org/en-US/docs/Web/CSS/background-size)

* [border-bottom-color](https://developer.mozilla.org/en-US/docs/Web/CSS/border-bottom-color)

* [border-bottom-left-radius](https://developer.mozilla.org/en-US/docs/Web/CSS/border-bottom-left-radius)

* [border-bottom-right-radius](https://developer.mozilla.org/en-US/docs/Web/CSS/border-bottom-right-radius)

* [border-bottom-width](https://developer.mozilla.org/en-US/docs/Web/CSS/border-bottom-width)

* [border-left-color](https://developer.mozilla.org/en-US/docs/Web/CSS/border-left-color)

* [border-left-width](https://developer.mozilla.org/en-US/docs/Web/CSS/border-left-width)

* [border-right-color](https://developer.mozilla.org/en-US/docs/Web/CSS/border-right-color)

* [border-right-width](https://developer.mozilla.org/en-US/docs/Web/CSS/border-right-width)

* [border-top-color](https://developer.mozilla.org/en-US/docs/Web/CSS/border-top-color)

* [border-top-left-radius](https://developer.mozilla.org/en-US/docs/Web/CSS/border-top-left-radius)

* [border-top-right-radius](https://developer.mozilla.org/en-US/docs/Web/CSS/border-top-right-radius)

* [border-top-width](https://developer.mozilla.org/en-US/docs/Web/CSS/border-top-width)

* [color](https://developer.mozilla.org/en-US/docs/Web/CSS/color)

* [cursor](https://developer.mozilla.org/en-US/docs/Web/CSS/cursor): Custom cursors not yet supported (but supported by the engine, only not implemented in the CSS side).

* [font-family](https://developer.mozilla.org/en-US/docs/Web/CSS/font-family)

* [font-size](https://developer.mozilla.org/en-US/docs/Web/CSS/font-size)

* [font-style](https://developer.mozilla.org/en-US/docs/Web/CSS/font-style)

* [margin-bottom](https://developer.mozilla.org/en-US/docs/Web/CSS/margin-bottom)

* [margin-left](https://developer.mozilla.org/en-US/docs/Web/CSS/margin-left)

* [margin-right](https://developer.mozilla.org/en-US/docs/Web/CSS/margin-right)

* [margin-top](https://developer.mozilla.org/en-US/docs/Web/CSS/margin-top)

* [max-height](https://developer.mozilla.org/en-US/docs/Web/CSS/max-height)

* [max-width](https://developer.mozilla.org/en-US/docs/Web/CSS/max-width)

* [min-height](https://developer.mozilla.org/en-US/docs/Web/CSS/min-height)

* [min-width](https://developer.mozilla.org/en-US/docs/Web/CSS/min-width)

* [opacity](https://developer.mozilla.org/en-US/docs/Web/CSS/opacity)

* [padding-bottom](https://developer.mozilla.org/en-US/docs/Web/CSS/padding-bottom)

* [padding-left](https://developer.mozilla.org/en-US/docs/Web/CSS/padding-left)

* [padding-right](https://developer.mozilla.org/en-US/docs/Web/CSS/padding-right)

* [padding-top](https://developer.mozilla.org/en-US/docs/Web/CSS/padding-top)

* [text-align](https://developer.mozilla.org/en-US/docs/Web/CSS/text-align)

* [transition-delay](https://developer.mozilla.org/en-US/docs/Web/CSS/transition-delay)

* [transition-duration](https://developer.mozilla.org/en-US/docs/Web/CSS/transition-duration)

* [transition-property](https://developer.mozilla.org/en-US/docs/Web/CSS/transition-property)

* [transition-timing-function](https://developer.mozilla.org/en-US/docs/Web/CSS/transition-timing-function): Timing function names are custom, and not the same as the standard, but `cubic-bezier` will be added soon. Current timing functions supported: linear, quadratic-in, quadratic-out, quadratic-in-out, sine-in, sine-out, sine-in-out, exponential-in, exponential-out, exponential-in-out, quartic-in, quartic-out, quartic-in-out, circular-in, circular-out, circular-in-out, cubic-in, cubic-out, cubic-in-out, back-in, back-out, back-in-out, bounce-in, bounce-out, bounce-in-out, elastic-in, elastic-out, elastic-in-out, none.

### Custom eepp CSS properties

* allow-dot

* allow-editing

* animation-speed

* arc-start-angle

* background-expand

* base-alpha

* blend-mode

* border-auto-size

* border-size

* border-type

* button-margin

* buttons-position-offset

* buttons-separation

* change-page-percent

* click-step

* clip

* column-mode

* column-span

* column-weight

* column-width

* decoration-auto-size

* decoration-size

* display-percent

* drag-resistance

* enabled

* fill-color

* filler-padding-bottom

* filler-padding-left

* filler-padding-right

* filler-padding-top

* first-button-margin-left

* flags

* foreground-color

* foreground-image

* foreground-position-x

* foreground-position-y

* foreground-radius

* foreground-repeat

* foreground-size

* free-editing

* gravity

* half-slider

* hint

* hint-color

* hint-font-family

* hint-font-size

* hint-font-style

* hint-shadow-color

* hint-stroke-color

* hint-stroke-width

* hscroll-mode

* icon

* icon-auto-margin

* icon-horizontal-margin

* indeterminate

* layout-gravity

* layout-height

* layout-to-bottom-of

* layout-to-left-of

* layout-to-right-of

* layout-to-top-of

* layout-weight

* layout-width

* line-below-tabs

* line-below-tabs-color

* line-below-tabs-y-offset

* margin-between-buttons

* max-edge-resistance

* max-length

* max-progress

* max-tab-width

* max-text-length

* max-value

* max-visible-items

* menu-height

* min-corner-distance

* min-icon-size

* min-icon-space

* min-margin-right

* min-tab-width

* min-value

* min-window-size

* movement-speed

* name

* only-numbers

* orientation

* outline-thickness

* owns

* page-locked

* page-step

* popup-to-main-control

* progress

* radius

* reverse-draw

* rotation

* rotation-origin-point-x

* rotation-origin-point-y

* row-height

* row-mode

* row-span

* row-weight

* scale

* scale-origin-point-x

* scale-origin-point-y

* scale-type

* scrollbar-mode

* scrollbar-type

* selected

* selected-color

* selected-index

* selectex-text

* selection-back-color

* shadow-color

* skin

* skin-color

* span

* special-border-tabs

* src

* tab-closable

* tab-separation

* text

* text-selection

* text-stroke-color

* text-stroke-width

* theme

* timing-function

* tint

* tooltip

* total-steps

* touch-drag

* touch-drag-deceleration

* value

* vertical-expand

* visible

* vscroll-mode

* window-flags

* word-wrap

## CSS Shorthands

[Shorthand properties](https://developer.mozilla.org/en-US/docs/Web/CSS/Shorthand_properties) are CSS properties that let you set the values of multiple other CSS properties simultaneously. Using a shorthand property, you can write more concise (and often more readable) style sheets, saving time and energy.

* [animation](https://developer.mozilla.org/en-US/docs/Web/CSS/animation)

* [background](https://developer.mozilla.org/en-US/docs/Web/CSS/background): Only for the CSS properties supported by eepp.

* [border-color](https://developer.mozilla.org/en-US/docs/Web/CSS/border-color)

* [border-radius](https://developer.mozilla.org/en-US/docs/Web/CSS/border-radius)

* [border-width](https://developer.mozilla.org/en-US/docs/Web/CSS/border-width)

* foreground: Same as the background but for the foreground of the node. The foreground is rendered on the top of the element.

* [margin](https://developer.mozilla.org/en-US/docs/Web/CSS/margin)

* [padding](https://developer.mozilla.org/en-US/docs/Web/CSS/padding)

* rotation-origin-point: The origin point of the rotation expressed as two numeric lengths or percentages separated by a space. Default value is "center center". For example:

```CSS
rotation-origin-point: 50dp 10dp;
rotation-origin-point: center top;
rotation-origin-point: 100% 0%;
```

* scale-origin-point: The origin point of the node scale expressed as two numeric lengths or percentages separated by a space. Default value is "center center". For example:

```CSS
scale-origin-point: 50dp 10dp;
scale-origin-point: center top;
scale-origin-point: 100% 0%;
```

* [transition](https://developer.mozilla.org/en-US/docs/Web/CSS/transition)

## At-rules

[At-rules](https://developer.mozilla.org/en-US/docs/Web/CSS/At-rule) are CSS statements that instructs CSS how to behave. They begin with an at sign, '@' (U+0040 COMMERCIAL AT), followed by an identifier and includes everything up to the next semicolon, ';' (U+003B SEMICOLON), or the next CSS block, whichever comes first.

The following at-rules are currently supported:

* [@import](https://developer.mozilla.org/en-US/docs/Web/CSS/@import) — Tells the CSS engine to include an external style sheet.

* [@media](https://developer.mozilla.org/en-US/docs/Web/CSS/@media) — A conditional group rule that will apply its content if the device meets the criteria of the condition defined using a media query.

* [@font-face](https://developer.mozilla.org/en-US/docs/Web/CSS/@font-face) — Describes the aspect of an external font to be downloaded.

* [@keyframes](https://developer.mozilla.org/en-US/docs/Web/CSS/@keyframes) — Describes the aspect of intermediate steps in a CSS animation sequence.

## Pseudo-class selectors

A [CSS pseudo-class](https://developer.mozilla.org/en-US/docs/Web/CSS/Pseudo-classes) is a keyword added to a selector that specifies a special state of the selected element(s). For example, :hover can be used to change a button's color when the user's pointer hovers over it.

Most common pseudo-class selectors are supported. The list is:

* [:checked](https://developer.mozilla.org/en-US/docs/Web/CSS/:checked)

* [:disabled](https://developer.mozilla.org/en-US/docs/Web/CSS/:disabled)

* [:empty](https://developer.mozilla.org/en-US/docs/Web/CSS/:empty)

* [:enabled](https://developer.mozilla.org/en-US/docs/Web/CSS/:enabled)

* [:first-child](https://developer.mozilla.org/en-US/docs/Web/CSS/:first-child)

* [:first-of-type](https://developer.mozilla.org/en-US/docs/Web/CSS/:first-of-type)

* [:hover](https://developer.mozilla.org/en-US/docs/Web/CSS/:hover)

* [:last-child](https://developer.mozilla.org/en-US/docs/Web/CSS/:last-child)

* [:last-of-type](https://developer.mozilla.org/en-US/docs/Web/CSS/:last-of-type)

* [:not()](https://developer.mozilla.org/en-US/docs/Web/CSS/:not)

* [:nth-child()](https://developer.mozilla.org/en-US/docs/Web/CSS/:nth-child)

* [:nth-last-child()](https://developer.mozilla.org/en-US/docs/Web/CSS/:nth-last-child)

* [::nth-last-of-type()](https://developer.mozilla.org/en-US/docs/Web/CSS/:nth-last-of-type)

* [:nth-of-type()](https://developer.mozilla.org/en-US/docs/Web/CSS/:nth-of-type)

* [:only-child](https://developer.mozilla.org/en-US/docs/Web/CSS/:only-child)

* [:root](https://developer.mozilla.org/en-US/docs/Web/CSS/:root)
