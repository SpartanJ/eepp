#ifndef EE_SCENE_NODE_HPP
#define EE_SCENE_NODE_HPP

#include <eepp/scene/actions/runnable.hpp>
#include <eepp/scene/event.hpp>
#include <eepp/scene/eventdispatcher.hpp>
#include <eepp/scene/keyevent.hpp>
#include <eepp/scene/mouseevent.hpp>
#include <eepp/scene/nodemessage.hpp>

#include <eepp/graphics/blendmode.hpp>
using namespace EE::Graphics;

#include <eepp/math/math.hpp>
#include <eepp/math/originpoint.hpp>
#include <eepp/math/polygon2.hpp>
#include <eepp/math/rect.hpp>
#include <eepp/math/transformable.hpp>
#include <eepp/math/vector2.hpp>
using namespace EE::Math;

#include <eepp/system/color.hpp>
using namespace EE::System;

namespace EE { namespace Scene {
class Action;
class ActionManager;
class SceneNode;
}} // namespace EE::Scene
using namespace EE::Scene;

namespace EE { namespace Scene {

/**
 * @enum NodeFlags
 * @brief Bitmask flags used to control and track node state.
 *
 * These flags control various aspects of node behavior
 * and state. They can be combined using bitwise operations.
 *
 * The flags are divided into categories:
 * - Update and dirty flags (position, polygon, view)
 * - Transformation flags (rotation, scale)
 * - State flags (mouse over, focus, selected, dragging)
 * - Rendering flags (reverse draw, frame buffer, clip enable)
 * - Type identification flags (SceneNode, UISceneNode, UINode, Widget, Window)
 * - Layout and behavior flags
 */
enum NodeFlags {
	NODE_FLAG_SCHEDULED_UPDATE = ( 1 << 0 ),
	NODE_FLAG_VIEW_DIRTY = ( 1 << 1 ),
	NODE_FLAG_POSITION_DIRTY = ( 1 << 2 ),
	NODE_FLAG_POLYGON_DIRTY = ( 1 << 3 ),
	NODE_FLAG_ROTATED = ( 1 << 4 ),
	NODE_FLAG_SCALED = ( 1 << 5 ),
	NODE_FLAG_CLOSE = ( 1 << 6 ),
	NODE_FLAG_MOUSEOVER = ( 1 << 7 ),
	NODE_FLAG_HAS_FOCUS = ( 1 << 8 ),
	NODE_FLAG_SELECTED = ( 1 << 9 ),
	NODE_FLAG_MOUSEOVER_ME_OR_CHILD = ( 1 << 10 ),
	NODE_FLAG_DRAGGING = ( 1 << 11 ),
	NODE_FLAG_SKIN_OWNER = ( 1 << 12 ),
	NODE_FLAG_TOUCH_DRAGGING = ( 1 << 13 ),
	NODE_FLAG_DROPPABLE_HOVERING = ( 1 << 14 ),
	NODE_FLAG_OWNED_BY_NODE = ( 1 << 15 ),
	NODE_FLAG_REVERSE_DRAW = ( 1 << 16 ),
	NODE_FLAG_FRAME_BUFFER = ( 1 << 17 ),
	NODE_FLAG_CLIP_ENABLE = ( 1 << 18 ),
	NODE_FLAG_REPORT_SIZE_CHANGE_TO_CHILDREN = ( 1 << 19 ),
	NODE_FLAG_OVER_FIND_ALLOWED = ( 1 << 20 ),

	NODE_FLAG_SCENENODE = ( 1 << 21 ),
	NODE_FLAG_UISCENENODE = ( 1 << 22 ),
	NODE_FLAG_UINODE = ( 1 << 23 ),
	NODE_FLAG_WIDGET = ( 1 << 24 ),
	NODE_FLAG_WINDOW = ( 1 << 25 ),
	NODE_FLAG_LAYOUT = ( 1 << 26 ),

	NODE_FLAG_LOADING = ( 1 << 27 ),
	NODE_FLAG_CLOSING_CHILDREN = ( 1 << 28 ),
	NODE_FLAG_DISABLE_CLICK_FOCUS = ( 1 << 29 ),
	NODE_FLAG_FREE_USE = ( 1 << 30 )
};

/**
 * @brief Core node class for scene graph management.
 *
 * Node is the fundamental building block for the scene graph system. It provides
 * hierarchical organization with parent-child relationships, transformation support
 * (position, rotation, scale), event handling, and rendering capabilities.
 *
 * Nodes can be used for general scene management or as base classes for specialized
 * nodes like SceneNode and UISceneNode. The class supports both simple scene
 * composition and complex UI hierarchies.
 *
 * Key features:
 * - Hierarchical parent-child relationships
 * - 2D transformations (position, rotation, scale)
 * - Event system with callbacks
 * - Action system for animations and timed operations
 * - Coordinate space conversion
 * - Visibility and enabled state management
 * - Drawing with optional clipping and reverse order
 * - Dirty flag system for efficient updates
 *
 * @see SceneNode
 * @see UISceneNode
 * @see UIWidget
 */
class EE_API Node : public Transformable {
  public:
	/**
	 * @brief Creates a new Node instance.
	 *
	 * This factory method creates a new Node with default values. Direct instantiation
	 * of Node is uncommon; typically derived classes like SceneNode or UIWidget are used.
	 *
	 * @return Pointer to the newly created Node instance.
	 */
	static Node* New();

	typedef std::function<void( const Event* )> EventCallback;

	/**
	 * @brief Destructor.
	 *
	 * Cleans up the node and its children, removes it from parent, and handles
	 * cleanup of actions, event listeners, and focus state.
	 */
	virtual ~Node();

	/**
	 * @brief Transforms a world position to node-local position.
	 *
	 * Converts a world coordinate position to this node's local coordinate space
	 * by subtracting the positions of all parent nodes up to the root.
	 *
	 * @param position Reference to the position to transform (modified in place).
	 */
	virtual void worldToNodeTranslation( Vector2f& position ) const;

	/**
	 * @brief Transforms a node-local position to world position.
	 *
	 * Converts a node-local coordinate position to world coordinate space
	 * by adding the positions of all parent nodes.
	 *
	 * @param position Reference to the position to transform (modified in place).
	 */
	virtual void nodeToWorldTranslation( Vector2f& position ) const;

	/**
	 * @brief Converts world integer coordinates to node-local coordinates.
	 *
	 * Transforms a world position (with integer coordinates) to this node's
	 * local coordinate space.
	 *
	 * @param pos Reference to the integer position to transform (modified in place).
	 */
	virtual void worldToNode( Vector2i& pos ) const;

	/**
	 * @brief Converts node-local integer coordinates to world coordinates.
	 *
	 * Transforms a node-local position (with integer coordinates) to world
	 * coordinate space.
	 *
	 * @param pos Reference to the integer position to transform (modified in place).
	 */
	virtual void nodeToWorld( Vector2i& pos ) const;

	/**
	 * @brief Converts world floating-point coordinates to node-local coordinates.
	 *
	 * Transforms a world position (with floating-point coordinates) to this node's
	 * local coordinate space.
	 *
	 * @param pos Reference to the floating-point position to transform (modified in place).
	 */
	virtual void worldToNode( Vector2f& pos ) const;

	/**
	 * @brief Converts node-local floating-point coordinates to world coordinates.
	 *
	 * Transforms a node-local position (with floating-point coordinates) to world
	 * coordinate space.
	 *
	 * @param pos Reference to the floating-point position to transform (modified in place).
	 */
	virtual void nodeToWorld( Vector2f& pos ) const;

	/**
	 * @brief Gets the node type identifier.
	 *
	 * Returns a unique type identifier for this node class. Derived classes should
	 * override this to return their specific type constant.
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
	 * @brief Posts a message to this node and its ancestors.
	 *
	 * Sends a node message up the parent chain until a node handles it. Messages
	 * are used for custom communication between nodes.
	 *
	 * @param Msg Pointer to the NodeMessage to post.
	 */
	void messagePost( const NodeMessage* Msg );

	/**
	 * @brief Sets the node position in density-independent pixels (dp).
	 *
	 * Sets the position of the node. The position is relative to the parent node's
	 * coordinate system. Setting a new position marks the node as dirty and triggers
	 * layout updates.
	 *
	 * @param Pos The new position.
	 */
	virtual void setPosition( const Vector2f& Pos );

	/**
	 * @brief Sets the node position using separate coordinates.
	 *
	 * Convenience method to set the X and Y coordinates individually.
	 *
	 * @param x The X coordinate.
	 * @param y The Y coordinate.
	 * @return Pointer to this node for method chaining.
	 */
	virtual Node* setPosition( const Float& x, const Float& y );

	/**
	 * @brief Sets the node size in density-independent pixels (dp).
	 *
	 * Sets the size of the node. The size will be validated against minimum/maximum
	 * constraints if applicable.
	 *
	 * @param size The new size.
	 * @return Pointer to this node for method chaining.
	 */
	virtual Node* setSize( const Sizef& size );

	/**
	 * @brief Sets the node size using separate width and height values.
	 *
	 * Convenience method to set width and height individually.
	 *
	 * @param Width The width in dp.
	 * @param Height The height in dp.
	 * @return Pointer to this node for method chaining.
	 */
	Node* setSize( const Float& Width, const Float& Height );

	/**
	 * @brief Gets the node size in density-independent pixels (dp).
	 *
	 * Returns the current size of the node. This value may differ from the
	 * pixel size if scaling is applied.
	 *
	 * @return The size as a Sizef.
	 */
	virtual const Sizef& getSize() const;

	/**
	 * @brief Gets the node size in actual screen pixels.
	 *
	 * Returns the current size of the node in raw pixel units, accounting for
	 * any scaling factors.
	 *
	 * @return The pixel size as a Sizef.
	 */
	const Sizef& getPixelsSize() const;

	/**
	 * @brief Sets the visibility of the node.
	 *
	 * Controls whether the node is rendered. Invisible nodes and their children
	 * are not drawn and do not receive mouse events.
	 *
	 * @param visible True to make the node visible, false to hide it.
	 * @param emitEventNotification If true, emits a visibility change event.
	 * @return Pointer to this node for method chaining.
	 */
	Node* setVisible( const bool& visible, bool emitEventNotification = true );

	/**
	 * @brief Sets visibility for all children recursively.
	 *
	 * Changes the visibility state of this node and all its descendants.
	 *
	 * @param visible The visibility state to set.
	 * @param emitEventNotification If true, emits visibility change events.
	 * @return Pointer to this node for method chaining.
	 */
	Node* setChildrenVisibility( bool visible, bool emitEventNotification = true );

	/**
	 * @brief Checks if the node is visible.
	 *
	 * Returns the current visibility state of this node only, not considering
	 * parent visibility.
	 *
	 * @return True if the node is visible, false otherwise.
	 */
	bool isVisible() const;

	/**
	 * @brief Checks if the node and all its parents are visible.
	 *
	 * Traverses up the parent chain to verify that this node and all ancestors
	 * are visible. Useful for determining if the node will actually be rendered.
	 *
	 * @return True if the node and all parents are visible, false otherwise.
	 */
	bool hasVisibility() const;

	/**
	 * @brief Enables or disables the node.
	 *
	 * Disabled nodes do not receive input events and are typically rendered
	 * with a disabled appearance.
	 *
	 * @param enabled True to enable the node, false to disable.
	 * @return Pointer to this node for method chaining.
	 */
	Node* setEnabled( const bool& enabled );

	/**
	 * @brief Checks if the node is enabled.
	 *
	 * @return True if the node is enabled, false if disabled.
	 */
	bool isEnabled() const;

	/**
	 * @brief Checks if the node is disabled.
	 *
	 * Convenience method equivalent to !isEnabled().
	 *
	 * @return True if the node is disabled, false otherwise.
	 */
	bool isDisabled() const;

	/**
	 * @brief Gets the parent node.
	 *
	 * Returns the direct parent of this node in the scene graph, or nullptr if
	 * the node has no parent.
	 *
	 * @return Pointer to the parent node or nullptr.
	 */
	Node* getParent() const;

	/**
	 * @brief Sets the parent node.
	 *
	 * Re-parents this node to a new parent. The node is automatically removed
	 * from its current parent if any.
	 *
	 * @param parent The new parent node.
	 * @return Pointer to this node for method chaining.
	 */
	Node* setParent( Node* parent );

	/**
	 * @brief Closes the node.
	 *
	 * Marks the node for closure. The node will be removed from the scene during
	 * the next update cycle. This is the safe way to remove nodes as it prevents
	 * iterator invalidation issues.
	 */
	virtual void close();

	/**
	 * @brief Draws the node.
	 *
	 * Virtual method that derived classes should override to implement custom
	 * rendering. The default implementation does nothing. This is called during
	 * the render cycle after the transformation matrix is set up.
	 */
	virtual void draw();

	/**
	 * @brief Updates the node.
	 *
	 * Called once per frame to update the node's state. The default implementation
	 * recursively updates all children. Derived classes should call the base
	 * implementation to maintain child updates.
	 *
	 * @param time The time elapsed since the last frame.
	 */
	virtual void update( const Time& time );

	/**
	 * @brief Performs scheduled updates for nodes with update subscriptions.
	 *
	 * Called by the scene manager for nodes that have subscribed to scheduled updates.
	 * Override to implement time-based update logic for this node.
	 *
	 * @param time The time elapsed since the last frame.
	 */
	virtual void scheduledUpdate( const Time& time );

	/**
	 * @brief Gets the next sibling node in the parent's child list.
	 *
	 * Returns the node that comes after this one in the parent's linked list of
	 * children, or nullptr if this is the last child.
	 *
	 * @return Pointer to the next sibling node or nullptr.
	 */
	Node* getNextNode() const;

	/**
	 * @brief Gets the previous sibling node in the parent's child list.
	 *
	 * Returns the node that comes before this one in the parent's linked list of
	 * children, or nullptr if this is the first child.
	 *
	 * @return Pointer to the previous sibling node or nullptr.
	 */
	Node* getPrevNode() const;

	/**
	 * @brief Gets the next sibling node, wrapping to first if at end.
	 *
	 * Returns the next sibling node, or if this is the last child, returns the
	 * parent's first child (creating a circular traversal).
	 *
	 * @return Pointer to the next node in the loop.
	 */
	Node* getNextNodeLoop() const;

	/**
	 * @brief Attaches arbitrary user data to this node.
	 *
	 * Sets a user data pointer that can be used to associate custom data with
	 * this node. The data is not managed by the node and must be cleaned up
	 * by the user.
	 *
	 * @param data The user data pointer to store.
	 * @return Pointer to this node for method chaining.
	 */
	Node* setData( const UintPtr& data );

	/**
	 * @brief Gets the user data previously set with setData().
	 *
	 * @return The stored user data pointer.
	 */
	const UintPtr& getData() const;

	/**
	 * @brief Sets the blend mode for this node.
	 *
	 * Controls how this node's colors blend with the background when rendered.
	 * Common blend modes include Alpha, Add, Multiply, etc.
	 *
	 * @param blend The blend mode to use.
	 * @return Pointer to this node for method chaining.
	 */
	Node* setBlendMode( const BlendMode& blend );

	/**
	 * @brief Gets the current blend mode.
	 *
	 * @return The current blend mode.
	 */
	const BlendMode& getBlendMode() const;

	/**
	 * @brief Moves this node to the front of its parent's child list.
	 *
	 * Changes the rendering order so this node is drawn after all its siblings.
	 * This affects the visual stacking order (nodes drawn later appear on top).
	 *
	 * @return Pointer to this node for method chaining.
	 */
	Node* toFront();

	/**
	 * @brief Moves this node to the back of its parent's child list.
	 *
	 * Changes the rendering order so this node is drawn before all its siblings.
	 * This affects the visual stacking order (nodes drawn earlier appear behind).
	 *
	 * @return Pointer to this node for method chaining.
	 */
	Node* toBack();

	/**
	 * @brief Moves this node to a specific position in the parent's child list.
	 *
	 * Changes the rendering order by placing this node at the specified index
	 * among its siblings.
	 *
	 * @param position The zero-based index position in the parent's child list.
	 */
	void toPosition( const Uint32& position );

	/**
	 * @brief Gets the current node flags.
	 *
	 * Returns the bitmask of flags that control various node behaviors and states.
	 *
	 * @return The flags as a Uint32 bitmask.
	 */
	const Uint32& getNodeFlags() const;

	/**
	 * @brief Sets the node flags directly.
	 *
	 * Use with caution - this replaces all flags and may cause unexpected behavior.
	 * Prefer specific flag manipulation methods when available.
	 *
	 * @param flags The new flags bitmask.
	 */
	void setNodeFlags( const Uint32& flags );

	/**
	 * @brief Checks if this node is a SceneNode.
	 *
	 * SceneNode is the base class for nodes that handle rendering and scene management.
	 *
	 * @return True if this node is a SceneNode, false otherwise.
	 */
	bool isSceneNode() const;

	/**
	 * @brief Checks if this node is a UISceneNode.
	 *
	 * UISceneNode is the root node for UI rendering with CSS styling support.
	 *
	 * @return True if this node is a UISceneNode, false otherwise.
	 */
	bool isUISceneNode() const;

	/**
	 * @brief Checks if this node is a UINode.
	 *
	 * UINode is the base class for UI elements with theming and layout support.
	 *
	 * @return True if this node is a UINode, false otherwise.
	 */
	bool isUINode() const;

	/**
	 * @brief Checks if this node is a UIWidget.
	 *
	 * UIWidget is the base class for interactive UI elements with CSS support.
	 *
	 * @return True if this node is a UIWidget, false otherwise.
	 */
	bool isWidget() const;

	/**
	 * @brief Checks if this node is a Window.
	 *
	 * Window nodes represent top-level application windows.
	 *
	 * @return True if this node is a Window, false otherwise.
	 */
	bool isWindow() const;

	/**
	 * @brief Checks if this node is a Layout.
	 *
	 * Layout nodes are specialized containers that manage the positioning of
	 * their children according to layout rules.
	 *
	 * @return True if this node is a Layout, false otherwise.
	 */
	bool isLayout() const;

	/**
	 * @brief Checks if clipping is enabled for this node.
	 *
	 * When clipping is enabled, node content is restricted to the node's bounds.
	 *
	 * @return True if clipping is enabled, false otherwise.
	 */
	bool isClipped() const;

	/**
	 * @brief Checks if this node has rotation applied.
	 *
	 * @return True if the node's rotation is non-zero, false otherwise.
	 */
	bool isRotated() const;

	/**
	 * @brief Checks if this node has scaling applied.
	 *
	 * @return True if the node's scale is not (1,1), false otherwise.
	 */
	bool isScaled() const;

	/**
	 * @brief Checks if this node uses a frame buffer.
	 *
	 * Frame buffer nodes render to an off-screen texture first.
	 *
	 * @return True if using frame buffer rendering, false otherwise.
	 */
	bool isFrameBuffer() const;

	/**
	 * @brief Checks if the mouse is currently over this node.
	 *
	 * @return True if the mouse cursor is over this node, false otherwise.
	 */
	bool isMouseOver() const;

	/**
	 * @brief Checks if the mouse is over this node or any of its children.
	 *
	 * @return True if the mouse is over this node or any descendant, false otherwise.
	 */
	bool isMouseOverMeOrChildren() const;

	/**
	 * @brief Checks if this node and all its parents are visible in the tree.
	 *
	 * Similar to hasVisibility() but optimized for tree traversal.
	 *
	 * @return True if this node and all ancestors are visible, false otherwise.
	 */
	bool isMeOrParentTreeVisible() const;

	/**
	 * @brief Checks if this node or any parent has rotation.
	 *
	 * @return True if this node or any ancestor is rotated, false otherwise.
	 */
	bool isMeOrParentTreeRotated() const;

	/**
	 * @brief Checks if this node or any parent has scaling.
	 *
	 * @return True if this node or any ancestor is scaled, false otherwise.
	 */
	bool isMeOrParentTreeScaled() const;

	/**
	 * @brief Checks if this node or any parent has scaling or rotation.
	 *
	 * @return True if this node or any ancestor is scaled or rotated, false otherwise.
	 */
	bool isMeOrParentTreeScaledOrRotated() const;

	/**
	 * @brief Checks if this node or any parent has scaling, rotation, or frame buffer.
	 *
	 * This is used to determine if special clipping planes are needed.
	 *
	 * @return True if this node or any ancestor has any of these transforms, false otherwise.
	 */
	bool isMeOrParentTreeScaledOrRotatedOrFrameBuffer() const;

	/**
	 * @brief Adds an event listener for a specific event type.
	 *
	 * Registers a callback function to be invoked when the specified event occurs.
	 * Returns a unique ID that can be used to remove the listener later.
	 *
	 * @param eventType The event type constant (e.g., Event::MouseClick).
	 * @param callback The function to call when the event occurs.
	 * @return A unique callback identifier.
	 */
	Uint32 addEventListener( const Uint32& eventType, const EventCallback& callback );

	/**
	 * @brief Adds an event listener (alias for addEventListener).
	 *
	 * @param eventType The event type constant.
	 * @param callback The function to call when the event occurs.
	 * @return A unique callback identifier.
	 */
	Uint32 on( const Uint32& eventType, const EventCallback& callback );

	/**
	 * @brief Adds a mouse click event listener.
	 *
	 * Convenience method to listen for mouse click events, optionally filtered
	 * by mouse button.
	 *
	 * @param callback The function to call on mouse click.
	 * @param button The mouse button to filter for (default: left button).
	 * @return A unique callback identifier.
	 */
	Uint32 onClick( const std::function<void( const MouseEvent* )>& callback,
					const MouseButton& button = MouseButton::EE_BUTTON_LEFT );

	/**
	 * @brief Adds a mouse double-click event listener.
	 *
	 * Convenience method to listen for mouse double-click events.
	 *
	 * @param callback The function to call on double click.
	 * @param button The mouse button to filter for (default: left button).
	 * @return A unique callback identifier.
	 */
	Uint32 onDoubleClick( const std::function<void( const MouseEvent* )>& callback,
						  const MouseButton& button = MouseButton::EE_BUTTON_LEFT );

	/**
	 * @brief Removes all event listeners of a specific type.
	 *
	 * @param eventType The event type to remove listeners for.
	 */
	void removeEventsOfType( const Uint32& eventType );

	/**
	 * @brief Removes a specific event listener by its callback ID.
	 *
	 * @param callbackId The ID returned by addEventListener or on().
	 */
	void removeEventListener( const Uint32& callbackId );

	/**
	 * @brief Removes multiple event listeners by their callback IDs.
	 *
	 * @param callbacksIds Vector of callback IDs to remove.
	 */
	void removeEventListener( const std::vector<Uint32>& callbacksIds );

	/**
	 * @brief Removes all event listeners from this node.
	 *
	 * Clears the entire event listener registry.
	 */
	void clearEventListener();

	/**
	 * @brief Gets the first child node.
	 *
	 * @return Pointer to the first child or nullptr if no children exist.
	 */
	Node* getFirstChild() const;

	/**
	 * @brief Gets the last child node.
	 *
	 * @return Pointer to the last child or nullptr if no children exist.
	 */
	Node* getLastChild() const;

	/**
	 * @brief Gets the world polygon of this node.
	 *
	 * Returns the axis-aligned bounding polygon in world coordinates, accounting
	 * for all transformations (position, rotation, scale, parent transforms).
	 * The polygon is cached and only recalculated when dirty.
	 *
	 * @return Const reference to the Polygon2f representing world bounds.
	 */
	const Polygon2f& getWorldPolygon();

	/**
	 * @brief Gets the world bounding rectangle.
	 *
	 * Returns the axis-aligned bounding box in world coordinates. This is
	 * derived from the world polygon and is also cached.
	 *
	 * @return Const reference to the Rectf representing world bounds.
	 */
	const Rectf& getWorldBounds();

	/**
	 * @brief Checks if a node is a descendant of this node.
	 *
	 * Determines whether the specified node is in this node's subtree.
	 *
	 * @param node The node to check.
	 * @return True if node is a child or grandchild, false otherwise.
	 */
	bool isParentOf( const Node* node ) const;

	/**
	 * @brief Sends an event to this node for handling.
	 *
	 * Dispatches the event to all registered listeners for the event type.
	 * The event is processed synchronously in the calling thread.
	 *
	 * @param Event Pointer to the event to send.
	 */
	void sendEvent( const Event* Event );

	/**
	 * @brief Sends a mouse event to this node.
	 *
	 * Creates and sends a MouseEvent with the specified parameters.
	 *
	 * @param Event The mouse event type (e.g., Event::MouseClick).
	 * @param position The mouse position in pixels.
	 * @param flags The mouse event flags (button states, modifiers).
	 */
	void sendMouseEvent( const Uint32& Event, const Vector2i& position, const Uint32& flags );

	/**
	 * @brief Sends a common event to this node.
	 *
	 * Creates and sends an Event with the specified event type.
	 *
	 * @param Event The event type.
	 */
	void sendCommonEvent( const Uint32& Event );

	/**
	 * @brief Sends a text event to this node.
	 *
	 * Creates and sends a TextEvent with the specified text content.
	 *
	 * @param event The event type (typically text-related).
	 * @param text The text string to send.
	 */
	void sendTextEvent( const Uint32& event, const std::string& text );

	/**
	 * @brief Closes all children recursively.
	 *
	 * Marks all descendant nodes for closure. The nodes will be removed during
	 * the next update cycle.
	 */
	void closeAllChildren();

	/**
	 * @brief Gets the node's identifier string.
	 *
	 * Returns the unique ID string set with setId(). An empty string means
	 * no ID is set.
	 *
	 * @return The node ID as a const string reference.
	 */
	const std::string& getId() const;

	/**
	 * @brief Sets the node's identifier string.
	 *
	 * Assigns a unique ID to this node. The ID can be used for CSS selectors
	 * and for finding the node with find(). Calling setId() also updates
	 * the cached hash value and triggers onIdChange().
	 *
	 * @param id The ID string to set.
	 * @return Pointer to this node for method chaining.
	 */
	virtual Node* setId( const std::string& id );

	/**
	 * @brief Gets the precomputed hash of the node ID.
	 *
	 * Returns the cached hash of the ID string for efficient lookups.
	 *
	 * @return The ID hash value.
	 */
	const String::HashType& getIdHash() const;

	/**
	 * @brief Finds a descendant node by its ID string.
	 *
	 * Searches the subtree rooted at this node for a node with the specified
	 * ID. The search is depth-first and stops at the first match.
	 *
	 * @param id The ID string to search for.
	 * @return Pointer to the matching node or nullptr if not found.
	 */
	Node* find( const std::string& id ) const;

	/**
	 * @brief Checks if this node has a direct child with the specified ID.
	 *
	 * Only searches among immediate children, not the full subtree.
	 *
	 * @param id The ID string to search for.
	 * @return Pointer to the child node or nullptr if not found.
	 */
	Node* hasChild( const std::string& id ) const;

	/**
	 * @brief Template version of find() with type casting.
	 *
	 * Finds a descendant node by ID and casts it to the specified type.
	 * Returns nullptr if not found or if the cast would fail.
	 *
	 * @tparam T The type to cast the found node to.
	 * @param id The ID string to search for.
	 * @return Pointer to the node cast to T* or nullptr.
	 */
	template <typename T> T* find( const std::string& id ) const {
		return reinterpret_cast<T*>( find( id ) );
	}

	/**
	 * @brief Template version of hasChild() with type casting.
	 *
	 * Finds a direct child by ID and casts it to the specified type.
	 *
	 * @tparam T The type to cast the found child to.
	 * @param id The ID string to search for.
	 * @param node Output parameter that receives the found node pointer.
	 * @return Pointer to the node cast to T* or nullptr.
	 */
	template <typename T> T* bind( const std::string& id, T*& node ) {
		node = find<T>( id );
		return node;
	}

	/**
	 * @brief Casts this node to a different type without checking.
	 *
	 * Dangerous - use only when you are certain of the actual type.
	 *
	 * @tparam T The type to cast to.
	 * @return Pointer to this node as T*.
	 */
	template <typename T> T* asType() { return reinterpret_cast<T*>( this ); }

	/**
	 * @brief Const version of asType().
	 *
	 * @tparam T The type to cast to.
	 * @return Const pointer to this node as const T*.
	 */
	template <typename T> const T* asConstType() const {
		return reinterpret_cast<const T*>( this );
	}

	/**
	 * @brief Finds a descendant node by type.
	 *
	 * Searches the subtree for the first node of the specified type. The search
	 * is depth-first and checks node types using isType().
	 *
	 * @param type The node type constant to search for.
	 * @return Pointer to the first matching node or nullptr.
	 */
	Node* findByType( const Uint32& type ) const;

	/**
	 * @brief Template version of findByType() with type casting.
	 *
	 * Finds a descendant node by type and casts it to the specified template type.
	 *
	 * @tparam T The type to cast to.
	 * @param type The node type constant to search for.
	 * @return Pointer to the node cast to T* or nullptr.
	 */
	template <typename T> T* findByType( const Uint32& type ) const {
		return reinterpret_cast<T*>( findByType( type ) );
	}

	/**
	 * @brief Template helper for findByType() with output parameter.
	 *
	 * Finds a descendant node by type and stores the result in the output parameter.
	 *
	 * @tparam T The type to cast to.
	 * @param type The node type constant to search for.
	 * @param node Output parameter that receives the found node pointer.
	 * @return Pointer to the found node cast to T* or nullptr.
	 */
	template <typename T> T* bindByType( const Uint32& type, T*& node ) {
		node = findByType<T>( type );
		return node;
	}

	/**
	 * @brief Finds all descendant nodes of a specific type.
	 *
	 * Searches the entire subtree and collects all nodes matching the specified type.
	 *
	 * @param type The node type constant to search for.
	 * @return Vector of pointers to all matching nodes (may be empty).
	 */
	std::vector<Node*> findAllByType( const Uint32& type ) const;

	/**
	 * @brief Template version of findAllByType() with type casting.
	 *
	 * Finds all descendant nodes of a type and casts them to the specified template type.
	 *
	 * @tparam T The type to cast each node to.
	 * @param type The node type constant to search for.
	 * @return Vector of pointers to nodes cast to T* (may be empty).
	 */
	template <typename T> std::vector<T*> findAllByType( const Uint32& type ) const {
		std::vector<T*> casted;
		auto all( findAllByType( type ) );
		casted.reserve( all.size() );
		for ( auto* node : all )
			casted.push_back( reinterpret_cast<T*>( node ) );
		return casted;
	}

	/**
	 * @brief Checks if a node is in this node's tree.
	 *
	 * Determines whether the specified node is this node itself or any descendant.
	 *
	 * @param node The node to check.
	 * @return True if the node is in this tree, false otherwise.
	 */
	bool inNodeTree( Node* node ) const;

	/**
	 * @brief Checks if this node draws in reverse order.
	 *
	 * When reverse draw is enabled, children are drawn from last to first,
	 * causing the last child to appear on top.
	 *
	 * @return True if reverse drawing is enabled, false otherwise.
	 */
	bool isReverseDraw() const;

	/**
	 * @brief Enables or disables reverse drawing order.
	 *
	 * Controls whether children are drawn in normal order (first to last)
	 * or reverse order (last to first).
	 *
	 * @param reverseDraw True to draw children in reverse order.
	 */
	void setReverseDraw( bool reverseDraw );

	/**
	 * @brief Invalidates this node's draw state.
	 *
	 * Marks the node as needing to be redrawn. This triggers a redraw in the
	 * next frame. The invalidation is propagated to the draw invalidator node.
	 */
	void invalidateDraw();

	/**
	 * @brief Sets the rotation angle in degrees.
	 *
	 * Sets the node's rotation around its rotation origin point. The rotation
	 * is applied in world coordinates after scaling.
	 *
	 * @param angle The rotation angle in degrees.
	 */
	void setRotation( float angle );

	/**
	 * @brief Sets the rotation angle and origin point.
	 *
	 * Sets both the rotation angle and the point around which rotation occurs.
	 *
	 * @param angle The rotation angle in degrees.
	 * @param center The rotation origin point in dp.
	 */
	void setRotation( const Float& angle, const OriginPoint& center );

	/**
	 * @brief Gets the current rotation origin point.
	 *
	 * Returns the point around which rotation is applied, in dp units.
	 *
	 * @return Const reference to the rotation origin point.
	 */
	const OriginPoint& getRotationOriginPoint() const;

	/**
	 * @brief Sets the rotation origin point in dp.
	 *
	 * Defines the point around which rotation occurs, using dp units.
	 *
	 * @param center The rotation origin point in dp.
	 */
	void setRotationOriginPoint( const OriginPoint& center );

	/**
	 * @brief Sets the rotation origin point in pixels.
	 *
	 * Like setRotationOriginPoint but uses pixel units directly without dp conversion.
	 *
	 * @param center The rotation origin point in pixels.
	 */
	void setRotationOriginPointPixels( const OriginPoint& center );

	/**
	 * @brief Sets the X coordinate of the rotation origin using a CSS-like expression.
	 *
	 * Allows dynamic origin specification with expressions like "50%" or "10px".
	 *
	 * @param xEq The X coordinate expression string.
	 */
	void setRotationOriginPointX( const std::string& xEq );

	/**
	 * @brief Sets the Y coordinate of the rotation origin using a CSS-like expression.
	 *
	 * Allows dynamic origin specification with expressions like "50%" or "10px".
	 *
	 * @param yEq The Y coordinate expression string.
	 */
	void setRotationOriginPointY( const std::string& yEq );

	/**
	 * @brief Gets the actual rotation center in screen coordinates.
	 *
	 * Calculates and returns the world-space point around which rotation occurs,
	 * based on the origin point setting and current size.
	 *
	 * @return The rotation center as a Vector2f in dp.
	 */
	Vector2f getRotationCenter() const;

	/**
	 * @brief Sets uniform scaling factor.
	 *
	 * Sets the same scale factor for both X and Y axes. Scale of 1.0 means
	 * no scaling, values > 1 enlarge, values < 1 shrink.
	 *
	 * @param scale The uniform scale factor.
	 */
	void setScale( const Float& scale );

	/**
	 * @brief Sets scaling with separate factors for X and Y.
	 *
	 * @param scale The scale vector (x, y factors).
	 */
	void setScale( const Vector2f& scale );

	/**
	 * @brief Sets scaling with origin point.
	 *
	 * Sets the scale factors and the origin point around which scaling occurs.
	 *
	 * @param scale The scale vector (x, y factors).
	 * @param center The scaling origin point in dp.
	 */
	void setScale( const Vector2f& scale, const OriginPoint& center );

	/**
	 * @brief Sets uniform scaling with origin point.
	 *
	 * Sets uniform scaling and the origin point around which scaling occurs.
	 *
	 * @param scale The uniform scale factor.
	 * @param center The scaling origin point in dp.
	 */
	void setScale( const Float& scale, const OriginPoint& center );

	/**
	 * @brief Gets the current scale origin point.
	 *
	 * Returns the point around which scaling is applied, in dp units.
	 *
	 * @return Const reference to the scale origin point.
	 */
	const OriginPoint& getScaleOriginPoint() const;

	/**
	 * @brief Sets the scale origin point in dp.
	 *
	 * Defines the point around which scaling occurs.
	 *
	 * @param center The scale origin point in dp.
	 */
	void setScaleOriginPoint( const OriginPoint& center );

	/**
	 * @brief Sets the scale origin point in pixels.
	 *
	 * Like setScaleOriginPoint but uses pixel units directly without dp conversion.
	 *
	 * @param center The scale origin point in pixels.
	 */
	void setScaleOriginPointPixels( const OriginPoint& center );

	/**
	 * @brief Sets the X coordinate of the scale origin using a CSS-like expression.
	 *
	 * Allows dynamic origin specification with expressions like "50%" or "10px".
	 *
	 * @param xEq The X coordinate expression string.
	 */
	void setScaleOriginPointX( const std::string& xEq );

	/**
	 * @brief Sets the Y coordinate of the scale origin using a CSS-like expression.
	 *
	 * Allows dynamic origin specification with expressions like "50%" or "10px".
	 *
	 * @param yEq The Y coordinate expression string.
	 */
	void setScaleOriginPointY( const std::string& yEq );

	/**
	 * @brief Gets the actual scale center in screen coordinates.
	 *
	 * Calculates and returns the world-space point around which scaling occurs,
	 * based on the origin point setting and current size.
	 *
	 * @return The scale center as a Vector2f in dp.
	 */
	Vector2f getScaleCenter() const;

	/**
	 * @brief Sets non-uniform scaling with raw float factors.
	 *
	 * Virtual method that derived classes can override to add custom scaling behavior.
	 *
	 * @param factorX The X scale factor.
	 * @param factorY The Y scale factor.
	 */
	virtual void setScale( float factorX, float factorY );

	/**
	 * @brief Sets the scale origin with raw coordinates.
	 *
	 * Virtual method that sets the point around which scaling occurs using raw
	 * float coordinates.
	 *
	 * @param x The X coordinate of the scale origin.
	 * @param y The Y coordinate of the scale origin.
	 */
	virtual void setScaleOrigin( float x, float y );

	/**
	 * @brief Sets the rotation origin with raw coordinates.
	 *
	 * Virtual method that sets the point around which rotation occurs using raw
	 * float coordinates.
	 *
	 * @param x The X coordinate of the rotation origin.
	 * @param y The Y coordinate of the rotation origin.
	 */
	virtual void setRotationOrigin( float x, float y );

	/**
	 * @brief Gets the current alpha (transparency) value.
	 *
	 * Returns the alpha value that affects this node's opacity. Range is typically
	 * 0-255 where 0 is fully transparent and 255 is fully opaque.
	 *
	 * @return The alpha value as a Float (typically 0-255).
	 */
	const Float& getAlpha() const;

	/**
	 * @brief Sets the alpha (transparency) for this node.
	 *
	 * Controls the opacity of this node. Values typically range from 0 (transparent)
	 * to 255 (opaque). This affects both the node and its children.
	 *
	 * @param alpha The alpha value to set.
	 */
	virtual void setAlpha( const Float& alpha );

	/**
	 * @brief Sets alpha for this node and all children recursively.
	 *
	 * Convenience method to set uniform alpha across the entire subtree.
	 *
	 * @param alpha The alpha value to set for all nodes.
	 */
	virtual void setChildrenAlpha( const Float& alpha );

	/**
	 * @brief Gets the action manager associated with this node.
	 *
	 * Returns the ActionManager that controls actions (animations, timed callbacks)
	 * for this node. The action manager is obtained from the scene node.
	 *
	 * @return Pointer to the ActionManager.
	 */
	ActionManager* getActionManager() const;

	/**
	 * @brief Runs an action on this node.
	 *
	 * Starts an action (animation, delay, callback, etc.) that will be updated
	 * automatically. The action will be owned by the action manager.
	 *
	 * @param action Pointer to the Action to run.
	 * @return Pointer to this node for method chaining.
	 */
	Node* runAction( Action* action );

	/**
	 * @brief Removes a specific action from this node.
	 *
	 * Stops and removes the specified action if it is running on this node.
	 *
	 * @param action Pointer to the action to remove.
	 * @return True if the action was found and removed, false otherwise.
	 */
	bool removeAction( Action* action );

	/**
	 * @brief Removes multiple actions from this node.
	 *
	 * Stops and removes all actions in the provided vector.
	 *
	 * @param actions Vector of action pointers to remove.
	 * @return True if any actions were removed, false otherwise.
	 */
	bool removeActions( const std::vector<Action*>& actions );

	/**
	 * @brief Removes all actions with a specific tag from this node.
	 *
	 * Useful for stopping a group of related actions identified by a common tag.
	 *
	 * @param tag The unique tag identifier.
	 * @return True if any actions were removed, false otherwise.
	 */
	bool removeActionsByTag( const Action::UniqueID& tag );

	/**
	 * @brief Gets all actions currently running on this node.
	 *
	 * @return Vector of pointers to all active actions.
	 */
	std::vector<Action*> getActions();

	/**
	 * @brief Gets all actions with a specific tag.
	 *
	 * @param tag The tag to search for.
	 * @return Vector of pointers to matching actions (may be empty).
	 */
	std::vector<Action*> getActionsByTag( const Action::UniqueID& tag );

	/**
	 * @brief Removes all actions from this node.
	 *
	 * Stops and clears every action currently running on this node.
	 */
	void clearActions();

	/**
	 * @brief Gets the local transformation matrix.
	 *
	 * Returns the transformation matrix representing this node's local transform
	 * (position, rotation, scale) without parent influences.
	 *
	 * @return The local Transform matrix.
	 */
	Transform getLocalTransform() const;

	/**
	 * @brief Gets the global transformation matrix.
	 *
	 * Returns the complete transformation matrix from this node's local space
	 * to world space, including all parent transformations.
	 *
	 * @return The global Transform matrix.
	 */
	Transform getGlobalTransform() const;

	/**
	 * @brief Gets the node-to-world transformation matrix.
	 *
	 * Alias for getGlobalTransform().
	 *
	 * @return The node-to-world Transform matrix.
	 */
	Transform getNodeToWorldTransform() const;

	/**
	 * @brief Gets the world-to-node transformation matrix.
	 *
	 * Returns the inverse of the node-to-world transform, useful for converting
	 * world coordinates to this node's local space.
	 *
	 * @return The world-to-node Transform matrix.
	 */
	Transform getWorldToNodeTransform() const;

	/**
	 * @brief Converts a world point to node-local coordinates.
	 *
	 * Transforms a point from world space to this node's local space using
	 * the transformation matrices.
	 *
	 * @param worldPoint The point in world coordinates.
	 * @return The point in node-local coordinates.
	 */
	Vector2f convertToNodeSpace( const Vector2f& worldPoint ) const;

	/**
	 * @brief Converts a node-local point to world coordinates.
	 *
	 * Transforms a point from this node's local space to world space.
	 *
	 * @param nodePoint The point in node-local coordinates.
	 * @return The point in world coordinates.
	 */
	Vector2f convertToWorldSpace( const Vector2f& nodePoint ) const;

	/**
	 * @brief Gets the node's local bounding rectangle.
	 *
	 * Returns the axis-aligned bounding box in the node's local coordinate space
	 * (from origin to size). This is the untransformed bounds.
	 *
	 * @return Rectf representing local bounds (0,0 to width,height).
	 */
	Rectf getLocalBounds() const;

	/**
	 * @brief Checks if this node currently has input focus.
	 *
	 * Focus means this node receives keyboard and other input events.
	 *
	 * @return True if this node has focus, false otherwise.
	 */
	bool hasFocus() const;

	/**
	 * @brief Checks if this node or any descendant has focus.
	 *
	 * @return True if this node or any child has focus, false otherwise.
	 */
	bool hasFocusWithin() const;

	/**
	 * @brief Requests input focus for this node.
	 *
	 * Attempts to give this node keyboard focus. Derived classes may override
	 * to implement custom focus behavior.
	 *
	 * @param reason The reason for the focus request.
	 * @return Pointer to this node if focus was granted, nullptr otherwise.
	 */
	virtual Node* setFocus( NodeFocusReason reason = NodeFocusReason::Unknown );

	/**
	 * @brief Gets the first widget child.
	 *
	 * Searches among direct children and returns the first one that is a UIWidget.
	 *
	 * @return Pointer to the first child widget or nullptr if none found.
	 */
	Node* getFirstWidget() const;

	/**
	 * @brief Gets the nearest widget ancestor.
	 *
	 * Traverses up the parent chain looking for a widget. Useful for finding
	 * a UIWidget container.
	 *
	 * @return Pointer to the parent widget or nullptr if not found.
	 */
	Node* getParentWidget() const;

	/**
	 * @brief Enables reporting of size changes to children.
	 *
	 * When enabled, children will receive onParentSizeChange() calls when
	 * this node's size changes.
	 */
	void enableReportSizeChangeToChildren();

	/**
	 * @brief Disables reporting of size changes to children.
	 *
	 * When disabled, children will not be notified when this node's size changes.
	 */
	void disableReportSizeChangeToChildren();

	/**
	 * @brief Checks if size change reporting to children is enabled.
	 *
	 * @return True if size changes are reported to children, false otherwise.
	 */
	bool reportSizeChangeToChildren() const;

	/**
	 * @brief Centers this node horizontally within its parent.
	 *
	 * Adjusts the node's X position so it is centered in the parent's width.
	 *
	 * @return Pointer to this node for method chaining.
	 */
	Node* centerHorizontal();

	/**
	 * @brief Centers this node vertically within its parent.
	 *
	 * Adjusts the node's Y position so it is centered in the parent's height.
	 *
	 * @return Pointer to this node for method chaining.
	 */
	Node* centerVertical();

	/**
	 * @brief Centers this node both horizontally and vertically.
	 *
	 * Convenience method that calls both centerHorizontal() and centerVertical().
	 *
	 * @return Pointer to this node for method chaining.
	 */
	Node* center();

	/**
	 * @brief Enables clipping to the node's bounds.
	 *
	 * When clipping is enabled, content drawn outside this node's rectangle
	 * will be cut off.
	 *
	 * @return Pointer to this node for method chaining.
	 */
	Node* clipEnable();

	/**
	 * @brief Disables clipping to the node's bounds.
	 *
	 * Content will be allowed to draw outside the node's rectangle.
	 *
	 * @return Pointer to this node for method chaining.
	 */
	Node* clipDisable();

	/**
	 * @brief Sets or clears a specific node flag bit.
	 *
	 * Internal helper method for modifying individual flag bits without affecting
	 * other flags.
	 *
	 * @param Flag The flag bit to modify (one of the NODE_FLAG_* constants).
	 * @param Val The value to set (1 to set, 0 to clear).
	 */
	void writeNodeFlag( const Uint32& Flag, const Uint32& Val );

	/**
	 * @brief Gets the SceneNode that contains this node.
	 *
	 * Traverses up the parent chain to find the nearest SceneNode ancestor.
	 *
	 * @return Pointer to the SceneNode or nullptr if not found.
	 */
	SceneNode* getSceneNode() const;

	/**
	 * @brief Gets the event dispatcher associated with this node.
	 *
	 * Returns the EventDispatcher from the scene node, or nullptr if this node
	 * is not in a scene.
	 *
	 * @return Pointer to the EventDispatcher or nullptr.
	 */
	EventDispatcher* getEventDispatcher() const;

	/**
	 * @brief Checks if this node is a draw invalidator.
	 *
	 * Draw invalidators are nodes that trigger redraws of their entire subtree
	 * when they become invalidated. The base Node class returns false; derived
	 * classes like SceneNode override this to return true.
	 *
	 * @return True if this node invalidates children on draw invalidation.
	 */
	virtual bool isDrawInvalidator() const;

	/**
	 * @brief Checks if this node has been invalidated.
	 *
	 * Returns whether the view dirty flag is set, indicating the node needs
	 * to be redrawn.
	 *
	 * @return True if the node is invalidated, false otherwise.
	 */
	bool invalidated() const;

	/**
	 * @brief Invalidates this node and optionally its children.
	 *
	 * Marks this node as needing redraw. If the node is visible and has non-zero
	 * alpha, sets the view dirty flag. Derived classes may override to add
	 * custom invalidation logic.
	 *
	 * @param invalidator The node that caused the invalidation (unused in base).
	 */
	virtual void invalidate( Node* invalidator );

	/**
	 * @brief Gets the number of direct children.
	 *
	 * @return Count of immediate child nodes.
	 */
	Uint32 getChildCount() const;

	/**
	 * @brief Counts children of a specific type.
	 *
	 * Counts how many immediate children have the specified node type.
	 *
	 * @param type The node type to count.
	 * @return Number of children of that type.
	 */
	Uint32 getChildOfTypeCount( const Uint32& type ) const;

	/**
	 * @brief Gets the child node at the specified index.
	 *
	 * Returns the child at the given zero-based index in the linked list of children.
	 * Children are stored in the order they were added.
	 *
	 * @param index Zero-based index of the child to retrieve.
	 * @return Pointer to the child node or nullptr if index is out of range.
	 */
	Node* getChildAt( Uint32 index ) const;

	/**
	 * @brief Gets the index of this node in its parent's child list.
	 *
	 * Returns the zero-based position of this node among its siblings.
	 *
	 * @return The node index, or 0 if the node has no parent.
	 */
	Uint32 getNodeIndex() const;

	/**
	 * @brief Gets the index of this node among siblings of the same type.
	 *
	 * Returns the position of this node when counting only siblings that have
	 * the same type as this node.
	 *
	 * @return The type-based node index, or 0 if the node has no parent.
	 */
	Uint32 getNodeOfTypeIndex() const;

	/**
	 * @brief Enqueues a runnable to be executed on the main thread.
	 *
	 * Schedules the runnable function to be executed during the scene node's update
	 * cycle, on the main thread. Useful for thread-safe operations from background
	 * threads.
	 *
	 * @param runnable The function to execute.
	 * @param delay The time to wait before execution (default: 0).
	 * @param uniqueIdentifier Optional unique ID for the action.
	 */
	void runOnMainThread( Actions::Runnable::RunnableFunc runnable,
						  const Time& delay = Seconds( 0 ),
						  const Action::UniqueID& uniqueIdentifier = 0 );

	/**
	 * @brief Executes runnable immediately if on main thread, otherwise queues it.
	 *
	 * Checks if called from the main thread. If so, executes the runnable immediately.
	 * If not, queues it for execution on the main thread via runOnMainThread().
	 *
	 * @param runnable The function to execute.
	 * @param uniqueIdentifier Optional unique ID for the action.
	 * @return True if executed immediately, false if queued.
	 */
	bool ensureMainThread( Actions::Runnable::RunnableFunc runnable,
						   const Action::UniqueID& uniqueIdentifier = 0 );

	/**
	 * @brief Executes a runnable after a delay.
	 *
	 * Schedules the runnable to be executed once after the specified delay.
	 *
	 * @param runnable The function to execute.
	 * @param delay The time to wait before execution.
	 * @param uniqueIdentifier Optional unique ID for the action.
	 */
	void setTimeout( Actions::Runnable::RunnableFunc runnable, const Time& delay = Seconds( 0 ),
					 const Action::UniqueID& uniqueIdentifier = 0 );

	/**
	 * @brief Executes a runnable repeatedly at fixed intervals.
	 *
	 * Schedules the runnable to be executed repeatedly, with a fixed time delay
	 * between each call. The runnable will continue until cancelled.
	 *
	 * @param runnable The function to execute.
	 * @param interval The time between executions.
	 * @param uniqueIdentifier Optional unique ID for the action.
	 */
	void setInterval( Actions::Runnable::RunnableFunc runnable, const Time& interval,
					  const Action::UniqueID& uniqueIdentifier = 0 );

	/**
	 * @brief Debounces a runnable execution.
	 *
	 * If the debounce function is called again before the delay expires, the previous
	 * pending execution is cancelled and the timer resets. Useful for rate-limiting
	 * rapid successive calls.
	 *
	 * @param runnable The function to execute after the delay.
	 * @param delay The debounce delay.
	 * @param uniqueIdentifier Unique ID used to identify and cancel previous debounced calls.
	 */
	void debounce( Actions::Runnable::RunnableFunc runnable, const Time& delay,
				   const Action::UniqueID& uniqueIdentifier );

	/**
	 * @brief Checks if a node is a direct child of this node.
	 *
	 * @param child The node to check.
	 * @return True if the node is an immediate child, false otherwise.
	 */
	bool isChild( Node* child ) const;

	/**
	 * @brief Checks if a node is in this node's parent tree.
	 *
	 * Determines whether the specified node is an ancestor of this node.
	 *
	 * @param child The node to treat as child (checking if this node is its parent).
	 * @return True if this node is in the parent chain of the given node.
	 */
	bool inParentTreeOf( Node* child ) const;

	/**
	 * @brief Checks if any ancestor has a specific node type.
	 *
	 * Traverses the parent chain looking for a node of the specified type.
	 *
	 * @param type The node type to look for.
	 * @return True if an ancestor of that type exists, false otherwise.
	 */
	bool inParentTreeOfType( Uint32 type ) const;

	/**
	 * @brief Finds the nearest ancestor of a specific type.
	 *
	 * Traverses up the parent chain and returns the first node matching the type.
	 *
	 * @param type The node type to search for.
	 * @return Pointer to the parent node of that type, or nullptr if not found.
	 */
	Node* getParentOfType( Uint32 type ) const;

	/**
	 * @brief Sets the loading state flag.
	 *
	 * Marks the node as loading or not loading. This flag can be used to
	 * suppress certain operations during loading.
	 *
	 * @param loading True to set loading state, false to clear.
	 */
	void setLoadingState( bool loading );

	/**
	 * @brief Checks if the node is in loading state.
	 *
	 * @return True if the node's loading flag is set, false otherwise.
	 */
	bool isLoadingState() const;

	/**
	 * @brief Called when the node's ID changes.
	 *
	 * Virtual method that can be overridden to respond to ID changes.
	 * The default implementation sends an OnIdChange event.
	 */
	virtual void onIdChange();

	/**
	 * @brief Checks if the node is marked for closure.
	 *
	 * @return True if the close flag is set (node will be removed), false otherwise.
	 */
	bool isClosing() const;

	/**
	 * @brief Checks if the node is in the process of closing children.
	 *
	 * @return True if the closing children flag is set, false otherwise.
	 */
	bool isClosingChildren() const;

	/**
	 * @brief Finds the node under a point, considering hit testing.
	 *
	 * Used for mouse picking. Recursively searches children (in reverse order) to
	 * find the topmost node containing the point. Only nodes with the OVER_FIND_ALLOWED
	 * flag are considered.
	 *
	 * @param Point The point to test in world coordinates.
	 * @return The topmost node at that point, or nullptr.
	 */
	virtual Node* overFind( const Vector2f& Point );

	/**
	 * @brief Removes the node from its parent.
	 *
	 * Detaches this node from its parent without triggering deletion. After calling
	 * this, the node has no parent and must be managed manually.
	 *
	 * Use with caution - this bypasses normal deletion lifecycle.
	 */
	void detach();

	/**
	 * @brief Applies a function to this node and all descendants.
	 *
	 * Traverses the entire subtree in depth-first order, calling func on each node.
	 *
	 * @param func The function to apply to each node.
	 */
	void forEachNode( std::function<void( Node* )> func );

	/**
	 * @brief Applies a function to all direct children.
	 *
	 * Calls func on each immediate child of this node (not including this node itself).
	 *
	 * @param func The function to apply to each child.
	 */
	void forEachChild( std::function<void( Node* )> func );

	/**
	 * @brief Performs the node's custom drawing operations.
	 *
	 * Virtual method that can be overridden to implement custom rendering. This
	 * is called during the draw cycle after the transform is set. The default
	 * implementation does nothing.
	 */
	virtual void nodeDraw();

	/**
	 * @brief Simulates a key down event on this node.
	 *
	 * Forces a key down event to be processed by this node, as if the user
	 * pressed a key while this node had focus.
	 *
	 * @param event The key event to simulate.
	 * @return The event handling result.
	 */
	Uint32 forceKeyDown( const KeyEvent& event );

	/**
	 * @brief Simulates a key up event on this node.
	 *
	 * Forces a key up event to be processed by this node.
	 *
	 * @param event The key event to simulate.
	 * @return The event handling result.
	 */
	Uint32 foceKeyUp( const KeyEvent& event );

	/**
	 * @brief Simulates a text input event on this node.
	 *
	 * Forces a text input event to be processed by this node.
	 *
	 * @param Event The text input event to simulate.
	 * @return The event handling result.
	 */
	Uint32 forceTextInput( const TextInputEvent& Event );

	/**
	 * @brief Gets the screen-space position of this node.
	 *
	 * Returns the computed position in actual screen pixels. This is the world
	 * position after all parent transformations have been applied.
	 *
	 * @return Const reference to the screen position as Vector2f.
	 */
	const Vector2f& getScreenPos() const;

	/**
	 * @brief Gets the screen-space rectangle of this node.
	 *
	 * Returns the node's bounds in actual screen pixels as a rectangle.
	 *
	 * @return Rectf representing the screen-space bounds.
	 */
	Rectf getScreenRect() const;

	/**
	 * @brief Checks if this node has listeners for a specific event type.
	 *
	 * Queries whether there are any event callbacks registered for the given
	 * event type on this node.
	 *
	 * @param eventType The event type to check.
	 * @return True if listeners exist, false otherwise.
	 */
	bool hasEventsOfType( const Uint32& eventType ) const;

	/**
	 * @brief Enables clipping for the node's bounds.
	 *
	 * Convenience overload that automatically determines if clipping planes
	 * are needed based on transforms.
	 *
	 * @param x Left edge.
	 * @param y Top edge.
	 * @param Width Width.
	 * @param Height Height.
	 */
	void clipSmartEnable( const Int32& x, const Int32& y, const Uint32& Width,
						  const Uint32& Height );

	/**
	 * @brief Disables clipping.
	 *
	 * Convenience overload that automatically determines if clipping planes
	 * are active.
	 */
	void clipSmartDisable();

  protected:
	/** @brief Map of event type to callback ID to callback function. */
	typedef UnorderedMap<Uint32, std::map<Uint32, EventCallback>> EventsMap;

	/** @brief Forward declaration for EventDispatcher. */
	friend class EventDispatcher;

	std::string mId;
	String::HashType mIdHash{ 0 };
	Vector2f mScreenPos;
	Sizef mSize;
	Float mAlpha{ 255.f };
	UintPtr mData{ 0 };
	Node* mParentNode{ nullptr };
	SceneNode* mSceneNode{ nullptr };
	Node* mNodeDrawInvalidator{ nullptr };
	Node* mChild{ nullptr };
	Node* mChildLast{ nullptr };
	Node* mNext{ nullptr };
	Node* mPrev{ nullptr };
	Uint32 mNodeFlags{ NODE_FLAG_POSITION_DIRTY | NODE_FLAG_POLYGON_DIRTY };
	BlendMode mBlend{ BlendMode::Alpha() };
	bool mVisible{ true };
	bool mEnabled{ true };
	Uint32 mNumCallBacks{ 0 };
	mutable Polygon2f mPoly;
	mutable Rectf mWorldBounds;
	Vector2f mCenter;
	EventsMap mEvents;
	OriginPoint mRotationOriginPoint;
	OriginPoint mScaleOriginPoint;

	Node();

	/**
	 * @brief Handles an incoming node message.
	 *
	 * Called when a message is posted to this node. Override to implement custom
	 * message handling. Return 0 to continue propagating to parent, non-zero to stop.
	 *
	 * @param msg The message to handle.
	 * @return 0 to continue propagation, non-zero to stop.
	 */
	virtual Uint32 onMessage( const NodeMessage* msg );

	/**
	 * @brief Handles text input events.
	 *
	 * Called when text is input (e.g., from an IME). Default implementation forwards
	 * the event via sendTextEvent().
	 *
	 * @param event The text input event.
	 * @return Event handling result.
	 */
	virtual Uint32 onTextInput( const TextInputEvent& event );

	/**
	 * @brief Handles text editing events.
	 *
	 * Called during text composition (e.g., IME editing). Default implementation
	 * forwards the event.
	 *
	 * @param event The text editing event.
	 * @return Event handling result.
	 */
	virtual Uint32 onTextEditing( const TextEditingEvent& event );

	/**
	 * @brief Handles key down events.
	 *
	 * Called when a key is pressed while this node has focus. Default implementation
	 * sends a KeyEvent to registered listeners.
	 *
	 * @param event The key event.
	 * @return Event handling result.
	 */
	virtual Uint32 onKeyDown( const KeyEvent& event );

	/**
	 * @brief Handles key up events.
	 *
	 * Called when a key is released while this node has focus. Default implementation
	 * sends a KeyEvent to registered listeners.
	 *
	 * @param event The key event.
	 * @return Event handling result.
	 */
	virtual Uint32 onKeyUp( const KeyEvent& event );

	/**
	 * @brief Handles mouse motion events.
	 *
	 * Called when the mouse moves over this node. Default implementation sends
	 * a MouseMove event and returns 1.
	 *
	 * @param position Mouse position in pixels.
	 * @param flags Mouse state flags.
	 * @return Event handling result (1 stops propagation).
	 */
	virtual Uint32 onMouseMove( const Vector2i& position, const Uint32& flags );

	/**
	 * @brief Handles mouse button down events.
	 *
	 * Called when a mouse button is pressed over this node. Default implementation
	 * sends a MouseDown event and returns 1.
	 *
	 * @param position Mouse position in pixels.
	 * @param flags Mouse state flags.
	 * @return Event handling result (1 stops propagation).
	 */
	virtual Uint32 onMouseDown( const Vector2i& position, const Uint32& flags );

	/**
	 * @brief Handles mouse button up events.
	 *
	 * Called when a mouse button is released over this node. Default implementation
	 * sends a MouseUp event and returns 1.
	 *
	 * @param position Mouse position in pixels.
	 * @param flags Mouse state flags.
	 * @return Event handling result (1 stops propagation).
	 */
	virtual Uint32 onMouseUp( const Vector2i& position, const Uint32& flags );

	/**
	 * @brief Handles mouse click events.
	 *
	 * Called after a complete click (down + up). Default implementation sends
	 * a MouseClick event and returns 1.
	 *
	 * @param position Mouse position in pixels.
	 * @param flags Mouse state flags.
	 * @return Event handling result (1 stops propagation).
	 */
	virtual Uint32 onMouseClick( const Vector2i& position, const Uint32& flags );

	/**
	 * @brief Handles mouse double-click events.
	 *
	 * Called when a double-click is detected. Default implementation sends
	 * a MouseDoubleClick event and returns 1.
	 *
	 * @param position Mouse position in pixels.
	 * @param flags Mouse state flags.
	 * @return Event handling result (1 stops propagation).
	 */
	virtual Uint32 onMouseDoubleClick( const Vector2i& position, const Uint32& flags );

	/**
	 * @brief Handles mouse enter/mouse over events.
	 *
	 * Called when the mouse enters this node's area. Updates the mouse over flag
	 * and sends MouseEnter/MouseOver events. Returns 1 to stop propagation.
	 *
	 * @param position Mouse position in pixels.
	 * @param flags Mouse state flags.
	 * @return Event handling result (1 stops propagation).
	 */
	virtual Uint32 onMouseOver( const Vector2i& position, const Uint32& flags );

	/**
	 * @brief Handles mouse leave events.
	 *
	 * Called when the mouse leaves this node's area. Clears the mouse over flag
	 * and sends MouseLeave/MouseOut events. Returns 1 to stop propagation.
	 *
	 * @param position Mouse position in pixels.
	 * @param flags Mouse state flags.
	 * @return Event handling result (1 stops propagation).
	 */
	virtual Uint32 onMouseLeave( const Vector2i& position, const Uint32& flags );

	/**
	 * @brief Handles mouse wheel scroll events.
	 *
	 * Called when the mouse wheel is scrolled. Default implementation returns 1
	 * to stop propagation but does not forward the event.
	 *
	 * @param offset Scroll offset vector.
	 * @param flipped Whether the scroll direction is flipped (e.g., on Mac).
	 * @return Event handling result (1 stops propagation).
	 */
	virtual Uint32 onMouseWheel( const Vector2f& offset, bool flipped );

	/**
	 * @brief Calculates drag data for drag operations.
	 *
	 * Called during drag operations to calculate drag data. Default implementation
	 * returns 1 to indicate drag is handled.
	 *
	 * @param position Current drag position.
	 * @param flags Drag state flags.
	 * @return Event handling result (1 stops propagation).
	 */
	virtual Uint32 onCalculateDrag( const Vector2f& position, const Uint32& flags );

	/**
	 * @brief Called when the node is being closed.
	 *
	 * Sends an OnClose event to notify listeners that the node is closing.
	 */
	void onClose();

	/**
	 * @brief Called when visibility changes.
	 *
	 * Sends an OnVisibleChange event and invalidates the node for redraw.
	 * Can be overridden for custom visibility handling.
	 */
	virtual void onVisibilityChange();

	/**
	 * @brief Called when enabled state changes.
	 *
	 * Sends an OnEnabledChange event, invalidates the node, and may clear focus
	 * if the node is disabled. Can be overridden for custom enabled handling.
	 */
	virtual void onEnabledChange();

	/**
	 * @brief Called when position changes.
	 *
	 * Sends an OnPositionChange event and invalidates the node for redraw.
	 * Can be overridden for custom position handling.
	 */
	virtual void onPositionChange();

	/**
	 * @brief Called when size changes.
	 *
	 * Updates origin points, invalidates the node, and sends OnSizeChange event.
	 * Can be overridden for custom size handling.
	 */
	virtual void onSizeChange();

	/**
	 * @brief Called when the parent's size changes.
	 *
	 * Sends an OnParentSizeChange event and invalidates the node. Derived classes
	 * should call base implementation to maintain this behavior.
	 *
	 * @param SizeChange The size change delta.
	 */
	virtual void onParentSizeChange( const Vector2f& SizeChange );

	/**
	 * @brief Called when the parent node changes.
	 *
	 * Invalidates the node to trigger layout updates. Called during re-parenting.
	 */
	virtual void onParentChange();

	/**
	 * @brief Updates the world polygon and bounds cache.
	 *
	 * Recalculates the node's world-space polygon and bounding rectangle,
	 * accounting for all transformations. Clears the dirty flag when complete.
	 */
	void updateWorldPolygon();

	/**
	 * @brief Updates the node's center point in screen space.
	 *
	 * Recalculates the center point based on current screen position and size.
	 * Called when position or size changes.
	 */
	void updateCenter();

	/**
	 * @brief Sets up OpenGL transformation matrices.
	 *
	 * Called before drawing this node. Pushes the current matrix state and
	 * applies scale and rotation transformations. Override to add custom
	 * matrix operations.
	 */
	virtual void matrixSet();

	/**
	 * @brief Restores OpenGL transformation matrices.
	 *
	 * Called after drawing this node. Pops the matrix state undone by matrixSet().
	 * Override to clean up custom matrix operations.
	 */
	virtual void matrixUnset();

	/**
	 * @brief Draws all child nodes in appropriate order.
	 *
	 * Called during the draw cycle. Iterates through children and calls nodeDraw()
	 * on each visible child. The order respects the reverseDraw flag.
	 */
	virtual void drawChildren();

	/**
	 * @brief Called when a child is added or removed.
	 *
	 * Sends an OnChildCountChanged event and invalidates the node. Override to
	 * respond to child list changes.
	 *
	 * @param child The child node that changed.
	 * @param removed True if child was removed, false if added.
	 */
	virtual void onChildCountChange( Node* child, const bool& removed );

	/**
	 * @brief Called when rotation angle changes.
	 *
	 * Sends an OnAngleChange event and invalidates the node. Override for custom
	 * rotation change handling.
	 */
	virtual void onAngleChange();

	/**
	 * @brief Called when scale changes.
	 *
	 * Sends an OnScaleChange event and invalidates the node. Override for custom
	 * scale change handling.
	 */
	virtual void onScaleChange();

	/**
	 * @brief Called when alpha changes.
	 *
	 * Sends an OnAlphaChange event and invalidates the node. Override for custom
	 * alpha change handling.
	 */
	virtual void onAlphaChange();

	/**
	 * @brief Called when the node's scene changes.
	 *
	 * Updates the scene node pointer and propagates the change to all children.
	 * Called when the node is moved to a different scene or when the scene changes.
	 */
	virtual void onSceneChange();

	/**
	 * @brief Called when this node gains input focus.
	 *
	 * Sets the focus flag and sends an OnFocus event. Override to implement
	 * custom focus behavior. Return 0 to allow focus, non-zero to reject.
	 *
	 * @param reason The reason focus was requested.
	 * @return Event handling result.
	 */
	virtual Uint32 onFocus( NodeFocusReason reason );

	/**
	 * @brief Called when this node loses input focus.
	 *
	 * Clears the focus flag and sends an OnFocusLoss event. Override for custom
	 * focus loss handling.
	 *
	 * @return Event handling result.
	 */
	virtual Uint32 onFocusLoss();

	/**
	 * @brief Enables clipping planes for the node's bounds.
	 *
	 * Internal method called during drawing to set up clipping. Uses either
	 * scissor test or clipping planes depending on needsClipPlanes.
	 *
	 * @param x Left edge in pixels.
	 * @param y Top edge in pixels.
	 * @param Width Width in pixels.
	 * @param Height Height in pixels.
	 * @param needsClipPlanes True to use clipping planes instead of scissor.
	 */
	void clipStart( bool needsClipPlanes );

	/**
	 * @brief Disables clipping planes for the node's bounds.
	 *
	 * Internal method called during drawing to restore clipping state.
	 *
	 * @param needsClipPlanes True to disable clipping planes.
	 */
	void clipEnd( bool needsClipPlanes );

	/**
	 * @brief Updates the screen position from the local position.
	 *
	 * Recalculates mScreenPos by converting the local position to world space.
	 * Clears the position dirty flag and sends OnUpdateScreenPosition event.
	 */
	void updateScreenPos();

	/**
	 * @brief Internal size setter without triggering events.
	 *
	 * Sets the size directly and marks polygon dirty. Does not trigger change
	 * notifications. Called by public setSize() methods.
	 *
	 * @param size The new size.
	 */
	virtual void setInternalSize( const Sizef& size );

	/**
	 * @brief Checks if the node should be closed.
	 *
	 * Internal method that checks closure conditions. Currently just calls close()
	 * but exists for potential subclass customization.
	 */
	void checkClose();

	/**
	 * @brief Propagates parent size change to children.
	 *
	 * If reportSizeChangeToChildren() is true, calls onParentSizeChange() on
	 * each direct child.
	 *
	 * @param sizeChange The size change delta.
	 */
	void sendParentSizeChange( const Vector2f& sizeChange );

	/**
	 * @brief Deletes all children recursively.
	 *
	 * Removes and deletes every child node from this node. Used during destruction
	 * or when clearing the node tree.
	 */
	void childDeleteAll();

	/**
	 * @brief Adds a child node to the end of the child list.
	 *
	 * Appends the node to the parent's child list and sets the node's parent pointer.
	 * Called by setParent().
	 *
	 * @param node The child node to add.
	 */
	void childAdd( Node* node );

	/**
	 * @brief Adds a child node at a specific index.
	 *
	 * Inserts the node into the parent's child list at the given position.
	 * Used by setParent() and toPosition().
	 *
	 * @param node The child node to add.
	 * @param index Zero-based position in the child list.
	 */
	void childAddAt( Node* node, Uint32 index );

	/**
	 * @brief Removes a child node without deleting it.
	 *
	 * Detaches the given child from this parent but does not delete it.
	 * Called during re-parenting and node destruction.
	 *
	 * @param node The child node to remove.
	 */
	void childRemove( Node* node );

	/**
	 * @brief Gets the node's bounds in screen coordinates.
	 *
	 * Returns an axis-aligned rectangle representing the node's position and size
	 * in actual screen pixels (untransformed).
	 *
	 * @return Rectf with screen position and pixel size.
	 */
	Rectf getScreenBounds() const;

	/**
	 * @brief Internal position setter without triggering events.
	 *
	 * Sets the position directly and marks the node dirty. Does not trigger change
	 * notifications. Called by public setPosition() method.
	 *
	 * @param Pos The new position in dp.
	 */
	void setInternalPosition( const Vector2f& Pos );

	/**
	 * @brief Internal width setter.
	 *
	 * Sets only the width component of the size. Only changes size if the new
	 * width differs from current.
	 *
	 * @param width The new width.
	 */
	void setInternalWidth( const Float& width );

	/**
	 * @brief Internal height setter.
	 *
	 * Sets only the height component of the size. Only changes size if the new
	 * height differs from current.
	 *
	 * @param height The new height.
	 */
	void setInternalHeight( const Float& height );

	/**
	 * @brief Calculates the color with this node's alpha applied.
	 *
	 * Blends the given color with this node's alpha value, returning a new color
	 * with modified alpha component.
	 *
	 * @param Col The base color.
	 * @return A Color with alpha multiplied by this node's alpha.
	 */
	Color getColor( const Color& Col );

	/**
	 * @brief Finds a node by its precomputed ID hash.
	 *
	 * Depth-first search for a descendant node with matching ID hash.
	 * Returns this node if it matches and is not closing.
	 *
	 * @param idHash The ID hash to search for.
	 * @return Pointer to matching node or nullptr.
	 */
	Node* findIdHash( const String::HashType& idHash ) const;

	/**
	 * @brief Checks if any direct child has the given ID hash.
	 *
	 * Only searches immediate children, not the full subtree.
	 *
	 * @param idHash The ID hash to look for.
	 * @return Pointer to child node or nullptr.
	 */
	Node* hasChildHash( const String::HashType& idHash ) const;

	/**
	 * @brief Updates origin point calculations.
	 *
	 * Recalculates rotation and scale origin points based on size and origin type.
	 * Called when size changes or origin configuration changes.
	 */
	virtual void updateOriginPoint();

	/**
	 * @brief Marks the node and children as dirty.
	 *
	 * Sets both position and polygon dirty flags, which triggers recalculation
	 * of screen positions and world bounds. Propagates to all children.
	 */
	void setDirty();

	/**
	 * @brief Marks all children as dirty.
	 *
	 * Calls setDirty() on each child in the tree. Used when this node becomes
	 * dirty to ensure children recalculate too.
	 */
	void setChildrenDirty();

	/**
	 * @brief Enables clipping planes for specific rectangle.
	 *
	 * Overload that takes pixel coordinates and dimensions. Used during draw cycle.
	 *
	 * @param x Left edge.
	 * @param y Top edge.
	 * @param Width Width.
	 * @param Height Height.
	 * @param needsClipPlanes Whether clipping planes are needed.
	 */
	void clipSmartEnable( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height,
						  bool needsClipPlanes );

	/**
	 * @brief Disables clipping planes.
	 *
	 * Overload that takes only the needsClipPlanes flag.
	 *
	 * @param needsClipPlanes Whether clipping planes are active.
	 */
	void clipSmartDisable( bool needsClipPlanes );

	/**
	 * @brief Finds the nearest draw invalidator in the parent chain.
	 *
	 * Traverses up the parent tree to find a node that reports itself as a
	 * draw invalidator. SceneNode returns true for this.
	 *
	 * @return The nearest draw invalidator node or nullptr.
	 */
	Node* getDrawInvalidator();

	/**
	 * @brief Finds the SceneNode ancestor.
	 *
	 * Traverses up the parent chain looking for a SceneNode. Returns this node
	 * if it is a SceneNode itself.
	 *
	 * @return Pointer to the SceneNode or nullptr.
	 */
	SceneNode* findSceneNode();

	/**
	 * @brief Updates the draw invalidator cache.
	 *
	 * Recalculates and caches which node will handle draw invalidation for
	 * this subtree. Call when parent changes or manually to force update.
	 *
	 * @param force If true, forces update even if already set.
	 */
	void updateDrawInvalidator( bool force = false );

	/**
	 * @brief Subscribes this node to scheduled updates.
	 *
	 * Registers this node with the SceneNode to receive scheduledUpdate() calls
	 * each frame. Sets NODE_FLAG_SCHEDULED_UPDATE flag.
	 */
	void subscribeScheduledUpdate();

	/**
	 * @brief Unsubscribes from scheduled updates.
	 *
	 * Removes this node from the SceneNode's update list. Clears
	 * NODE_FLAG_SCHEDULED_UPDATE flag.
	 */
	void unsubscribeScheduledUpdate();

	/**
	 * @brief Checks if node is subscribed to scheduled updates.
	 *
	 * @return True if NODE_FLAG_SCHEDULED_UPDATE is set.
	 */
	bool isSubscribedForScheduledUpdate();
};

}} // namespace EE::Scene

#endif
