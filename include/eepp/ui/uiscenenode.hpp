#ifndef EE_UISCENENODE_HPP
#define EE_UISCENENODE_HPP

#include <eepp/network/cookiemanager.hpp>
#include <eepp/network/uri.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/system/translator.hpp>
#include <eepp/ui/colorschemepreferences.hpp>
#include <eepp/ui/css/stylesheet.hpp>
#include <eepp/ui/keyboardshortcut.hpp>

using namespace EE::Network;

namespace EE { namespace Graphics {
class Font;
}} // namespace EE::Graphics

namespace EE { namespace UI {

class UITheme;
class UIThemeManager;
class UIIconThemeManager;
class UIEventDispatcher;
class UIWidget;
class UIWindow;
class UIWidget;
class UILayout;
class UIIcon;

struct NavigationRequest {
	URI uri;
	std::string method{ "GET" };
	std::string body;
	std::map<std::string, std::string> extraHeaders;
};

class EE_API UISceneNode : public SceneNode {
  public:
	/**
	 * @brief Creates a new UISceneNode instance.
	 *
	 * This is the factory method for creating UISceneNode instances.
	 *
	 * @param window Pointer to the window to associate with this UI scene node.
	 *               If NULL, uses the current window from Engine.
	 * @return Pointer to the newly created UISceneNode instance.
	 */
	static UISceneNode* New( EE::Window::Window* window = NULL );

	/**
	 * @brief Destroys the UISceneNode and cleans up resources.
	 *
	 * Deletes theme managers, icon theme manager, font faces, and event dispatcher.
	 * Also calls childDeleteAll() to ensure proper cleanup order (before thread pool).
	 */
	virtual ~UISceneNode();

	/**
	 * @brief Sets the size in density-independent pixels (dp).
	 *
	 * Override of SceneNode::setSize to also update dp size and trigger media changes.
	 *
	 * @param size The new size in dp.
	 * @return Pointer to this node for method chaining.
	 */
	virtual Node* setSize( const Sizef& size );

	/**
	 * @brief Sets the size in density-independent pixels (dp).
	 *
	 * Override of SceneNode::setSize with individual dimensions.
	 *
	 * @param Width The width in dp.
	 * @param Height The height in dp.
	 * @return Pointer to this node for method chaining.
	 */
	virtual Node* setSize( const Float& Width, const Float& Height );

	/**
	 * @brief Sets the size in actual screen pixels.
	 *
	 * Sets the pixel size directly and updates the dp size accordingly.
	 *
	 * @param size The new size in pixels.
	 * @return Pointer to this node for method chaining.
	 */
	UISceneNode* setPixelsSize( const Sizef& size );

	/**
	 * @brief Sets the size in actual screen pixels.
	 *
	 * Sets the pixel size using individual dimensions.
	 *
	 * @param x The width in pixels.
	 * @param y The height in pixels.
	 * @return Pointer to this node for method chaining.
	 */
	UISceneNode* setPixelsSize( const Float& x, const Float& y );

	/**
	 * @brief Gets the size in density-independent pixels (dp).
	 *
	 * @return The size as a const Sizef reference in dp.
	 */
	const Sizef& getSize() const;

	/**
	 * @brief Updates the UISceneNode.
	 *
	 * Override that adds UI-specific update logic including:
	 * - Updating dirty styles, style states, and layouts
	 * - Handling multiple invalidation passes if needed
	 * - Processing scheduled updates
	 *
	 * @param elapsed The time elapsed since the last update.
	 */
	virtual void update( const Time& elapsed );

	/**
	 * @brief Sets the translator for internationalization.
	 *
	 * The translator is used to translate strings throughout the UI.
	 *
	 * @param translator The Translator object to set.
	 */
	void setTranslator( Translator translator );

	/**
	 * @brief Sets the translator for internationalization.
	 *
	 * The translator is used to translate strings throughout the UI.
	 *
	 * @param translator The Translator object to set.
	 */
	void setTranslator( Translator&& translator );

	/**
	 * @brief Gets the translator as a const reference.
	 *
	 * @return The const Translator reference.
	 */
	const Translator& getTranslator() const;

	/**
	 * @brief Gets the translator as a non-const reference.
	 *
	 * @return The Translator reference for modification.
	 */
	Translator& getTranslator();

	/**
	 * @brief Gets a translated string.
	 *
	 * Translates the given string using the translator. Supports special
	 * @string syntax for lookups and function-style expressions.
	 *
	 * @param str The string to translate.
	 * @return The translated string, or the original if no translation found.
	 */
	String getTranslatorString( const std::string& str );

	/**
	 * @brief Gets a translated string with default value.
	 *
	 * Similar to getTranslatorString() but returns defaultValue if translation
	 * is not found.
	 *
	 * @param str The string to translate.
	 * @param defaultValue The default value to use if translation fails.
	 * @return The translated string or default value.
	 */
	String getTranslatorString( const std::string& str, const String& defaultValue );

	/**
	 * @brief Gets a translated string from a translation key.
	 *
	 * Looks up the translation using the specified key.
	 *
	 * @param key The translation key to look up.
	 * @param defaultValue The default value if key not found.
	 * @return The translated string or default value.
	 */
	String getTranslatorStringFromKey( const std::string& key, const String& defaultValue );

	/**
	 * @brief Translates a string (internationalization shorthand).
	 *
	 * Convenience method equivalent to getTranslatorStringFromKey().
	 *
	 * @param key The translation key.
	 * @param defaultValue The default value if translation not found.
	 * @return The translated string or default value.
	 */
	String i18n( const std::string& key, const String& defaultValue );

	/**
	 * @brief Loads UI layout from an XML node.
	 *
	 * Parses the XML node and creates UIWidgets accordingly.
	 *
	 * @param node The XML node to load from.
	 * @param parent The parent node to attach widgets to (default: this).
	 * @param marker A marker value to associate with loaded styles.
	 * @return The root UIWidget created, or NULL if none.
	 */
	UIWidget* loadLayoutNodes( pugi::xml_node node, Node* parent, const Uint32& marker );

	/**
	 * @brief Loads a UI layout from a file.
	 *
	 * Parses an XML layout file and creates the UI hierarchy.
	 *
	 * @param layoutPath Path to the layout file.
	 * @param parent Parent node for the layout (default: this).
	 * @param marker Marker for style association.
	 * @return The root widget, or NULL if loading failed.
	 */
	UIWidget* loadLayoutFromFile( const std::string& layoutPath, Node* parent = NULL,
								  const Uint32& marker = 0 );

	/**
	 * @brief Loads a UI layout from a string.
	 *
	 * Parses an XML string and creates the UI hierarchy.
	 *
	 * @param layoutString The XML layout string.
	 * @param parent Parent node for the layout (default: this).
	 * @param marker Marker for style association.
	 * @return The root widget, or NULL if parsing failed.
	 */
	UIWidget* loadLayoutFromString( const std::string& layoutString, Node* parent = NULL,
									const Uint32& marker = 0 );

	/**
	 * @brief Loads a UI layout from a C string.
	 *
	 * Parses an XML C-string and creates the UI hierarchy.
	 *
	 * @param layoutString The XML layout C-string.
	 * @param parent Parent node for the layout (default: this).
	 * @param marker Marker for style association.
	 * @return The root widget, or NULL if parsing failed.
	 */
	UIWidget* loadLayoutFromString( const char* layoutString, Node* parent = NULL,
									const Uint32& marker = 0 );

	/**
	 * @brief Loads a UI layout from a memory buffer.
	 *
	 * Parses XML from a memory buffer and creates the UI hierarchy.
	 *
	 * @param buffer Pointer to the XML data in memory.
	 * @param bufferSize Size of the buffer in bytes.
	 * @param parent Parent node for the layout (default: this).
	 * @param marker Marker for style association.
	 * @return The root widget, or NULL if parsing failed.
	 */
	UIWidget* loadLayoutFromMemory( const void* buffer, Int32 bufferSize, Node* parent = NULL,
									const Uint32& marker = 0 );

	/**
	 * @brief Loads a UI layout from an I/O stream.
	 *
	 * Reads XML from an IOStream and creates the UI hierarchy.
	 *
	 * @param stream The input stream to read from.
	 * @param parent Parent node for the layout (default: this).
	 * @param marker Marker for style association.
	 * @return The root widget, or NULL if reading or parsing failed.
	 */
	UIWidget* loadLayoutFromStream( IOStream& stream, Node* parent = NULL,
									const Uint32& marker = 0 );

	/**
	 * @brief Loads a UI layout from a pack file.
	 *
	 * Extracts XML from a pack (archive) and creates the UI hierarchy.
	 *
	 * @param pack Pointer to the Pack to read from.
	 * @param FilePackPath Path within the pack to the layout file.
	 * @param parent Parent node for the layout (default: this).
	 * @return The root widget, or NULL if extraction or parsing failed.
	 */
	UIWidget* loadLayoutFromPack( Pack* pack, const std::string& FilePackPath,
								  Node* parent = NULL );

	/**
	 * @brief Sets the stylesheet for this UISceneNode.
	 *
	 * Replaces the current stylesheet with a new one and optionally loads
	 * the styles immediately.
	 *
	 * @param styleSheet The CSS StyleSheet to set.
	 * @param loadStyle If true, applies the styles immediately (default: true).
	 */
	void setStyleSheet( const CSS::StyleSheet& styleSheet, bool loadStyle = true );

	/**
	 * @brief Sets the stylesheet from an inline CSS string.
	 *
	 * Parses the CSS string and sets it as the stylesheet.
	 *
	 * @param inlineStyleSheet The CSS stylesheet as a string.
	 */
	void setStyleSheet( const std::string& inlineStyleSheet );

	/**
	 * @brief Combines a stylesheet with the existing one.
	 *
	 * Merges the given stylesheet into the current stylesheet.
	 *
	 * @param styleSheet The CSS StyleSheet to combine.
	 * @param forceReloadStyle If true, forces immediate style reload (default: true).
	 * @param baseURI If the resource was loaded from an URI, pass the URI in order to solve
	 * relative paths in CSS
	 */
	void combineStyleSheet( const CSS::StyleSheet& styleSheet, bool forceReloadStyle = true,
							URI baseURI = {} );

	/**
	 * @brief Combines an inline stylesheet with the existing one.
	 *
	 * Parses the CSS string and merges it with the current stylesheet.
	 *
	 * @param inlineStyleSheet The CSS stylesheet as a string.
	 * @param forceReloadStyle If true, forces immediate style reload (default: true).
	 * @param marker Marker to associate with the new styles.
	 * @param baseURI If the resource was loaded from an URI, pass the URI in order to solve
	 * relative paths in CSS
	 */
	void combineStyleSheet( const std::string& inlineStyleSheet, bool forceReloadStyle = true,
							const Uint32& marker = 0, URI baseURI = {} );

	/**
	 * @brief Gets the reference to the current stylesheet.
	 *
	 * @return Reference to the CSS StyleSheet.
	 */
	CSS::StyleSheet& getStyleSheet();

	/**
	 * @brief Checks if a stylesheet is set.
	 *
	 * @return True if a non-empty stylesheet exists, false otherwise.
	 */
	bool hasStyleSheet();

	/**
	 * @brief Checks if the UISceneNode is currently loading.
	 *
	 * This flag is set during layout loading operations.
	 *
	 * @return Const reference to the loading state boolean.
	 */
	bool isLoading() const;

	/**
	 * @brief Gets the UIThemeManager.
	 *
	 * The theme manager is responsible for loading and providing UI themes.
	 *
	 * @return Pointer to the UIThemeManager.
	 */
	UIThemeManager* getUIThemeManager() const;

	/**
	 * @brief Gets the root widget of this UISceneNode.
	 *
	 * The root is a UIRoot widget that contains all other UI widgets.
	 *
	 * @return Pointer to the root UIWidget.
	 */
	UIWidget* getRoot() const;

	/**
	 * @brief Invalidates the style of a widget.
	 *
	 * Marks the widget's style as needing to be reloaded. The widget will
	 * have its CSS re-applied during the next update cycle.
	 *
	 * @param widget Pointer to the UIWidget to invalidate.
	 * @param tryReinsert If true, attempts to reposition the widget in the dirty set.
	 */
	void invalidateStyle( UIWidget* widget, bool tryReinsert = false );

	/**
	 * @brief Invalidates the style state of a widget.
	 *
	 * Marks the widget's style state (pseudo-classes) as needing to be
	 * re-evaluated and re-applied.
	 *
	 * @param widget Pointer to the UIWidget to invalidate.
	 * @param disableCSSAnimations If true, disables CSS animations during the update.
	 * @param tryReinsert If true, attempts to reposition the widget in the dirty set.
	 */
	void invalidateStyleState( UIWidget* widget, bool disableCSSAnimations = false,
							   bool tryReinsert = false );

	/**
	 * @brief Invalidates the layout of a UILayout widget.
	 *
	 * Marks the layout as needing to be recalculated. The layout will be
	 * updated during the next update cycle.
	 *
	 * @param widget Pointer to the UILayout to invalidate.
	 */
	void invalidateLayout( UILayout* widget );

	/**
	 * @brief Sets the loading state flag.
	 *
	 * This is typically managed internally but can be set manually if needed.
	 *
	 * @param isLoading The loading state to set.
	 */
	void setIsLoading( bool isLoading );

	/**
	 * @brief Updates all dirty layouts.
	 *
	 * Processes the mDirtyLayouts set and calls updateLayoutTree() on each
	 * layout that needs recalculation.
	 */
	void updateDirtyLayouts();

	/**
	 * @brief Updates all dirty styles.
	 *
	 * Processes the mDirtyStyle set and calls reloadStyle() on each widget
	 * that needs its CSS style re-applied.
	 */
	void updateDirtyStyles();

	/**
	 * @brief Updates all dirty style states.
	 *
	 * Processes the mDirtyStyleState set and calls
	 * reportStyleStateChangeRecursive() on each widget that needs its
	 * pseudo-class state re-evaluated.
	 */
	void updateDirtyStyleStates();

	/**
	 * @brief Checks if dirty layouts are currently being updated.
	 *
	 * Useful to avoid re-entrancy or to check if layout updates are in progress.
	 *
	 * @return Const reference to the boolean indicating layout update status.
	 */
	bool isUpdatingLayouts() const;

	/**
	 * @brief Gets the UIIconThemeManager.
	 *
	 * The icon theme manager handles lookups of icon drawables by name.
	 *
	 * @return Pointer to the UIIconThemeManager.
	 */
	UIIconThemeManager* getUIIconThemeManager() const;

	/**
	 * @brief Finds an icon by name.
	 *
	 * Searches the icon theme manager for an icon with the specified name.
	 *
	 * @param iconName The name of the icon to find.
	 * @return Pointer to the UIIcon, or nullptr if not found.
	 */
	UIIcon* findIcon( const std::string& iconName );

	/**
	 * @brief Finds an icon drawable by name and size.
	 *
	 * Convenience method that gets an icon and then retrieves a drawable
	 * of the specified size.
	 *
	 * @param iconName The name of the icon to find.
	 * @param drawableSize The desired size of the drawable in pixels.
	 * @return Pointer to the Drawable, or nullptr if not found.
	 */
	Drawable* findIconDrawable( const std::string& iconName, const size_t& drawableSize );

	/**
	 * @brief Gets the keybindings manager.
	 *
	 * The keybindings system maps keyboard shortcuts to commands.
	 *
	 * @return Reference to the KeyBindings object.
	 */
	KeyBindings& getKeyBindings();

	/**
	 * @brief Sets the keybindings.
	 *
	 * Replaces the current keybindings with a new set.
	 *
	 * @param keyBindings The KeyBindings object to set.
	 */
	void setKeyBindings( const KeyBindings& keyBindings );

	/**
	 * @brief Adds a keybinding from string shortcut to command.
	 *
	 * Shortcut format is typically like "Ctrl+S" or "Alt+Enter".
	 *
	 * @param shortcut The string representation of the shortcut.
	 * @param command The command to execute when shortcut is pressed.
	 */
	void addKeyBindingString( const std::string& shortcut, const std::string& command );

	/**
	 * @brief Adds a keybinding from Shortcut to command.
	 *
	 * @param shortcut The KeyBindings::Shortcut structure.
	 * @param command The command to execute when shortcut is pressed.
	 */
	void addKeyBinding( const KeyBindings::Shortcut& shortcut, const std::string& command );

	/**
	 * @brief Replaces a keybinding using string shortcut.
	 *
	 * If the shortcut already exists, it is replaced; otherwise it is added.
	 *
	 * @param shortcut The string representation of the shortcut.
	 * @param command The command to execute.
	 */
	void replaceKeyBindingString( const std::string& shortcut, const std::string& command );

	/**
	 * @brief Replaces a keybinding using Shortcut.
	 *
	 * If the shortcut already exists, it is replaced; otherwise it is added.
	 *
	 * @param shortcut The KeyBindings::Shortcut structure.
	 * @param command The command to execute.
	 */
	void replaceKeyBinding( const KeyBindings::Shortcut& shortcut, const std::string& command );

	/**
	 * @brief Adds multiple keybindings from string map.
	 *
	 * @param binds Map of shortcut strings to command strings.
	 */
	void addKeyBindsString( const std::map<std::string, std::string>& binds );

	/**
	 * @brief Adds multiple keybindings from Shortcut map.
	 *
	 * @param binds Map of KeyBindings::Shortcut to command strings.
	 */
	void addKeyBinds( const std::map<KeyBindings::Shortcut, std::string>& binds );

	typedef std::function<void()> KeyBindingCommand;

	/**
	 * @brief Sets a function to execute for a command.
	 *
	 * Associates a command string with a callable function. This allows
	 * keybindings to trigger custom code.
	 *
	 * @param command The command string.
	 * @param func The function to call when the command is executed.
	 */
	void setKeyBindingCommand( const std::string& command, KeyBindingCommand func );

	/**
	 * @brief Executes a keybinding command.
	 *
	 * Triggers the function associated with the given command string.
	 *
	 * @param command The command string to execute.
	 */
	void executeKeyBindingCommand( const std::string& command );

	/**
	 * @brief Gets the UI-specific event dispatcher.
	 *
	 * Casts the generic event dispatcher to a UIEventDispatcher.
	 *
	 * @return Pointer to the UIEventDispatcher, or nullptr if not set.
	 */
	UIEventDispatcher* getUIEventDispatcher() const;

	/**
	 * @brief Gets the current color scheme preference.
	 *
	 * @return The ColorSchemePreference (Light or Dark).
	 */
	ColorSchemePreference getColorSchemePreference() const;

	/**
	 * @brief Sets the color scheme preference from extended preference.
	 *
	 * Converts extended preference (Light/Dark/System) to standard preference.
	 * For System, detects OS preference automatically.
	 *
	 * @param colorSchemePreference The extended ColorSchemeExtPreference.
	 */
	void setColorSchemePreference( const ColorSchemeExtPreference& colorSchemePreference );

	/**
	 * @brief Sets the color scheme preference directly.
	 *
	 * Controls whether the UI uses light or dark color scheme by default.
	 *
	 * @param colorSchemePreference The ColorSchemePreference.
	 */
	void setColorSchemePreference( const ColorSchemePreference& colorSchemePreference );

	/**
	 * @brief Gets the maximum invalidation depth.
	 *
	 * This controls how many times the update cycle will re-process dirty
	 * states to ensure all cascading style/layout changes are applied.
	 *
	 * @return Const reference to max invalidation depth.
	 */
	const Uint32& getMaxInvalidationDepth() const;

	/**
	 * @brief Sets the maximum invalidation depth.
	 *
	 * @param maxInvalidationDepth The maximum number of invalidation passes.
	 */
	void setMaxInvalidationDepth( const Uint32& maxInvalidationDepth );

	/**
	 * @brief Transforms node-local coordinates to world coordinates.
	 *
	 * Override that uses dp (density-independent pixels) positions.
	 *
	 * @param Pos Reference to the position to transform (modified in place).
	 */
	void nodeToWorldTranslation( Vector2f& Pos ) const;

	/**
	 * @brief Reloads the UI styles.
	 *
	 * Forces all widgets to re-apply their CSS styles, optionally disabling
	 * animations, forcing re-application, or resetting property caches.
	 *
	 * @param disableAnimations If true, CSS animations are disabled during reload.
	 * @param forceReApplyProperties If true, all properties are re-applied even if unchanged.
	 * @param resetPropertiesCache If true, property cache is cleared.
	 */
	void reloadStyle( bool disableAnimations = false, bool forceReApplyProperties = false,
					  bool resetPropertiesCache = false );

	/**
	 * @brief Checks if a thread pool is available.
	 *
	 * @return True if a thread pool has been set, false otherwise.
	 */
	bool hasThreadPool() const;

	/**
	 * @brief Gets the thread pool.
	 *
	 * @return Shared pointer to the ThreadPool, or nullptr if none set.
	 */
	std::shared_ptr<ThreadPool> getThreadPool();

	/**
	 * @brief Sets the thread pool for background tasks.
	 *
	 * @param threadPool Shared pointer to the ThreadPool to use.
	 */
	void setThreadPool( const std::shared_ptr<ThreadPool>& threadPool );

	/**
	 * @brief Sets the theme for the entire UI scene.
	 *
	 * Applies the theme to the root widget and all children.
	 *
	 * @param theme Pointer to the UITheme to set.
	 */
	void setTheme( UITheme* theme );

	/**
	 * @brief Gets the current media features.
	 *
	 * Returns information about the current media environment (screen size,
	 * resolution, color scheme, etc.) for CSS media queries.
	 *
	 * @return CSS::MediaFeatures structure with current media values.
	 */
	CSS::MediaFeatures getMediaFeatures() const;

	/**
	 * @brief Loads UI nodes from XML.
	 *
	 * Core method that parses XML and creates widget hierarchy.
	 *
	 * @param node The XML node to parse.
	 * @param parent The parent to attach widgets to.
	 * @param marker Marker for style association.
	 * @return Vector of root widgets created.
	 */
	std::vector<UIWidget*> loadNode( pugi::xml_node node, Node* parent, const Uint32& marker = 0 );

	/** Sets the document / scene URI used to resolve paths of inner elements */
	void setURI( const URI& uri );

	/** Sets the document / scene URI used to resolve paths from a complete URI (with
	 * path+query+fragment+etc) */
	void setURIFromURL( const URI& url );

	/** @return the document / scene URI used to resolve paths of inner elements */
	const URI& getURI() const { return mURI; }

	/** Handles opening an specific URI */
	void openURL( URI uri );

	/** Handles navigation (GET/POST) with request body and custom headers. */
	void navigate( const NavigationRequest& request );

	/** Sets a callback to intercept navigate() calls. Return true to handle the request,
	 * false to fall through to the URL interceptor and default handling. */
	void setNavigationInterceptorCb( std::function<bool( const NavigationRequest& request )> cb ) {
		mNavigationInterceptorCb = cb;
	};

	/**
	 * Solves a relative path with no scheme or authority into a complete URI.
	 * @param baseURI If must solve from a specific baseURI it must be passed here.
	 */
	URI solveRelativePath( URI uri, URI baseURI = {} );

	/** @return The document referer */
	URI getReferer() const { return mReferer; };

	const Network::CookieManager& getCookieManager() const { return mCookieManager; }

	Network::CookieManager& getCookieManager() { return mCookieManager; }

	Font* getFontFromNamesList( std::string_view names ) const;

  protected:
	friend class EE::UI::UIWindow;
	friend class EE::UI::UIWidget;

	UIWidget* mRoot{ nullptr };
	Sizef mDpSize;
	Uint32 mFlags;
	Translator mTranslator;
	std::vector<UIWindow*> mWindowsList;
	CSS::StyleSheet mStyleSheet;
	bool mIsLoading{ false };
	bool mUpdatingLayouts{ false };
	UIThemeManager* mUIThemeManager{ nullptr };
	UIIconThemeManager* mUIIconThemeManager{ nullptr };
	std::vector<Font*> mFontFaces;
	KeyBindings mKeyBindings;
	std::map<std::string, KeyBindingCommand> mKeyBindingCommands;
	UnorderedSet<UIWidget*> mDirtyStyle;
	UnorderedSet<UIWidget*> mDirtyStyleState;
	UnorderedMap<UIWidget*, bool> mDirtyStyleStateCSSAnimations;
	UnorderedSet<UILayout*> mDirtyLayouts;
	std::vector<std::pair<Float, std::string>> mTimes;
	ColorSchemePreference mColorSchemePreference{ ColorSchemePreference::Dark };
	Uint32 mMaxInvalidationDepth{ 2 };
	Node* mCurParent{ nullptr };
	Uint32 mCurOnSizeChangeListener{ 0 };
	std::shared_ptr<ThreadPool> mThreadPool;
	URI mURI;
	URI mReferer;
	std::function<bool( const NavigationRequest& request )> mNavigationInterceptorCb;
	Network::CookieManager mCookieManager;

	/**
	 * @brief Protected constructor.
	 *
	 * Creates a UISceneNode with optional window association.
	 *
	 * @param window Pointer to the window, or NULL for default.
	 */
	explicit UISceneNode( EE::Window::Window* window = NULL );

	/**
	 * @brief Handles node resize.
	 *
	 * Called when the window resizes. Updates the node size accordingly.
	 *
	 * @param win Pointer to the window that was resized.
	 */
	virtual void resizeNode( EE::Window::Window* win );

	/**
	 * @brief Called when debug data drawing setting changes.
	 *
	 * Override to respond to changes in drawDebugData flag.
	 */
	virtual void onDrawDebugDataChange();

	/**
	 * @brief Requests focus for this scene node.
	 *
	 * Delegates to the event dispatcher to set focus on the root widget.
	 *
	 * @param reason The reason for the focus request.
	 * @return Pointer to this node, or nullptr if focus failed.
	 */
	virtual Node* setFocus( NodeFocusReason reason = NodeFocusReason::Unknown );

	/**
	 * @brief Called when this node's parent changes.
	 *
	 * Handles event dispatcher updates and size propagation.
	 */
	virtual void onParentChange();

	/**
	 * @brief Sets the internal pixel size without triggering update cycles.
	 *
	 * Used internally for size adjustments that shouldn't generate events.
	 *
	 * @param size The new size in pixels.
	 */
	void setInternalPixelsSize( const Sizef& size );

	/**
	 * @brief Sets the active window for this scene.
	 *
	 * @param window Pointer to the UIWindow to set as active.
	 */
	void setActiveWindow( UIWindow* window );

	/**
	 * @brief Manages focus when a window loses focus.
	 *
	 * Ensures proper focus restoration when switching windows.
	 *
	 * @param window The window that was focused.
	 */
	void setFocusLastWindow( UIWindow* window );

	/**
	 * @brief Adds a window to the scene's window list.
	 *
	 * @param win Pointer to the UIWindow to add.
	 */
	void windowAdd( UIWindow* win );

	/**
	 * @brief Removes a window from the scene's window list.
	 *
	 * @param win Pointer to the UIWindow to remove.
	 */
	void windowRemove( UIWindow* win );

	/**
	 * @brief Checks if a window exists in the scene's window list.
	 *
	 * @param win Pointer to the UIWindow to check.
	 * @return True if the window is in the list, false otherwise.
	 */
	bool windowExists( UIWindow* win );

	/**
	 * @brief Sets the internal size in dp.
	 *
	 * Called internally when the size needs to be set in dp units.
	 * Updates the dp size and triggers size change notifications.
	 *
	 * @param size The new size in dp.
	 */
	virtual void setInternalSize( const Sizef& size );

	/**
	 * @brief Handles media query changes.
	 *
	 * Called when media features might have changed (like DPI or size).
	 * Updates stylesheet media queries and triggers style re-evaluation if needed.
	 *
	 * @param forceReApplyStyles If true, forces all styles to be re-applied.
	 * @return True if media queries changed and styles were invalidated.
	 */
	bool onMediaChanged( bool forceReApplyStyles = false );

	/**
	 * @brief Called when a child is added or removed.
	 *
	 * Override to handle child count changes. Automatically re-parents
	 * non-root children to the root widget.
	 *
	 * @param child The child node that changed.
	 * @param removed True if removed, false if added.
	 */
	virtual void onChildCountChange( Node* child, const bool& removed );

	/**
	 * @brief Called when the node's size changes.
	 *
	 * Updates the root widget's pixel size and triggers size change events.
	 */
	virtual void onSizeChange();

	/**
	 * @brief Processes @font-face and @glyph-icon rules in a stylesheet.
	 *
	 * Extracts font face definitions and glyph icon definitions from
	 * the stylesheet's at-rules and loads them.
	 *
	 * @param styleSheet The stylesheet to process.
	 * @param baseURI If the resource was loaded from an URI, pass the URI in order to solve
	 * relative paths in CSS
	 */
	void processStyleSheetAtRules( const CSS::StyleSheet& styleSheet, URI baseURI = {} );

	/** Resolves relative URLs in all CSS property values against the stylesheet's base URI. */
	void resolveStyleSheetRelativeURLs( CSS::StyleSheet& styleSheet, URI baseURI = {} );

	/**
	 * @brief Loads font faces from @font-face rules.
	 *
	 * Parses font face styles and loads the fonts from various sources
	 * (files, URLs, VFS).
	 *
	 * @param styles Vector of stylesheet styles from @font-face rules.
	 * @param baseURI If the resource was loaded from an URI, pass the URI in order to solve
	 * relative paths in CSS
	 */
	void loadFontFaces( const CSS::StyleSheetStyleVector& styles, URI baseURI = {} );

	/**
	 * @brief Loads CSS files from URI
	 *
	 * Parses CSS and loads the CSS from various sources
	 * (files, URLs, VFS).
	 *
	 * @param uri URI to load
	 */
	void loadCSS( URI uri );

	/**
	 * @brief Loads glyph icons from @glyph-icon rules.
	 *
	 * Parses glyph icon definitions and registers them with the icon theme manager.
	 *
	 * @param styles Vector of stylesheet styles from @glyph-icon rules.
	 */
	void loadGlyphIcon( const CSS::StyleSheetStyleVector& styles );

	/**
	 * @brief Handles key down events.
	 *
	 * Checks key bindings and executes associated commands. Override for
	 * custom keyboard handling.
	 *
	 * @param event The key event.
	 * @return 0 if the event was handled by a binding, otherwise base class return.
	 */
	virtual Uint32 onKeyDown( const KeyEvent& event );

	/**
	 * @brief Called when a node is deleted.
	 *
	 * Cleans up dirty state tracking for the deleted node if it's a widget.
	 *
	 * @param node The node that was deleted.
	 */
	void onWidgetDelete( Node* node );

	/**
	 * @brief Recursively resets tooltips for a node and its children.
	 *
	 * Used when debug mode is turned off to hide all tooltips.
	 *
	 * @param node The node to reset tooltips for.
	 */
	void resetTooltips( Node* node );

	/**
	 * @brief Applies a theme to a node and its subtree.
	 *
	 * Recursively applies the specified UITheme to all widgets in the subtree.
	 *
	 * @param theme Pointer to the UITheme to apply.
	 * @param to The root node of the subtree to theme.
	 */
	void setTheme( UITheme* theme, Node* to );

	/** @return The document / scene URI used to resolve paths from a complete URI (with
	 * path+query+fragment+etc) */
	URI getURIFromURL( const URI& url ) const;
};

}} // namespace EE::UI

#endif
