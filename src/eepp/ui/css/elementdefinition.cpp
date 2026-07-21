#include <eepp/system/functionstring.hpp>
#include <eepp/ui/css/elementdefinition.hpp>

namespace EE { namespace UI { namespace CSS {

ElementDefinition::ElementDefinition( const StyleSheetStyleVector& styleSheetStyles ) :
	mStyles( styleSheetStyles ), mStructurallyVolatile( false ) {
	refresh();
}

StyleSheetProperty* ElementDefinition::getProperty( const Uint32& id ) {
	auto it = mProperties.find( id );
	return it != mProperties.end() ? &it->second : nullptr;
}

const PropertyIdSet& ElementDefinition::getPropertyIds() const {
	return mPropertyIds;
}

const StyleSheetProperties& ElementDefinition::getProperties() const {
	return mProperties;
}

const std::vector<const StyleSheetProperty*>& ElementDefinition::getTransitionProperties() const {
	return mTransitionProperties;
}

const std::vector<const StyleSheetProperty*>& ElementDefinition::getAnimationProperties() const {
	return mAnimationProperties;
}

const StyleSheetVariables& ElementDefinition::getVariables() const {
	return mVariables;
}

bool ElementDefinition::isStructurallyVolatile() const {
	return mStructurallyVolatile;
}

const StyleSheetStyleVector& ElementDefinition::getStyles() const {
	return mStyles;
}

void ElementDefinition::refresh() {
	mProperties.clear();
	mTransitionProperties.clear();
	mAnimationProperties.clear();
	mPropertyIds.clear();
	mStructurallyVolatile = false;
	for ( auto& styleSheetStyle : mStyles ) {
		const StyleSheetProperties& properties = styleSheetStyle->getProperties();

		if ( styleSheetStyle->getSelector().isStructurallyVolatile() )
			mStructurallyVolatile = true;

		for ( auto iterator = properties.begin(); iterator != properties.end(); ++iterator ) {
			const StyleSheetProperty& property = iterator->second;
			const auto& it = mProperties.find( property.getId() );

			if ( it == mProperties.end() ||
				 property.getSpecificity() >= it->second.getSpecificity() ) {
				mProperties[property.getId()] = property;
			}

			if ( String::startsWith( property.getName(), "transition" ) )
				mTransitionProperties.push_back( &property );
			else if ( String::startsWith( property.getName(), "animation" ) )
				mAnimationProperties.push_back( &property );
		}

		findVariables( styleSheetStyle );
	}

	resolveVariables();

	for ( auto& property : mProperties )
		mPropertyIds.insert( property.first );
}

void ElementDefinition::findVariables( const StyleSheetStyle* style ) {
	for ( const auto& vars : style->getVariables() ) {
		const StyleSheetVariable& variable = vars.second;
		const auto& it = mVariables.find( variable.getNameHash() );
		if ( it == mVariables.end() || variable.getSpecificity() >= it->second.getSpecificity() ) {
			mVariables[variable.getNameHash()] = variable;
		}
	}
}

static void resolveVarValue( std::string& value, const StyleSheetVariables& variables,
							 UnorderedSet<String::HashType>& visited, int depth = 0 ) {
	static constexpr int maxDepth = 32;
	if ( depth > maxDepth )
		return;

	std::string::size_type tokenStart = 0;

	while ( true ) {
		tokenStart = value.find( "var(", tokenStart );
		if ( tokenStart == std::string::npos )
			return;

		std::string::size_type tokenEnd = String::findCloseBracket( value, tokenStart, '(', ')' );
		if ( tokenEnd == std::string::npos )
			return;

		std::string varDef( value, tokenStart, tokenEnd + 1 - tokenStart );
		System::FunctionString functionType = System::FunctionString::parse( varDef );

		if ( functionType.getParameters().empty() ) {
			tokenStart = tokenEnd;
			continue;
		}

		bool resolved = false;
		for ( auto& param : functionType.getParameters() ) {
			if ( String::startsWith( param, "--" ) ) {
				auto it = variables.find( String::hash( param ) );
				if ( it != variables.end() &&
					 visited.find( it->second.getNameHash() ) == visited.end() ) {
					visited.insert( it->second.getNameHash() );
					std::string deepValue( it->second.getValue() );
					resolveVarValue( deepValue, variables, visited, depth + 1 );
					String::replaceAll( value, varDef, deepValue );
					resolved = true;
					break;
				}
			}
		}

		if ( resolved )
			tokenStart = 0;
		else
			tokenStart = tokenEnd;
	}
}

void ElementDefinition::resolveVariables() {
	static UnorderedSet<String::HashType> visited;
	for ( auto& varPair : mVariables ) {
		std::string value( varPair.second.getValue() );
		visited.clear();
		resolveVarValue( value, mVariables, visited );
		varPair.second.setValue( value );
	}
}

}}} // namespace EE::UI::CSS
