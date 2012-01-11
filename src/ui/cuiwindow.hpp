#ifndef EE_UICUIWINDOW_HPP
#define EE_UICUIWINDOW_HPP

#include "cuicomplexcontrol.hpp"
#include "cuipushbutton.hpp"
#include "cuitextbox.hpp"

namespace EE { namespace UI {

class cUIWindow : public cUIComplexControl {
	public:
		class CreateParams : public cUIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					cUIComplexControl::CreateParams(),
					WinFlags( UI_WIN_DEFAULT_FLAGS ),
					ButtonsSeparation( 4 ),
					MinCornerDistance( 24 ),
					TitleFontColor( 255, 255, 255, 255),
					BaseAlpha( 255 ),
					DecorationAutoSize( true ),
					BorderAutoSize( true )
				{
				}

				inline ~CreateParams() {}

				Uint32		WinFlags;
				eeSize		DecorationSize;
				eeSize		BorderSize;
				eeSize		MinWindowSize;
				eeVector2i	ButtonsPositionFixer;
				Uint32		ButtonsSeparation;
				Int32		MinCornerDistance;
				eeColorA	TitleFontColor;
				Uint8		BaseAlpha;
				bool		DecorationAutoSize;
				bool		BorderAutoSize;
		};

		cUIWindow( const cUIWindow::CreateParams& Params );

		virtual ~cUIWindow();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void Size( const eeSize& Size );

		void Size( const Int32& Width, const Int32& Height );

		const eeSize& Size();

		virtual void SetTheme( cUITheme * Theme );

		virtual Uint32 OnMessage( const cUIMessage *Msg );

		cUIControlAnim * Container() const;

		cUIComplexControl * ButtonClose() const;

		cUIComplexControl * ButtonMaximize() const;

		cUIComplexControl * ButtonMinimize() const;

		virtual void Draw();

		virtual bool Show();

		virtual bool Hide();

		virtual void Update();

		virtual void CloseWindow();

		void BaseAlpha( const Uint8& Alpha );

		const Uint8& BaseAlpha() const;

		void Title( const String& Text );

		String Title() const;

		cUITextBox * TitleTextBox() const;

		bool AddShortcut( const Uint32& KeyCode, const Uint32& Mod, cUIPushButton * Button );

		bool RemoveShortcut( const Uint32& KeyCode, const Uint32& Mod );

		bool IsModal();

		cUIControl * GetModalControl() const;
	protected:
		class KeyboardShortcut {
			public:
				KeyboardShortcut() :
					KeyCode(0),
					Mod(0),
					Button(NULL)
				{}

				KeyboardShortcut( const Uint32& KeyCode, const Uint32& Mod, cUIPushButton * Button ) :
					KeyCode( KeyCode ),
					Mod( Mod ),
					Button( Button )
				{}

				Uint32 KeyCode;
				Uint32 Mod;
				cUIPushButton * Button;
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

		cUIControlAnim *	mWindowDecoration;
		cUIControlAnim *	mBorderLeft;
		cUIControlAnim *	mBorderRight;
		cUIControlAnim *	mBorderBottom;
		cUIComplexControl *	mContainer;

		cUIComplexControl *	mButtonClose;
		cUIComplexControl *	mButtonMinimize;
		cUIComplexControl *	mButtonMaximize;
		cUITextBox *		mTitle;

		cUIControl *		mModalCtrl;

		eeSize				mDecoSize;
		eeSize				mBorderSize;
		eeSize				mMinWindowSize;
		eeVector2i			mNonMaxPos;
		eeSize				mNonMaxSize;
		eeVector2i			mButtonsPositionFixer;
		Uint32				mButtonsSeparation;
		Int32				mMinCornerDistance;

		UI_RESIZE_TYPE		mResizeType;
		eeVector2i			mResizePos;

		eeColorA			mTitleFontColor;

		KeyboardShortcuts	mKbShortcuts;

		Uint8				mBaseAlpha;

		bool				mDecoAutoSize;
		bool				mBorderAutoSize;

		virtual void OnSizeChange();

		virtual void OnAlphaChange();

		virtual Uint32 OnKeyDown( const cUIEventKey &Event );

		void ButtonCloseClick( const cUIEvent * Event );

		void ButtonMaximizeClick( const cUIEvent * Event );

		void ButtonMinimizeClick( const cUIEvent * Event );

		void ContainerPosChange( const cUIEvent * Event );

		void FixChildsSize();

		void DoResize ( const cUIMessage * Msg );

		void DecideResizeType( cUIControl * Control );

		void TryResize( const UI_RESIZE_TYPE& Type );

		void EndResize();

		void UpdateResize();

		void InternalSize( eeSize Size );

		void InternalSize( const Int32& w, const Int32& h );

		void GetMinWinSize();

		void FixTitleSize();

		Uint32 OnMouseDoubleClick( const eeVector2i &Pos, Uint32 Flags );

		void CheckShortcuts( const Uint32& KeyCode, const Uint32& Mod );

		KeyboardShortcuts::iterator ExistsShortcut( const Uint32& KeyCode, const Uint32& Mod );
};

}}

#endif
