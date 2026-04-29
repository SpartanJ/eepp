#ifndef EE_UIUINODE_HPP
#define EE_UIUINODE_HPP

#include <eepp/scene/node.hpp>
#include <eepp/ui/base.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/css/stylesheetlength.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/uiclip.hpp>
#include <eepp/ui/uihelper.hpp>
#include <eepp/ui/uiskin.hpp>
#include <eepp/ui/uiskinstate.hpp>
#include <eepp/ui/uistate.hpp>

namespace EE { namespace Graphics {
class Drawable;
}} // namespace EE::Graphics

namespace EE { namespace Scene {
class Action;
class ActionManager;
}} // namespace EE::Scene
using namespace EE::Scene;

namespace EE { namespace UI {

class UISceneNode;
class UITheme;
class UINodeDrawable;
class UIBorderDrawable;
class UIWidget;

class EE_API UINode : public Node {
  public:
	friend class BlockLayouter;
	friend class InlineLayouter;
	friend class TableLayouter;
	/**
	 * @brief Creates a new UINode instance.
	 *
	 * This is the factory method for creating UINode instances.
	 *
	 * @return Pointer to the newly created UINode instance.
	 */
	static UINode* New();

	typedef std::function<void( const Event* )> EventCallback;

	virtual ~UINode();

	/**
	 * @brief Transforms a world position to node-local position.
	 *
	 * Converts a world coordinate position to this node's local coordinate space.
	 * The position is modified in place.
	 *
	 * @param position Reference to the position to transform.
	 */
	void worldToNodeTranslation( Vector2f& position ) const;

	/**
	 * @brief Transforms a node-local position to world position.
	 *
	 * Converts a node-local coordinate position to world coordinate space.
	 * The position is modified in place.
	 *
	 * @param position Reference to the position to transform.
	 */
	void nodeToWorldTranslation( Vector2f& position ) const;

	/**
	 * @brief Converts world integer coordinates to node-local coordinates.
	 *
	 * Converts world coordinate integer position to this node's local coordinate space.
	 * The position is modified in place.
	 *
	 * @param pos Reference to the integer position to transform.
	 */
	void worldToNode( Vector2i& pos ) const;

	/**
	 * @brief Converts node-local integer coordinates to world coordinates.
	 *
	 * Converts node-local coordinate integer position to world coordinate space.
	 * The position is modified in place.
	 *
	 * @param pos Reference to the integer position to transform.
	 */
	void nodeToWorld( Vector2i& pos ) const;

	/**
	 * @brief Converts world floating-point coordinates to node-local coordinates.
	 *
	 * Converts world coordinate floating-point position to this node's local coordinate space.
	 * The position is modified in place.
	 *
	 * @param pos Reference to the floating-point position to transform.
	 */
	void worldToNode( Vector2f& pos ) const;

	/**
	 * @brief Converts node-local floating-point coordinates to world coordinates.
	 *
	 * Converts node-local coordinate floating-point position to world coordinate space.
	 * The position is modified in place.
	 *
	 * @param pos Reference to the floating-point position to transform.
	 */
	void nodeToWorld( Vector2f& pos ) const;

	/**
	 * @brief Gets the node type identifier.
	 *
	 * Returns a unique type identifier for this node class.
	 *
	 * @return The node type as a Uint32.
	 */
	virtual Uint32 getType() const;

	/**
	 * @brief Checks if the node is of a specific type.
	 *
	 * Determines whether this node is of the specified type or derived from it.
	 *
	 * @param type The type identifier to check.
	 * @return True if the node is of the specified type, false otherwise.
	 */
	virtual bool isType( const Uint32& type ) const;

	/**
	 * @brief Sets the node position in density-independent pixels (dp).
	 *
	 * Sets the position of the node using density-independent pixels (dp) for
	 * resolution-independent layout. The position is relative to the parent.
	 *
	 * @param Pos The new position in dp.
	 */
	virtual void setPosition( const Vector2f& Pos );

	/**
	 * @brief Sets the node position in density-independent pixels (dp).
	 *
	 * Sets the position of the node using individual coordinates in dp.
	 *
	 * @param x The X coordinate in dp.
	 * @param y The Y coordinate in dp.
	 * @return Pointer to this node for method chaining.
	 */
	virtual Node* setPosition( const Float& x, const Float& y );

	/**
	 * @brief Sets the node position in actual screen pixels.
	 *
	 * Sets the position of the node using raw screen pixels. This bypasses
	 * density scaling and should be used when exact pixel control is needed.
	 *
	 * @param position The new position in pixels.
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setPixelsPosition( const Vector2f& position );

	/**
	 * @brief Sets the node position in actual screen pixels.
	 *
	 * Sets the position of the node using individual pixel coordinates.
	 *
	 * @param x The X coordinate in pixels.
	 * @param y The Y coordinate in pixels.
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setPixelsPosition( const Float& x, const Float& y );

	/**
	 * @brief Gets the node position in density-independent pixels (dp).
	 *
	 * Returns the current position of the node in dp.
	 *
	 * @return The position as a Vector2f in dp.
	 */
	const Vector2f& getPosition() const;

	/**
	 * @brief Gets the node position in actual screen pixels.
	 *
	 * Returns the current position of the node in raw screen pixels.
	 *
	 * @return The position as a Vector2f in pixels.
	 */
	const Vector2f& getPixelsPosition() const;

	/**
	 * @brief Sets the node size in density-independent pixels (dp).
	 *
	 * Sets the size of the node using dp units. The size will be constrained
	 * by minimum size requirements.
	 *
	 * @param size The new size in dp.
	 * @return Pointer to this node for method chaining.
	 */
	virtual Node* setSize( const Sizef& size );

	/**
	 * @brief Sets the node size in density-independent pixels (dp).
	 *
	 * Sets the size of the node using individual dimensions in dp.
	 *
	 * @param Width The width in dp.
	 * @param Height The height in dp.
	 * @return Pointer to this node for method chaining.
	 */
	virtual Node* setSize( const Float& Width, const Float& Height );

	/**
	 * @brief Sets the node size in actual screen pixels.
	 *
	 * Sets the size of the node using raw pixel units. The size will be
	 * constrained by minimum size requirements.
	 *
	 * @param size The new size in pixels.
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setPixelsSize( const Sizef& size );

	/**
	 * @brief Sets the node size in actual screen pixels.
	 *
	 * Sets the size of the node using individual pixel dimensions.
	 *
	 * @param x The width in pixels.
	 * @param y The height in pixels.
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setPixelsSize( const Float& x, const Float& y );

	/**
	 * @brief Gets the node size in density-independent pixels (dp).
	 *
	 * Returns the current size of the node in dp.
	 *
	 * @return The size as a Sizef in dp.
	 */
	const Sizef& getSize() const;

	/**
	 * @brief Gets the node rectangle in integer coordinates.
	 *
	 * Returns the node's bounds as an integer rectangle based on the dp position
	 * and size.
	 *
	 * @return The rectangle representing the node's bounds.
	 */
	Rect getRect() const;

	/**
	 * @brief Gets the node bounding box in floating-point coordinates.
	 *
	 * Returns the node's bounds as a floating-point rectangle in pixel coordinates.
	 *
	 * @return The bounding box as a Rectf.
	 */
	Rectf getRectBox() const;

	/**
	 * @brief Draws the node.
	 *
	 * Virtual method that can be overridden to implement custom drawing behavior.
	 * The default implementation does nothing. This is called during the render cycle.
	 */
	virtual void draw();

	/**
	 * @brief Gets the horizontal alignment flags.
	 *
	 * Returns the current horizontal alignment setting for text/content within the node.
	 *
	 * @return The horizontal alignment as a Uint32 bitmask.
	 */
	Uint32 getHorizontalAlign() const;

	/**
	 * @brief Sets the horizontal alignment.
	 *
	 * Sets how content should be aligned horizontally within the node.
	 *
	 * @param halign The horizontal alignment flags.
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setHorizontalAlign( Uint32 halign );

	/**
	 * @brief Gets the vertical alignment flags.
	 *
	 * Returns the current vertical alignment setting for text/content within the node.
	 *
	 * @return The vertical alignment as a Uint32 bitmask.
	 */
	Uint32 getVerticalAlign() const;

	/**
	 * @brief Sets the vertical alignment.
	 *
	 * Sets how content should be aligned vertically within the node.
	 *
	 * @param valign The vertical alignment flags.
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setVerticalAlign( Uint32 valign );

	/**
	 * @brief Sets both horizontal and vertical alignment.
	 *
	 * Convenience method to set the gravity (combined horizontal and vertical alignment).
	 *
	 * @param hvalign The combined horizontal and vertical alignment flags.
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setGravity( Uint32 hvalign );

	/**
	 * @brief Enables or disables background fill and returns the background drawable.
	 *
	 * If enabled, creates or returns the background drawable. If disabled,
	 * the background fill is disabled.
	 *
	 * @param enabled True to enable background fill, false to disable.
	 * @return Pointer to the UINodeDrawable for the background.
	 */
	UINodeDrawable* setBackgroundFillEnabled( bool enabled );

	/**
	 * @brief Sets a background drawable from a Drawable object.
	 *
	 * Enables background fill and sets the specified drawable at the given index.
	 *
	 * @param drawable Pointer to the Drawable to use.
	 * @param ownIt If true, the node takes ownership of the drawable.
	 * @param index The layer index (0-based).
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setBackgroundDrawable( Drawable* drawable, bool ownIt = false, int index = 0 );

	/**
	 * @brief Sets a background drawable from a skin name.
	 *
	 * Enables background fill and sets a drawable using the specified skin name.
	 *
	 * @param drawable The skin name for the drawable.
	 * @param index The layer index (0-based).
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setBackgroundDrawable( const std::string& drawable, int index );

	/**
	 * @brief Sets the background color.
	 *
	 * Sets a solid color for the background. This will enable background fill if not already
	 * enabled.
	 *
	 * @param color The background color.
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setBackgroundColor( const Color& color );

	/**
	 * @brief Sets the background tint for a specific layer.
	 *
	 * Applies a color tint to the background drawable at the specified layer index.
	 *
	 * @param color The tint color.
	 * @param index The layer index (0-based).
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setBackgroundTint( const Color& color, int index );

	/**
	 * @brief Sets the background position X CSS property.
	 *
	 * Controls the horizontal positioning of the background drawable at the given layer.
	 *
	 * @param positionX The CSS position value (e.g., "left", "center", "right", or length).
	 * @param index The layer index (0-based).
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setBackgroundPositionX( const std::string& positionX, int index = 0 );

	/**
	 * @brief Sets the background position Y CSS property.
	 *
	 * Controls the vertical positioning of the background drawable at the given layer.
	 *
	 * @param positionY The CSS position value (e.g., "top", "center", "bottom", or length).
	 * @param index The layer index (0-based).
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setBackgroundPositionY( const std::string& positionY, int index = 0 );

	/**
	 * @brief Sets the background repeat CSS property.
	 *
	 * Controls how the background drawable repeats at the specified layer.
	 *
	 * @param repeatRule The repeat rule (e.g., "repeat", "no-repeat", "repeat-x", "repeat-y").
	 * @param index The layer index (0-based).
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setBackgroundRepeat( const std::string& repeatRule, int index = 0 );

	/**
	 * @brief Sets the background size CSS property.
	 *
	 * Controls the size of the background drawable at the specified layer.
	 *
	 * @param size The size value (e.g., "auto", "cover", "contain", or length).
	 * @param index The layer index (0-based).
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setBackgroundSize( const std::string& size, int index = 0 );

	/**
	 * @brief Gets the background color.
	 *
	 * Returns the current background color. If no background is set, returns transparent.
	 *
	 * @return The background color.
	 */
	Color getBackgroundColor() const;

	/**
	 * @brief Gets the background tint color for a specific layer.
	 *
	 * Returns the tint color applied to the background drawable at the specified layer.
	 *
	 * @param index The layer index (0-based).
	 * @return The background tint color at the specified layer.
	 */
	Color getBackgroundTint( int index = 0 ) const;

	/**
	 * @brief Sets the border radius for all corners.
	 *
	 * Enables the border and sets the radius for all corners. This affects both
	 * the border and background fill.
	 *
	 * @param corners The border radius in pixels.
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setBorderRadius( const unsigned int& corners );

	/**
	 * @brief Sets the top-left border radius.
	 *
	 * Sets the radius for the top-left corner using a CSS-style value.
	 *
	 * @param radius The radius value as a string (e.g., "5px", "50%").
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setTopLeftRadius( const std::string& radius );

	/**
	 * @brief Sets the top-right border radius.
	 *
	 * Sets the radius for the top-right corner using a CSS-style value.
	 *
	 * @param radius The radius value as a string (e.g., "5px", "50%").
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setTopRightRadius( const std::string& radius );

	/**
	 * @brief Sets the bottom-left border radius.
	 *
	 * Sets the radius for the bottom-left corner using a CSS-style value.
	 *
	 * @param radius The radius value as a string (e.g., "5px", "50%").
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setBottomLeftRadius( const std::string& radius );

	/**
	 * @brief Sets the bottom-right border radius.
	 *
	 * Sets the radius for the bottom-right corner using a CSS-style value.
	 *
	 * @param radius The radius value as a string (e.g., "5px", "50%").
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setBottomRightRadius( const std::string& radius );

	/**
	 * @brief Gets the current border radius.
	 *
	 * Returns the border radius value in pixels. Returns 0 if no border is enabled.
	 *
	 * @return The border radius in pixels.
	 */
	Uint32 getBorderRadius() const;

	/**
	 * @brief Enables or disables foreground fill and returns the foreground drawable.
	 *
	 * If enabled, creates or returns the foreground drawable. If disabled,
	 * the foreground fill is disabled.
	 *
	 * @param enabled True to enable foreground fill, false to disable.
	 * @return Pointer to the UINodeDrawable for the foreground.
	 */
	UINodeDrawable* setForegroundFillEnabled( bool enabled );

	/**
	 * @brief Sets a foreground drawable from a Drawable object.
	 *
	 * Enables foreground fill and sets the specified drawable at the given index.
	 *
	 * @param drawable Pointer to the Drawable to use.
	 * @param ownIt If true, the node takes ownership of the drawable.
	 * @param index The layer index (0-based).
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setForegroundDrawable( Drawable* drawable, bool ownIt = false, int index = 0 );

	/**
	 * @brief Sets a foreground drawable from a skin name.
	 *
	 * Enables foreground fill and sets a drawable using the specified skin name.
	 *
	 * @param drawable The skin name for the drawable.
	 * @param index The layer index (0-based).
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setForegroundDrawable( const std::string& drawable, int index = 0 );

	/**
	 * @brief Gets the foreground color.
	 *
	 * Returns the current foreground color. If no foreground is set, returns transparent.
	 *
	 * @return The foreground color.
	 */
	Color getForegroundColor() const;

	/**
	 * @brief Gets the foreground tint color for a specific layer.
	 *
	 * Returns the tint color applied to the foreground drawable at the specified layer.
	 *
	 * @param index The layer index (0-based).
	 * @return The foreground tint color at the specified layer.
	 */
	Color getForegroundTint( int index ) const;

	/**
	 * @brief Sets the foreground color.
	 *
	 * Sets a solid color for the foreground. This will enable foreground fill if not already
	 * enabled.
	 *
	 * @param color The foreground color.
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setForegroundColor( const Color& color );

	/**
	 * @brief Sets the foreground tint for a specific layer.
	 *
	 * Applies a color tint to the foreground drawable at the specified layer.
	 *
	 * @param color The tint color.
	 * @param index The layer index (0-based).
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setForegroundTint( const Color& color, int index );

	/**
	 * @brief Sets the foreground position X CSS property.
	 *
	 * Controls the horizontal positioning of the foreground drawable at the given layer.
	 *
	 * @param positionX The CSS position value.
	 * @param index The layer index (0-based).
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setForegroundPositionX( const std::string& positionX, int index = 0 );

	/**
	 * @brief Sets the foreground position Y CSS property.
	 *
	 * Controls the vertical positioning of the foreground drawable at the given layer.
	 *
	 * @param positionY The CSS position value.
	 * @param index The layer index (0-based).
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setForegroundPositionY( const std::string& positionY, int index = 0 );

	/**
	 * @brief Sets the foreground repeat CSS property.
	 *
	 * Controls how the foreground drawable repeats at the specified layer.
	 *
	 * @param repeatRule The repeat rule.
	 * @param index The layer index (0-based).
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setForegroundRepeat( const std::string& repeatRule, int index = 0 );

	/**
	 * @brief Sets the foreground size CSS property.
	 *
	 * Controls the size of the foreground drawable at the specified layer.
	 *
	 * @param size The size value.
	 * @param index The layer index (0-based).
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setForegroundSize( const std::string& size, int index = 0 );

	/**
	 * @brief Sets the foreground border radius for all corners.
	 *
	 * Sets the radius for the foreground content's corners.
	 *
	 * @param corners The border radius in pixels.
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setForegroundRadius( const unsigned int& corners );

	/**
	 * @brief Gets the current foreground border radius.
	 *
	 * Returns the foreground border radius in pixels. Returns 0 if no foreground is enabled.
	 *
	 * @return The foreground border radius in pixels.
	 */
	Uint32 getForegroundRadius() const;

	/**
	 * @brief Enables or disables the border and returns the border drawable.
	 *
	 * If enabled, creates or returns the border drawable. If disabled, the border is hidden.
	 *
	 * @param enabled True to enable the border, false to disable.
	 * @return Pointer to the UIBorderDrawable for the border.
	 */
	UIBorderDrawable* setBorderEnabled( bool enabled ) const;

	/**
	 * @brief Sets the border color.
	 *
	 * Sets the color of the border. This will enable the border if not already enabled.
	 *
	 * @param color The border color.
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setBorderColor( const Color& color );

	/**
	 * @brief Gets the border color.
	 *
	 * Returns the current border color. The border drawable will be created if it doesn't exist.
	 *
	 * @return The border color.
	 */
	Color getBorderColor();

	/**
	 * @brief Sets the border width.
	 *
	 * Sets the thickness of the border in pixels. This will enable the border if not already
	 * enabled.
	 *
	 * @param width The border width in pixels.
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setBorderWidth( const unsigned int& width );

	/**
	 * @brief Gets the border width.
	 *
	 * Returns the current border width in pixels. Defaults to 1.0 if no border is set.
	 *
	 * @return The border width as a Float.
	 */
	Float getBorderWidth() const;

	/**
	 * @brief Gets the current node flags.
	 *
	 * Returns the bitmask of flags that control various node behaviors and states.
	 *
	 * @return The flags as a Uint32 bitmask.
	 */
	const Uint32& getFlags() const;

	/**
	 * @brief Sets multiple flags on the node.
	 *
	 * Adds the specified flags to the node's current flag set. This can enable
	 * various behaviors like fill modes, drag support, alignment, etc.
	 *
	 * @param flags Bitwise combination of flags to set.
	 * @return Pointer to this node for method chaining.
	 */
	virtual UINode* setFlags( const Uint32& flags );

	/**
	 * @brief Unsets multiple flags on the node.
	 *
	 * Removes the specified flags from the node's current flag set.
	 *
	 * @param flags Bitwise combination of flags to unset.
	 * @return Pointer to this node for method chaining.
	 */
	virtual UINode* unsetFlags( const Uint32& flags );

	/**
	 * @brief Resets all flags to a specific value.
	 *
	 * Clears all current flags and optionally sets new flags.
	 *
	 * @param newFlags The new flags bitmask (default is 0 to clear all).
	 * @return Pointer to this node for method chaining.
	 */
	virtual UINode* resetFlags( Uint32 newFlags = 0 );

	/**
	 * @brief Gets the background drawable.
	 *
	 * Returns the background drawable object, creating it if it doesn't exist.
	 *
	 * @return Pointer to the UINodeDrawable for the background.
	 */
	UINodeDrawable* getBackground() const;

	/**
	 * @brief Checks if the node has a background drawable.
	 *
	 * @return True if a background drawable exists, false otherwise.
	 */
	bool hasBackground() const;

	/**
	 * @brief Gets the foreground drawable.
	 *
	 * Returns the foreground drawable object, creating it if it doesn't exist.
	 *
	 * @return Pointer to the UINodeDrawable for the foreground.
	 */
	UINodeDrawable* getForeground() const;

	/**
	 * @brief Checks if the node has a foreground drawable.
	 *
	 * @return True if a foreground drawable exists, false otherwise.
	 */
	bool hasForeground() const;

	/**
	 * @brief Gets the border drawable.
	 *
	 * Returns the border drawable object, creating it if it doesn't exist.
	 *
	 * @return Pointer to the UIBorderDrawable for the border.
	 */
	UIBorderDrawable* getBorder() const;

	/**
	 * @brief Sets the theme by name.
	 *
	 * Applies a theme to this node by looking up the theme by its name from the
	 * UIThemeManager. The theme will control the visual appearance.
	 *
	 * @param Theme The name of the theme to apply.
	 */
	void setThemeByName( const std::string& Theme );

	/**
	 * @brief Sets the theme for this node.
	 *
	 * Applies the specified UITheme to this node, affecting its visual appearance
	 * through skins and styles.
	 *
	 * @param Theme Pointer to the UITheme to apply.
	 */
	virtual void setTheme( UITheme* Theme );

	/**
	 * @brief Sets the theme skin with an explicit theme.
	 *
	 * Applies a specific skin from the specified theme to this node.
	 *
	 * @param Theme Pointer to the UITheme to use.
	 * @param skinName Name of the skin to apply from the theme.
	 * @return Pointer to this node for method chaining.
	 */
	virtual UINode* setThemeSkin( UITheme* Theme, const std::string& skinName );

	/**
	 * @brief Sets the theme skin using the default theme.
	 *
	 * Applies a specific skin from the default theme to this node.
	 *
	 * @param skinName Name of the skin to apply.
	 * @return Pointer to this node for method chaining.
	 */
	virtual UINode* setThemeSkin( const std::string& skinName );

	/**
	 * @brief Sets the theme for all children recursively.
	 *
	 * Applies the specified theme to this node and all its descendants.
	 *
	 * @param Theme Pointer to the UITheme to apply to children.
	 */
	void setThemeToChildren( UITheme* Theme );

	/**
	 * @brief Gets the current skin.
	 *
	 * Returns the UISkin object currently applied to this node, or nullptr if none.
	 *
	 * @return Pointer to the UISkin or nullptr.
	 */
	UISkin* getSkin() const;

	/**
	 * @brief Sets the skin from a UISkin reference.
	 *
	 * Applies the specified skin to this node. The skin is copied.
	 *
	 * @param Skin Reference to the UISkin to apply.
	 * @return Pointer to this node for method chaining.
	 */
	virtual UINode* setSkin( const UISkin& Skin );

	/**
	 * @brief Sets the skin from a UISkin pointer.
	 *
	 * Applies the specified skin to this node without copying.
	 *
	 * @param skin Pointer to the UISkin to apply.
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setSkin( UISkin* skin );

	/**
	 * @brief Sets the skin color tint.
	 *
	 * Applies a color tint to the skin/sprite. This multiplies with the skin's colors.
	 *
	 * @param color The tint color to apply.
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setSkinColor( const Color& color );

	/**
	 * @brief Gets the current skin color tint.
	 *
	 * Returns the color tint currently applied to the skin.
	 *
	 * @return The skin color as a const Color reference.
	 */
	const Color& getSkinColor() const;

	/**
	 * @brief Removes the current skin.
	 *
	 * Deletes and removes the skin from this node, reverting to no custom skin.
	 */
	void removeSkin();

	/**
	 * @brief Pushes a state onto the state stack.
	 *
	 * Adds a new state (like hover, pressed, focused) to the node. States can be
	 * combined and affect the skin rendering. If emitEvent is true, a state change
	 * event will be triggered.
	 *
	 * @param State The state to push (use UIState::StateFlag* values).
	 * @param emitEvent Whether to emit a state change event (default: true).
	 */
	virtual void pushState( const Uint32& State, bool emitEvent = true );

	/**
	 * @brief Pops a state from the state stack.
	 *
	 * Removes a state from the node. This can revert visual changes from that state.
	 * If emitEvent is true, a state change event will be triggered.
	 *
	 * @param State The state to pop.
	 * @param emitEvent Whether to emit a state change event (default: true).
	 */
	virtual void popState( const Uint32& State, bool emitEvent = true );

	/**
	 * @brief Gets the current skin size for a specific state.
	 *
	 * Returns the size of the current skin at the specified state. Useful for
	 * determining the preferred size of a skinned node.
	 *
	 * @param state The state flag (default: UIState::StateFlagNormal).
	 * @return The skin size as a Sizef.
	 */
	Sizef getSkinSize( const Uint32& state = UIState::StateFlagNormal ) const;

	/**
	 * @brief Applies the default theme to this node.
	 *
	 * Requests the UIThemeManager to apply the default theme to this node,
	 * setting up default skins and styles.
	 */
	void applyDefaultTheme();

	/**
	 * @brief Gets the window container node.
	 *
	 * Finds and returns the container node of the window that contains this node,
	 * or the root if not in a window.
	 *
	 * @return Pointer to the window container node or the scene node.
	 */
	Node* getWindowContainer() const;

	/**
	 * @brief Checks if this node is tab focusable.
	 *
	 * Determines whether this node can receive focus via Tab key navigation.
	 *
	 * @return True if the node is tab focusable, false otherwise.
	 */
	bool isTabFocusable() const;

	/**
	 * @brief Checks if the node is currently being dragged.
	 *
	 * @return True if the node is in a drag operation, false otherwise.
	 */
	bool isDragging() const;

	/**
	 * @brief Sets or clears the dragging state.
	 *
	 * Changes whether the node is considered to be dragging. If emitDropEvent
	 * is true and ending the drag, a drop event will be sent to the node under
	 * the cursor.
	 *
	 * @param dragging True to start dragging, false to stop.
	 * @param emitDropEvent Whether to emit a drop event when stopping (default: true).
	 */
	void setDragging( bool dragging, bool emitDropEvent = true );

	/**
	 * @brief Starts a drag operation from a specific position.
	 *
	 * Begins dragging this node, recording the starting position for drag calculations.
	 *
	 * @param position The starting position of the drag in dp.
	 */
	void startDragging( const Vector2f& position );

	/**
	 * @brief Checks if this node owns its children's positions.
	 *
	 * Determines whether this node is responsible for positioning its children
	 * (as opposed to children positioning themselves).
	 *
	 * @return True if children positions are owned, false otherwise.
	 */
	bool ownsChildPosition() const;

	/**
	 * @brief Gets the current drag point.
	 *
	 * Returns the point where dragging started or the current drag position.
	 *
	 * @return The drag point as a Vector2f in dp.
	 */
	const Vector2f& getDragPoint() const;

	/**
	 * @brief Sets the drag point.
	 *
	 * Sets the reference point used during dragging operations.
	 *
	 * @param Point The new drag point in dp.
	 */
	void setDragPoint( const Vector2f& Point );

	/**
	 * @brief Checks if dragging is enabled for this node.
	 *
	 * Determines whether this node can be dragged by the user.
	 *
	 * @return True if dragging is enabled, false otherwise.
	 */
	bool isDragEnabled() const;

	/**
	 * @brief Enables or disables dragging.
	 *
	 * Controls whether this node can be dragged by the user.
	 *
	 * @param enable True to enable dragging, false to disable.
	 */
	void setDragEnabled( const bool& enable );

	/**
	 * @brief Sets which mouse button initiates dragging.
	 *
	 * Specifies which mouse button (left, right, middle) should be used to
	 * initiate dragging on this node.
	 *
	 * @param Button The mouse button mask (EE_BUTTON_* constants).
	 */
	void setDragButton( const Uint32& Button );

	/**
	 * @brief Gets the mouse button that initiates dragging.
	 *
	 * Returns which mouse button is configured to start dragging.
	 *
	 * @return The mouse button mask.
	 */
	const Uint32& getDragButton() const;

	/**
	 * @brief Requests focus for this node.
	 *
	 * Attempts to give this node input focus. The focus change may be denied
	 * depending on node state and focus policies.
	 *
	 * @param reason The reason for the focus request (default: Unknown).
	 * @return Pointer to this node if focus was granted, nullptr otherwise.
	 */
	virtual Node* setFocus( NodeFocusReason reason = NodeFocusReason::Unknown );

	/**
	 * @brief Gets a property's value relative to a container length.
	 *
	 * Evaluates a CSS property that has a relative target (like percentage or
	 * viewport units) and converts it to an absolute length based on the
	 * appropriate container dimensions.
	 *
	 * @param relativeTarget The relative target container (width, height, etc.).
	 * @param defaultValue The default value if the property cannot be evaluated.
	 * @param propertyIndex The property index for multi-value properties (default: 0).
	 * @return The computed length in pixels.
	 */
	Float
	getPropertyRelativeTargetContainerLength( const CSS::PropertyRelativeTarget& relativeTarget,
											  const Float& defaultValue = 0,
											  const Uint32& propertyIndex = 0 ) const;

	/**
	 * @brief Converts a CSS length to pixels.
	 *
	 * Parses and converts a CSS-style length value (with units like px, dp, %, em, rem, etc.)
	 * to an absolute pixel value based on the container length and current context.
	 *
	 * @param length The CSS length to convert.
	 * @param containerLength The reference container length in pixels.
	 * @return The computed length in pixels.
	 */
	virtual Float convertLength( const CSS::StyleSheetLength& length,
								 const Float& containerLength ) const;

	/**
	 * @brief Converts a CSS length to density-independent pixels (dp).
	 *
	 * Similar to convertLength but returns the result in dp units instead of pixels.
	 *
	 * @param length The CSS length to convert.
	 * @param containerLength The reference container length in pixels.
	 * @return The computed length in dp.
	 */
	Float convertLengthAsDp( const CSS::StyleSheetLength& length,
							 const Float& containerLength ) const;

	/**
	 * @brief Evaluates a CSS length string to a pixel value.
	 *
	 * Parses a string containing a CSS length value and converts it to pixels
	 * using the specified relative target for percentage/relative units.
	 *
	 * @param value The CSS length string (e.g., "10px", "50%", "1em").
	 * @param relativeTarget The relative target for relative units.
	 * @param defaultValue The default value if parsing fails (default: 0).
	 * @param propertyIndex The property index for multi-value properties (default: 0).
	 * @return The computed length in pixels.
	 */
	Float lengthFromValue( const std::string& value,
						   const CSS::PropertyRelativeTarget& relativeTarget,
						   const Float& defaultValue = 0, const Uint32& propertyIndex = 0 ) const;

	/**
	 * @brief Evaluates a CSS property to a pixel value.
	 *
	 * Convenience method that extracts the value from a StyleSheetProperty and
	 * converts it to pixels.
	 *
	 * @param property The StyleSheetProperty containing the value to evaluate.
	 * @param defaultValue The default value if the property is not set (default: 0).
	 * @return The computed length in pixels.
	 */
	Float lengthFromValue( const CSS::StyleSheetProperty& property,
						   const Float& defaultValue = 0 ) const;

	/**
	 * @brief Evaluates a CSS length string to a dp value.
	 *
	 * Similar to lengthFromValue but returns the result in dp units.
	 *
	 * @param value The CSS length string.
	 * @param relativeTarget The relative target for relative units.
	 * @param defaultValue The default value if parsing fails (default: 0).
	 * @param propertyIndex The property index for multi-value properties (default: 0).
	 * @return The computed length in dp.
	 */
	Float lengthFromValueAsDp( const std::string& value,
							   const CSS::PropertyRelativeTarget& relativeTarget,
							   const Float& defaultValue = 0,
							   const Uint32& propertyIndex = 0 ) const;

	/**
	 * @brief Evaluates a CSS property to a dp value.
	 *
	 * Convenience method that extracts the value from a StyleSheetProperty and
	 * converts it to dp.
	 *
	 * @param property The StyleSheetProperty containing the value to evaluate.
	 * @param defaultValue The default value if the property is not set (default: 0).
	 * @return The computed length in dp.
	 */
	Float lengthFromValueAsDp( const CSS::StyleSheetProperty& property,
							   const Float& defaultValue = 0 ) const;

	/**
	 * @brief Gets the UISceneNode that contains this node.
	 *
	 * Returns the UI scene node which is the root of the UI rendering hierarchy
	 * that this node belongs to.
	 *
	 * @return Pointer to the UISceneNode or nullptr.
	 */
	UISceneNode* getUISceneNode() const;

	/**
	 * @brief Gets the input manager.
	 *
	 * Returns the Input object used for handling keyboard and mouse input
	 * from the window associated with this UI node.
	 *
	 * @return Pointer to the Input object.
	 */
	Input* getInput() const;

	/**
	 * @brief Sets the minimum width constraint.
	 *
	 * Sets the minimum width that this node can be sized to. The node will not
	 * shrink below this width during layout.
	 *
	 * @param width The minimum width in dp.
	 */
	void setMinWidth( const Float& width );

	/**
	 * @brief Sets the minimum height constraint.
	 *
	 * Sets the minimum height that this node can be sized to. The node will not
	 * shrink below this height during layout.
	 *
	 * @param height The minimum height in dp.
	 */
	void setMinHeight( const Float& height );

	/**
	 * @brief Sets both minimum width and height constraints.
	 *
	 * Sets the minimum size constraints for this node.
	 *
	 * @param size The minimum size in dp.
	 */
	void setMinSize( const Sizef& size );

	/**
	 * @brief Gets the current minimum size.
	 *
	 * Returns the minimum size constraints currently set on this node.
	 *
	 * @return The minimum size as a Sizef in dp.
	 */
	const Sizef& getCurMinSize() const;

	/**
	 * @brief Gets the local bounds in density-independent pixels.
	 *
	 * Returns the rectangle representing the node's local coordinate space
	 * (0,0 to width,height) in dp units.
	 *
	 * @return The local bounds as a Rectf.
	 */
	Rectf getLocalDpBounds() const;

	/**
	 * @brief Virtual method for custom node drawing.
	 *
	 * Override this method to implement custom drawing for the node.
	 * This is called after background and before foreground/border.
	 * Default implementation does nothing.
	 */
	virtual void nodeDraw();

	/**
	 * @brief Clears the foreground drawable.
	 *
	 * Deletes and removes the foreground drawable from this node.
	 */
	void clearForeground();

	/**
	 * @brief Clears the background drawable.
	 *
	 * Deletes and removes the background drawable from this node.
	 */
	void clearBackground();

	/**
	 * @brief Gets the current clipping type.
	 *
	 * Returns what type of clipping is applied to this node's content.
	 *
	 * @return The current ClipType.
	 */
	const ClipType& getClipType() const;

	/**
	 * @brief Sets the clipping type.
	 *
	 * Controls how the node's content is clipped to its bounds or padding.
	 *
	 * @param clipType The clipping type to apply.
	 * @return Pointer to this node for method chaining.
	 */
	UINode* setClipType( const ClipType& clipType );

	/**
	 * @brief Checks if the node has a border.
	 *
	 * @return True if border is enabled, false otherwise.
	 */
	bool hasBorder() const;

	/**
	 * @brief Gets the padding in actual pixels.
	 *
	 * Returns the padding values in pixel units. For UINode this returns
	 * a zero-sized rect as padding is typically managed by UIWidget.
	 *
	 * @return The padding as a Rectf in pixels.
	 */
	virtual const Rectf& getPixelsPadding() const;

	/**
	 * @brief Gets the minimum width equation.
	 *
	 * Returns the CSS-style expression that defines the minimum width,
	 * if one was set (e.g., "50%", "100px").
	 *
	 * @return The minimum width equation string.
	 */
	const std::string& getMinWidthEq() const;

	/**
	 * @brief Sets both minimum width and height equations.
	 *
	 * Sets dynamic minimum size constraints using CSS-style expressions.
	 * These are evaluated during layout to determine the minimum size.
	 *
	 * @param minWidthEq The minimum width expression (e.g., "50%").
	 * @param minHeightEq The minimum height expression.
	 */
	void setMinSizeEq( const std::string& minWidthEq, const std::string& minHeightEq );

	/**
	 * @brief Sets the minimum width equation.
	 *
	 * Sets a dynamic minimum width constraint using a CSS-style expression.
	 *
	 * @param minWidthEq The minimum width expression.
	 */
	void setMinWidthEq( const std::string& minWidthEq );

	/**
	 * @brief Gets the minimum height equation.
	 *
	 * Returns the CSS-style expression for minimum height.
	 *
	 * @return The minimum height equation string.
	 */
	const std::string& getMinHeightEq() const;

	/**
	 * @brief Sets the minimum height equation.
	 *
	 * Sets a dynamic minimum height constraint using a CSS-style expression.
	 *
	 * @param minHeightEq The minimum height expression.
	 */
	void setMinHeightEq( const std::string& minHeightEq );

	/**
	 * @brief Gets the maximum width equation.
	 *
	 * Returns the CSS-style expression that defines the maximum width.
	 *
	 * @return The maximum width equation string.
	 */
	const std::string& getMaxWidthEq() const;

	/**
	 * @brief Sets both maximum width and height equations.
	 *
	 * Sets dynamic maximum size constraints using CSS-style expressions.
	 *
	 * @param maxWidthEq The maximum width expression.
	 * @param maxHeightEq The maximum height expression.
	 */
	void setMaxSizeEq( const std::string& maxWidthEq, const std::string& maxHeightEq );

	/**
	 * @brief Sets the maximum width equation.
	 *
	 * Sets a dynamic maximum width constraint using a CSS-style expression.
	 *
	 * @param maxWidthEq The maximum width expression.
	 */
	void setMaxWidthEq( const std::string& maxWidthEq );

	/**
	 * @brief Gets the maximum height equation.
	 *
	 * Returns the CSS-style expression for maximum height.
	 *
	 * @return The maximum height equation string.
	 */
	const std::string& getMaxHeightEq() const;

	/**
	 * @brief Sets the maximum height equation.
	 *
	 * Sets a dynamic maximum height constraint using a CSS-style expression.
	 *
	 * @param maxHeightEq The maximum height expression.
	 */
	void setMaxHeightEq( const std::string& maxHeightEq );

	/**
	 * @brief Gets the computed minimum size in dp.
	 *
	 * Evaluates any minimum size equations and returns the actual minimum size
	 * that should be enforced during layout.
	 *
	 * @return The computed minimum size in dp.
	 */
	Sizef getMinSize() const;

	/**
	 * @brief Gets the computed maximum size in dp.
	 *
	 * Evaluates any maximum size equations and returns the actual maximum size
	 * that should be enforced during layout.
	 *
	 * @return The computed maximum size in dp.
	 */
	Sizef getMaxSize() const;

	/**
	 * @brief Gets the computed minimum size in pixels.
	 *
	 * Similar to getMinSize() but returns the result in pixel units.
	 *
	 * @return The computed minimum size in pixels.
	 */
	Sizef getMinSizePx() const;

	/**
	 * @brief Gets the computed maximum size in pixels.
	 *
	 * Similar to getMaxSize() but returns the result in pixel units.
	 *
	 * @return The computed maximum size in pixels.
	 */
	Sizef getMaxSizePx() const;

	/**
	 * @brief Fits a size to the minimum and maximum constraints in dp.
	 *
	 * Takes a proposed size and adjusts it to fall within the minimum and
	 * maximum size constraints, accounting for equations.
	 *
	 * @param size The proposed size in dp.
	 * @return The adjusted size that satisfies min/max constraints in dp.
	 */
	Sizef fitMinMaxSizeDp( const Sizef& size ) const;

	/**
	 * @brief Fits a size to the minimum and maximum constraints in pixels.
	 *
	 * Similar to fitMinMaxSizeDp but operates on pixel values.
	 *
	 * @param size The proposed size in pixels.
	 * @return The adjusted size that satisfies min/max constraints in pixels.
	 */
	Sizef fitMinMaxSizePx( const Sizef& size ) const;

	/**
	 * @brief Checks if the node is scrollable.
	 *
	 * Determines whether this node supports scrolling behavior.
	 *
	 * @return True if the node is scrollable, false otherwise.
	 */
	virtual bool isScrollable() const;

	/** @brief Get a widget's computed absolute font size in pixels. */
	Float getAbsoluteFontSize( const UIWidget* widget ) const;

  protected:
	Vector2f mDpPos;
	Sizef mDpSize;
	Sizef mMinSize;
	Uint32 mFlags;
	Uint32 mState;
	UISkinState* mSkinState;
	mutable UINodeDrawable* mBackground;
	mutable UINodeDrawable* mForeground;
	mutable UIBorderDrawable* mBorder;
	Vector2f mDragPoint;
	Uint32 mDragButton;
	Color mSkinColor;
	UISceneNode* mUISceneNode;
	Rectf mPadding;
	Rectf mPaddingPx;
	UIClip mClip;
	std::string mMinWidthEq;
	std::string mMinHeightEq;
	std::string mMaxWidthEq;
	std::string mMaxHeightEq;

	/**
	 * @brief Default constructor.
	 *
	 * Creates a UINode with default values. Typically nodes should be created
	 * using the New() factory method or by derived widget classes.
	 */
	UINode();

	/**
	 * @brief Handles mouse down events.
	 *
	 * Called when a mouse button is pressed while over this node.
	 * Can be overridden to implement custom mouse down handling.
	 *
	 * @param position The mouse position in pixels.
	 * @param flags The mouse event flags.
	 * @return The event handling result (non-zero to consume event).
	 */
	virtual Uint32 onMouseDown( const Vector2i& position, const Uint32& flags );

	/**
	 * @brief Handles mouse up events.
	 *
	 * Called when a mouse button is released. Can be overridden to implement
	 * custom mouse up handling.
	 *
	 * @param position The mouse position in pixels.
	 * @param flags The mouse event flags.
	 * @return The event handling result.
	 */
	virtual Uint32 onMouseUp( const Vector2i& position, const Uint32& flags );

	/**
	 * @brief Handles value change events.
	 *
	 * Called when a value associated with this node has changed.
	 * Default implementation sends the OnValueChange event.
	 *
	 * @return The event handling result.
	 */
	virtual Uint32 onValueChange();

	/**
	 * @brief Handles state changes.
	 *
	 * Called when the node's state bitmask changes. Default implementation
	 * invalidates the node for redraw.
	 */
	virtual void onStateChange();

	/**
	 * @brief Handles enabled state changes.
	 *
	 * Called when the node's enabled state changes. Updates the disabled state
	 * accordingly and calls Node::onEnabledChange().
	 */
	virtual void onEnabledChange();

	/**
	 * @brief Handles alignment changes.
	 *
	 * Called when the node's alignment flags change. Default implementation
	 * invalidates the node for redraw.
	 */
	virtual void onAlignChange();

	/**
	 * @brief Draws the node's skin.
	 *
	 * Called during the render cycle to draw the skin/sprite. Override to
	 * customize skin rendering.
	 */
	virtual void drawSkin();

	/**
	 * @brief Draws the background.
	 *
	 * Called during the render cycle to draw the background. Default implementation
	 * draws the background drawable if the fill flag is set.
	 */
	virtual void drawBackground();

	/**
	 * @brief Draws the foreground.
	 *
	 * Called during the render cycle to draw the foreground. Default implementation
	 * draws the foreground drawable if the fill flag is set.
	 */
	virtual void drawForeground();

	/**
	 * @brief Draws the border.
	 *
	 * Called during the render cycle to draw the border. Default implementation
	 * draws the border drawable if the border flag is set.
	 */
	virtual void drawBorder();

	/**
	 * @brief Handles theme loading.
	 *
	 * Called when a theme has been loaded or applied to this node. Default
	 * implementation invalidates the node for redraw.
	 */
	virtual void onThemeLoaded();

	/**
	 * @brief Handles child count changes.
	 *
	 * Called when a child is added to or removed from this node.
	 * Can be overridden to respond to child count changes.
	 *
	 * @param child The child node that was added or removed.
	 * @param removed True if the child was removed, false if added.
	 */
	virtual void onChildCountChange( Node* child, const bool& removed );

	/**
	 * @brief Handles drag calculation.
	 *
	 * Called during dragging to calculate whether and how the node should move.
	 *
	 * @param position The current mouse position.
	 * @param flags The mouse event flags.
	 * @return Non-zero to allow the drag to continue.
	 */
	virtual Uint32 onCalculateDrag( const Vector2f& position, const Uint32& flags );

	/**
	 * @brief Handles drag movement.
	 *
	 * Called during a drag operation when the mouse moves. Can modify the
	 * node's position based on the drag difference.
	 *
	 * @param position The current mouse position.
	 * @param flags The mouse event flags.
	 * @param dragDiff The distance dragged since last call.
	 * @return Non-zero to indicate handling.
	 */
	virtual Uint32 onDrag( const Vector2f& position, const Uint32& flags, const Sizef& dragDiff );

	/**
	 * @brief Handles drag start.
	 *
	 * Called when a drag operation begins. Default implementation sends
	 * the OnDragStart event.
	 *
	 * @param position The starting mouse position.
	 * @param flags The mouse event flags.
	 * @return The event handling result.
	 */
	virtual Uint32 onDragStart( const Vector2i& position, const Uint32& flags );

	/**
	 * @brief Handles drag stop.
	 *
	 * Called when a drag operation ends. Default implementation sends
	 * the OnDragStop event and may send a drop event.
	 *
	 * @param position The final mouse position.
	 * @param flags The mouse event flags.
	 * @return The event handling result.
	 */
	virtual Uint32 onDragStop( const Vector2i& position, const Uint32& flags );

	/**
	 * @brief Handles drop events.
	 *
	 * Called when a dragged node is dropped onto this node. Default implementation
	 * sends the OnNodeDropped event.
	 *
	 * @param widget The node that was dropped.
	 * @return The event handling result.
	 */
	virtual Uint32 onDrop( UINode* widget );

	/**
	 * @brief Handles mouse over events.
	 *
	 * Called when the mouse cursor enters this node's area. Default implementation
	 * pushes the hover state.
	 *
	 * @param position The mouse position.
	 * @param flags The mouse event flags.
	 * @return The event handling result.
	 */
	virtual Uint32 onMouseOver( const Vector2i& position, const Uint32& flags );

	/**
	 * @brief Handles mouse leave events.
	 *
	 * Called when the mouse cursor leaves this node's area. Default implementation
	 * pops the hover and pressed states.
	 *
	 * @param position The mouse position.
	 * @param flags The mouse event flags.
	 * @return The event handling result.
	 */
	virtual Uint32 onMouseLeave( const Vector2i& position, const Uint32& flags );

	/**
	 * @brief Handles focus gain events.
	 *
	 * Called when this node gains input focus. Default implementation pushes
	 * the focus state and calls Node::onFocus().
	 *
	 * @param reason The reason for the focus gain.
	 * @return The event handling result.
	 */
	virtual Uint32 onFocus( NodeFocusReason reason );

	/**
	 * @brief Handles focus loss events.
	 *
	 * Called when this node loses input focus. Default implementation pops
	 * the focus state and calls Node::onFocusLoss().
	 *
	 * @return The event handling result.
	 */
	virtual Uint32 onFocusLoss();

	/**
	 * @brief Handles scene changes.
	 *
	 * Called when the node's scene changes. Updates the mUISceneNode pointer
	 * if the new scene is a UISceneNode.
	 */
	virtual void onSceneChange();

	/**
	 * @brief Draws droppable hovering feedback.
	 *
	 * Called when this node is a potential drop target. Draws visual feedback
	 * indicating the node can accept the drop.
	 */
	virtual void drawDroppableHovering();

	/**
	 * @brief Checks and handles close operations.
	 *
	 * Internal method to check if the node should be closed and handle cleanup.
	 */
	void checkClose();

	/**
	 * @brief Handles widget focus loss.
	 *
	 * Called when a widget child loses focus. Sends the OnWidgetFocusLoss event
	 * and invalidates the node.
	 */
	virtual void onWidgetFocusLoss();

	/**
	 * @brief Writes a flag bit.
	 *
	 * Internal helper to set or clear a specific flag bit.
	 *
	 * @param Flag The flag bit to modify.
	 * @param Val The value to set (1 to set, 0 to clear).
	 */
	void writeFlag( const Uint32& Flag, const Uint32& Val );

	/**
	 * @brief Creates padding rectangle from skin border size.
	 *
	 * Internal method to calculate padding based on the skin's border dimensions.
	 *
	 * @param PadLeft Whether to include left padding (default: true).
	 * @param PadRight Whether to include right padding (default: true).
	 * @param PadTop Whether to include top padding (default: true).
	 * @param PadBottom Whether to include bottom padding (default: true).
	 * @param SkipFlags If true, ignores UI_AUTO_PADDING flag (default: false).
	 * @return The calculated padding in dp.
	 */
	Rectf makePadding( bool PadLeft = true, bool PadRight = true, bool PadTop = true,
					   bool PadBottom = true, bool SkipFlags = false ) const;

	/**
	 * @brief Gets the skin size for a specific skin and state.
	 *
	 * Internal method to query the size of a given skin at a specific state.
	 *
	 * @param Skin Pointer to the UISkin to query.
	 * @param State The state flag (default: UIState::StateFlagNormal).
	 * @return The skin size as a Sizef.
	 */
	Sizef getSkinSize( UISkin* Skin, const Uint32& State = UIState::StateFlagNormal ) const;

	/**
	 * @brief Draws the focus highlight.
	 *
	 * Internal method to draw the focus rectangle around the node if it has focus.
	 */
	void drawHighlightFocus();

	/**
	 * @brief Draws overlay highlight (e.g., mouse over).
	 *
	 * Internal method to draw an overlay highlight when the node is under the mouse.
	 */
	void drawOverNode();

	/**
	 * @brief Updates debug visualization data.
	 *
	 * Internal method that updates debug information display, such as showing
	 * bounds, tags, IDs, etc. when debug mode is enabled.
	 */
	void updateDebugData();

	/**
	 * @brief Draws the bounding box for debugging.
	 *
	 * Internal method to draw a rectangle around the node when box drawing is enabled.
	 */
	void drawBox();

	/**
	 * @brief Sets the internal position without triggering callbacks.
	 *
	 * Internal method to set position without calling onPositionChange().
	 *
	 * @param Pos The new position in dp.
	 */
	void setInternalPosition( const Vector2f& Pos );

	/**
	 * @brief Sets the internal size without constraints.
	 *
	 * Internal method to set size without applying min/max constraints or
	 * triggering child notifications.
	 *
	 * @param size The new size in dp.
	 */
	virtual void setInternalSize( const Sizef& size );

	/**
	 * @brief Sets the internal size in pixels without constraints.
	 *
	 * Internal method to set size in pixels without applying constraints.
	 *
	 * @param size The new size in pixels.
	 */
	void setInternalPixelsSize( const Sizef& size );

	/**
	 * @brief Sets the internal width in pixels.
	 *
	 * Internal method to set width directly in pixels.
	 *
	 * @param width The new width in pixels.
	 */
	void setInternalPixelsWidth( const Float& width );

	/**
	 * @brief Sets the internal height in pixels.
	 *
	 * Internal method to set height directly in pixels.
	 *
	 * @param height The new height in pixels.
	 */
	void setInternalPixelsHeight( const Float& height );

	/**
	 * @brief Updates the origin point for transformations.
	 *
	 * Internal method called when size or rotation origin settings change.
	 * Recalculates rotation and scale origin points if they use equations.
	 */
	virtual void updateOriginPoint();

	/**
	 * @brief Starts smart clipping.
	 *
	 * Internal method that enables clipping planes based on the specified clip type.
	 *
	 * @param reqClipType The requested clipping type.
	 * @param needsClipPlanes Whether clip planes are needed.
	 */
	void smartClipStart( const ClipType& reqClipType, bool needsClipPlanes );

	/**
	 * @brief Ends smart clipping.
	 *
	 * Internal method that disables clipping planes set by smartClipStart.
	 *
	 * @param reqClipType The clipping type to end.
	 * @param needsClipPlanes Whether clip planes were used.
	 */
	void smartClipEnd( const ClipType& reqClipType, bool needsClipPlanes );

	/**
	 * @brief Starts smart clipping (simplified version).
	 *
	 * Internal variant that automatically determines if clip planes are needed.
	 *
	 * @param reqClipType The requested clipping type.
	 */
	void smartClipStart( const ClipType& reqClipType );

	/**
	 * @brief Ends smart clipping (simplified version).
	 *
	 * Internal variant that automatically determines if clip planes were used.
	 *
	 * @param reqClipType The clipping type to end.
	 */
	void smartClipEnd( const ClipType& reqClipType );

	/**
	 * @brief Gets the droppable hovering color.
	 *
	 * Internal method that returns the color to use when showing drop feedback.
	 * Queries the "droppable-hovering-color" CSS property.
	 *
	 * @return The droppable hover color.
	 */
	Color getDroppableHoveringColor();
};

}} // namespace EE::UI

#endif
