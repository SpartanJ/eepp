#ifndef EE_UI_UISTYLE_HPP
#define EE_UI_UISTYLE_HPP

#include <eepp/ui/uistate.hpp>
#include <eepp/scene/nodeattribute.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/graphics/fontstyleconfig.hpp>
#include <eepp/math/ease.hpp>

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

				const Time& getDelay() const { return delay; };

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

		CSS::StyleSheetProperty getStyleSheetProperty( const Uint32& state, const std::string& attributeName ) const;

		CSS::StyleSheetProperty getStyleSheetPropertyFromNames( const Uint32& state, const std::vector<std::string>& propertiesNames ) const;

		NodeAttribute getNodeAttribute( const Uint32& state, const std::string& attributeName ) const;

		bool hasStyleSheetProperty( const Uint32& state, const std::string& propertyName ) const;

		void addStyleSheetProperties( const Uint32& state, const CSS::StyleSheetProperties& properties );

		void addStyleSheetProperty( const Uint32& state, const CSS::StyleSheetProperty& property );

		bool hasTransition( const Uint32& state, const std::string& propertyName );

		TransitionInfo getTransition( const Uint32& state, const std::string& propertyName );
	protected:
		typedef std::map<std::string, TransitionInfo> TransitionsMap;

		UIWidget * mWidget;
		std::map<Uint32, CSS::StyleSheetProperties> mStates;
		std::map<Uint32, TransitionsMap> mTransitions;
		std::map<Uint32, std::vector<CSS::StyleSheetProperty>> mTransitionAttributes;

		void updateState();

		void parseTransitions( const Uint32& state );
};

}}

#endif
