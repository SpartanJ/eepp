#ifndef EE_UISCENENODE_HPP
#define EE_UISCENENODE_HPP

#include <eepp/scene/scenenode.hpp>
#include <eepp/system/translator.hpp>
#include <eepp/ui/css/stylesheet.hpp>
#include <eepp/ui/keyboardshortcut.hpp>
#include <list>

namespace EE { namespace Graphics {
class Font;
}} // namespace EE::Graphics

namespace EE { namespace UI {

class UIThemeManager;
class UIIconThemeManager;
class UIWidget;
class UIWindow;
class UIWidget;
class UILayout;

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

	Translator& getTranslator();

	String getTranslatorString( const std::string& str );

	String getTranslatorString( const std::string& str, const String& defaultValue );

	UIWidget* loadLayoutFromFile( const std::string& layoutPath, Node* parent = NULL );

	UIWidget* loadLayoutFromString( const std::string& layoutString, Node* parent = NULL );

	UIWidget* loadLayoutFromMemory( const void* buffer, Int32 bufferSize, Node* parent = NULL );

	UIWidget* loadLayoutFromStream( IOStream& stream, Node* parent = NULL );

	UIWidget* loadLayoutFromPack( Pack* pack, const std::string& FilePackPath,
								  Node* parent = NULL );

	UIWidget* loadLayoutNodes( pugi::xml_node node, Node* parent );

	void setStyleSheet( const CSS::StyleSheet& styleSheet );

	void setStyleSheet( const std::string& inlineStyleSheet );

	void combineStyleSheet( const CSS::StyleSheet& styleSheet,
							const bool& forceReloadStyle = true );

	void combineStyleSheet( const std::string& inlineStyleSheet,
							const bool& forceReloadStyle = true );

	CSS::StyleSheet& getStyleSheet();

	bool hasStyleSheet();

	const bool& isLoading() const;

	UIThemeManager* getUIThemeManager() const;

	UIWidget* getRoot() const;

	bool getVerbose() const;

	void setVerbose( bool verbose );

	void invalidateStyle( UIWidget* widget );

	void invalidateStyleState( UIWidget* widget, bool disableCSSAnimations = false );

	void invalidateLayout( UILayout* widget );

	void setIsLoading( bool isLoading );

	void updateDirtyLayouts();

	void updateDirtyStyles();

	void updateDirtyStyleStates();

	const bool& isUpdatingLayouts() const;

	UIIconThemeManager* getUIIconThemeManager() const;

	Drawable* findIcon( const std::string& iconName );

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

  protected:
	friend class EE::UI::UIWindow;
	friend class EE::UI::UIWidget;
	UIWidget* mRoot;
	Sizef mDpSize;
	Uint32 mFlags;
	Translator mTranslator;
	std::list<UIWindow*> mWindowsList;
	CSS::StyleSheet mStyleSheet;
	bool mIsLoading;
	bool mVerbose;
	bool mUpdatingLayouts;
	UIThemeManager* mUIThemeManager;
	UIIconThemeManager* mUIIconThemeManager;
	std::vector<Font*> mFontFaces;
	KeyBindings mKeyBindings;
	std::map<std::string, KeyBindingCommand> mKeyBindingCommands;
	std::unordered_set<UIWidget*> mDirtyStyle;
	std::unordered_set<UIWidget*> mDirtyStyleState;
	std::unordered_map<UIWidget*, bool> mDirtyStyleStateCSSAnimations;
	std::unordered_set<UILayout*> mDirtyLayouts;
	std::vector<std::pair<Float, std::string>> mTimes;

	virtual void resizeNode( EE::Window::Window* win );

	virtual void onDrawDebugDataChange();

	virtual void setFocus();

	void setInternalPixelsSize( const Sizef& size );

	void setActiveWindow( UIWindow* window );

	void setFocusLastWindow( UIWindow* window );

	void windowAdd( UIWindow* win );

	void windowRemove( UIWindow* win );

	bool windowExists( UIWindow* win );

	virtual void setInternalSize( const Sizef& size );

	void reloadStyle( const bool& disableAnimations = false );

	bool onMediaChanged();

	virtual void onChildCountChange( Node* child, const bool& removed );

	virtual void onSizeChange();

	void processStyleSheetAtRules( const CSS::StyleSheet& styleSheet );

	void loadFontFaces( const CSS::StyleSheetStyleVector& styles );

	std::vector<UIWidget*> loadNode( pugi::xml_node node, Node* parent );

	virtual Uint32 onKeyDown( const KeyEvent& event );

	void onWidgetDelete( Node* node );

	void resetTooltips( Node* node );
};

}} // namespace EE::UI

#endif
