#ifndef EE_UICUIMANAGER_H
#define EE_UICUIMANAGER_H

#include "cuicontrol.hpp"
#include "cuicontrolanim.hpp"
#include "../window/cinput.hpp"
#include "../window/cwindow.hpp"

namespace EE { namespace UI {

class cUIControl;

class EE_API cUIManager : public tSingleton<cUIManager> {
	friend class tSingleton<cUIManager>;
	public:
		cUIManager( cWindow * window = NULL );

		~cUIManager();

		cUIControlAnim * MainControl() const;

		cUIControl * FocusControl() const;

		void FocusControl( cUIControl * Ctrl );

		cUIControl * OverControl() const;

		void OverControl( cUIControl * Ctrl );

		void Init();

		void Shutdown();

		void Update();

		void Draw();

		const eeFloat& Elapsed() const;

		void ResizeControl();

		void SendMsg( cUIControl * Ctrl, const Uint32& Msg, const Uint32& Flags = 0 );

		eeVector2i GetMousePos();

		cInput * GetInput() const;

		const Uint32& PressTrigger() const;

		const Uint32& LastPressTrigger() const;

		void ClipEnable( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height );

		void ClipDisable();

		void SendKeyUp( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod );

		void SendKeyDown( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod );
	protected:
		cWindow *			mWindow;
		cInput *			mKM;

		bool				mInit;
		cUIControlAnim *	mControl;
		cUIControl *		mFocusControl;
		cUIControl *		mOverControl;
		cUIControl * 		mDownControl;
		bool 				mFirstPress;
		eeFloat 			mElapsed;
		Int32 				mCbId;
		Uint32				mResizeCb;

		void				InputCallback( InputEvent * Event );
};

}}

#endif
