#ifndef EE_UICUIMANAGER_H
#define EE_UICUIMANAGER_H

#include "cuicontrol.hpp"
#include "cuicontrolanim.hpp"

namespace EE { namespace UI {

class cUIControl;

class EE_API cUIManager : public tSingleton<cUIManager> {
	friend class tSingleton<cUIManager>;
	public:
		cUIManager();
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

		void SetTheme( const std::string& Theme );

		void SetTheme( cUITheme * Theme );

		eeVector2i GetMousePos();

		cInput * GetInput() const;

		void DefaultTheme( cUITheme * Theme );

		void DefaultTheme( const std::string& Theme );

		cUITheme * DefaultTheme() const;

		void ApplyDefaultTheme( cUIControl * Control );

		void AutoApplyDefaultTheme( const bool& apply );

		const bool& AutoApplyDefaultTheme() const;
	protected:
		cEngine *			mEE;
		cInput *			mKM;

		bool				mInit;
		cUIControlAnim *	mControl;
		cUIControl *		mFocusControl;
		cUIControl *		mOverControl;
		cUIControl * 		mDownControl;
		bool 				mFirstPress;

		eeFloat 			mElapsed;

		Int32 				mCbId;

		cUITheme * 			mThemeDefault;
		bool				mAutoApplyDefaultTheme;

		void				InputCallback( EE_Event * Event );
		void				SendKeyUp( EE_Event * Event );
		void				SendKeyDown( EE_Event * Event );
};

}}

#endif
