#ifndef EE_UICUIWINDOW_HPP
#define EE_UICUIWINDOW_HPP

#include "cuicomplexcontrol.hpp"
#include "cuipushbutton.hpp"

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
			UI_WIN_DRAGABLE_CONTAINER			= ( 1 << 6 )
		};

		class CreateParams : public cUIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					cUIComplexControl::CreateParams(),
					WinFlags( UI_WIN_CLOSE_BUTTON | UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS | UI_WIN_RESIZEABLE ),
					ButtonsSeparation( 4 ),
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
	protected:
		Uint32				mWinFlags;

		cUIControlAnim *	mWindowDecoration;
		cUIControlAnim *	mBorderLeft;
		cUIControlAnim *	mBorderRight;
		cUIControlAnim *	mBorderBottom;
		cUIControlAnim *	mContainer;

		cUIComplexControl *	mButtonClose;
		cUIComplexControl *	mButtonMinimize;
		cUIComplexControl *	mButtonMaximize;

		eeSize				mDecoSize;
		eeSize				mBorderSize;
		eeSize				mMinWindowSize;
		eeVector2i			mNonMaxPos;
		eeSize				mNonMaxSize;
		eeVector2i			mButtonsPositionFixer;
		Uint32				mButtonsSeparation;
		bool				mDecoAutoSize;
		bool				mBorderAutoSize;

		virtual void OnSizeChange();

		void ButtonCloseClick( const cUIEvent * Event );

		void ButtonMaximizeClick( const cUIEvent * Event );

		void ButtonMinimizeClick( const cUIEvent * Event );

		void ContainerPosChange( const cUIEvent * Event );

		void FixChildsSize();
};

}}

#endif
