#ifndef EE_UI_CSS_STYLESHEET_HPP
#define EE_UI_CSS_STYLESHEET_HPP

#include <eepp/ui/css/stylesheetnode.hpp>

namespace EE { namespace UI { namespace CSS {

class StyleSheetElement;

class EE_API StyleSheet {
	public:
		typedef std::map<std::string, StyleSheetNode> StyleSheetNodeList;
		StyleSheet();

		void addNode( const StyleSheetNode& node );

		void combineNode( const StyleSheetNode& node );

		bool isEmpty() const;

		void print();

		void combineStyleSheet( const StyleSheet& styleSheet );

		StyleSheetProperties getElementProperties( StyleSheetElement * element, const std::string& pseudoClass = "" );

		const StyleSheetNodeList& getNodes() const;
	protected:
		StyleSheetNodeList mNodes;
};

}}}

#endif
