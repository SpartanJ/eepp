#ifndef EE_SCENENODE_HPP
#define EE_SCENENODE_HPP

#include <eepp/scene/node.hpp>
#include <eepp/system/translator.hpp>
#include <eepp/window/cursor.hpp>
#include <unordered_set>

namespace EE { namespace Graphics {
class FrameBuffer;
}} // namespace EE::Graphics
using namespace EE::Graphics;

namespace EE { namespace Window {
class Window;
}} // namespace EE::Window

namespace EE { namespace Scene {

class EE_API SceneNode : public Node {
  public:
	/**
	 * @brief Creates a new SceneNode instance.
	 *
	 * This is the factory method for creating SceneNode instances.
	 *
	 * @param window Pointer to the window to associate with this scene node.
	 *               If NULL, uses the current window from Engine.
	 * @return Pointer to the newly created SceneNode instance.
	 */
	static SceneNode* New( EE::Window::Window* window = NULL );

	/**
	 * @brief Constructs a SceneNode with an optional window.
	 *
	 * Creates a SceneNode that can be used as a root node for rendering
	 * and scene management. Optionally specifies which window to use.
	 *
	 * @param window Pointer to the window to associate with this scene node.
	 *               If NULL, uses the current window from Engine.
	 */
	SceneNode( EE::Window::Window* window = NULL );

	/**
	 * @brief Destroys the SceneNode and cleans up resources.
	 *
	 * Deletes associated action manager, event dispatcher, and frame buffer.
	 * Also handles removal from window resize callback and closes children.
	 */
	~SceneNode();

	/**
	 * @brief Enables the use of a frame buffer for off-screen rendering.
	 *
	 * Creates a frame buffer if one doesn't exist. When enabled, rendering
	 * will occur to the frame buffer first before being drawn to the screen.
	 */
	void enableFrameBuffer();

	/**
	 * @brief Disables frame buffer usage and deletes the frame buffer.
	 *
	 * Turns off off-screen rendering and releases the frame buffer resource.
	 */
	void disableFrameBuffer();

	/**
	 * @brief Checks if the node owns its frame buffer.
	 *
	 * Ownership means the scene node created and manages the frame buffer.
	 *
	 * @return True if the frame buffer is owned by this node, false otherwise.
	 */
	bool ownsFrameBuffer() const;

	/**
	 * @brief Draws the scene node and its children.
	 *
	 * This is the main rendering method. Handles frame buffer binding,
	 * matrix transformations, clipping, and drawing children. Also draws
	 * debug visualizations if enabled.
	 */
	virtual void draw();

	/**
	 * @brief Updates the scene node and its children.
	 *
	 * Called each frame to update the node's state, actions, event dispatcher,
	 * and scheduled updates. Manages the update of children based on
	 * mUpdateAllChildren flag.
	 *
	 * @param elapsed The time elapsed since the last update.
	 */
	virtual void update( const Time& elapsed );

	/**
	 * @brief Enables draw invalidation.
	 *
	 * When enabled, the node will use dirty rectangle rendering and only
	 * redraw regions that have been invalidated. This can improve performance.
	 */
	void enableDrawInvalidation();

	/**
	 * @brief Disables draw invalidation.
	 *
	 * When disabled, the entire node will be redrawn every frame.
	 */
	void disableDrawInvalidation();

	/**
	 * @brief Gets the window associated with this scene node.
	 *
	 * @return Pointer to the Window object.
	 */
	EE::Window::Window* getWindow();

	/**
	 * @brief Gets the frame buffer used for off-screen rendering.
	 *
	 * @return Pointer to the FrameBuffer, or NULL if none is set.
	 */
	FrameBuffer* getFrameBuffer() const;

	/**
	 * @brief Sets the event dispatcher for this scene node.
	 *
	 * The event dispatcher handles input and other events. This allows
	 * custom event handling for this scene.
	 *
	 * @param eventDispatcher Pointer to the EventDispatcher to set.
	 */
	void setEventDispatcher( EventDispatcher* eventDispatcher );

	/**
	 * @brief Gets the event dispatcher.
	 *
	 * @return Pointer to the EventDispatcher, or NULL if none set.
	 */
	EventDispatcher* getEventDispatcher() const;

	/**
	 * @brief Enables or disables debug data drawing.
	 *
	 * When enabled, additional debug information may be rendered (like
	 * bounding boxes, invalidation regions, etc.).
	 *
	 * @param debug True to enable debug drawing, false to disable.
	 */
	void setDrawDebugData( bool debug );

	/**
	 * @brief Checks if debug data drawing is enabled.
	 *
	 * @return True if debug drawing is enabled, false otherwise.
	 */
	bool getDrawDebugData() const;

	/**
	 * @brief Enables or disables drawing of bounding boxes.
	 *
	 * When enabled, bounding boxes around nodes may be rendered for
	 * debugging purposes.
	 *
	 * @param draw True to draw boxes, false to hide them.
	 */
	void setDrawBoxes( bool draw );

	/**
	 * @brief Checks if bounding boxes are being drawn.
	 *
	 * @return True if boxes are drawn, false otherwise.
	 */
	bool getDrawBoxes() const;

	/**
	 * @brief Enables or disables highlighting for mouse-over state.
	 *
	 * When enabled and a node is moused over, a highlight effect may be shown.
	 *
	 * @param Highlight True to enable highlight on mouse over, false to disable.
	 */
	void setHighlightOver( bool Highlight );

	/**
	 * @brief Checks if mouse-over highlighting is enabled.
	 *
	 * @return True if highlighting is enabled, false otherwise.
	 */
	bool getHighlightOver() const;

	/**
	 * @brief Enables or disables highlighting for focus state.
	 *
	 * When enabled and a node has focus, a highlight effect may be shown.
	 *
	 * @param Highlight True to enable highlight on focus, false to disable.
	 */
	void setHighlightFocus( bool Highlight );

	/**
	 * @brief Checks if focus highlighting is enabled.
	 *
	 * @return True if focus highlighting is enabled, false otherwise.
	 */
	bool getHighlightFocus() const;

	/**
	 * @brief Enables or disables highlighting for invalidation regions.
	 *
	 * When enabled, areas of the screen that are invalidated (need redrawing)
	 * may be highlighted for debugging.
	 *
	 * @param Highlight True to enable invalidation highlighting, false to disable.
	 */
	void setHighlightInvalidation( bool Highlight );

	/**
	 * @brief Checks if invalidation highlighting is enabled.
	 *
	 * @return True if invalidation highlighting is enabled, false otherwise.
	 */
	bool getHighlightInvalidation() const;

	/**
	 * @brief Sets the color used for mouse-over highlighting.
	 *
	 * @param Color The highlight color to use.
	 */
	void setHighlightOverColor( const Color& Color );

	/**
	 * @brief Gets the mouse-over highlight color.
	 *
	 * @return The highlight color as a const Color reference.
	 */
	const Color& getHighlightOverColor() const;

	/**
	 * @brief Sets the color used for focus highlighting.
	 *
	 * @param Color The highlight color to use.
	 */
	void setHighlightFocusColor( const Color& Color );

	/**
	 * @brief Gets the focus highlight color.
	 *
	 * @return The highlight color as a const Color reference.
	 */
	const Color& getHighlightFocusColor() const;

	/**
	 * @brief Sets the color used for invalidation highlighting.
	 *
	 * @param Color The highlight color to use.
	 */
	void setHighlightInvalidationColor( const Color& Color );

	/**
	 * @brief Gets the invalidation highlight color.
	 *
	 * @return The highlight color as a const Color reference.
	 */
	const Color& getHighlightInvalidationColor() const;

	/**
	 * @brief Gets the elapsed time since the last frame.
	 *
	 * This is the time that was passed to the most recent update() call.
	 *
	 * @return The elapsed time as a const Time reference.
	 */
	const Time& getElapsed() const;

	/**
	 * @brief Checks if draw invalidation is being used.
	 *
	 * When using invalidation, only dirty regions are redrawn.
	 *
	 * @return True if invalidation is enabled, false otherwise.
	 */
	bool usesInvalidation() const;

	/**
	 * @brief Sets whether to use global cursor management.
	 *
	 * When enabled, cursor changes are managed globally through the window's
	 * cursor manager. When disabled, cursor changes may be managed locally.
	 *
	 * @param use True to use global cursors, false for local cursor handling.
	 */
	void setUseGlobalCursors( bool use );

	/**
	 * @brief Gets whether global cursor management is enabled.
	 *
	 * @return Reference to the boolean indicating global cursor usage.
	 */
	bool getUseGlobalCursors();

	/**
	 * @brief Sets the cursor type.
	 *
	 * Changes the cursor appearance if global cursors are enabled.
	 *
	 * @param cursor The cursor type to set (from Cursor::Type).
	 */
	void setCursor( Cursor::Type cursor );

	/**
	 * @brief Checks if this node is a draw invalidator.
	 *
	 * SceneNode always returns true, indicating it supports dirty rectangle
	 * rendering and will invalidate itself when needed.
	 *
	 * @return Always true for SceneNode.
	 */
	virtual bool isDrawInvalidator() const;

	/**
	 * @brief Gets the action manager associated with this scene node.
	 *
	 * The action manager handles timed actions and animations.
	 *
	 * @return Pointer to the ActionManager.
	 */
	ActionManager* getActionManager() const;

	/**
	 * @brief Subscribes a node for scheduled updates.
	 *
	 * The specified node will receive scheduledUpdate() calls each frame.
	 *
	 * @param node Pointer to the node to subscribe.
	 */
	void subscribeScheduledUpdate( Node* node );

	/**
	 * @brief Unsubscribes a node from scheduled updates.
	 *
	 * The node will no longer receive scheduledUpdate() calls.
	 *
	 * @param node Pointer to the node to unsubscribe.
	 */
	void unsubscribeScheduledUpdate( Node* node );

	/**
	 * @brief Checks if a node is subscribed for scheduled updates.
	 *
	 * @param node Pointer to the node to check.
	 * @return True if the node is subscribed, false otherwise.
	 */
	bool isSubscribedForScheduledUpdate( Node* node );

	/**
	 * @brief Adds a node to the mouse-over tracking list.
	 *
	 * This is used internally to track which nodes are currently under
	 * the mouse cursor.
	 *
	 * @param node Pointer to the node to track.
	 */
	void addMouseOverNode( Node* node );

	/**
	 * @brief Removes a node from the mouse-over tracking list.
	 *
	 * @param node Pointer to the node to stop tracking.
	 */
	void removeMouseOverNode( Node* node );

	/**
	 * @brief Gets whether all children are updated each frame.
	 *
	 * When enabled, all children receive update() calls. When disabled,
	 * only specific nodes (like those with scheduled updates) are updated.
	 *
	 * @return Reference to the boolean indicating update-all-children mode.
	 */
	bool getUpdateAllChildren() const;

	/**
	 * @brief Sets whether to update all children each frame.
	 *
	 * Controls whether all children receive update() calls or only
	 * subscribed nodes.
	 *
	 * @param updateAllChildren True to update all children, false to update selectively.
	 */
	void setUpdateAllChildren( bool updateAllChildren );

	/**
	 * @brief Gets the DPI (dots per inch) of the display.
	 *
	 * @return The DPI as a Float.
	 */
	const Float& getDPI() const;

	/**
	 * @brief Checks if verbose logging is enabled.
	 *
	 * When enabled, additional debug messages may be logged.
	 *
	 * @return True if verbose logging is enabled, false otherwise.
	 */
	bool getVerbose() const;

	/**
	 * @brief Enables or disables verbose logging.
	 *
	 * Controls whether detailed debug information is printed to the log.
	 *
	 * @param verbose True to enable verbose logging, false to disable.
	 */
	void setVerbose( bool verbose );

  protected:
	friend class Node;
	typedef std::unordered_set<Node*> CloseList;

	EE::Window::Window* mWindow;
	ActionManager* mActionManager;
	FrameBuffer* mFrameBuffer;
	EventDispatcher* mEventDispatcher;
	CloseList mCloseList;
	Clock mClock;
	bool mFrameBufferBound;
	bool mUseInvalidation;
	bool mUseGlobalCursors;
	bool mUpdateAllChildren;
	Int32 mResizeCb;
	bool mDrawDebugData;
	bool mDrawBoxes;
	bool mHighlightOver;
	bool mHighlightFocus;
	bool mHighlightInvalidation;
	bool mFirstUpdate{ true };
	bool mFirstFrame{ true };
	bool mVerbose{ false };
	Color mHighlightFocusColor;
	Color mHighlightOverColor;
	Color mHighlightInvalidationColor;
	Time mElapsed;
	UnorderedSet<Node*> mScheduledUpdate;
	UnorderedSet<Node*> mScheduledUpdateRemove;
	UnorderedSet<Node*> mMouseOverNodes;
	Float mDPI;

	virtual void onSizeChange();

	virtual void matrixSet();

	virtual void matrixUnset();

	virtual void preDraw();

	virtual void postDraw();

	virtual void onDrawDebugDataChange();

	void sendMsg( Node* node, const Uint32& msg, const Uint32& flags = 0 );

	virtual void resizeNode( EE::Window::Window* win );

	void addToCloseQueue( Node* node );

	bool removeFromCloseQueue( Node* node );

	void checkClose();

	void createFrameBuffer();

	void drawFrameBuffer();

	Sizei getFrameBufferSize();
};

}} // namespace EE::Scene

#endif
