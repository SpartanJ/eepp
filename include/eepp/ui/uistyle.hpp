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

		void addAttribute( int state, NodeAttribute attribute );

		void load();

		void onStateChange();

		Font * getFontFamily( const Uint32& state = StateFlagNormal ) const;

		int getFontCharacterSize( const Uint32& state = StateFlagNormal, const int& defaultValue = 12 ) const;

		Color getTextColor( const Uint32& state = StateFlagNormal ) const;

		Color getTextShadowColor( const Uint32& state = StateFlagNormal ) const;

		Uint32 getTextStyle( const Uint32& state = StateFlagNormal ) const;

		Float getFontOutlineThickness( const Uint32& state = StateFlagNormal ) const;

		Color getFontOutlineColor( const Uint32& state = StateFlagNormal ) const;

		FontStyleConfig getFontStyleConfig( const Uint32& state = StateFlagNormal ) const;

		NodeAttribute getAttribute( const Uint32& state, const std::string& attributeName ) const;

		NodeAttribute getAttributeFromNames( const Uint32& state, const std::vector<std::string>& attributeNames ) const;

		void addStyleSheetProperties( const Uint32& state, const CSS::StyleSheetProperties& properties );

		void addStyleSheetProperty( const Uint32& state, const CSS::StyleSheetProperty& property );

		bool hasTransition( const Uint32& state, const std::string& propertyName );

		TransitionInfo getTransition( const Uint32& state, const std::string& propertyName );
	protected:
		typedef std::map<std::string, NodeAttribute> AttributesMap;
		typedef std::map<std::string, TransitionInfo> TransitionsMap;

		UIWidget * mWidget;
		std::map<Uint32, AttributesMap> mStates;
		std::map<Uint32, TransitionsMap> mTransitions;
		std::map<Uint32, std::vector<NodeAttribute>> mTransitionAttributes;

		void updateState();

		void parseTransitions( const Uint32& state );
};

}}

#endif
