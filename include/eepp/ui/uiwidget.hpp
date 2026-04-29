#ifndef EE_UIUIWIDGET_HPP
#define EE_UIUIWIDGET_HPP

#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetselector.hpp>
#include <eepp/ui/uinode.hpp>

namespace pugi {
class xml_node;
}

namespace EE { namespace UI { namespace CSS {
class PropertyDefinition;
}}} // namespace EE::UI::CSS

using namespace EE::UI::CSS;

namespace EE { namespace UI {

class UITooltip;
class UIStyle;

/**
 * @brief Base class for all UI widgets in the eepp framework.
 *
 * UIWidget provides the core functionality for building graphical user interface
 * elements. It serves as the foundation for all GUI components including text
 * views, buttons, input fields, and containers. The class integrates with the
 * CSS styling system, supports layout management, and handles user interactions.
 *
 * Typical usage: Derive from UIWidget to create custom widgets or use built-in
 * derived classes like UITextView, UIPushButton, etc.
 *
 * @see UITextView
 * @see UIPushButton
 * @see UITextInput
 */
class EE_API UIWidget : public UINode {
  public:
	/**
	 * @brief Creates a new UIWidget with default tag "widget".
	 *
	 * This is the primary factory method for creating UIWidget instances.
	 *
	 * @return Pointer to the newly created UIWidget instance.
	 * @see NewWithTag
	 */
	static UIWidget* New();

	/**
	 * @brief Creates a new UIWidget with a specific CSS tag.
	 *
	 * The tag parameter sets the CSS element tag which can be used for styling
	 * with CSS selectors. This is useful when you need to apply specific styles
	 * to certain widget types.
	 *
	 * @param tag The CSS element tag for the widget.
	 * @return Pointer to the newly created UIWidget instance.
	 * @see New
	 */
	static UIWidget* NewWithTag( const std::string& tag );

	virtual ~UIWidget();

	/**
	 * @brief Gets the widget type identifier.
	 *
	 * Returns a unique type identifier for this widget class.
	 *
	 * @return The widget type as a Uint32.
	 */
	virtual Uint32 getType() const;

	/**
	 * @brief Checks if the widget is of a specific type.
	 *
	 * Determines whether this widget is of the specified type or derived from it.
	 *
	 * @param type The type identifier to check.
	 * @return True if the widget is of the specified type, false otherwise.
	 */
	virtual bool isType( const Uint32& type ) const;

	/**
	 * @brief Sets multiple flags on the widget.
	 *
	 * Flags control various widget behaviors and states. Common flags include:
	 * - UI_ANCHOR_LEFT, UI_ANCHOR_TOP, UI_ANCHOR_RIGHT, UI_ANCHOR_BOTTOM
	 * - UI_AUTO_SIZE, UI_TAB_FOCUSABLE, UI_TOOLTIP_ENABLED
	 *
	 * @see UIFlag
	 * @param flags Bitwise combination of flags to set.
	 * @return Pointer to this widget for method chaining.
	 */
	virtual UINode* setFlags( const Uint32& flags );

	/**
	 * @brief Unsets multiple flags on the widget.
	 *
	 * Removes the specified flags from the widget's current flag set.
	 *
	 * @see UIFlag
	 * @param flags Bitwise combination of flags to unset.
	 * @return Pointer to this widget for method chaining.
	 */
	virtual UINode* unsetFlags( const Uint32& flags );

	/**
	 * @brief Sets anchor flags for relative positioning.
	 *
	 * Anchors control how the widget positions itself relative to its parent.
	 * Use combinations of UI_ANCHOR_LEFT, UI_ANCHOR_TOP, UI_ANCHOR_RIGHT, UI_ANCHOR_BOTTOM.
	 *
	 * @param flags Bitwise combination of anchor flags.
	 * @return Pointer to this widget for method chaining.
	 */
	virtual UIWidget* setAnchors( const Uint32& flags );

	/**
	 * @brief Sets the theme for this widget.
	 *
	 * Applies the specified theme to the widget, affecting its visual appearance.
	 * The theme controls colors, fonts, borders, and other visual properties.
	 *
	 * @param Theme Pointer to the UITheme to apply.
	 */
	virtual void setTheme( UITheme* Theme );

	/**
	 * @brief Sets the theme skin for this widget.
	 *
	 * Applies a specific skin from the current theme. If no theme is set,
	 * the default theme will be used.
	 *
	 * @param skinName Name of the skin to apply.
	 * @return Pointer to this widget for method chaining.
	 */
	virtual UINode* setThemeSkin( const std::string& skinName );

	/**
	 * @brief Sets the theme skin for this widget with a specific theme.
	 *
	 * Applies a specific skin from the specified theme.
	 *
	 * @param Theme Pointer to the UITheme to use.
	 * @param skinName Name of the skin to apply.
	 * @return Pointer to this widget for method chaining.
	 */
	virtual UINode* setThemeSkin( UITheme* Theme, const std::string& skinName );

	/**
	 * @brief Sets the unique identifier for this widget.
	 *
	 * The ID can be used for CSS selection and widget identification.
	 *
	 * @param id Unique identifier string.
	 * @return Pointer to this widget for method chaining.
	 */
	virtual Node* setId( const std::string& id );

	/**
	 * @brief Checks if this widget can accept a dropped widget.
	 *
	 * Determines whether another widget can be dropped onto this widget.
	 *
	 * @param widget The widget to check for drop acceptance.
	 * @return True if the widget can be dropped, false otherwise.
	 */
	virtual bool acceptsDropOfWidget( const UIWidget* widget );

	/**
	 * @brief Checks if this widget can accept a dropped widget in its tree.
	 *
	 * Recursively checks if this widget or any of its children can accept
	 * the specified widget as a drop target.
	 *
	 * @param widget The widget to check for drop acceptance.
	 * @return Pointer to the widget that accepts the drop, or nullptr if none.
	 */
	UIWidget* acceptsDropOfWidgetInTree( const UIWidget* widget );

	/**
	 * @brief Gets the tooltip associated with this widget.
	 *
	 * Returns the tooltip object if one exists, or nullptr if no tooltip is set.
	 *
	 * @return Pointer to the UITooltip object or nullptr.
	 */
	UITooltip* getTooltip();

	/**
	 * @brief Removes the tooltip from this widget.
	 *
	 * Detaches and deletes the tooltip object associated with this widget.
	 */
	void tooltipRemove();

	/**
	 * @brief Sets the tooltip text for this widget.
	 *
	 * Creates a tooltip if one doesn't exist and sets its text content.
	 *
	 * @param text The text to display in the tooltip.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setTooltipText( const String& text );

	/**
	 * @brief Sets the tooltip text only if the text is not empty.
	 *
	 * This is useful for conditional tooltip setting where you only want
	 * to set a tooltip if there's actual content to display.
	 *
	 * @param text The text to display in the tooltip.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setTooltipTextIfNotEmpty( const String& text );

	/**
	 * @brief Gets the current tooltip text.
	 *
	 * Returns the text that will be displayed in the tooltip.
	 *
	 * @return The tooltip text string.
	 */
	String getTooltipText();

	/**
	 * @brief Updates the distances to parent borders for anchoring.
	 *
	 * Recalculates the distances from this widget to the borders of its
	 * parent widget. This is used for anchor-based positioning.
	 */
	void updateAnchorsDistances();

	/**
	 * @brief Gets the layout margin in density-independent pixels (dp).
	 *
	 * Layout margins define the space around the widget within its parent
	 * layout container. These values are in dp to support different screen densities.
	 *
	 * @return The layout margin as a Rectf.
	 */
	const Rectf& getLayoutMargin() const;

	/**
	 * @brief Gets the layout margin in actual pixels.
	 *
	 * Returns the margin values converted to actual screen pixels.
	 *
	 * @return The layout margin in pixels as a Rectf.
	 */
	const Rectf& getLayoutPixelsMargin() const;

	/**
	 * @brief Sets the layout margin for all sides.
	 *
	 * Sets the margin around the widget in density-independent pixels (dp).
	 *
	 * @param margin The margin values for left, top, right, bottom.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setLayoutMargin( const Rectf& margin );

	/**
	 * @brief Sets the left layout margin.
	 *
	 * Sets the left margin in density-independent pixels (dp).
	 *
	 * @param marginLeft The left margin value.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setLayoutMarginLeft( const Float& marginLeft );

	/**
	 * @brief Sets the right layout margin.
	 *
	 * Sets the right margin in density-independent pixels (dp).
	 *
	 * @param marginRight The right margin value.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setLayoutMarginRight( const Float& marginRight );

	/**
	 * @brief Sets the top layout margin.
	 *
	 * Sets the top margin in density-independent pixels (dp).
	 *
	 * @param marginTop The top margin value.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setLayoutMarginTop( const Float& marginTop );

	/**
	 * @brief Sets the bottom layout margin.
	 *
	 * Sets the bottom margin in density-independent pixels (dp).
	 *
	 * @param marginBottom The bottom margin value.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setLayoutMarginBottom( const Float& marginBottom );

	UIWidget* setLayoutMarginLeftAuto( bool isAuto );

	UIWidget* setLayoutMarginRightAuto( bool isAuto );

	UIWidget* setLayoutMarginTopAuto( bool isAuto );

	UIWidget* setLayoutMarginBottomAuto( bool isAuto );

	UIWidget* setLayoutMarginAuto( bool left, bool right, bool top, bool bottom );

	bool hasLayoutMarginLeftAuto() const;

	bool hasLayoutMarginRightAuto() const;

	bool hasLayoutMarginTopAuto() const;

	bool hasLayoutMarginBottomAuto() const;

	/**
	 * @brief Sets the layout margin for all sides in pixels.
	 *
	 * Sets the margin around the widget in actual screen pixels.
	 *
	 * @param margin The margin values for left, top, right, bottom in pixels.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setLayoutPixelsMargin( const Rectf& margin );

	/**
	 * @brief Sets the left margin in pixels.
	 *
	 * Sets the left margin in actual screen pixels.
	 *
	 * @param marginLeft The left margin value in pixels.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setLayoutPixelsMarginLeft( const Float& marginLeft );

	/**
	 * @brief Sets the right margin in pixels.
	 *
	 * Sets the right margin in actual screen pixels.
	 *
	 * @param marginRight The right margin value in pixels.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setLayoutPixelsMarginRight( const Float& marginRight );

	/**
	 * @brief Sets the top margin in pixels.
	 *
	 * Sets the top margin in actual screen pixels.
	 *
	 * @param marginTop The top margin value in pixels.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setLayoutPixelsMarginTop( const Float& marginTop );

	/**
	 * @brief Sets the bottom margin in pixels.
	 *
	 * Sets the bottom margin in actual screen pixels.
	 *
	 * @param marginBottom The bottom margin value in pixels.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setLayoutPixelsMarginBottom( const Float& marginBottom );

	/**
	 * @brief Gets the layout weight for this widget.
	 *
	 * Layout weight determines how much space this widget should take
	 * relative to other widgets in a LinearLayout. A weight of 0 means
	 * the widget takes only the space it needs.
	 *
	 * @return The layout weight as a Float.
	 */
	Float getLayoutWeight() const;

	/**
	 * @brief Sets the layout weight for this widget.
	 *
	 * Used in LinearLayout to distribute extra space among widgets.
	 *
	 * @param weight The layout weight value.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setLayoutWeight( const Float& weight );

	/**
	 * @brief Gets the layout gravity for this widget.
	 *
	 * Layout gravity determines how the widget is positioned within
	 * its parent layout. Common values include:
	 * - Gravity::Left, Gravity::Right, Gravity::Top, Gravity::Bottom
	 * - Gravity::Center, Gravity::CenterHorizontal, Gravity::CenterVertical
	 *
	 * @return The layout gravity as a Uint32 bitmask.
	 */
	Uint32 getLayoutGravity() const;

	/**
	 * @brief Sets the layout gravity for this widget.
	 *
	 * Controls how the widget is aligned within its parent layout.
	 *
	 * @param layoutGravity The gravity bitmask.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setLayoutGravity( const Uint32& layoutGravity );

	/**
	 * @brief Gets the width size policy for this widget.
	 *
	 * Size policies control how the widget resizes:
	 * - SizePolicy::WrapContent: Size to content
	 * - SizePolicy::MatchParent: Match parent size
	 * - SizePolicy::Exact: Exact size
	 * - SizePolicy::FillParent: Fill parent (may exceed)
	 *
	 * @return The width size policy.
	 */
	const SizePolicy& getLayoutWidthPolicy() const;

	/**
	 * @brief Sets the width size policy for this widget.
	 *
	 * Controls how the widget's width is determined.
	 *
	 * @param widthPolicy The size policy to apply.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setLayoutWidthPolicy( const SizePolicy& widthPolicy );

	/**
	 * @brief Gets the height size policy for this widget.
	 *
	 * Size policies control how the widget resizes:
	 * - SizePolicy::WrapContent: Size to content
	 * - SizePolicy::MatchParent: Match parent size
	 * - SizePolicy::Exact: Exact size
	 * - SizePolicy::FillParent: Fill parent (may exceed)
	 *
	 * @return The height size policy.
	 */
	const SizePolicy& getLayoutHeightPolicy() const;

	/**
	 * @brief Sets the height size policy for this widget.
	 *
	 * Controls how the widget's height is determined.
	 *
	 * @param heightPolicy The size policy to apply.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setLayoutHeightPolicy( const SizePolicy& heightPolicy );

	/**
	 * @brief Sets both width and height size policies.
	 *
	 * Convenience method to set both size policies at once.
	 *
	 * @param widthPolicy The width size policy.
	 * @param heightPolicy The height size policy.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setLayoutSizePolicy( const SizePolicy& widthPolicy, const SizePolicy& heightPolicy );

	/**
	 * @brief Sets the position policy for relative positioning.
	 *
	 * Position policies allow positioning this widget relative to another widget:
	 * - PositionPolicy::Above, Below, LeftOf, RightOf
	 * - PositionPolicy::AlignLeft, AlignRight, AlignTop, AlignBottom
	 * - PositionPolicy::CenterIn
	 *
	 * @param layoutPositionPolicy The position policy to apply.
	 * @param of The reference widget for positioning.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setLayoutPositionPolicy( const PositionPolicy& layoutPositionPolicy, UIWidget* of );

	/**
	 * @brief Gets the reference widget for position policy.
	 *
	 * Returns the widget that this widget is positioned relative to.
	 *
	 * @return Pointer to the reference widget or nullptr.
	 */
	UIWidget* getLayoutPositionPolicyWidget() const;

	/**
	 * @brief Gets the current position policy.
	 *
	 * Returns the position policy that determines how this widget is positioned
	 * relative to another widget.
	 *
	 * @return The position policy.
	 */
	PositionPolicy getLayoutPositionPolicy() const;

	/**
	 * @brief Gets the minimum intrinsic width of the widget.
	 *
	 * The minimum intrinsic width is the absolute minimum width the widget needs
	 * to display its content without overflowing. For text, this is typically
	 * the width of the longest unbreakable word.
	 *
	 * @return The minimum intrinsic width in pixels.
	 */
	virtual Float getMinIntrinsicWidth() const;

	/**
	 * @brief Gets the maximum intrinsic width of the widget.
	 *
	 * The maximum intrinsic width is the ideal width of the widget if it had
	 * infinite horizontal space (i.e., no wrapping).
	 *
	 * @return The maximum intrinsic width in pixels.
	 */
	virtual Float getMaxIntrinsicWidth() const;

	/**
	 * @brief Invalidates the cached intrinsic width.
	 *
	 * Forces a recalculation of the intrinsic widths on the next call to
	 * getMinIntrinsicWidth() or getMaxIntrinsicWidth().
	 */
	virtual void invalidateIntrinsicSize();

	/**
	 * @brief Loads widget configuration from an XML node.
	 *
	 * Parses XML configuration to set up the widget's properties, styles, and
	 * layout attributes. This is used for loading UI layouts from XML files.
	 *
	 * @param node The XML node containing the widget configuration.
	 */
	virtual void loadFromXmlNode( const pugi::xml_node& node );

	/**
	 * @brief Boolean that indicates if the widget is in charge of loading its children nodes
	 */
	bool loadsItsChildren() const;

	/**
	 * @brief Notifies that layout attributes have changed.
	 *
	 * Triggers layout recalculation when layout-related properties change.
	 * This should be called after modifying layout attributes like margins,
	 * padding, size policies, etc.
	 */
	void notifyLayoutAttrChange();

	/**
	 * @brief Notifies parent that layout attributes have changed.
	 *
	 * Triggers layout recalculation in the parent widget when this widget's
	 * layout attributes change. This ensures proper layout propagation.
	 */
	void notifyLayoutAttrChangeParent();

	/**
	 * @brief Sets an inline CSS property.
	 *
	 * Applies a CSS property directly to this widget with high specificity.
	 * This overrides styles from stylesheets and themes.
	 *
	 * @param name The CSS property name.
	 * @param value The CSS property value.
	 * @param specificity The specificity level (default is inline specificity).
	 */
	void setStyleSheetInlineProperty( const std::string& name, const std::string& value,
									  const Uint32& specificity = UINT32_MAX -
																  1 /*SpecificityInline*/ );

	/**
	 * @brief Applies a CSS property to this widget.
	 *
	 * Applies the specified CSS property to the widget, updating its style
	 * and triggering any necessary style changes.
	 *
	 * @param attribute The CSS property to apply.
	 * @return True if the property was applied successfully, false otherwise.
	 */
	virtual bool applyProperty( const StyleSheetProperty& attribute );

	void propagateInheritedProperty( const CSS::StyleSheetProperty& property );

	/**
	 * @brief Gets the padding in density-independent pixels (dp).
	 *
	 * Padding defines the space between the widget's content and its border.
	 * These values are in dp to support different screen densities.
	 *
	 * @return The padding as a Rectf.
	 */
	const Rectf& getPadding() const;

	/**
	 * @brief Gets the padding in actual pixels.
	 *
	 * Returns the padding values converted to actual screen pixels.
	 *
	 * @return The padding in pixels as a Rectf.
	 */
	const Rectf& getPixelsPadding() const;

	/**
	 * @brief Gets the content offset area (padding + border).
	 *
	 * Returns a Rectf containing padding + border for all 4 sides.
	 *
	 * @return The content offset as a Rectf.
	 */
	Rectf getPixelsContentOffset() const;

	/**
	 * @brief Sets the padding for all sides.
	 *
	 * Sets the padding around the widget's content in density-independent pixels (dp).
	 *
	 * @param padding The padding values for left, top, right, bottom.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setPadding( const Rectf& padding );

	/**
	 * @brief Sets the left padding.
	 *
	 * Sets the left padding in density-independent pixels (dp).
	 *
	 * @param paddingLeft The left padding value.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setPaddingLeft( const Float& paddingLeft );

	/**
	 * @brief Sets the right padding.
	 *
	 * Sets the right padding in density-independent pixels (dp).
	 *
	 * @param paddingRight The right padding value.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setPaddingRight( const Float& paddingRight );

	/**
	 * @brief Sets the top padding.
	 *
	 * Sets the top padding in density-independent pixels (dp).
	 *
	 * @param paddingTop The top padding value.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setPaddingTop( const Float& paddingTop );

	/**
	 * @brief Sets the bottom padding.
	 *
	 * Sets the bottom padding in density-independent pixels (dp).
	 *
	 * @param paddingBottom The bottom padding value.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setPaddingBottom( const Float& paddingBottom );

	/**
	 * @brief Sets the padding for all sides in pixels.
	 *
	 * Sets the padding around the widget's content in actual screen pixels.
	 *
	 * @param padding The padding values for left, top, right, bottom in pixels.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setPaddingPixels( const Rectf& padding );

	/**
	 * @brief Sets the left padding in pixels.
	 *
	 * Sets the left padding in actual screen pixels.
	 *
	 * @param paddingLeft The left padding value in pixels.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setPaddingPixelsLeft( const Float& paddingLeft );

	/**
	 * @brief Sets the right padding in pixels.
	 *
	 * Sets the right padding in actual screen pixels.
	 *
	 * @param paddingRight The right padding value in pixels.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setPaddingPixelsRight( const Float& paddingRight );

	/**
	 * @brief Sets the top padding in pixels.
	 *
	 * Sets the top padding in actual screen pixels.
	 *
	 * @param paddingTop The top padding value in pixels.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setPaddingPixelsTop( const Float& paddingTop );

	/**
	 * @brief Sets the bottom padding in pixels.
	 *
	 * Sets the bottom padding in actual screen pixels.
	 *
	 * @param paddingBottom The bottom padding value in pixels.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setPaddingPixelsBottom( const Float& paddingBottom );

	/**
	 * @brief Gets the CSS tag for this widget.
	 *
	 * Returns the CSS element tag that can be used for styling with CSS selectors.
	 *
	 * @return The CSS tag string.
	 */
	const std::string& getStyleSheetTag() const;

	/**
	 * @brief Gets the CSS ID for this widget.
	 *
	 * Returns the CSS ID that can be used for styling with CSS selectors.
	 *
	 * @return The CSS ID string.
	 */
	const std::string& getStyleSheetId() const;

	/**
	 * @brief Gets all CSS classes for this widget.
	 *
	 * Returns a vector of all CSS classes applied to this widget.
	 *
	 * @return Vector of CSS class strings.
	 */
	inline const std::vector<std::string>& getStyleSheetClasses() const { return mClasses; }

	/**
	 * @brief Gets the parent element for CSS styling.
	 *
	 * Returns the parent widget that serves as the parent element in CSS
	 * selector resolution.
	 *
	 * @return Pointer to the parent element widget or nullptr.
	 */
	UIWidget* getStyleSheetParentElement() const;

	/**
	 * @brief Gets the previous sibling element for CSS styling.
	 *
	 * Returns the previous sibling widget in the CSS selector context.
	 *
	 * @return Pointer to the previous sibling element widget or nullptr.
	 */
	UIWidget* getStyleSheetPreviousSiblingElement() const;

	/**
	 * @brief Gets the next sibling element for CSS styling.
	 *
	 * Returns the next sibling widget in the CSS selector context.
	 *
	 * @return Pointer to the next sibling element widget or nullptr.
	 */
	UIWidget* getStyleSheetNextSiblingElement() const;

	/**
	 * @brief Gets the active pseudo-classes for this widget.
	 *
	 * Returns a bitmask of active pseudo-classes (hover, focus, active, etc.).
	 *
	 * @return Bitmask of active pseudo-classes.
	 */
	Uint32 getStyleSheetPseudoClasses() const { return mPseudoClasses; }

	/**
	 * @brief Gets the pseudo-classes as string array.
	 *
	 * Returns an array of strings representing the active pseudo-classes.
	 *
	 * @return Vector of pseudo-class strings.
	 */
	std::vector<const char*> getStyleSheetPseudoClassesStrings() const;

	/**
	 * @brief Resets all CSS classes and removes them.
	 *
	 * Clears all CSS classes from this widget.
	 *
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* resetClass();

	/**
	 * @brief Sets a single CSS class.
	 *
	 * Resets all existing classes and sets the specified class.
	 *
	 * @param cls The CSS class to set.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setClass( const std::string& cls );

	/**
	 * @brief Sets a single CSS class (move version).
	 *
	 * Resets all existing classes and sets the specified class using move semantics.
	 *
	 * @param cls The CSS class to set (moved).
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setClass( std::string&& cls );

	/**
	 * @brief Sets multiple CSS classes.
	 *
	 * Resets all existing classes and sets the specified vector of classes.
	 *
	 * @param classes Vector of CSS classes to set.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* setClasses( const std::vector<std::string>& classes );

	/**
	 * @brief Adds a CSS class.
	 *
	 * Adds the specified class to the widget's existing classes.
	 *
	 * @param cls The CSS class to add.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* addClass( const std::string& cls );

	/**
	 * @brief Adds multiple CSS classes.
	 *
	 * Adds the specified vector of classes to the widget's existing classes.
	 *
	 * @param classes Vector of CSS classes to add.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* addClasses( const std::vector<std::string>& classes );

	/**
	 * @brief Removes a CSS class.
	 *
	 * Removes the specified class from the widget's existing classes.
	 *
	 * @param cls The CSS class to remove.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* removeClass( const std::string& cls );

	/**
	 * @brief Removes multiple CSS classes.
	 *
	 * Removes the specified vector of classes from the widget's existing classes.
	 *
	 * @param classes Vector of CSS classes to remove.
	 * @return Pointer to this widget for method chaining.
	 */
	UIWidget* removeClasses( const std::vector<std::string>& classes );

	/**
	 * @brief Checks if the widget has a specific CSS class.
	 *
	 * Determines whether the widget has the specified CSS class.
	 *
	 * @param cls The CSS class to check.
	 * @return True if the widget has the class, false otherwise.
	 */
	bool hasClass( const std::string_view& cls ) const;

	/**
	 * @brief Toggles a CSS class.
	 *
	 * Adds the class if it doesn't exist, removes it if it does.
	 *
	 * @param cls The CSS class to toggle.
	 */
	void toggleClass( const std::string& cls );

	/**
	 * @brief Sets the CSS element tag.
	 *
	 * Sets the CSS element tag that can be used for styling with CSS selectors.
	 *
	 * @param tag The CSS element tag to set.
	 */
	void setElementTag( const std::string& tag );

	/**
	 * @brief Gets all CSS classes for this widget.
	 *
	 * Returns a vector of all CSS classes applied to this widget.
	 *
	 * @return Vector of CSS class strings.
	 */
	const std::vector<std::string>& getClasses() const;

	/**
	 * @brief Gets the CSS element tag.
	 *
	 * Returns the CSS element tag that can be used for styling with CSS selectors.
	 *
	 * @return The CSS element tag string.
	 */
	inline const std::string& getElementTag() const { return mTag; }

	/**
	 * @brief Pushes a state onto the widget's state stack.
	 *
	 * Adds a new state to the widget's current state stack. This is used to
	 * track multiple states (hover, pressed, etc.) that can be combined.
	 *
	 * @param State The state to push.
	 * @param emitEvent Whether to emit a state change event.
	 */
	virtual void pushState( const Uint32& State, bool emitEvent = true );

	/**
	 * @brief Pops a state from the widget's state stack.
	 *
	 * Removes a state from the widget's current state stack. This is used to
	 * revert to a previous state.
	 *
	 * @param State The state to pop.
	 * @param emitEvent Whether to emit a state change event.
	 */
	virtual void popState( const Uint32& State, bool emitEvent = true );

	/**
	 * @brief Gets the UIStyle object for this widget.
	 *
	 * Returns the style object that manages CSS properties, animations, and
	 * transitions for this widget.
	 *
	 * @return Pointer to the UIStyle object.
	 */
	UIStyle* getUIStyle() const;

	/**
	 * @brief Reloads the widget's style.
	 *
	 * Forces the widget to reload its style from CSS stylesheets, themes, and
	 * inline properties. This is useful when style changes need to be applied
	 * immediately.
	 *
	 * @param reloadChildren Whether to reload styles for child widgets.
	 * @param disableAnimations Whether to disable CSS animations during reload.
	 * @param reportStateChange Whether to report state changes.
	 * @param forceReApplyProperties Whether to force re-application of all properties.
	 * @param resetPropertyCache Whether to reset the property cache.
	 */
	void reloadStyle( bool reloadChildren = true, bool disableAnimations = false,
					  bool reportStateChange = true, bool forceReApplyProperties = false,
					  bool resetPropertyCache = false );

	/**
	 * @brief Begins an attributes transaction.
	 *
	 * Starts a transaction for batch attribute changes. This prevents multiple
	 * style recalculations during bulk updates.
	 */
	void beginAttributesTransaction();

	/**
	 * @brief Ends an attributes transaction.
	 *
	 * Ends a transaction and applies all pending attribute changes at once.
	 */
	void endAttributesTransaction();

	/**
	 * @brief Gets the current style state.
	 *
	 * Returns the current state bitmask (hover, focus, pressed, etc.).
	 *
	 * @return Current style state bitmask.
	 */
	const Uint32& getStyleState() const;

	/**
	 * @brief Gets the previous style state.
	 *
	 * Returns the previous state bitmask before the current state change.
	 *
	 * @return Previous style state bitmask.
	 */
	const Uint32& getStylePreviousState() const;

	/**
	 * @brief Finds all widgets by CSS class.
	 *
	 * Searches the widget tree for all widgets with the specified CSS class.
	 *
	 * @param className The CSS class to search for.
	 * @return Vector of matching UIWidget pointers.
	 */
	std::vector<UIWidget*> findAllByClass( const std::string& className );

	/**
	 * @brief Finds all widgets by CSS tag.
	 *
	 * Searches the widget tree for all widgets with the specified CSS tag.
	 *
	 * @param tag The CSS tag to search for.
	 * @return Vector of matching UIWidget pointers.
	 */
	std::vector<UIWidget*> findAllByTag( const std::string& tag );

	/**
	 * @brief Finds a widget by CSS class.
	 *
	 * Searches for the first widget with the specified CSS class.
	 *
	 * @param className The CSS class to search for.
	 * @return Pointer to the first matching widget or nullptr.
	 */
	UIWidget* findByClass( const std::string& className );

	template <typename T> T* findByClass( const std::string& className ) {
		return reinterpret_cast<T*>( findByClass( className ) );
	}

	/**
	 * @brief Finds a widget by CSS tag.
	 *
	 * Searches for the first widget with the specified CSS tag.
	 *
	 * @param tag The CSS tag to search for.
	 * @return Pointer to the first matching widget or nullptr.
	 */
	UIWidget* findByTag( const std::string& tag );

	template <typename T> T* findByTag( const std::string& tag ) {
		return reinterpret_cast<T*>( findByTag( tag ) );
	}

	/**
	 * @brief Queries a widget using a CSS selector.
	 *
	 * Finds the first widget matching the specified CSS selector.
	 *
	 * @param selector The CSS selector to use.
	 * @return Pointer to the first matching widget or nullptr.
	 */
	UIWidget* querySelector( const CSS::StyleSheetSelector& selector );

	/**
	 * @brief Queries a widget using a CSS selector string.
	 *
	 * Finds the first widget matching the specified CSS selector string.
	 *
	 * @param selector The CSS selector string to use.
	 * @return Pointer to the first matching widget or nullptr.
	 */
	UIWidget* querySelector( const std::string& selector );

	template <typename T> T* querySelector( const std::string& selector ) {
		return reinterpret_cast<T*>( querySelector( selector ) );
	}

	/**
	 * @brief Queries all widgets using a CSS selector.
	 *
	 * Finds all widgets matching the specified CSS selector.
	 *
	 * @param selector The CSS selector to use.
	 * @return Vector of all matching UIWidget pointers.
	 */
	std::vector<UIWidget*> querySelectorAll( const CSS::StyleSheetSelector& selector );

	/**
	 * @brief Queries all widgets using a CSS selector string.
	 *
	 * Finds all widgets matching the specified CSS selector string.
	 *
	 * @param selector The CSS selector string to use.
	 * @return Vector of all matching UIWidget pointers.
	 */
	std::vector<UIWidget*> querySelectorAll( const std::string& selector );

	/**
	 * @brief Gets a property value as a string.
	 *
	 * Returns the value of the specified CSS property as a string.
	 *
	 * @param property The CSS property name.
	 * @return The property value as a string.
	 */
	std::string getPropertyString( const std::string& property ) const;

	/**
	 * @brief Gets a property value as a string with property definition.
	 *
	 * Returns the value of the specified CSS property as a string, using
	 * the property definition for proper formatting.
	 *
	 * @param propertyDef The property definition.
	 * @param propertyIndex The property index (for multi-value properties).
	 * @return The property value as a string.
	 */
	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	/**
	 * @brief Gets the list of properties implemented by this widget.
	 *
	 * Returns a vector of property IDs that this widget implements.
	 * This is used for CSS property introspection and validation.
	 *
	 * @return Vector of implemented property IDs.
	 */
	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	/**
	 * @brief Checks if the scene node is currently loading.
	 *
	 * Determines whether the scene node is in the process of loading,
	 * which can affect style application and event handling.
	 *
	 * @return True if the scene node is loading, false otherwise.
	 */
	bool isSceneNodeLoading() const;

	/**
	 * @brief Reports style state change recursively.
	 *
	 * Propagates style state changes to this widget and all its children.
	 * This is used when style changes need to be applied throughout the widget tree.
	 *
	 * @param disableAnimations Whether to disable CSS animations during the change.
	 * @param forceReApplyStyles Whether to force re-application of all styles.
	 */
	void reportStyleStateChangeRecursive( bool disableAnimations = false,
										  bool forceReApplyStyles = false );

	/**
	 * @brief Creates a tooltip for this widget.
	 *
	 * Creates and returns a tooltip object if one doesn't already exist.
	 *
	 * @return Pointer to the UITooltip object.
	 */
	UITooltip* createTooltip();

	/**
	 * @brief Checks if this widget is a tab stop.
	 *
	 * Determines whether this widget can receive focus when the user presses the
	 * Tab key to navigate through widgets.
	 *
	 * @return True if this widget is a tab stop, false otherwise.
	 */
	bool isTabStop() const;

	/**
	 * @brief Sets this widget as a tab stop.
	 *
	 * Makes this widget able to receive focus when the user presses the Tab key.
	 */
	void setTabStop();

	/**
	 * @brief Unsets this widget as a tab stop.
	 *
	 * Prevents this widget from receiving focus when the user presses the Tab key.
	 */
	void unsetTabStop();

	/**
	 * @brief Checks if this widget is tab focusable.
	 *
	 * Determines whether this widget can receive focus via tab navigation.
	 *
	 * @return True if this widget is tab focusable, false otherwise.
	 */
	bool isTabFocusable() const;

	/**
	 * @brief Sets this widget as tab focusable.
	 *
	 * Makes this widget able to receive focus via tab navigation.
	 */
	void setTabFocusable();

	/**
	 * @brief Unsets this widget as tab focusable.
	 *
	 * Prevents this widget from receiving focus via tab navigation.
	 */
	void unsetTabFocusable();

	/**
	 * @brief Gets the previous widget in tab order.
	 *
	 * Returns the widget that comes before this one in the tab navigation sequence.
	 *
	 * @return Pointer to the previous tab widget or nullptr.
	 */
	UIWidget* getPrevTabWidget() const;

	/**
	 * @brief Gets the next widget in tab order.
	 *
	 * Returns the widget that comes after this one in the tab navigation sequence.
	 *
	 * @return Pointer to the next tab widget or nullptr.
	 */
	UIWidget* getNextTabWidget() const;

	/**
	 * @brief Checks if this widget has a specific pseudo-class.
	 *
	 * Determines whether this widget currently has the specified pseudo-class
	 * (e.g., hover, focus, active).
	 *
	 * @param pseudoCls The pseudo-class to check.
	 * @return True if the widget has the pseudo-class, false otherwise.
	 */
	bool hasPseudoClass( const std::string& pseudoCls ) const;

	/**
	 * @brief Checks if the tooltip is enabled for this widget.
	 *
	 * Determines whether tooltips are enabled for this widget.
	 *
	 * @return True if tooltips are enabled, false otherwise.
	 */
	bool isTooltipEnabled() const;

	/**
	 * @brief Sets whether the tooltip is enabled for this widget.
	 *
	 * Enables or disables tooltips for this widget.
	 *
	 * @param enabled True to enable tooltips, false to disable.
	 */
	void setTooltipEnabled( bool enabled );

	/**
	 * @brief Gets the previous widget in the widget sequence.
	 *
	 * Returns the previous widget in the overall widget sequence (not necessarily
	 * tab order).
	 *
	 * @return Pointer to the previous widget or nullptr.
	 */
	UIWidget* getPrevWidget() const;

	/**
	 * @brief Gets the next widget in the widget sequence.
	 *
	 * Returns the next widget in the overall widget sequence (not necessarily
	 * tab order).
	 *
	 * @return Pointer to the next widget or nullptr.
	 */
	UIWidget* getNextWidget() const;

	/**
	 * @brief Gets a translated string.
	 *
	 * Returns the translation for the specified string using the current
	 * translation system.
	 *
	 * @param str The string to translate.
	 * @return The translated string.
	 */
	String getTranslatorString( const std::string& str );

	/**
	 * @brief Gets a translated string with default value.
	 *
	 * Returns the translation for the specified string using the current
	 * translation system, or returns defaultValue if no translation is found.
	 *
	 * @param str The string to translate.
	 * @param defaultValue The default value to return if translation not found.
	 * @return The translated string or default value.
	 */
	String getTranslatorString( const std::string& str, const String& defaultValue );

	/**
	 * @brief Translates a string (shorthand method).
	 *
	 * Convenience method for translating strings using the current translation system.
	 *
	 * @param str The string to translate.
	 * @return The translated string.
	 */
	String i18n( const std::string& str );

	/**
	 * @brief Translates a string with default value (shorthand method).
	 *
	 * Convenience method for translating strings using the current translation system,
	 * with a default value if no translation is found.
	 *
	 * @param str The string to translate.
	 * @param defaultValue The default value to return if no translation is found.
	 * @return The translated string or default value.
	 */
	String i18n( const std::string& str, const String& defaultValue );

	/**
	 * @brief Handles widget creation events.
	 *
	 * Called after the widget is created and initialized. This can be overridden
	 * to implement custom initialization behavior.
	 * WARNING: Do not manually call.
	 */
	virtual void onWidgetCreated();

	Float getPropertyWidth() const;

	Float getPropertyHeight() const;

  protected:
	friend class UIManager;
	friend class UISceneNode;
	friend class UIEventDispatcher;
	friend class UILayout;

	std::string mTag;
	UITheme* mTheme;
	UIStyle* mStyle;
	UITooltip* mTooltip;
	Rect mDistToBorder;
	Rectf mLayoutMargin;
	Rectf mLayoutMarginPx;
	Rectf mPadding;
	Rectf mPaddingPx;
	Float mLayoutWeight;
	Uint32 mLayoutGravity;
	SizePolicy mWidthPolicy;
	SizePolicy mHeightPolicy;
	PositionPolicy mLayoutPositionPolicy;
	UIWidget* mLayoutPositionPolicyWidget;
	int mAttributesTransactionCount;
	Uint32 mPseudoClasses{ 0 };
	std::string mSkinName;
	std::vector<std::string> mClasses;
	String mTooltipText;
	mutable Float mMinIntrinsicWidth{ 0 };
	mutable Float mMaxIntrinsicWidth{ 0 };
	mutable bool mIntrinsicWidthsDirty{ true };
	Uint8 mMarginAuto{ 0 };

	static constexpr Uint8 MarginAutoLeft = ( 1 << 0 );
	static constexpr Uint8 MarginAutoRight = ( 1 << 1 );
	static constexpr Uint8 MarginAutoTop = ( 1 << 2 );
	static constexpr Uint8 MarginAutoBottom = ( 1 << 3 );

	void calculateAutoMargin();

	/**
	 * @brief Default constructor.
	 *
	 * Creates a UIWidget with the default tag "widget".
	 * This constructor is protected and widgets should be created via New() methods.
	 */
	UIWidget();

	/**
	 * @brief Constructor with specific tag.
	 *
	 * Creates a UIWidget with the specified CSS tag.
	 * This constructor is protected and widgets should be created via New() methods.
	 *
	 * @param tag The CSS element tag for the widget.
	 */
	explicit UIWidget( const std::string& tag );

	/**
	 * @brief Updates pseudo-classes based on current state.
	 *
	 * Updates the internal pseudo-class bitmask based on the widget's current
	 * state (hover, focus, active, etc.). This is called automatically when
	 * state changes occur.
	 */
	void updatePseudoClasses();

	/**
	 * @brief Handles child count changes.
	 *
	 * Called when a child is added or removed from this widget. This can be
	 * overridden to implement custom behavior when the widget's child count changes.
	 *
	 * @param child The child node that was added or removed.
	 * @param removed True if the child was removed, false if added.
	 */
	virtual void onChildCountChange( Node* child, const bool& removed );

	/**
	 * @brief Handles key down events.
	 *
	 * Called when a key is pressed while this widget has focus. This can be
	 * overridden to implement custom keyboard handling.
	 *
	 * @param event The key event.
	 * @return The event handling result.
	 */
	virtual Uint32 onKeyDown( const KeyEvent& event );

	/**
	 * @brief Handles mouse move events.
	 *
	 * Called when the mouse moves over this widget. This can be overridden to
	 * implement custom mouse move handling.
	 *
	 * @param Pos The mouse position.
	 * @param Flags The mouse event flags.
	 * @return The event handling result.
	 */
	virtual Uint32 onMouseMove( const Vector2i& Pos, const Uint32& Flags );

	/**
	 * @brief Handles mouse over events.
	 *
	 * Called when the mouse enters this widget. This can be overridden to
	 * implement custom mouse over handling.
	 *
	 * @param Pos The mouse position.
	 * @param Flags The mouse event flags.
	 * @return The event handling result.
	 */
	virtual Uint32 onMouseOver( const Vector2i& Pos, const Uint32& Flags );

	/**
	 * @brief Handles mouse leave events.
	 *
	 * Called when the mouse leaves this widget. This can be overridden to
	 * implement custom mouse leave handling.
	 *
	 * @param Pos The mouse position.
	 * @param Flags The mouse event flags.
	 * @return The event handling result.
	 */
	virtual Uint32 onMouseLeave( const Vector2i& Pos, const Uint32& Flags );

	/**
	 * @brief Handles parent size change events.
	 *
	 * Called when the parent widget's size changes. This can be overridden to
	 * implement custom handling of parent size changes.
	 *
	 * @param sizeChange The change in parent size.
	 */
	virtual void onParentSizeChange( const Vector2f& sizeChange );

	/**
	 * @brief Handles position change events.
	 *
	 * Called when this widget's position changes. This can be overridden to
	 * implement custom handling of position changes.
	 */
	virtual void onPositionChange();

	/**
	 * @brief Handles visibility change events.
	 *
	 * Called when this widget's visibility changes. This can be overridden to
	 * implement custom handling of visibility changes.
	 */
	virtual void onVisibilityChange();

	/**
	 * @brief Handles size change events.
	 *
	 * Called when this widget's size changes. This can be overridden to
	 * implement custom handling of size changes.
	 */
	virtual void onSizeChange();

	/**
	 * @brief Handles size policy change events.
	 *
	 * Called when this widget's size policy changes. This can be overridden to
	 * implement custom handling of size policy changes.
	 */
	virtual void onSizePolicyChange();

	/**
	 * @brief Handles auto-size events.
	 *
	 * Called when the widget automatically resizes to fit its content. This can
	 * be overridden to implement custom auto-sizing behavior.
	 */
	virtual void onAutoSize();

	/**
	 * @brief Handles padding change events.
	 *
	 * Called when the widget's padding changes. This can be overridden to
	 * implement custom handling of padding changes.
	 */
	virtual void onPaddingChange();

	/**
	 * @brief Handles margin change events.
	 *
	 * Called when the widget's margin changes. This can be overridden to
	 * implement custom handling of margin changes.
	 */
	virtual void onMarginChange();

	/**
	 * @brief Handles theme load events.
	 *
	 * Called when the widget's theme is loaded. This can be overridden to
	 * implement custom handling of theme loading.
	 */
	virtual void onThemeLoaded();

	/**
	 * @brief Handles parent change events.
	 *
	 * Called when the widget's parent changes. This can be overridden to
	 * implement custom handling of parent changes.
	 */
	virtual void onParentChange();

	/**
	 * @brief Handles class change events.
	 *
	 * Called when the widget's CSS classes change. This can be overridden to
	 * implement custom handling of class changes.
	 */
	virtual void onClassChange();

	/**
	 * @brief Handles tag change events.
	 *
	 * Called when the widget's CSS tag changes. This can be overridden to
	 * implement custom handling of tag changes.
	 */
	virtual void onTagChange();

	/**
	 * @brief Handles focus previous widget events.
	 *
	 * Called when the focus moves to the previous widget. This can be overridden
	 * to implement custom handling of focus changes.
	 */
	virtual void onFocusPrevWidget();

	/**
	 * @brief Handles focus next widget events.
	 *
	 * Called when the focus moves to the next widget. This can be overridden
	 * to implement custom handling of focus changes.
	 */
	virtual void onFocusNextWidget();

	/**
	 * @brief Handles focus gain events.
	 *
	 * Called when this widget gains focus. This can be overridden to implement
	 * custom focus handling.
	 *
	 * @param reason The reason for focus gain.
	 * @return The event handling result.
	 */
	virtual Uint32 onFocus( NodeFocusReason reason );

	/**
	 * @brief Handles focus loss events.
	 *
	 * Called when this widget loses focus. This can be overridden to implement
	 * custom focus handling.
	 *
	 * @return The event handling result.
	 */
	virtual Uint32 onFocusLoss();

	/**
	 * @brief Updates anchors based on size change.
	 *
	 * Updates the widget's anchor distances when the parent's size changes.
	 * This is used for anchor-based positioning.
	 *
	 * @param sizeChange The change in parent size.
	 */
	void updateAnchors( const Vector2f& sizeChange );

	/**
	 * @brief Aligns widget against layout.
	 *
	 * Aligns the widget according to the current layout settings and gravity.
	 * This is used for layout positioning.
	 */
	void alignAgainstLayout();

	/**
	 * @brief Reports style state change.
	 *
	 * Reports a style state change to the widget and its parent, optionally
	 * disabling animations or forcing re-application of styles.
	 *
	 * @param disableAnimations Whether to disable CSS animations.
	 * @param forceReApplyStyles Whether to force re-application of all styles.
	 */
	void reportStyleStateChange( bool disableAnimations = false, bool forceReApplyStyles = false );

	/**
	 * @brief Gets layout width policy as string.
	 *
	 * Returns the width size policy as a human-readable string.
	 *
	 * @return The layout width policy string.
	 */
	std::string getLayoutWidthPolicyString() const;

	/**
	 * @brief Gets layout height policy as string.
	 *
	 * Returns the height size policy as a human-readable string.
	 *
	 * @return The layout height policy string.
	 */
	std::string getLayoutHeightPolicyString() const;

	/**
	 * @brief Gets layout gravity as string.
	 *
	 * Returns the layout gravity as a human-readable string.
	 *
	 * @return The layout gravity string.
	 */
	std::string getLayoutGravityString() const;

	/**
	 * @brief Gets gravity as string.
	 *
	 * Returns the gravity as a human-readable string.
	 *
	 * @return The gravity string.
	 */
	std::string getGravityString() const;

	/**
	 * @brief Gets flags as string.
	 *
	 * Returns the widget's flags as a human-readable string.
	 *
	 * @return The flags string.
	 */
	std::string getFlagsString() const;

	/**
	 * @brief Checks property definition.
	 *
	 * Checks whether a CSS property definition is valid for this widget.
	 *
	 * @param property The CSS property to check.
	 * @return True if the property is valid, false otherwise.
	 */
	bool checkPropertyDefinition( const StyleSheetProperty& property );

	/**
	 * @brief Gets tooltip position.
	 *
	 * Calculates the position where the tooltip should be displayed.
	 *
	 * @return The tooltip position as a Vector2f.
	 */
	Vector2f getTooltipPosition();

	/**
	 * @brief Creates the style object.
	 *
	 * Creates and initializes the UIStyle object for this widget.
	 */
	void createStyle();

	/**
	 * @brief Enables CSS animations.
	 *
	 * Enables CSS animations for this widget.
	 */
	void enableCSSAnimations();

	/**
	 * @brief Disables CSS animations.
	 *
	 * Disables CSS animations for this widget.
	 */
	void disableCSSAnimations();

	/**
	 * @brief Reloads font family.
	 *
	 * Reloads the font family for this widget, typically after a theme change.
	 */
	void reloadFontFamily();

	/* @return The width of the widget when size policy is match_parent */
	Float getMatchParentWidth() const;

	/* @return The height of the widget when size policy is match_parent */
	Float getMatchParentHeight() const;

	/* @return The size of the widget when size policy is match_parent */
	Sizef getSizeFromLayoutPolicy();

	UIWidget* setLayoutMarginAuto( Uint32 dir, bool isAuto );
};

}} // namespace EE::UI

#endif
