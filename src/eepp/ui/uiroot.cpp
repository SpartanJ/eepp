#include <eepp/ui/uiroot.hpp>

namespace EE { namespace UI {

UIRoot* UIRoot::New() {
	return eeNew( UIRoot, () );
}

UIRoot::UIRoot() : UIWidget( ":root" ) {}

Node* UIRoot::overFind( const Vector2f& point ) {
	if ( !mHasChildHitTestTraversalPixelsSize )
		return UIWidget::overFind( point );

	Node* pOver = nullptr;

	if ( ( mNodeFlags & NODE_FLAG_OVER_FIND_ALLOWED ) && mEnabled && mVisible ) {
		updateWorldPolygon();

		const bool selfHit = mWorldBounds.contains( point ) && mPoly.pointInside( point );
		if ( selfHit || childHitTestTraversalContains( point ) ) {
			Node* child = getLastChild();
			while ( nullptr != child ) {
				Node* childOver = child->overFind( point );
				if ( nullptr != childOver ) {
					pOver = childOver;
					break;
				}
				child = child->getPrevNode();
			}

			if ( nullptr != pOver || selfHit ) {
				writeNodeFlag( NODE_FLAG_MOUSEOVER_ME_OR_CHILD, 1 );
				mSceneNode->addMouseOverNode( this );
			}

			if ( nullptr == pOver && selfHit )
				pOver = this;
		}
	}

	return pOver;
}

void UIRoot::setChildHitTestTraversalPixelsSize( const Sizef& size ) {
	if ( mHasChildHitTestTraversalPixelsSize && mChildHitTestTraversalPixelsSize == size )
		return;

	mChildHitTestTraversalPixelsSize = size;
	mHasChildHitTestTraversalPixelsSize = true;
}

void UIRoot::clearChildHitTestTraversalPixelsSize() {
	if ( !mHasChildHitTestTraversalPixelsSize )
		return;

	mChildHitTestTraversalPixelsSize = Sizef::Zero;
	mHasChildHitTestTraversalPixelsSize = false;
}

bool UIRoot::hasChildHitTestTraversalPixelsSize() const {
	return mHasChildHitTestTraversalPixelsSize;
}

const Sizef& UIRoot::getChildHitTestTraversalPixelsSize() const {
	return mChildHitTestTraversalPixelsSize;
}

bool UIRoot::childHitTestTraversalContains( const Vector2f& point ) {
	updateWorldPolygon();

	Polygon2f traversalPoly = Polygon2f( Rectf(
		mScreenPos.x, mScreenPos.y, mScreenPos.x + mChildHitTestTraversalPixelsSize.getWidth(),
		mScreenPos.y + mChildHitTestTraversalPixelsSize.getHeight() ) );
	traversalPoly.rotate( getRotation(), getRotationCenter() );
	traversalPoly.scale( getScale(), getScaleCenter() );

	Node* parent = getParent();
	while ( parent ) {
		traversalPoly.rotate( parent->getRotation(), parent->getRotationCenter() );
		traversalPoly.scale( parent->getScale(), parent->getScaleCenter() );
		parent = parent->getParent();
	}

	const Rectf traversalWorldBounds = traversalPoly.getBounds();
	return traversalWorldBounds.contains( point ) && traversalPoly.pointInside( point );
}

std::string UIRoot::getPropertyString( const PropertyDefinition* propertyDef,
									   const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::DroppableHoveringColor:
			return mDroppableHoveringColor.toHexString();
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

bool UIRoot::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::DroppableHoveringColor:
			mDroppableHoveringColor = attribute.asColor();
			break;
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

std::vector<PropertyId> UIRoot::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	props.push_back( PropertyId::DroppableHoveringColor );
	return props;
}

}} // namespace EE::UI
