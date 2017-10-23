#ifndef EE_UICUIWINDOW_HPP
#define EE_UICUIWINDOW_HPP

#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uitextview.hpp>

namespace EE { namespace Graphics {
class FrameBuffer;
}}

namespace EE { namespace UI {

class EE_API UIWindow : public UIWidget {
	public:
		enum WindowBaseContainerType {
			SIMPLE_LAYOUT,
			LINEAR_LAYOUT,
			RELATIVE_LAYOUT
		};

		static UIWindow * New( WindowBaseContainerType type = SIMPLE_LAYOUT );

		UIWindow( WindowBaseContainerType type = SIMPLE_LAYOUT );

		virtual ~UIWindow();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual UIControl * setSize( const Sizei& size );

		UIControl * setSize( const Int32& Width, const Int32& Height );

		UIWindow * setSizeWithDecoration( const Int32& Width, const Int32& Height );

		UIWindow * setSizeWithDecoration( const Sizei& size );

		const Sizei& getSize();

		virtual void setTheme( UITheme * Theme );

		virtual Uint32 onMessage( const UIMessage *Msg );

		UIWidget * getContainer() const;

		UIControlAnim * getButtonClose() const;

		UIControlAnim * getButtonMaximize() const;

		UIControlAnim * getButtonMinimize() const;

		virtual bool show();

		virtual bool hide();

		virtual void update();

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

		UIWindowStyleConfig getStyleConfig() const;

		UIWindow * setStyleConfig(const UIWindowStyleConfig & styleConfig);

		UIWindow * setMinWindowSize( Sizei size );

		UIWindow * setMinWindowSize( const Int32& width, const Int32& height );

		const Sizei& getMinWindowSize();

		bool ownsFrameBuffer();

		virtual void loadFromXmlNode( const pugi::xml_node& node );

		virtual void internalDraw();

		void invalidate();
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
		UIWindowStyleConfig	mStyleConfig;
		UIControlAnim *	mWindowDecoration;
		UIControlAnim *	mBorderLeft;
		UIControlAnim *	mBorderRight;
		UIControlAnim *	mBorderBottom;
		UIWidget *	mContainer;

		UIControlAnim *	mButtonClose;
		UIControlAnim *	mButtonMinimize;
		UIControlAnim *	mButtonMaximize;
		UITextView *		mTitle;

		UIWidget *	mModalCtrl;

		Vector2i			mNonMaxPos;
		Sizei				mNonMaxSize;
		UI_RESIZE_TYPE		mResizeType;
		Vector2i			mResizePos;
		KeyboardShortcuts	mKbShortcuts;

		Uint32				mCloseListener;
		Uint32				mMaximizeListener;
		Uint32				mMinimizeListener;

		virtual void onSizeChange();

		virtual void onAlphaChange();

		virtual void onChildCountChange();

		virtual Uint32 onKeyDown( const UIEventKey &Event );

		virtual void matrixSet();

		virtual void matrixUnset();

		void onButtonCloseClick( const UIEvent * Event );

		void onButtonMaximizeClick( const UIEvent * Event );

		void onButtonMinimizeClick( const UIEvent * Event );

		void onContainerPosChange( const UIEvent * Event );

		void fixChildsSize();

		void doResize( const UIMessage * Msg );

		void decideResizeType( UIControl * Control );

		void tryResize( const UI_RESIZE_TYPE& getType );

		void endResize();

		void updateResize();

		void internalSize( Sizei size );

		void internalSize( const Int32& w, const Int32& h );

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

		virtual void drawShadow();

		Sizei getFrameBufferSize();
};

}}

#endif
