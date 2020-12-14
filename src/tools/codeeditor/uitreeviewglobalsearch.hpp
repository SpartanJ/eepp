#ifndef UITREEVIEWGLOBALSEARCH_HPP
#define UITREEVIEWGLOBALSEARCH_HPP

#include "projectsearch.hpp"
#include <eepp/ui/doc/syntaxcolorscheme.hpp>
#include <eepp/ui/uitreeview.hpp>

using namespace EE::UI;
using namespace EE::UI::Doc;

class UITreeViewCellGlobalSearch : public UITreeViewCell {
  public:
	static UITreeViewCellGlobalSearch* New() { return eeNew( UITreeViewCellGlobalSearch, () ); }

	UITreeViewCellGlobalSearch() : UITreeViewCell() {}

	UIPushButton* setText( const String& text );

	UIPushButton* updateText( const std::string& text );

	virtual void draw();
  protected:
	std::pair<size_t, size_t> mSearchStrPos;
};

class UITreeViewGlobalSearch : public UITreeView {
  public:
	static UITreeViewGlobalSearch* New( const SyntaxColorScheme& colorScheme ) {
		return eeNew( UITreeViewGlobalSearch, ( colorScheme ) );
	}

	UITreeViewGlobalSearch( const SyntaxColorScheme& colorScheme );

	UIWidget* createCell( UIWidget* rowWidget, const ModelIndex& index );

	const SyntaxColorScheme& getColorScheme() const { return mColorScheme; }

	const Color& getLineNumColor() const { return mLineNumColor; }

	void updateColorScheme( const SyntaxColorScheme& colorScheme ) { mColorScheme = colorScheme; }

	void setSearchStr( const String& searchStr ) { mSearchStr = searchStr; }

	const String& getSearchStr() const { return mSearchStr; }

  protected:
	Color mLineNumColor;
	SyntaxColorScheme mColorScheme;
	String mSearchStr;
};

#endif // UITREEVIEWGLOBALSEARCH_HPP
