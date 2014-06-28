#ifndef EE_UICUIDROPDOWNLIST_HPP
#define EE_UICUIDROPDOWNLIST_HPP

#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uilistbox.hpp>

namespace EE { namespace UI {

class EE_API UIDropDownList : public UITextInput {
	public:
		class CreateParams : public UITextInput::CreateParams {
			public:
				inline CreateParams() :
					UITextInput::CreateParams(),
					ListBox( NULL ),
					MinNumVisibleItems( 6 ),
					PopUpToMainControl( false )
				{
				}

				inline ~CreateParams() {}

				UIListBox * 	ListBox;
				Uint32			MinNumVisibleItems;
				bool			PopUpToMainControl;
		};

		UIDropDownList( UIDropDownList::CreateParams& Params );

		virtual ~UIDropDownList();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( UITheme * Theme );

		UIListBox * ListBox() const;

		virtual void Update();
	protected:
		UIListBox *	mListBox;
		Uint32			mMinNumVisibleItems;
		bool			mPopUpToMainControl;

		void ShowListBox();

		void OnListBoxFocusLoss( const UIEvent * Event );

		virtual void OnItemSelected( const UIEvent * Event );

		virtual void Show();

		virtual void Hide();

		Uint32 OnMouseClick( const Vector2i& Pos, const Uint32 Flags );

		virtual void OnItemClicked( const UIEvent * Event );

		virtual void OnItemKeyDown( const UIEvent * Event );

		virtual void OnControlClear( const UIEvent * Event );

		Uint32 OnKeyDown( const UIEventKey &Event );

		virtual void OnSizeChange();

		virtual void AutoSize();

		virtual void AutoSizeControl();

		void DestroyListBox();
};

}}

#endif
