#ifndef EE_UICUIWINDOW_HPP
#define EE_UICUIWINDOW_HPP

#include <eepp/ui/uicomplexcontrol.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uitextbox.hpp>

namespace EE { namespace UI {

class EE_API UIWindow : public UIComplexControl {
	public:
		class CreateParams : public UIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					UIComplexControl::CreateParams(),
					WinFlags( UI_WIN_DEFAULT_FLAGS ),
					ButtonsSeparation( 4 ),
					MinCornerDistance( 24 ),
					TitleFontColor( 255, 255, 255, 255 ),
					BaseAlpha( 255 ),
					DecorationAutoSize( true ),
					BorderAutoSize( true )
				{
				}

				inline ~CreateParams() {}

				Uint32		WinFlags;
				Sizei		DecorationSize;
				Sizei		BorderSize;
				Sizei		MinWindowSize;
				Vector2i	ButtonsPositionFixer;
				Uint32		ButtonsSeparation;
				Int32		MinCornerDistance;
				ColorA	TitleFontColor;
				Uint8		BaseAlpha;
				bool		DecorationAutoSize;
				bool		BorderAutoSize;
		};

		UIWindow( const UIWindow::CreateParams& Params );

		UIWindow();

		virtual ~UIWindow();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual UIControl * setSize( const Sizei& size );

		UIControl * setSize( const Int32& Width, const Int32& Height );

		const Sizei& getSize();

		virtual void setTheme( UITheme * Theme );

		virtual Uint32 onMessage( const UIMessage *Msg );

		UIControlAnim * getContainer() const;

		UIComplexControl * getButtonClose() const;

		UIComplexControl * getButtonMaximize() const;

		UIComplexControl * getButtonMinimize() const;

		virtual void draw();

		virtual bool show();

		virtual bool hide();

		virtual void update();

		virtual void closeWindow();

		virtual void close();

		void setBaseAlpha( const Uint8& alpha );

		const Uint8& getBaseAlpha() const;

		void setTitle( const String& Text );

		String getTitle() const;

		UITextBox * getTitleTextBox() const;

		bool addShortcut( const Uint32& KeyCode, const Uint32& Mod, UIPushButton * Button );

		bool removeShortcut( const Uint32& KeyCode, const Uint32& Mod );

		bool isModal();

		UIControlAnim * getModalControl() const;

		void maximize();

		bool isMaximizable();

		Uint32 getWinFlags() const;

		UIWindow * setWinFlags(const Uint32 & winFlags);
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

		Uint32				mWinFlags;

		UIControlAnim *	mWindowDecoration;
		UIControlAnim *	mBorderLeft;
		UIControlAnim *	mBorderRight;
		UIControlAnim *	mBorderBottom;
		UIComplexControl *	mContainer;

		UIComplexControl *	mButtonClose;
		UIComplexControl *	mButtonMinimize;
		UIComplexControl *	mButtonMaximize;
		UITextBox *		mTitle;

		UIControlAnim *	mModalCtrl;

		Sizei				mDecoSize;
		Sizei				mBorderSize;
		Sizei				mMinWindowSize;
		Vector2i			mNonMaxPos;
		Sizei				mNonMaxSize;
		Vector2i			mButtonsPositionFixer;
		Uint32				mButtonsSeparation;
		Int32				mMinCornerDistance;

		UI_RESIZE_TYPE		mResizeType;
		Vector2i			mResizePos;

		ColorA			mTitleFontColor;

		KeyboardShortcuts	mKbShortcuts;

		Uint8				mBaseAlpha;

		bool				mDecoAutoSize;
		bool				mBorderAutoSize;

		virtual void onSizeChange();

		virtual void onAlphaChange();

		virtual Uint32 onKeyDown( const UIEventKey &Event );

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

		void getMinWinSize();

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
};

}}

#endif
