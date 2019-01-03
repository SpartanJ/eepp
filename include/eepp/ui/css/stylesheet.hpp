#ifndef EE_UI_CSS_STYLESHEET_HPP
#define EE_UI_CSS_STYLESHEET_HPP

#include <eepp/ui/css/stylesheetnode.hpp>

namespace EE { namespace UI { namespace CSS {

class StyleSheetElement;

class EE_API StyleSheet {
	public:
		typedef std::map<std::string, StyleSheetNode> StyleSheetNodeList;
		typedef std::map<std::string,StyleSheetProperties> StyleSheetPseudoClassProperties;

		StyleSheet();

		void addNode( const StyleSheetNode& node );

		void combineNode( const StyleSheetNode& node );

		bool isEmpty() const;

		void print();

		void combineStyleSheet( const StyleSheet& styleSheet );

		StyleSheetPseudoClassProperties getElementProperties( StyleSheetElement * element );

		const StyleSheetNodeList& getNodes() const;
	protected:
		StyleSheetNodeList mNodes;
};

}}}

#endif
