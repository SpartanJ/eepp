#ifndef EE_UISCENENODE_HPP
#define EE_UISCENENODE_HPP

#include <eepp/scene/scenenode.hpp>
#include <eepp/system/translator.hpp>
#include <eepp/ui/css/stylesheet.hpp>
#include <eepp/ui/keyboardshortcut.hpp>

namespace EE { namespace Graphics {
class Font;
}} // namespace EE::Graphics

namespace EE { namespace UI {

class UIThemeManager;
class UIWidget;
class UIWindow;
class UIWidget;

class EE_API UISceneNode : public SceneNode {
  public:
	static UISceneNode* New( EE::Window::Window* window = NULL );

	explicit UISceneNode( EE::Window::Window* window = NULL );

	virtual ~UISceneNode();

	virtual Node* setSize( const Sizef& size );

	virtual Node* setSize( const Float& Width, const Float& Height );

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

	bool addShortcut( const Uint32& KeyCode, const Uint32& Mod, UIWidget* Widget );

	bool removeShortcut( const Uint32& KeyCode, const Uint32& Mod );

	bool getVerbose() const;

	void setVerbose( bool verbose );

	void invalidateStyle( UIWidget* widget );

	void invalidateStyleState( UIWidget* widget, bool disableCSSAnimations = false );

	void setIsLoading( bool isLoading );

	void updateDirtyStyles();

	void updateDirtyStyleStates();

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
	UIThemeManager* mUIThemeManager;
	std::vector<Font*> mFontFaces;
	KeyboardShortcuts mKbShortcuts;
	std::unordered_set<UIWidget*> mDirtyStyle;
	std::unordered_set<UIWidget*> mDirtyStyleState;
	std::unordered_map<UIWidget*, bool> mDirtyStyleStateCSSAnimations;
	std::vector<std::pair<Float, std::string>> mTimes;

	virtual void resizeControl( EE::Window::Window* win );

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

	virtual Uint32 onKeyDown( const KeyEvent& Event );

	void checkShortcuts( const Uint32& KeyCode, const Uint32& Mod );

	KeyboardShortcuts::iterator existsShortcut( const Uint32& KeyCode, const Uint32& Mod );

	void onWidgetDelete( Node* node );
};

}} // namespace EE::UI

#endif
