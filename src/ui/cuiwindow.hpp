#ifndef EE_UICUIWINDOW_HPP
#define EE_UICUIWINDOW_HPP

#include "cuicomplexcontrol.hpp"
#include "cuipushbutton.hpp"
#include "cuitextbox.hpp"

namespace EE { namespace UI {

class cUIWindow : public cUIComplexControl {
	public:
		enum UIWindowFlags {
			UI_WIN_NO_BORDER					= ( 1 << 0 ),
			UI_WIN_CLOSE_BUTTON					= ( 1 << 1 ),
			UI_WIN_MINIMIZE_BUTTON				= ( 1 << 2 ),
			UI_WIN_MAXIMIZE_BUTTON				= ( 1 << 3 ),
			UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS	= ( 1 << 4 ),
			UI_WIN_RESIZEABLE					= ( 1 << 5 ),
			UI_WIN_DRAGABLE_CONTAINER			= ( 1 << 6 ),
			UI_WIN_SHARE_ALPHA_WITH_CHILDS		= ( 1 << 7 )
		};

		class CreateParams : public cUIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					cUIComplexControl::CreateParams(),
					WinFlags( UI_WIN_CLOSE_BUTTON | UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS | UI_WIN_RESIZEABLE | UI_WIN_SHARE_ALPHA_WITH_CHILDS ),
					ButtonsSeparation( 4 ),
					MinCornerDistance( 24 ),
					TitleFontColor( 255, 255, 255, 255),
					BaseAlpha( 255 ),
					DecorationAutoSize( true ),
					BorderAutoSize( true )
				{
				}

				inline ~CreateParams() {};

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

		~cUIWindow();

		virtual void Size( const eeSize& Size );

		virtual void SetTheme( cUITheme * Theme );

		virtual Uint32 OnMessage( const cUIMessage *Msg );

		cUIControlAnim * Container() const;

		cUIComplexControl * ButtonClose() const;

		cUIComplexControl * ButtonMaximize() const;

		cUIComplexControl * ButtonMinimize() const;

		virtual bool Show();

		virtual bool Hide();

		virtual void Update();

		virtual void CloseWindow();

		void BaseAlpha( const Uint8& Alpha );

		const Uint8& BaseAlpha() const;

		void Title( const String& Text );

		String Title() const;

		cUITextBox * TitleTextBox() const;
	protected:
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

		Uint8				mBaseAlpha;

		bool				mDecoAutoSize;
		bool				mBorderAutoSize;

		virtual void OnSizeChange();

		virtual void OnAlphaChange();

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
};

}}

#endif
