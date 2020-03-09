#include <eepp/core.hpp>
#include <eepp/graphics/renderer/opengl.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/vertexbuffer.hpp>
#include <eepp/ui/uiborderdrawable.hpp>

namespace EE { namespace UI {

static void borderAddArc( VertexBuffer* vbo, Vector2f pos, Float radiW, Float radiH,
						  double arcStartAngle, double arcEndAngle, Color color, Float lineW,
						  Vector2f basePos, bool decrease = false ) {
	Float angleShift = 1;
	double startAngle = eemin( arcStartAngle, arcEndAngle );
	double endAngle = eemax( arcStartAngle, arcEndAngle );
	Vector2f startPos = ( radiW > lineW ) ? pos : basePos;
	double lastAngle = startAngle;
	double i;

	if ( decrease ) {
		for ( i = endAngle; i >= startAngle; i -= angleShift ) {
			vbo->addVertex(
				Vector2f( pos.x + radiW * Math::cosAng( i ), pos.y + radiH * Math::sinAng( i ) ) );
			vbo->addColor( color );

			if ( radiW > lineW ) {
				vbo->addVertex( Vector2f( pos.x + ( radiW - lineW ) * Math::cosAng( i ),
										  pos.y + ( radiH - lineW ) * Math::sinAng( i ) ) );
			} else {
				vbo->addVertex( startPos );
			}

			vbo->addColor( color );
			lastAngle = i;
		}

		if ( lastAngle != endAngle ) {
			i = endAngle;
			vbo->addVertex(
				Vector2f( pos.x + radiW * Math::cosAng( i ), pos.y + radiH * Math::sinAng( i ) ) );
			vbo->addColor( color );

			if ( radiW > lineW ) {
				vbo->addVertex( Vector2f( pos.x + ( radiW - lineW ) * Math::cosAng( i ),
										  pos.y + ( radiH - lineW ) * Math::sinAng( i ) ) );
			} else {
				vbo->addVertex( startPos );
			}

			vbo->addColor( color );
		}
	} else {
		for ( i = startAngle; i <= endAngle; i += angleShift ) {
			if ( radiW > lineW ) {
				vbo->addVertex( Vector2f( pos.x + ( radiW - lineW ) * Math::cosAng( i ),
										  pos.y + ( radiH - lineW ) * Math::sinAng( i ) ) );
			} else {
				vbo->addVertex( startPos );
			}

			vbo->addColor( color );

			vbo->addVertex(
				Vector2f( pos.x + radiW * Math::cosAng( i ), pos.y + radiH * Math::sinAng( i ) ) );
			vbo->addColor( color );

			lastAngle = i;
		}

		if ( lastAngle != endAngle ) {
			i = endAngle;

			if ( radiW > lineW ) {
				vbo->addVertex( Vector2f( pos.x + ( radiW - lineW ) * Math::cosAng( i ),
										  pos.y + ( radiH - lineW ) * Math::sinAng( i ) ) );
			} else {
				vbo->addVertex( startPos );
			}

			vbo->addColor( color );

			vbo->addVertex(
				Vector2f( pos.x + radiW * Math::cosAng( i ), pos.y + radiH * Math::sinAng( i ) ) );
			vbo->addColor( color );
		}
	}
}

void UIBorderDrawable::createBorders( VertexBuffer* vbo, const UIBorderDrawable::Borders& borders,
									  const Vector2f& pos, const Sizef& size ) {
	vbo->clear();

	int borderTop = 0;
	int borderBottom = 0;
	int borderLeft = 0;
	int borderRight = 0;
	Float halfWidth = size.getWidth() * 0.5f;
	Float halfHeight = size.getHeight() * 0.5f;

	if ( borders.top.width >= 0 ) {
		borderTop = eemin( (int)( size.getHeight() * 0.5f ), (int)borders.top.width );
	}

	if ( borders.bottom.width >= 0 ) {
		borderBottom = eemin( (int)( size.getHeight() * 0.5f ), (int)borders.bottom.width );
	}

	if ( borders.left.width >= 0 ) {
		borderLeft = eemin( (int)( size.getWidth() * 0.5f ), (int)borders.left.width );
	}

	if ( borders.right.width >= 0 ) {
		borderRight = eemin( (int)( size.getWidth() * 0.5f ), (int)borders.right.width );
	}

	// draw top border
	if ( borderTop ) {
		double leftW = eemin( halfWidth, eemax( 0.f, borders.radius.topLeftX ) );
		double rightW = eemin( halfHeight, eemax( 0.f, borders.radius.topRightX ) );
		double leftH = eemin( halfWidth, eemax( 0.f, borders.radius.topLeftY ) );
		double rightH = eemin( halfHeight, eemax( 0.f, borders.radius.topRightY ) );

		if ( leftW ) {
			double endAngle = 270;
			double startAngle = 225;

			borderAddArc( vbo, Vector2f( pos.x + leftW, pos.y + leftH ), leftW, leftH, startAngle,
						  endAngle, borders.top.color, borderTop,
						  Vector2f( pos.x + borderLeft, pos.y + borderTop ) );
		} else {
			vbo->addVertex( Vector2f( pos.x, pos.y ) );
			vbo->addColor( borders.top.color );
			vbo->addVertex( Vector2f( pos.x + borderLeft, pos.y + borderTop ) );
			vbo->addColor( borders.top.color );
		}

		if ( rightW ) {
			double startAngle = 270;
			double endAngle = 315;
			Vector2f basePos( pos.x + size.getWidth() - borderRight, pos.y + borderTop );
			Vector2f tPos( pos.x + size.getWidth() - rightW, pos.y + rightH );

			if ( rightW > borderTop ) {
				vbo->addVertex(
					Vector2f( tPos.x + ( rightW - borderTop ) * Math::cosAng( startAngle ),
							  tPos.y + ( rightH - borderTop ) * Math::sinAng( startAngle ) ) );
			} else {
				vbo->addVertex( basePos );
			}

			vbo->addColor( borders.top.color );

			vbo->addVertex( Vector2f( pos.x + size.getWidth() - rightW, pos.y ) );
			vbo->addColor( borders.top.color );

			borderAddArc( vbo, tPos, rightW, rightH, startAngle, endAngle, borders.top.color,
						  borderTop, basePos );
		} else {
			vbo->addVertex( Vector2f( pos.x + size.getWidth(), pos.y ) );
			vbo->addColor( borders.top.color );
			vbo->addVertex( Vector2f( pos.x + size.getWidth() - borderRight, pos.y + borderTop ) );
			vbo->addColor( borders.top.color );
		}
	}

	// draw right border
	if ( borderRight ) {
		double topW = eemin( halfWidth, eemax( 0.f, borders.radius.topRightX ) );
		double bottomW = eemin( halfHeight, eemax( 0.f, borders.radius.bottomRightX ) );
		double topH = eemin( halfWidth, eemax( 0.f, borders.radius.topRightY ) );
		double bottomH = eemin( halfHeight, eemax( 0.f, borders.radius.bottomRightY ) );

		if ( topW ) {
			double startAngle = 315;
			double endAngle = 360;
			Vector2f basePos( pos.x + size.getWidth() - borderRight, pos.y + borderTop );

			borderAddArc( vbo, Vector2f( pos.x + size.getWidth() - topW, pos.y + topH ), topW, topH,
						  startAngle, endAngle, borders.right.color, borderRight, basePos );
		} else {
			vbo->addVertex( Vector2f( pos.x + size.getWidth(), pos.y ) );
			vbo->addColor( borders.right.color );
			vbo->addVertex( Vector2f( pos.x + size.getWidth() - borderRight, pos.y + borderTop ) );
			vbo->addColor( borders.right.color );
		}

		if ( bottomH ) {
			double startAngle = 0;
			double endAngle = 45;
			Vector2f basePos( pos.x + size.getWidth() - borderRight,
							  pos.y + size.getHeight() - borderBottom );
			Vector2f tPos( pos.x + size.getWidth() - bottomW, pos.y + size.getHeight() - bottomH );

			if ( bottomW > borderRight ) {
				vbo->addVertex(
					Vector2f( tPos.x + ( bottomW - borderRight ) * Math::cosAng( startAngle ),
							  tPos.y + ( bottomH - borderRight ) * Math::sinAng( startAngle ) ) );
			} else {
				vbo->addVertex( basePos );
			}
			vbo->addColor( borders.right.color );

			vbo->addVertex(
				Vector2f( pos.x + size.getWidth(), pos.y + size.getHeight() - bottomH ) );
			vbo->addColor( borders.right.color );

			borderAddArc( vbo, tPos, bottomW, bottomH, startAngle, endAngle, borders.right.color,
						  borderRight, basePos );

		} else {
			vbo->addVertex( Vector2f( pos.x + size.getWidth(), pos.y + size.getHeight() ) );
			vbo->addColor( borders.right.color );
			vbo->addVertex( Vector2f( pos.x + size.getWidth() - borderRight,
									  pos.y + size.getHeight() - borderBottom ) );
			vbo->addColor( borders.right.color );
		}
	}

	// draw bottom border
	if ( borderBottom ) {
		double leftW = eemin( halfWidth, eemax( 0.f, borders.radius.bottomLeftX ) );
		double rightW = eemin( halfHeight, eemax( 0.f, borders.radius.bottomRightX ) );
		double leftH = eemin( halfWidth, eemax( 0.f, borders.radius.bottomLeftY ) );
		double rightH = eemin( halfHeight, eemax( 0.f, borders.radius.bottomRightY ) );

		if ( rightW ) {
			double startAngle = 45;
			double endAngle = 90;
			Vector2f basePos( pos.x + size.getWidth() - borderRight,
							  pos.y + size.getHeight() - borderBottom );

			borderAddArc(
				vbo,
				Vector2f( pos.x + size.getWidth() - rightW, pos.y + size.getHeight() - rightH ),
				rightW, rightH, startAngle, endAngle, borders.bottom.color, borderBottom, basePos );
		} else {
			vbo->addVertex( Vector2f( pos.x + size.getWidth(), pos.y + size.getHeight() ) );
			vbo->addColor( borders.bottom.color );

			vbo->addVertex( Vector2f( pos.x + size.getWidth() - borderRight,
									  pos.y + size.getHeight() - borderBottom ) );
			vbo->addColor( borders.bottom.color );
		}

		if ( leftW ) {
			double startAngle = 90;
			double endAngle = 135;
			Vector2f basePos( pos.x + borderLeft, pos.y + size.getHeight() - borderBottom );
			Vector2f tPos( Vector2f( pos.x + leftW, pos.y + size.getHeight() - leftH ) );

			if ( leftW > borderBottom ) {
				vbo->addVertex(
					Vector2f( tPos.x + ( leftW - borderBottom ) * Math::cosAng( startAngle ),
							  tPos.y + ( leftH - borderBottom ) * Math::sinAng( startAngle ) ) );
			} else {
				vbo->addVertex( basePos );
			}

			vbo->addColor( borders.bottom.color );

			vbo->addVertex( Vector2f( tPos.x + leftW * Math::cosAng( startAngle ),
									  tPos.y + leftH * Math::sinAng( startAngle ) ) );
			vbo->addColor( borders.bottom.color );

			borderAddArc( vbo, tPos, leftW, leftH, startAngle, endAngle, borders.bottom.color,
						  borderBottom, basePos );
		} else {
			vbo->addVertex( Vector2f( pos.x, pos.y + size.getHeight() ) );
			vbo->addColor( borders.bottom.color );
			vbo->addVertex(
				Vector2f( pos.x + borderLeft, pos.y + size.getHeight() - borderBottom ) );
			vbo->addColor( borders.bottom.color );
		}
	}

	// draw left border
	if ( borderLeft ) {
		double topW = eemin( halfWidth, eemax( 0.f, borders.radius.topLeftX ) );
		double bottomW = eemin( halfHeight, eemax( 0.f, borders.radius.bottomLeftX ) );
		double topH = eemin( halfWidth, eemax( 0.f, borders.radius.topLeftY ) );
		double bottomH = eemin( halfHeight, eemax( 0.f, borders.radius.bottomLeftY ) );

		if ( bottomW ) {
			double startAngle = 135;
			double endAngle = 180;
			Vector2f basePos( pos.x + borderLeft, pos.y + size.getHeight() - borderBottom );

			borderAddArc( vbo, Vector2f( pos.x + bottomW, pos.y + size.getHeight() - bottomH ),
						  bottomW, bottomH, startAngle, endAngle, borders.left.color, borderLeft,
						  basePos );
		} else {
			vbo->addVertex( Vector2f( pos.x, pos.y + size.getHeight() ) );
			vbo->addColor( borders.left.color );
			vbo->addVertex(
				Vector2f( pos.x + borderLeft, pos.y + size.getHeight() - borderBottom ) );
			vbo->addColor( borders.left.color );
		}

		if ( topW ) {
			double startAngle = 180;
			double endAngle = 225;
			Vector2f basePos( pos.x + borderLeft, pos.y + borderTop );
			Vector2f tPos( pos.x + topW, pos.y + topW );

			if ( topW > borderLeft ) {
				vbo->addVertex(
					Vector2f( tPos.x + ( topW - borderLeft ) * Math::cosAng( startAngle ),
							  tPos.y + ( topH - borderLeft ) * Math::sinAng( startAngle ) ) );
			} else {
				vbo->addVertex( basePos );
			}

			vbo->addColor( borders.left.color );

			vbo->addVertex( Vector2f( tPos.x + topW * Math::cosAng( startAngle ),
									  tPos.y + topH * Math::sinAng( startAngle ) ) );

			vbo->addColor( borders.left.color );

			borderAddArc( vbo, tPos, topW, topH, startAngle, endAngle, borders.left.color,
						  borderLeft, basePos );
		} else {
			vbo->addVertex( Vector2f( pos.x, pos.y ) );
			vbo->addColor( borders.left.color );
			vbo->addVertex( Vector2f( pos.x + borderLeft, pos.y + borderTop ) );
			vbo->addColor( borders.left.color );
		}
	}
}

UIBorderDrawable* UIBorderDrawable::New() {
	return eeNew( UIBorderDrawable, () );
}

UIBorderDrawable::UIBorderDrawable() :
	Drawable( UIBORDERDRAWABLE ),
	mVertexBuffer(
		VertexBuffer::NewVertexArray( VERTEX_FLAGS_PRIMITIVE, PRIMITIVE_TRIANGLE_STRIP ) ),
	mBorderType( BorderType::Inside ),
	mNeedsUpdate( false ),
	mColorNeedsUpdate( false ) {}

UIBorderDrawable::~UIBorderDrawable() {}

void UIBorderDrawable::draw() {}

void UIBorderDrawable::draw( const Vector2f& position ) {}

void UIBorderDrawable::draw( const Vector2f& position, const Sizef& size ) {
	if ( mPosition != position ) {
		mPosition = position;
		mNeedsUpdate = true;
	}

	if ( mSize != size ) {
		mSize = size;
		mNeedsUpdate = true;
	}

	if ( mNeedsUpdate || mColorNeedsUpdate ) {
		update();
	}

	mVertexBuffer->bind();
	mVertexBuffer->draw();
	mVertexBuffer->unbind();
}

bool UIBorderDrawable::isStateful() {
	return false;
}

void UIBorderDrawable::setLineWidth( const Float& width ) {
	if ( mBorders.top.width != width || mBorders.left.width != width ||
		 mBorders.right.width != width || mBorders.bottom.width != width ) {
		mBorders.top.width = mBorders.left.width = mBorders.right.width = mBorders.bottom.width =
			width;
		mNeedsUpdate = true;
	}
}

Sizef UIBorderDrawable::getSize() {
	return mSize;
}

Float UIBorderDrawable::getLineWidth() const {
	return mBorders.top.width;
}

Int32 UIBorderDrawable::getRadius() const {
	return mBorders.radius.topLeftX;
}

void UIBorderDrawable::setRadius( const Int32& radius ) {
	if ( mBorders.radius.topLeftX != radius ) {
		mBorders.radius.topLeftX = radius;
		mBorders.radius.topLeftY = radius;
		mBorders.radius.topRightX = radius;
		mBorders.radius.topRightY = radius;
		mBorders.radius.bottomLeftX = radius;
		mBorders.radius.bottomLeftY = radius;
		mBorders.radius.bottomRightX = radius;
		mBorders.radius.bottomRightY = radius;
		mNeedsUpdate = true;
	}
}

Color UIBorderDrawable::getColorLeft() const {
	return mBorders.left.color;
}

void UIBorderDrawable::setColorLeft( const Color& colorLeft ) {
	if ( mBorders.left.color != colorLeft ) {
		mBorders.left.color = colorLeft;
		mColorNeedsUpdate = true;
	}
}

Color UIBorderDrawable::getColorRight() const {
	return mBorders.right.color;
}

void UIBorderDrawable::setColorRight( const Color& colorRight ) {
	if ( mBorders.left.color != colorRight ) {
		mBorders.left.color = colorRight;
		mColorNeedsUpdate = true;
	}
}

Color UIBorderDrawable::getColorTop() const {
	return mBorders.top.color;
}

void UIBorderDrawable::setColorTop( const Color& colorTop ) {
	if ( mBorders.top.color != colorTop ) {
		mBorders.top.color = colorTop;
		mColorNeedsUpdate = true;
	}
}

Color UIBorderDrawable::getColorBottom() const {
	return mBorders.bottom.color;
}

void UIBorderDrawable::setColorBottom( const Color& colorBottom ) {
	if ( mBorders.bottom.color != colorBottom ) {
		mBorders.bottom.color = colorBottom;
		mColorNeedsUpdate = true;
	}
}

const BorderType& UIBorderDrawable::getBorderType() const {
	return mBorderType;
}

void UIBorderDrawable::setBorderType( const BorderType& borderType ) {
	if ( mBorderType != borderType ) {
		mBorderType = borderType;
		mNeedsUpdate = true;
	}
}

void UIBorderDrawable::onAlphaChange() {
	mBorders.left.color.a = mBorders.right.color.a = mBorders.top.color.a =
		mBorders.bottom.color.a = mColor.a;
	mColorNeedsUpdate = true;
}

void UIBorderDrawable::onColorFilterChange() {
	Drawable::onColorFilterChange();
	mBorders.left.color = mBorders.right.color = mBorders.top.color = mBorders.bottom.color =
		mColor;
	mColorNeedsUpdate = true;
}

void UIBorderDrawable::onPositionChange() {
	mNeedsUpdate = true;
}

void UIBorderDrawable::update() {
	mBorders.left.color = Color::Red;
	mBorders.right.color = Color::Green;
	mBorders.top.color = Color::Blue;
	mBorders.bottom.color = Color::Fuchsia;

	switch ( mBorderType ) {
		case BorderType::Outside: {
			Vector2f pos( mPosition );
			Sizef size( mSize );

			if ( mBorders.top.width > 0 ) {
				pos.y -= mBorders.top.width;
			}

			if ( mBorders.left.width > 0 ) {
				pos.x -= mBorders.left.width;
			}

			if ( mBorders.right.width > 0 ) {
				size.x += mBorders.right.width * 2;
			}

			if ( mBorders.bottom.width > 0 ) {
				size.y += mBorders.bottom.width * 2;
			}

			createBorders( mVertexBuffer, mBorders, pos, size );

			break;
		}
		case BorderType::Inside: {
			createBorders( mVertexBuffer, mBorders, mPosition, mSize );
			break;
		}
	}

	mColorNeedsUpdate = false;
	mNeedsUpdate = false;
}

void UIBorderDrawable::updateColor() {
	update();
}

}} // namespace EE::UI
