#ifndef EE_UI_CSS_STYLESHEETPARSER
#define EE_UI_CSS_STYLESHEETPARSER

#include <string>
#include <map>
#include <eepp/core/string.hpp>
#include <eepp/system/iostream.hpp>
#include <iostream>
#include <algorithm>
using namespace EE;
using namespace EE::System;

namespace EE { namespace UI { namespace CSS {

// This is not how it should be.
class StyleSheetSelector {
	public:
		enum SelectorType {
			TagName = 1 << 0,
			Id = 1 << 1,
			Class = 1 << 2,
			PseudoClass = 1 << 3
		};

		enum SpecificityVal {
			SpecificityId = 1000000,
			SpecificityClass = 100000,
			SpecificityTag = 10000,
			SpecificityPseudoClass = 0
		};

		explicit StyleSheetSelector( const std::string& selectorName ) :
			name( selectorName ),
			specificity(0)
		{
			auto parts = String::split( name, ' ' );

			for ( auto it = parts.begin(); it != parts.end(); ++it ) {
				parseSelector( *it );
			}
		}

		Uint32 getRequiredFlags() {
			Uint32 flags = 0;

			if ( hasTagName() )
				flags |= TagName;

			if ( hasId() )
				flags |= Id;

			if ( hasClasses() )
				flags |= Class;

			if ( hasPseudoClass() )
				flags |= PseudoClass;

			return flags;
		}

		const std::string& getName() const { return name; };

		const std::string& getTagName() const { return tagName; }

		const std::string getId() const { return id; }

		const std::vector<std::string> getClasses() const { return classes; }

		const std::string& getPseudoClass() const;

		bool hasTagName() { return !tagName.empty(); }

		bool hasId() { return !id.empty(); }

		bool hasClasses() { return !classes.empty(); }

		bool hasClass( std::string cls ) { return std::find(classes.begin(), classes.end(), cls) != classes.end(); }

		bool hasPseudoClass() { return !pseudoClass.empty(); }

		Uint32 getSpecificity() { return specificity; }
	protected:
		std::string name;
		std::string tagName;
		std::string id;
		std::vector<std::string> classes;
		std::string pseudoClass;
		Uint32 specificity;

		void parseSelector( const std::string& selector ) {
			auto selPseudo = String::split( selector, ':' );

			if ( !selPseudo.empty() ) {
				std::string rselector( selPseudo[0] );

				if ( !rselector.empty() ) {
					if ( rselector[0] == '.' ) {
						classes.push_back( rselector.substr(1) );
						specificity += SpecificityClass;
					} else if ( selector[0] == '#' ) {
						id = rselector.substr(1);
						specificity += SpecificityId;
					} else {
						tagName = rselector;
						specificity += SpecificityTag;
					}
				}

				if ( selPseudo.size() > 1 ) {
					pseudoClass = selPseudo[1];
					specificity += SpecificityPseudoClass;
				}
			}
		}
};

class StyleSheetProperty {
	public:
		StyleSheetProperty()
		{}

		explicit StyleSheetProperty( const std::string& name, const std::string& value ) :
			name( String::trim( name ) ),
			value( String::trim( value ) )
		{}

		std::string name;
		std::string value;
};

typedef std::map<std::string, StyleSheetProperty> StyleSheetProperties;

class StyleSheetSelectorParser {
	public:
		StyleSheetSelectorParser(){}

		explicit StyleSheetSelectorParser( std::string name )
		{
			std::vector<std::string> sels = String::split( name, ',' );

			for ( std::vector<std::string>::iterator it = sels.begin(); it != sels.end(); ++it )
			{
				std::string cur = String::trim( *it );

				selectors.push_back( StyleSheetSelector( cur ) );
			}
		}

		std::vector<StyleSheetSelector> selectors;
};

class StyleSheetPropertiesParser {
	public:
		typedef std::map<std::string, StyleSheetProperty> PropertiesDictionary;

		StyleSheetPropertiesParser(){}

		explicit StyleSheetPropertiesParser( const std::string& propsstr ) {
			std::vector<std::string> props = String::split( propsstr, ';' );

			if ( !props.empty() ) {
				parse( propsstr );
			}
		};

		void print();

		PropertiesDictionary properties;
	protected:
		enum ReadState {
			ReadingPropertyName,
			ReadingPropertyValue,
			ReadingValueUrl,
			ReadingComment
		};

		ReadState prevRs;

		void parse( std::string propsstr );

		int readPropertyName( ReadState& rs, std::size_t pos, std::string& buffer, const std::string& str );

		int readPropertyValue( ReadState& rs, std::size_t pos, std::string& buffer, const std::string& str );

		int readComment( ReadState& rs, std::size_t pos, std::string& buffer, const std::string& str );

		int readValueUrl( ReadState& rs, std::size_t pos, std::string& buffer, const std::string& str );
};

class StyleSheetNode {
	public:
		explicit StyleSheetNode( const std::string& selector, const StyleSheetProperties& properties ) :
			selector( selector ),
			properties( properties )
		{}

		void print() {
			std::cout << selector.getName() << " {" << std::endl;

			for ( StyleSheetProperties::iterator it = properties.begin(); it != properties.end(); ++it ) {
				StyleSheetProperty& prop = it->second;

				std::cout << "\t" << prop.name << ": " << prop.value << ";" << std::endl;
			}

			std::cout << "}" << std::endl;
		}

		StyleSheetSelector selector;
		StyleSheetProperties properties;
};

class StyleSheet {
	public:
		StyleSheet() {}

		void addNode( StyleSheetNode node ) {
			nodes.push_back( node );
		}

		StyleSheetProperties find( const std::string& tagName = "", const std::string& id = "", const std::vector<std::string>& classes = {}, const std::string& pseudoClass = "" ) {
			StyleSheetProperties propertiesSelected;
			Uint32 lastSpecificity = -1;

			for ( auto it = nodes.begin(); it != nodes.end(); ++it ) {
				StyleSheetNode& node = *it;
				StyleSheetSelector& selector = node.selector;

				Uint32 flags = 0;

				if ( selector.hasTagName() && !tagName.empty() && selector.getTagName() == tagName ) {
					flags |= StyleSheetSelector::TagName;
				}

				if ( selector.hasId() && !id.empty() && selector.getId() == id ) {
					flags |= StyleSheetSelector::Id;
				}

				if ( selector.hasClasses() && !classes.empty() ) {
					bool hasClasses = true;
					for ( auto cit = classes.begin(); cit != classes.end(); ++cit ) {
						if ( !selector.hasClass( *cit ) ) {
							hasClasses = false;
							break;
						}
					}

					if ( hasClasses ) {
						flags |= StyleSheetSelector::Class;
					}
				}

				if ( selector.hasPseudoClass() && !pseudoClass.empty() && selector.getPseudoClass() == pseudoClass ) {
					flags |= StyleSheetSelector::PseudoClass;
				}

				if ( flags == selector.getRequiredFlags() && selector.getSpecificity() > lastSpecificity ) {
					propertiesSelected = node.properties;
					lastSpecificity = selector.getSpecificity();
				}
			}

			return propertiesSelected;
		}

		std::vector<StyleSheetNode> nodes;
};

class StyleSheetParser {
	public:
		StyleSheetParser();

		bool loadFromStream( IOStream& stream );

		bool loadFromFile( const std::string& file );

		void print();
	protected:
		enum ReadState {
			ReadingStyle,
			ReadingProperty,
			ReadingComment
		};

		std::string mCSS;

		StyleSheet mStyleSheet;

		std::vector<std::string> mComments;

		bool parse();

		int readStyle( ReadState& rs, std::size_t pos, std::string& buffer );

		int readComment( ReadState& rs, std::size_t pos, std::string& buffer );

		int readProperty( ReadState& rs, std::size_t pos, std::string& buffer );
};

}}}

#endif
