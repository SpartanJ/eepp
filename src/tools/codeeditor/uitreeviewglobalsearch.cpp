#include "uitreeviewglobalsearch.hpp"
#include "projectsearch.hpp"
#include <eepp/graphics/primitives.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/doc/syntaxtokenizer.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>

UITreeViewGlobalSearch::UITreeViewGlobalSearch( const SyntaxColorScheme& colorScheme,
												bool searchReplace ) :
	UITreeView(), mColorScheme( colorScheme ), mSearchReplace( searchReplace ) {
	mLineNumColor = Color::fromString(
		mUISceneNode->getRoot()->getUIStyle()->getVariable( "--font-hint" ).getValue() );
}

UIWidget* UITreeViewGlobalSearch::createCell( UIWidget* rowWidget, const ModelIndex& index ) {
	UITableCell* widget = index.column() == (Int64)getModel()->treeColumn()
							  ? UITreeViewCellGlobalSearch::New( mSearchReplace )
							  : UITableCell::New();
	return setupCell( widget, rowWidget, index );
}

Uint32 UITreeViewGlobalSearch::onKeyDown( const KeyEvent& event ) {
	auto curIndex = getSelection().first();
	switch ( event.getKeyCode() ) {
		case KEY_SPACE: {
			if ( curIndex.isValid() )
				onOpenModelIndex( curIndex, &event );
			return 0;
		}
		default:
			break;
	}
	return UITreeView::onKeyDown( event );
}

void UITreeViewCellGlobalSearch::toggleSelected() {
	UICheckBox* chk = mTextBox->asType<UICheckBox>();
	if ( getCurIndex().internalId() != -1 ) {
		ProjectSearch::ResultData::Result* result = getResultPtr();
		if ( result ) {
			result->selected = !result->selected;
			chk->setChecked( result->selected );

			ProjectSearch::ResultData* parentData =
				(ProjectSearch::ResultData*)getDataPtr( getCurIndex().parent() );
			if ( parentData )
				parentData->selected = parentData->allResultsSelected();
		}
	} else {
		auto* result = getResultDataPtr();
		result->setResultsSelected( !result->selected );
		chk->setChecked( result->selected );
	}
}

std::function<UITextView*()> UITreeViewCellGlobalSearch::getCheckBoxFn() {
	return [&]() -> UITextView* {
		UICheckBox* chk = UICheckBox::New();
		addEventListener( Event::MouseClick, [&, chk]( const Event* event ) {
			const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
			Vector2f pos = convertToNodeSpace( mouseEvent->getPosition().asFloat() );
			Rectf box( { convertToNodeSpace( chk->getCurrentButton()->convertToWorldSpace(
							 chk->getCurrentButton()->getPixelsPosition() ) ),
						 chk->getCurrentButton()->getPixelsSize() } );
			if ( box.contains( pos ) )
				toggleSelected();
			return 1;
		} );
		return chk;
	};
}

void* UITreeViewCellGlobalSearch::getDataPtr( const ModelIndex& modelIndex ) {
	const ProjectSearch::ResultModel* model =
		static_cast<const ProjectSearch::ResultModel*>( modelIndex.model() );
	ModelIndex index =
		model->index( modelIndex.row(), ProjectSearch::ResultModel::Data, modelIndex.parent() );
	Variant var( model->data( index, Model::Role::Custom ) );
	if ( var.is( Variant::Type::DataPtr ) )
		return var.asDataPtr();
	return nullptr;
}

ProjectSearch::ResultData::Result* UITreeViewCellGlobalSearch::getResultPtr() {
	if ( getCurIndex().internalId() == -1 )
		return nullptr;
	void* ptr = getDataPtr( getCurIndex() );
	if ( ptr )
		return (ProjectSearch::ResultData::Result*)ptr;
	return nullptr;
}

ProjectSearch::ResultData* UITreeViewCellGlobalSearch::getResultDataPtr() {
	if ( getCurIndex().internalId() != -1 )
		return nullptr;
	void* ptr = getDataPtr( getCurIndex() );
	if ( ptr )
		return (ProjectSearch::ResultData*)ptr;
	return nullptr;
}

UITreeViewCellGlobalSearch::UITreeViewCellGlobalSearch( bool selectionEnabled ) :
	UITreeViewCell( selectionEnabled ? getCheckBoxFn() : nullptr ) {}

UIPushButton* UITreeViewCellGlobalSearch::setText( const String& text ) {
	auto* result = getResultPtr();
	if ( text != mTextBox->getText() ||
		 ( result && std::make_pair<size_t, size_t>( result->position.start().column() + 12,
													 result->position.end().column() + 12 ) !=
						 mSearchStrPos ) ) {
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

		auto* result = getResultPtr();
		mSearchStrPos = { result->position.start().column() + 12,
						  result->position.end().column() + 12 };

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
			Vector2f screenPos( mScreenPos );
			if ( mTextBox->isType( UI_TYPE_CHECKBOX ) ) {
				UICheckBox* chk = mTextBox->asType<UICheckBox>();
				screenPos.x += chk->getRealAlignOffset().x;
			}
			p.drawRectangle( Rectf(
				{ screenPos.x + mTextBox->getPixelsPosition().x +
					  getXOffsetCol( hspace, mTextBox->getTextCache()->getTabWidth(),
									 mTextBox->getText(), mSearchStrPos.first ),
				  screenPos.y + mTextBox->getPixelsPosition().y },
				Sizef( getTextWidth( hspace, mTextBox->getTextCache()->getTabWidth(), mResultStr ),
					   mTextBox->getPixelsSize().getHeight() ) ) );
		}
	}
}

void UITreeViewCellGlobalSearch::updateCell( Model* ) {
	if ( mTextBox->isType( UI_TYPE_CHECKBOX ) ) {
		UICheckBox* chk = mTextBox->asType<UICheckBox>();
		auto* result = getResultPtr();
		if ( result ) {
			chk->setChecked( result->selected );
		} else if ( auto* dataResult = getResultDataPtr() ) {
			chk->setChecked( dataResult->selected );
		}
	}
}
