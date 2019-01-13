#ifndef EE_UICUIWINDOW_HPP
#define EE_UICUIWINDOW_HPP

#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uitextview.hpp>

namespace EE { namespace Graphics {
class FrameBuffer;
}}

namespace EE { namespace UI {

class UISceneNode;

class EE_API UIWindow : public UIWidget {
	public:
		class StyleConfig {
			public:
				StyleConfig() {}

				StyleConfig( Uint32 winFlags ) :
					WinFlags( winFlags )
				{}

				Uint32		WinFlags = UI_WIN_DEFAULT_FLAGS;
				Sizei		DecorationSize;
				Sizei		BorderSize;
				Sizef		MinWindowSize;
				Vector2i	ButtonsPositionFixer;
				Uint32		ButtonsSeparation = 4;
				Int32		MinCornerDistance = 24;
				Uint8		BaseAlpha = 255;
				bool		DecorationAutoSize = true;
				bool		BorderAutoSize = true;
		};

		enum WindowBaseContainerType {
			SIMPLE_LAYOUT,
			LINEAR_LAYOUT,
			RELATIVE_LAYOUT
		};

		static UIWindow * NewOpt( WindowBaseContainerType type, const StyleConfig& windowStyleConfig );

		static UIWindow * New();

		explicit UIWindow( WindowBaseContainerType type, const StyleConfig& windowStyleConfig );

		explicit UIWindow( WindowBaseContainerType type = SIMPLE_LAYOUT );

		virtual ~UIWindow();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual UINode * setSize( const Sizef& size );

		UINode * setSize( const Float& Width, const Float& Height );

		UIWindow * setSizeWithDecoration( const Float& Width, const Float& Height );

		UIWindow * setSizeWithDecoration( const Sizef& size );

		const Sizef& getSize();

		virtual void setTheme( UITheme * Theme );

		virtual Uint32 onMessage( const NodeMessage *Msg );

		UIWidget * getContainer() const;

		UINode * getButtonClose() const;

		UINode * getButtonMaximize() const;

		UINode * getButtonMinimize() const;

		virtual bool show();

		virtual bool hide();

		virtual void update( const Time& time );

		virtual void closeWindow();

		virtual void close();

		void setBaseAlpha( const Uint8& alpha );

		const Uint8& getBaseAlpha() const;

		void setTitle( const String& Text );

		String getTitle() const;

		UITextView * getTitleTextBox() const;

		bool addShortcut( const Uint32& KeyCode, const Uint32& Mod, UIPushButton * Button );

		bool removeShortcut( const Uint32& KeyCode, const Uint32& Mod );

		bool isModal();

		UIWidget * getModalControl() const;

		void maximize();

		bool isMaximizable();

		bool isResizeable();

		Uint32 getWinFlags() const;

		UIWindow * setWinFlags(const Uint32 & winFlags);

		const StyleConfig& getStyleConfig() const;

		UIWindow * setStyleConfig(const StyleConfig & styleConfig);

		UIWindow * setMinWindowSize( Sizef size );

		UIWindow * setMinWindowSize( const Float& width, const Float& height );

		const Sizef& getMinWindowSize();

		bool ownsFrameBuffer();

		virtual void loadFromXmlNode( const pugi::xml_node& node );

		virtual bool setAttribute( const NodeAttribute& attribute, const Uint32& state = UIState::StateFlagNormal );

		virtual void internalDraw();

		void invalidate();

		bool invalidated();

		FrameBuffer * getFrameBuffer() const;

		virtual bool isDrawInvalidator() const;
	protected:
		class KeyboardShortcut {
			public:
				KeyboardShortcut() :
					KeyCode(0),
					Mod(0),
					Button(NULL)
				{}

				KeyboardShortcut( const Uint32& KeyCode, const Uint32& Mod, UIPushButton * Button ) :
					KeyCode( KeyCode ),
					Mod( Mod ),
					Button( Button )
				{}

				Uint32 KeyCode;
				Uint32 Mod;
				UIPushButton * Button;
		};

		typedef std::list< KeyboardShortcut > KeyboardShortcuts;

		enum UI_RESIZE_TYPE {
			RESIZE_NONE,
			RESIZE_LEFT,
			RESIZE_RIGHT,
			RESIZE_TOP,
			RESIZE_BOTTOM,
			RESIZE_LEFTBOTTOM,
			RESIZE_RIGHTBOTTOM,
			RESIZE_TOPLEFT,
			RESIZE_TOPRIGHT
		};

		FrameBuffer * mFrameBuffer;
		StyleConfig	mStyleConfig;
		UINode *	mWindowDecoration;
		UINode *	mBorderLeft;
		UINode *	mBorderRight;
		UINode *	mBorderBottom;
		UIWidget *	mContainer;

		UINode *	mButtonClose;
		UINode *	mButtonMinimize;
		UINode *	mButtonMaximize;
		UITextView *		mTitle;

		UIWidget *	mModalCtrl;

		Vector2f			mNonMaxPos;
		Sizef				mNonMaxSize;
		UI_RESIZE_TYPE		mResizeType;
		Vector2f			mResizePos;
		KeyboardShortcuts	mKbShortcuts;

		bool				mFrameBufferBound;

		virtual void onSizeChange();

		virtual void onAlphaChange();

		virtual void onChildCountChange();

		virtual Uint32 onKeyDown( const KeyEvent &Event );

		virtual void matrixSet();

		virtual void matrixUnset();

		void fixChildsSize();

		void doResize( const NodeMessage * Msg );

		void decideResizeType( Node * Control );

		void tryResize( const UI_RESIZE_TYPE& getType );

		void endResize();

		void updateResize();

		void internalSize( Sizef size );

		void internalSize( const Float& w, const Float& h );

		void calcMinWinSize();

		void fixTitleSize();

		Uint32 onMouseDoubleClick( const Vector2i &position, const Uint32 flags );

		void checkShortcuts( const Uint32& KeyCode, const Uint32& Mod );

		KeyboardShortcuts::iterator existsShortcut( const Uint32& KeyCode, const Uint32& Mod );

		void createModalControl();

		void enableByModal();

		void disableByModal();

		void resizeCursor();

		void applyMinWinSize();

		void updateWinFlags();

		void createFrameBuffer();

		void drawFrameBuffer();

		void drawHighlightInvalidation();

		virtual void drawShadow();

		virtual void onPaddingChange();

		virtual void preDraw();

		virtual void postDraw();

		virtual Sizei getFrameBufferSize();

		UISceneNode * getUISceneNode();

		void onContainerPositionChange(const Event * Event);
};

}}

#endif
