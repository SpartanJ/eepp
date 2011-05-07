#ifndef EE_UICUICOMMONDIALOG_HPP
#define EE_UICUICOMMONDIALOG_HPP

#include "base.hpp"
#include "cuicomplexcontrol.hpp"
#include "cuipushbutton.hpp"
#include "cuilistbox.hpp"
#include "cuitextinput.hpp"
#include "cuicombobox.hpp"
#include "cuiwindow.hpp"

namespace EE { namespace UI {

class cUICommonDialog : public cUIWindow {
	public:
		class CreateParams : public cUIWindow::CreateParams {
			public:
				inline CreateParams() :
					cUIWindow::CreateParams(),
					DefaultDirectory( GetProcessPath() )
				{
				}

				inline ~CreateParams() {}

				std::string DefaultDirectory;
		};

		cUICommonDialog( const cUICommonDialog::CreateParams& Params );

		void				RefreshFolder();

		virtual Uint32		OnMessage( const cUIMessage *Msg );

		virtual void		Open();

		std::string			GetCurPath() const;

		std::string			GetCurFile() const;

		cUIPushButton *		GetButtonOpen() const;

		cUIPushButton *		GetButtonCancel() const;

		cUIPushButton *		GetButtonUp() const;

		cUIListBox *		GetList() const;

		cUITextInput *		GetPathInput() const;

		cUITextInput *		GetFileInput() const;

		cUIDropDownList *	GetFiletypeList() const;

		void				AddFilePattern( std::string pattern, bool select = false );
	protected:
		std::string			mCurPath;
		cUIPushButton *		mButtonOpen;
		cUIPushButton *		mButtonCancel;
		cUIPushButton *		mButtonUp;
		cUIListBox *		mList;
		cUITextInput *		mPath;
		cUITextInput *		mFile;
		cUIDropDownList *	mFiletype;
};

}}

#endif
