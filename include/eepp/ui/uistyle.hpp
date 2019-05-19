#ifndef EE_UI_UISTYLE_HPP
#define EE_UI_UISTYLE_HPP

#include <eepp/ui/uistate.hpp>
#include <eepp/scene/nodeattribute.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetstyle.hpp>
#include <eepp/graphics/fontstyleconfig.hpp>
#include <eepp/math/ease.hpp>
#include <set>
#include <functional>

using namespace EE::Scene;

namespace EE { namespace Graphics {
class Font;
}}

namespace EE { namespace UI {

class UIWidget;

class EE_API UIStyle : public UIState {
	public:
		class TransitionInfo
		{
			public:
				TransitionInfo() :
					timingFunction( Ease::Linear )
				{}

				const std::string& getProperty() const { return property; }

				Ease::Interpolation getTimingFunction() const { return timingFunction; }

				const Time& getDelay() const { return delay; }

				const Time& getDuration() const { return duration; }

				std::string property;
				Ease::Interpolation timingFunction;
				Time delay;
				Time duration;
		};

		static UIStyle * New( UIWidget * widget );

		explicit UIStyle( UIWidget * widget );

		virtual ~UIStyle();

		bool stateExists( const Uint32& state ) const;

		void load();

		void onStateChange();

		CSS::StyleSheetProperty getStatelessStyleSheetProperty( const std::string& propertyName ) const;

		CSS::StyleSheetProperty getStyleSheetProperty( const std::string& propertyName ) const;

		NodeAttribute getNodeAttribute(const std::string& attributeName ) const;

		void addStyleSheetProperties( const CSS::StyleSheetProperties& properties );

		void addStyleSheetProperty( const CSS::StyleSheetProperty& property );

		bool hasTransition( const std::string& propertyName );

		TransitionInfo getTransition( const std::string& propertyName );
	protected:
		typedef std::map<std::string, TransitionInfo> TransitionsMap;

		UIWidget * mWidget;
		CSS::StyleSheetStyleVector mCacheableStyles;
		CSS::StyleSheetStyleVector mNoncacheableStyles;
		CSS::StyleSheetStyle mElementStyle;
		CSS::StyleSheetProperties mProperties;
		std::vector<CSS::StyleSheetProperty> mTransitionAttributes;
		TransitionsMap mTransitions;
		std::set<UIWidget*> mRelatedWidgets;
		std::set<UIWidget*> mSubscribedWidgets;

		void tryApplyStyle( const CSS::StyleSheetStyle& style );

		void updateState();

		void parseTransitions();

		void subscribeNonCacheableStyles();

		void unsubscribeNonCacheableStyles();

		void subscribeRelated( UIWidget * widget );

		void unsubscribeRelated( UIWidget * widget );

		void removeFromSubscribedWidgets( UIWidget * widget );

		void removeRelatedWidgets();
};

}}

#endif
