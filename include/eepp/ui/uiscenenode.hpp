#ifndef EE_UISCENENODE_HPP
#define EE_UISCENENODE_HPP

#include <eepp/scene/scenenode.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/system/translator.hpp>
#include <eepp/ui/css/stylesheet.hpp>
#include <eepp/ui/keyboardshortcut.hpp>

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

enum class ColorSchemePreference { Light, Dark };

class EE_API UISceneNode : public SceneNode {
  public:
	static UISceneNode* New( EE::Window::Window* window = NULL );

	explicit UISceneNode( EE::Window::Window* window = NULL );

	virtual ~UISceneNode();

	virtual Node* setSize( const Sizef& size );

	virtual Node* setSize( const Float& Width, const Float& Height );

	UISceneNode* setPixelsSize( const Sizef& size );

	UISceneNode* setPixelsSize( const Float& x, const Float& y );

	const Sizef& getSize() const;

	virtual void update( const Time& elapsed );

	void setTranslator( Translator translator );

	const Translator& getTranslator() const;

	Translator& getTranslator();

	String getTranslatorString( const std::string& str );

	String getTranslatorString( const std::string& str, const String& defaultValue );

	String getTranslatorStringFromKey( const std::string& key, const String& defaultValue );

	String i18n( const std::string& key, const String& defaultValue );

	UIWidget* loadLayoutNodes( pugi::xml_node node, Node* parent, const Uint32& marker );

	UIWidget* loadLayoutFromFile( const std::string& layoutPath, Node* parent = NULL,
								  const Uint32& marker = 0 );

	UIWidget* loadLayoutFromString( const std::string& layoutString, Node* parent = NULL,
									const Uint32& marker = 0 );

	UIWidget* loadLayoutFromString( const char* layoutString, Node* parent = NULL,
									const Uint32& marker = 0 );

	UIWidget* loadLayoutFromMemory( const void* buffer, Int32 bufferSize, Node* parent = NULL,
									const Uint32& marker = 0 );

	UIWidget* loadLayoutFromStream( IOStream& stream, Node* parent = NULL,
									const Uint32& marker = 0 );

	UIWidget* loadLayoutFromPack( Pack* pack, const std::string& FilePackPath,
								  Node* parent = NULL );

	void setStyleSheet( const CSS::StyleSheet& styleSheet );

	void setStyleSheet( const std::string& inlineStyleSheet );

	void combineStyleSheet( const CSS::StyleSheet& styleSheet,
							const bool& forceReloadStyle = true );

	void combineStyleSheet( const std::string& inlineStyleSheet,
							const bool& forceReloadStyle = true, const Uint32& marker = 0 );

	CSS::StyleSheet& getStyleSheet();

	bool hasStyleSheet();

	const bool& isLoading() const;

	UIThemeManager* getUIThemeManager() const;

	UIWidget* getRoot() const;

	void invalidateStyle( UIWidget* widget, bool tryReinsert = false );

	void invalidateStyleState( UIWidget* widget, bool disableCSSAnimations = false, bool tryReinsert = false );

	void invalidateLayout( UILayout* widget );

	void setIsLoading( bool isLoading );

	void updateDirtyLayouts();

	void updateDirtyStyles();

	void updateDirtyStyleStates();

	const bool& isUpdatingLayouts() const;

	UIIconThemeManager* getUIIconThemeManager() const;

	UIIcon* findIcon( const std::string& iconName );

	/** @param drawableSize Size in pixels */
	Drawable* findIconDrawable( const std::string& iconName, const size_t& drawableSize );

	typedef std::function<void()> KeyBindingCommand;

	KeyBindings& getKeyBindings();

	void setKeyBindings( const KeyBindings& keyBindings );

	void addKeyBindingString( const std::string& shortcut, const std::string& command );

	void addKeyBinding( const KeyBindings::Shortcut& shortcut, const std::string& command );

	void replaceKeyBindingString( const std::string& shortcut, const std::string& command );

	void replaceKeyBinding( const KeyBindings::Shortcut& shortcut, const std::string& command );

	void addKeyBindsString( const std::map<std::string, std::string>& binds );

	void addKeyBinds( const std::map<KeyBindings::Shortcut, std::string>& binds );

	void setKeyBindingCommand( const std::string& command, KeyBindingCommand func );

	void executeKeyBindingCommand( const std::string& command );

	UIEventDispatcher* getUIEventDispatcher() const;

	ColorSchemePreference getColorSchemePreference() const;

	void setColorSchemePreference( const ColorSchemePreference& colorSchemePreference );

	const Uint32& getMaxInvalidationDepth() const;

	void setMaxInvalidationDepth( const Uint32& maxInvalidationDepth );

	void nodeToWorldTranslation( Vector2f& Pos ) const;

	void reloadStyle( bool disableAnimations = false, bool forceReApplyProperties = false );

	bool hasThreadPool() const;

	std::shared_ptr<ThreadPool> getThreadPool();

	void setThreadPool( const std::shared_ptr<ThreadPool>& threadPool );

	void setTheme( UITheme* theme );

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

	virtual void resizeNode( EE::Window::Window* win );

	virtual void onDrawDebugDataChange();

	virtual Node* setFocus( NodeFocusReason reason = NodeFocusReason::Unknown);

	virtual void onParentChange();

	void setInternalPixelsSize( const Sizef& size );

	void setActiveWindow( UIWindow* window );

	void setFocusLastWindow( UIWindow* window );

	void windowAdd( UIWindow* win );

	void windowRemove( UIWindow* win );

	bool windowExists( UIWindow* win );

	virtual void setInternalSize( const Sizef& size );

	bool onMediaChanged( bool forceReApplyStyles = false );

	virtual void onChildCountChange( Node* child, const bool& removed );

	virtual void onSizeChange();

	void processStyleSheetAtRules( const CSS::StyleSheet& styleSheet );

	void loadFontFaces( const CSS::StyleSheetStyleVector& styles );

	void loadGlyphIcon( const CSS::StyleSheetStyleVector& styles );

	virtual Uint32 onKeyDown( const KeyEvent& event );

	void onWidgetDelete( Node* node );

	void resetTooltips( Node* node );

	CSS::MediaFeatures getMediaFeatures() const;

	std::vector<UIWidget*> loadNode( pugi::xml_node node, Node* parent, const Uint32& marker );

	void setTheme( UITheme* theme, Node* to );
};

}} // namespace EE::UI

#endif
