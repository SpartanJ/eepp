#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UIWidget * UIWidget::New() {
	return eeNew( UIWidget, () );
}

UIWidget::UIWidget() :
	UIControlAnim(),
	mTooltip( NULL ),
	mMinControlSize(),
	mLayoutWeight(0),
	mLayoutGravity(0),
	mLayoutWidthRules(WRAP_CONTENT),
	mLayoutHeightRules(WRAP_CONTENT),
	mLayoutPositionRule(LayoutPositionRules::NONE),
	mLayoutPositionRuleWidget(NULL)
{
	mControlFlags |= UI_CTRL_FLAG_COMPLEX;

	updateAnchorsDistances();
}

UIWidget::~UIWidget() {
	eeSAFE_DELETE( mTooltip );
}

Uint32 UIWidget::getType() const {
	return UI_TYPE_WIDGET;
}

bool UIWidget::isType( const Uint32& type ) const {
	return UIWidget::getType() == type ? true : UIControlAnim::isType( type );
}

void UIWidget::updateAnchorsDistances() {
	if ( NULL != mParentCtrl ) {
		mDistToBorder	= Recti( mRealPos.x, mRealPos.y, mParentCtrl->getRealSize().x - ( mRealPos.x + mRealSize.x ), mParentCtrl->getRealSize().y - ( mRealPos.y + mRealSize.y ) );
	}
}

Recti UIWidget::getLayoutMargin() const {
	return mLayoutMargin;
}

UIWidget * UIWidget::setLayoutMargin(const Recti & margin) {
	mLayoutMargin = margin;
	mRealMargin = PixelDensity::dpToPxI( margin );
	return this;
}

Float UIWidget::getLayoutWeight() const {
	return mLayoutWeight;
}

UIWidget * UIWidget::setLayoutWeight(const Float & weight) {
	mLayoutWeight = weight;
	return this;
}

Uint32 UIWidget::getLayoutGravity() const {
	return mLayoutGravity;
}

UIWidget * UIWidget::setLayoutGravity(const Uint32 & layoutGravity) {
	mLayoutGravity = layoutGravity;
	return this;
}

LayoutSizeRules UIWidget::getLayoutWidthRules() const {
	return mLayoutWidthRules;
}

UIWidget * UIWidget::setLayoutWidthRules(const LayoutSizeRules & layoutWidthRules) {
	mLayoutWidthRules = layoutWidthRules;
	return this;
}

LayoutSizeRules UIWidget::getLayoutHeightRules() const {
	return mLayoutHeightRules;
}

UIWidget * UIWidget::setLayoutHeightRules(const LayoutSizeRules & layoutHeightRules) {
	mLayoutHeightRules = layoutHeightRules;
	return this;
}

UIWidget * UIWidget::setLayoutSizeRules(const LayoutSizeRules & layoutWidthRules, const LayoutSizeRules & layoutHeightRules) {
	mLayoutWidthRules = layoutWidthRules;
	mLayoutHeightRules = layoutHeightRules;
	return this;
}

UIWidget * UIWidget::setLayoutPositionRule(const LayoutPositionRules & layoutPositionRule, UIWidget * of) {
	mLayoutPositionRule = layoutPositionRule;
	mLayoutPositionRuleWidget  = of;
	return this;
}

UIWidget * UIWidget::getLayoutPositionRuleWidget() const {
	return mLayoutPositionRuleWidget;
}

LayoutPositionRules UIWidget::getLayoutPositionRule() const {
	return mLayoutPositionRule;
}

void UIWidget::update() {
	if ( mVisible && NULL != mTooltip && mTooltip->getText().size() ) {
		if ( isMouseOverMeOrChilds() ) {
			UIManager * uiManager = UIManager::instance();
			UIThemeManager * themeManager = UIThemeManager::instance();

			Vector2i Pos = uiManager->getMousePos();
			Pos.x += themeManager->getCursorSize().x;
			Pos.y += themeManager->getCursorSize().y;

			if ( Pos.x + mTooltip->getRealSize().getWidth() > uiManager->getMainControl()->getRealSize().getWidth() ) {
				Pos.x = uiManager->getMousePos().x - mTooltip->getRealSize().getWidth();
			}

			if ( Pos.y + mTooltip->getRealSize().getHeight() > uiManager->getMainControl()->getRealSize().getHeight() ) {
				Pos.y = uiManager->getMousePos().y - mTooltip->getRealSize().getHeight();
			}

			if ( Time::Zero == themeManager->getTooltipTimeToShow() ) {
				if ( !mTooltip->isVisible() || themeManager->getTooltipFollowMouse() )
					mTooltip->setPosition( PixelDensity::pxToDpI( Pos ) );

				mTooltip->show();
			} else {
				if ( -1.f != mTooltip->getTooltipTime().asMilliseconds() ) {
					mTooltip->addTooltipTime( getElapsed() );
				}

				if ( mTooltip->getTooltipTime() >= themeManager->getTooltipTimeToShow() ) {
					if ( mTooltip->getTooltipTime().asMilliseconds() != -1.f ) {
						mTooltip->setPosition( PixelDensity::pxToDpI( Pos ) );

						mTooltip->show();

						mTooltip->setTooltipTime( Milliseconds( -1.f ) );
					}
				}
			}

			if ( themeManager->getTooltipFollowMouse() ) {
				mTooltip->setPosition( PixelDensity::pxToDpI( Pos ) );
			}
		} else {
			mTooltip->setTooltipTime( Milliseconds( 0.f ) );

			if ( mTooltip->isVisible() )
				mTooltip->hide();
		}
	}

	UIControlAnim::update();
}

void UIWidget::createTooltip() {
	if ( NULL != mTooltip )
		return;

	mTooltip = UITooltip::New();
	mTooltip->setVisible( false )->setEnabled( false );
	mTooltip->setTooltipOf( this );
}

UIWidget * UIWidget::setTooltipText( const String& Text ) {
	if ( NULL == mTooltip ) {	// If the tooltip wasn't created it will avoid to create a new one if the string is ""
		if ( Text.size() ) {
			createTooltip();

			mTooltip->setText( Text );
		}
	} else { // but if it's created, i will allow it
		mTooltip->setText( Text );
	}

	return this;
}

String UIWidget::getTooltipText() {
	if ( NULL != mTooltip )
		return mTooltip->getText();

	return String();
}

void UIWidget::tooltipRemove() {
	mTooltip = NULL;
}

UIControl * UIWidget::setSize( const Sizei& size ) {
	Sizei s( size );

	if ( s.x < mMinControlSize.x )
		s.x = mMinControlSize.x;

	if ( s.y < mMinControlSize.y )
		s.y = mMinControlSize.y;

	return UIControlAnim::setSize( s );
}

UIControl * UIWidget::setFlags(const Uint32 & flags) {
	if ( flags & ( UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM ) ) {
		updateAnchorsDistances();
	}

	if ( flags & UI_AUTO_SIZE ) {
		onAutoSize();
	}

	return UIControlAnim::setFlags( flags );
}

UIControl * UIWidget::unsetFlags(const Uint32 & flags) {
	if ( flags & ( UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM ) ) {
		updateAnchorsDistances();
	}

	return UIControlAnim::unsetFlags( flags );
}

UIWidget * UIWidget::setAnchors(const Uint32 & flags) {
	mFlags &= ~(UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM);
	mFlags |= flags;
	updateAnchorsDistances();
	return this;
}

UIControl * UIWidget::setSize( const Int32& Width, const Int32& Height ) {
	return UIControlAnim::setSize( Width, Height );
}

const Sizei& UIWidget::getSize() {
	return UIControlAnim::getSize();
}

void UIWidget::onParentSizeChange( const Vector2i& SizeChange ) {
	updateAnchors( SizeChange );
	UIControlAnim::onParentSizeChange( SizeChange );
}

void UIWidget::onPositionChange() {
	updateAnchorsDistances();
	UIControlAnim::onPositionChange();
}

void UIWidget::onAutoSize() {
}

void UIWidget::updateAnchors( const Vector2i& SizeChange ) {
	if ( !( mFlags & ( UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM ) ) )
		return;

	Sizei newSize( mSize );

	if ( !( mFlags & UI_ANCHOR_LEFT ) ) {
		setInternalPosition( Vector2i( mPos.x += SizeChange.x, mPos.y ) );
	}

	if ( mFlags & UI_ANCHOR_RIGHT ) {
		if ( NULL != mParentCtrl ) {
			newSize.x = mParentCtrl->getSize().getWidth() - mPos.x - PixelDensity::pxToDpI( mDistToBorder.Right );

			if ( newSize.x < mMinControlSize.getWidth() )
				newSize.x = mMinControlSize.getWidth();
		}
	}

	if ( !( mFlags & UI_ANCHOR_TOP ) ) {
		setInternalPosition( Vector2i( mPos.x, mPos.y += SizeChange.y ) );
	}

	if ( mFlags & UI_ANCHOR_BOTTOM ) {
		if ( NULL != mParentCtrl ) {
			newSize.y = mParentCtrl->getSize().y - mPos.y - PixelDensity::pxToDpI( mDistToBorder.Bottom );

			if ( newSize.y < mMinControlSize.getHeight() )
				newSize.y = mMinControlSize.getHeight();
		}
	}

	if ( newSize != mSize )
		setSize( newSize );
}

void UIWidget::alignAgainstLayout() {
	Vector2i pos = mPos;

	switch ( fontHAlignGet( mLayoutGravity ) ) {
		case UI_HALIGN_CENTER:
			pos.x = ( getParent()->getSize().getWidth() - mSize.getWidth() ) / 2;
			break;
		case UI_HALIGN_RIGHT:
			pos.x = getParent()->getSize().getWidth() - mLayoutMargin.Right;
			break;
		case UI_HALIGN_LEFT:
			pos.x = mLayoutMargin.Left;
			break;
	}

	switch ( fontVAlignGet( mLayoutGravity ) ) {
		case UI_VALIGN_CENTER:
			pos.y = ( getParent()->getSize().getHeight() - mSize.getHeight() ) / 2;
			break;
		case UI_VALIGN_BOTTOM:
			pos.y = getParent()->getSize().getHeight() - mLayoutMargin.Bottom;
			break;
		case UI_VALIGN_TOP:
			pos.y = mLayoutMargin.Top;
			break;
	}

	setInternalPosition( pos );
}

}}
