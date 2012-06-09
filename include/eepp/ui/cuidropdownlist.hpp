#ifndef EE_UICUIDROPDOWNLIST_HPP
#define EE_UICUIDROPDOWNLIST_HPP

#include <eepp/ui/cuitextinput.hpp>
#include <eepp/ui/cuilistbox.hpp>

namespace EE { namespace UI {

class EE_API cUIDropDownList : public cUITextInput {
	public:
		class CreateParams : public cUITextInput::CreateParams {
			public:
				inline CreateParams() :
					cUITextInput::CreateParams(),
					ListBox( NULL ),
					MinNumVisibleItems( 6 ),
					PopUpToMainControl( false )
				{
				}

				inline ~CreateParams() {}

				cUIListBox * 	ListBox;
				Uint32			MinNumVisibleItems;
				bool			PopUpToMainControl;
		};

		cUIDropDownList( cUIDropDownList::CreateParams& Params );

		virtual ~cUIDropDownList();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( cUITheme * Theme );

		cUIListBox * ListBox() const;

		virtual void Update();
	protected:
		cUIListBox *	mListBox;
		Uint32			mMinNumVisibleItems;
		bool			mPopUpToMainControl;

		void ShowListBox();

		void OnListBoxFocusLoss( const cUIEvent * Event );

		virtual void OnItemSelected( const cUIEvent * Event );

		virtual void Show();

		virtual void Hide();

		Uint32 OnMouseClick( const eeVector2i& Pos, const Uint32 Flags );

		virtual void OnItemClicked( const cUIEvent * Event );

		virtual void OnItemKeyDown( const cUIEvent * Event );

		virtual void OnControlClear( const cUIEvent * Event );

		Uint32 OnKeyDown( const cUIEventKey &Event );

		virtual void OnSizeChange();

		virtual void AutoSize();

		virtual void AutoSizeControl();

		void DestroyListBox();
};

}}

#endif
