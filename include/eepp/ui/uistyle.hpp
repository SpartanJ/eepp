#ifndef EE_UI_UISTYLE_HPP
#define EE_UI_UISTYLE_HPP

#include <eepp/ui/uistate.hpp>
#include <eepp/scene/nodeattribute.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/graphics/fontstyleconfig.hpp>

using namespace EE::Scene;

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

		NodeAttribute getAttribute( const Uint32& state, std::vector<std::string> attributeNames ) const;

		void addStyleSheetProperties( int state, const CSS::StyleSheetProperties& properties );

		void addStyleSheetProperty( int state, const CSS::StyleSheetProperty& property );
	protected:
		typedef std::map<std::string, NodeAttribute> AttributesMap;

		UIWidget * mWidget;
		std::map<int, AttributesMap> mStates;

		void updateState();
};

}}

#endif
