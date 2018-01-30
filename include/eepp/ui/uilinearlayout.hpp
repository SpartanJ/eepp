#ifndef UI_UILINEARLAYOUT_HPP
#define UI_UILINEARLAYOUT_HPP

#include <eepp/ui/uilayout.hpp>

namespace  EE { namespace UI {

class EE_API UILinearLayout : public UILayout {
	public:
		static UILinearLayout * New();

		static UILinearLayout * NewVertical();

		static UILinearLayout * NewHorizontal();

		UILinearLayout();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		UI_ORIENTATION getOrientation() const;

		UILinearLayout * setOrientation(const UI_ORIENTATION & getOrientation);

		UILinearLayout * add( UIWidget * widget );

		virtual void loadFromXmlNode( const pugi::xml_node& node );
	protected:
		UI_ORIENTATION mOrientation;

		virtual Uint32 onMessage( const UIMessage * Msg );

		virtual void onSizeChange();

		virtual void onParentSizeChange( const Vector2f& SizeChange );

		virtual void onChildCountChange();

		void pack();

		void packVertical();

		void packHorizontal();

		Sizei getTotalUsedSize();
};

}}

#endif

