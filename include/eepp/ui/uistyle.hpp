#ifndef EE_UI_UISTYLE_HPP
#define EE_UI_UISTYLE_HPP

#include <eepp/ui/uistate.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetstyle.hpp>
#include <eepp/ui/css/transitiondefinition.hpp>
#include <eepp/graphics/fontstyleconfig.hpp>
#include <eepp/math/ease.hpp>
#include <set>
#include <functional>

namespace EE { namespace Graphics {
class Font;
}}

namespace EE { namespace UI {

class UIWidget;

class EE_API UIStyle : public UIState {
	public:
		static UIStyle * New( UIWidget * widget );

		explicit UIStyle( UIWidget * widget );

		virtual ~UIStyle();

		bool stateExists( const Uint32& state ) const;

		void load();

		void onStateChange();

		CSS::StyleSheetProperty getStatelessStyleSheetProperty( const std::string& propertyName ) const;

		CSS::StyleSheetProperty getStyleSheetProperty( const std::string& propertyName ) const;

		void setStyleSheetProperties( const CSS::StyleSheetProperties& properties );

		void setStyleSheetProperty( const CSS::StyleSheetProperty& property );

		bool hasTransition( const std::string& propertyName );

		CSS::TransitionDefinition getTransition( const std::string& propertyName );

		const bool& isChangingState() const;
	protected:
		UIWidget * mWidget;
		CSS::StyleSheetStyleVector mCacheableStyles;
		CSS::StyleSheetStyleVector mNoncacheableStyles;
		CSS::StyleSheetStyle mElementStyle;
		CSS::StyleSheetProperties mProperties;
		std::vector<CSS::StyleSheetProperty> mTransitionAttributes;
		CSS::TransitionsMap mTransitions;
		std::set<UIWidget*> mRelatedWidgets;
		std::set<UIWidget*> mSubscribedWidgets;
		bool mChangingState;

		void tryApplyStyle( const CSS::StyleSheetStyle& style );

		void updateState();

		void subscribeNonCacheableStyles();

		void unsubscribeNonCacheableStyles();

		void subscribeRelated( UIWidget * widget );

		void unsubscribeRelated( UIWidget * widget );

		void removeFromSubscribedWidgets( UIWidget * widget );

		void removeRelatedWidgets();
};

}}

#endif
