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

		virtual ~UIWindow();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void Size( const Sizei& Size );

		void Size( const Int32& Width, const Int32& Height );

		const Sizei& Size();

		virtual void SetTheme( UITheme * Theme );

		virtual Uint32 OnMessage( const UIMessage *Msg );

		UIControlAnim * Container() const;

		UIComplexControl * ButtonClose() const;

		UIComplexControl * ButtonMaximize() const;

		UIComplexControl * ButtonMinimize() const;

		virtual void Draw();

		virtual bool Show();

		virtual bool Hide();

		virtual void Update();

		virtual void CloseWindow();

		virtual void Close();

		void BaseAlpha( const Uint8& Alpha );

		const Uint8& BaseAlpha() const;

		void Title( const String& Text );

		String Title() const;

		UITextBox * TitleTextBox() const;

		bool AddShortcut( const Uint32& KeyCode, const Uint32& Mod, UIPushButton * Button );

		bool RemoveShortcut( const Uint32& KeyCode, const Uint32& Mod );

		bool IsModal();

		UIControlAnim * GetModalControl() const;

		void Maximize();

		bool IsMaximixable();
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

		virtual void OnSizeChange();

		virtual void OnAlphaChange();

		virtual Uint32 OnKeyDown( const UIEventKey &Event );

		void ButtonCloseClick( const UIEvent * Event );

		void ButtonMaximizeClick( const UIEvent * Event );

		void ButtonMinimizeClick( const UIEvent * Event );

		void ContainerPosChange( const UIEvent * Event );

		void FixChildsSize();

		void DoResize ( const UIMessage * Msg );

		void DecideResizeType( UIControl * Control );

		void TryResize( const UI_RESIZE_TYPE& Type );

		void EndResize();

		void UpdateResize();

		void InternalSize( Sizei Size );

		void InternalSize( const Int32& w, const Int32& h );

		void GetMinWinSize();

		void FixTitleSize();

		Uint32 OnMouseDoubleClick( const Vector2i &Pos, const Uint32 Flags );

		void CheckShortcuts( const Uint32& KeyCode, const Uint32& Mod );

		KeyboardShortcuts::iterator ExistsShortcut( const Uint32& KeyCode, const Uint32& Mod );

		void CreateModalControl();

		void EnableByModal();

		void DisableByModal();

		void ResizeCursor();
};

}}

#endif
