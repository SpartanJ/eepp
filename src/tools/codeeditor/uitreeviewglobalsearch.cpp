#include "uitreeviewglobalsearch.hpp"
#include <eepp/graphics/primitives.hpp>
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
		updateText( text + '\n' );
		updateLayout();
	}
	return this;
}

UIPushButton* UITreeViewCellGlobalSearch::updateText( const std::string& text ) {
	if ( getCurIndex().internalId() != -1 ) {
		UITreeViewGlobalSearch* pp = getParent()->getParent()->asType<UITreeViewGlobalSearch>();

		ProjectSearch::ResultData* res = (ProjectSearch::ResultData*)getCurIndex().parent().data();

		auto styleDef = SyntaxDefinitionManager::instance()->getStyleByExtension( res->file );

		Uint32 from = text.find_first_not_of( ' ' );
		Uint32 to = from;
		if ( from != String::InvalidPos ) {
			to = text.find_first_of( ' ', from );
			mTextBox->setFontFillColor( pp->getLineNumColor(), from, to );
		}

		Int64 iniPos = 0;
		Int64 endPos = 0;
		Model* model = pp->getModel();
		ModelIndex curIndex = getCurIndex();
		ModelIndex indexStart = model->index(
			curIndex.row(), ProjectSearch::ResultModel::ColumnStart, curIndex.parent() );
		ModelIndex indexEnd = model->index( curIndex.row(), ProjectSearch::ResultModel::ColumnEnd,
											curIndex.parent() );
		Variant variantStart = model->data( indexStart, Model::Role::Custom );
		Variant variantEnd = model->data( indexEnd, Model::Role::Custom );
		iniPos = variantStart.asInt64();
		endPos = variantEnd.asInt64();
		mSearchStrPos = { iniPos + 12, endPos + 12 };
		const String& txt = mTextBox->getText();

		if ( mSearchStrPos.second < txt.size() ) {
			mResultStr =
				txt.substr( mSearchStrPos.first, mSearchStrPos.second - mSearchStrPos.first );
		} else {
			mResultStr = "";
		}

		auto tokens =
			SyntaxTokenizer::tokenize( styleDef, text, SYNTAX_TOKENIZER_STATE_NONE, to ).first;

		size_t start = to;
		for ( auto& token : tokens ) {
			mTextBox->setFontFillColor( pp->getColorScheme().getSyntaxStyle( token.type ).color,
										start, start + token.text.size() );
			start += token.text.size();
		}
	}
	return this;
}

static Float getTextWidth( Float glyphWidth, Uint32 tabWidth, const String& line ) {
	size_t len = line.length();
	Float x = 0;
	for ( size_t i = 0; i < len; i++ )
		x += ( line[i] == '\t' ) ? glyphWidth * tabWidth : glyphWidth;
	return x;
}

static Float getXOffsetCol( Float glyphWidth, Uint32 tabWidth, const String& line, Int64 col ) {
	Float x = 0;
	for ( auto i = 0; i < col; i++ ) {
		if ( line[i] == '\t' ) {
			x += glyphWidth * tabWidth;
		} else if ( line[i] != '\n' && line[i] != '\r' ) {
			x += glyphWidth;
		}
	}
	return x;
}

void UITreeViewCellGlobalSearch::draw() {
	UITreeViewCell::draw();
	if ( getCurIndex().internalId() != -1 ) {
		UITreeViewGlobalSearch* pp = getParent()->getParent()->asType<UITreeViewGlobalSearch>();
		if ( mSearchStrPos.first != std::string::npos && mSearchStrPos.second > 0 ) {
			auto hspace =
				mTextBox->getFont()->getGlyph( L' ', mTextBox->getPixelsFontSize(), false ).advance;
			Primitives p;
			p.setColor( pp->getColorScheme().getEditorSyntaxStyle( "selection" ).color );
			p.drawRectangle( Rectf(
				{ mScreenPos.x + mTextBox->getPixelsPosition().x +
					  getXOffsetCol( hspace, mTextBox->getTextCache()->getTabWidth(),
									 mTextBox->getText(), mSearchStrPos.first ),
				  mScreenPos.y + mTextBox->getPixelsPosition().y },
				Sizef( getTextWidth( hspace, mTextBox->getTextCache()->getTabWidth(), mResultStr ),
					   mTextBox->getPixelsSize().getHeight() ) ) );
		}
	}
}
