#include "uitreeviewglobalsearch.hpp"
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/doc/syntaxtokenizer.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>

UITreeViewGlobalSearch::UITreeViewGlobalSearch( const SyntaxColorScheme& colorScheme ) :
	UITreeView(), mColorScheme( colorScheme ) {
	mLineNumColor = Color::fromString(
		mUISceneNode->getRoot()->getUIStyle()->getVariable( "--font-hint" ).getValue() );
}

UIWidget* UITreeViewGlobalSearch::createCell( UIWidget* rowWidget, const ModelIndex& index ) {
	UITableCell* widget = index.column() == (Int64)getModel()->treeColumn()
							  ? UITreeViewCellGlobalSearch::New()
							  : UITableCell::New();
	return setupCell( widget, rowWidget, index );
}

UIPushButton* UITreeViewCellGlobalSearch::setText( const String& text ) {
	if ( text != mTextBox->getText() ) {
		mTextBox->setVisible( !text.empty() );
		mTextBox->setText( text );
		updateText( text );
		updateLayout();
	}
	return this;
}

UIPushButton* UITreeViewCellGlobalSearch::updateText( const String& text ) {
	if ( getCurIndex().internalId() != -1 ) {
		UITreeViewGlobalSearch* pp = getParent()->getParent()->asType<UITreeViewGlobalSearch>();

		ProjectSearch::ResultData* res = (ProjectSearch::ResultData*)getCurIndex().parent().data();

		auto styleDef = SyntaxDefinitionManager::instance()->getStyleByExtension( res->file );

		auto tokens =
			SyntaxTokenizer::tokenize( styleDef, text, SYNTAX_TOKENIZER_STATE_NONE ).first;

		size_t start = 0;
		for ( auto& token : tokens ) {
			mTextBox->setFontFillColor( pp->getColorScheme().getSyntaxStyle( token.type ).color,
										start, start + token.text.size() );
			start += token.text.size();
		}

		Uint32 from = text.find_first_not_of( ' ' );
		if ( from != String::InvalidPos )
			mTextBox->setFontFillColor( pp->getLineNumColor(), from,
										text.find_first_of( ' ', from ) );
	}
	return this;
}
