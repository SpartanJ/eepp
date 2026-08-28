#include "githistorytreeview.hpp"
#include "githistorymodel.hpp"
#include <eepp/ui/uistyle.hpp>

namespace ecode {

GitHistoryTreeViewCell::GitHistoryTreeViewCell() :
	UITreeViewCell(),
	mHintColor( Color::fromString(
		mUISceneNode->getRoot()->getUIStyle()->getVariable( "--font-hint" ).getValue() ) ) {
	mTextBox->setTextOverflow( "ellipsis" );
}

Sizef GitHistoryTreeViewCell::updateLayout() {
	Sizef size = UITreeViewCell::updateLayout();
	if ( mTextBox )
		mTextBox->setPixelsPosition(
			Vector2f{ mTextBox->getPixelsPosition().x, PixelDensity::dpToPx( 2 ) }.trunc() );
	return size;
}

void GitHistoryTreeViewCell::updateCell( Model* model ) {
	auto* historyModel = static_cast<GitHistoryModel*>( model );
	const auto* item = historyModel->node( getCurIndex() );
	if ( !item || item->type != GitHistoryModel::NodeType::Commit ) {
		mMetadataText.setString( "" );
		mTextBox->setTextAlign( UI_HALIGN_LEFT | UI_VALIGN_CENTER );
		return;
	}

	mTextBox->setTextAlign( UI_HALIGN_LEFT | UI_VALIGN_TOP );
	mMetadataText.setFont( mTextBox->getFont() );
	mMetadataText.setFontSize(
		eemax<Uint32>( 8, mTextBox->getFontSize() > 2 ? mTextBox->getFontSize() - 2 : 8 ) );
	mMetadataText.setString( item->message );
	mMetadataText.setFillColor( mHintColor );
	mMetadataText.setTextHints( mTextBox->getTextHints() );
}

void GitHistoryTreeViewCell::draw() {
	UITreeViewCell::draw();

	if ( mMetadataText.getString().empty() )
		return;

	const bool selected = getParent() && ( getParent()->asType<UIWidget>()->getStyleState() &
										   UIState::StateFlagSelected );

	mMetadataText.setFillColor( selected ? mTextBox->getFontColor() : mHintColor );
	mMetadataText.draw( std::floor( mScreenPos.x + mTextBox->getPixelsPosition().x ),
						std::floor( mScreenPos.y + PixelDensity::dpToPx( 19 ) ) );
}

UIWidget* GitHistoryTreeView::createCell( UIWidget* rowWidget, const ModelIndex& index ) {
	UITableCell* widget = index.column() == static_cast<Int64>( getModel()->treeColumn() )
							  ? GitHistoryTreeViewCell::New()
							  : UITableCell::New( mTag + "::cell" );
	return setupCell( widget, rowWidget, index );
}

} // namespace ecode
