#include "cprimitives.hpp"
#include "../utils/polygon2.hpp"

namespace EE { namespace Graphics {

cPrimitives::cPrimitives() {
	TF = cTextureFactory::instance();

	BR = cGlobalBatchRenderer::instance();
}

cPrimitives::~cPrimitives() {}

void cPrimitives::DrawRoundedRectangle(const eeFloat& x, const eeFloat& y, const eeFloat& width, const eeFloat& height, const eeColorA& TopLeft, const eeColorA& BottomLeft, const eeColorA& BottomRight, const eeColorA& TopRight, const eeFloat& Angle, const eeFloat& Scale, const EE_FILLMODE& fillmode, const  EE_RENDERALPHAS& blend, const eeFloat& lineWidth, const eeUint& Corners ) {
	BR->SetTexture( 0 );
	BR->SetBlendFunc( blend );

	eeUint i;
	eeFloat xscalediff = width * Scale - width;
	eeFloat yscalediff = height * Scale - height;
	eeVector2f poly;
	eeVector2f Center( x + width * 0.5f + xscalediff, y + height * 0.5f + yscalediff );

	eePolygon2f Poly = CreateRoundedPolygon( x - xscalediff, y - yscalediff, width + xscalediff, height + yscalediff, Corners );
	Poly.Rotate( Angle, Center );

	switch(fillmode) {
		case DRAW_FILL: {
			if ( TopLeft == BottomLeft && BottomLeft == BottomRight && BottomRight == TopRight ) {
				BR->PolygonSetColor( TopLeft );

				BR->BatchPolygon( Poly );
			} else {
				for ( i = 0; i < Poly.Size(); i++ ) {
					poly = Poly[i];

					if ( poly.x <= Center.x && poly.y <= Center.y )
						BR->PolygonSetColor( TopLeft );
					else if ( poly.x <= Center.x && poly.y >= Center.y )
						BR->PolygonSetColor( BottomLeft );
					else if ( poly.x > Center.x && poly.y > Center.y )
						BR->PolygonSetColor( BottomRight );
					else if ( poly.x > Center.x && poly.y < Center.y )
						BR->PolygonSetColor( TopRight );
					else
						BR->PolygonSetColor( TopLeft );

					BR->BatchPolygonByPoint( Poly[i] );
				}
			}

			break;
		}
		case DRAW_LINE:
			BR->SetLineWidth( lineWidth );

			BR->LineLoopBegin();
			BR->LineLoopSetColor( TopLeft );

			if ( TopLeft == BottomLeft && BottomLeft == BottomRight && BottomRight == TopRight ) {
				for ( i = 0; i < Poly.Size(); i+=2 )
					BR->BatchLineLoop( Poly[i], Poly[i+1] );
			} else {
				for ( eeUint i = 0; i < Poly.Size(); i++ ) {
					poly = Poly[i];

					if ( poly.x <= Center.x && poly.y <= Center.y )
						BR->LineLoopSetColor( TopLeft );
					else if ( poly.x < Center.x && poly.y > Center.y )
						BR->LineLoopSetColor( BottomLeft );
					else if ( poly.x > Center.x && poly.y > Center.y )
						BR->LineLoopSetColor( BottomRight );
					else if ( poly.x > Center.x && poly.y < Center.y )
						BR->LineLoopSetColor( TopRight );
					else
						BR->LineLoopSetColor( TopLeft );

					BR->BatchLineLoop( Poly[i] );
				}
			}

			break;
	}

	BR->Draw();
}

void cPrimitives::DrawRectangle(const eeFloat& x, const eeFloat& y, const eeFloat& width, const eeFloat& height, const eeColorA& TopLeft, const eeColorA& BottomLeft, const eeColorA& BottomRight, const eeColorA& TopRight, const eeFloat& Angle, const eeFloat& Scale, const EE_FILLMODE& fillmode, const  EE_RENDERALPHAS& blend, const eeFloat& lineWidth, const eeUint& Corners) {
	if ( 0 != Corners ) {
		DrawRoundedRectangle( x, y, width, height, TopLeft, BottomLeft, BottomRight, TopRight, Angle, Scale, fillmode, blend, lineWidth, Corners );
		return;
	}

	BR->SetTexture( 0 );
	BR->SetBlendFunc( blend );

	switch(fillmode) {
		case DRAW_FILL: {
			BR->QuadsBegin();
			BR->QuadsSetColorFree( TopLeft, BottomLeft, BottomRight, TopRight );
			BR->BatchQuadEx( x, y, width, height, Angle, Scale );
			break;
		}
		case DRAW_LINE:
			BR->SetLineWidth( lineWidth );

			BR->LineLoopBegin();
			BR->LineLoopSetColorFree( TopLeft, BottomLeft );

			if ( Scale != 1.0f || Angle != 0.0f ) {
				eeQuad2f Q;
				Q.V[0].x = x; Q.V[0].y = y;
				Q.V[1].x = x, Q.V[1].y = y + height;
				Q.V[2].x = x + width; Q.V[2].y = y + height;
				Q.V[3].x = x + width; Q.V[3].y = y;

				Q.Scale( Scale );
				Q.Rotate( Angle, eeVector2f( x + width * 0.5f, y + height * 0.5f ) );

				BR->BatchLineLoop( Q[0].x, Q[0].y, Q[1].x, Q[1].y );
				BR->LineLoopSetColorFree( BottomRight, TopRight );
				BR->BatchLineLoop( Q[2].x, Q[2].y, Q[3].x, Q[3].y );
			} else {
				BR->BatchLineLoop( x, y, x, y + height );
				BR->LineLoopSetColorFree( BottomRight, TopRight );
				BR->BatchLineLoop( x + width, y + height, x + width, y );
			}
			break;
	}

	BR->Draw();
}

void cPrimitives::DrawLine(const eeFloat& x, const eeFloat& y, const eeFloat& x2, const eeFloat& y2, const eeFloat& lineWidth) {
	BR->SetLineWidth( lineWidth );

	BR->SetTexture( 0 );
	BR->LinesBegin();
	BR->LinesSetColor( mColor );

	BR->BatchLine( x, y, x2, y2 );

	BR->Draw();
}

void cPrimitives::DrawPoint( const eeFloat& x, const eeFloat& y, const eeFloat& pointSize ) {
	BR->SetPointSize( pointSize );

	BR->SetTexture( 0 );
	BR->PointsBegin();
	BR->PointSetColor( mColor );

	BR->BatchPoint( x, y );

	BR->Draw();
}

void cPrimitives::DrawCircle( const eeFloat& x, const eeFloat& y, const eeFloat& radius, Uint32 points, const EE_FILLMODE& fillmode, const eeFloat& lineWidth ) {
	if(points < 6) points = 6;
    eeFloat angle_shift =  360 / static_cast<eeFloat>(points);

	BR->SetTexture( 0 );

	switch( fillmode ) {
		case DRAW_LINE:
			BR->SetLineWidth( lineWidth );
			BR->LineLoopBegin();
			BR->LineLoopSetColor( mColor );

			for( eeFloat i = 0; i < 360; i+= ( angle_shift + angle_shift ) )
				BR->BatchLineLoop( x + radius * sinAng(i), y + radius * cosAng(i), x + radius * sinAng( i + angle_shift ), y + radius * cosAng( i + angle_shift ) );

			break;
		case DRAW_FILL:
			BR->TriangleFanBegin();
			BR->TriangleFanSetColor( mColor );

			for( eeFloat i = 0; i < 360; i+= ( angle_shift + angle_shift + angle_shift ) )
				BR->BatchTriangleFan( x + radius * sinAng(i), y + radius * cosAng(i), x + radius * sinAng( i + angle_shift ), y + radius * cosAng( i + angle_shift ), x + radius * sinAng( i + angle_shift + angle_shift ), y + radius * cosAng( i + angle_shift + angle_shift ) );

			break;
	}

	BR->Draw();
}

void cPrimitives::DrawTriangle(const eeFloat& x1, const eeFloat& y1, const eeFloat& x2, const eeFloat& y2, const eeFloat& x3, const eeFloat& y3, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3, const EE_FILLMODE& fillmode, const EE_RENDERALPHAS& blend, const eeFloat& lineWidth) {
	BR->SetTexture( 0 );
	BR->SetBlendFunc( blend );

	switch(fillmode) {
		case DRAW_LINE:
			BR->SetLineWidth( lineWidth );

			BR->LineLoopBegin();

			BR->LineLoopSetColorFree( Color1, Color2 );
			BR->BatchLineLoop( x1, y1, x2, y2 );
			BR->LineLoopSetColorFree( Color2, Color3 );
			BR->BatchLineLoop( x2, y2, x3, y3 );
			break;
		default:
		case DRAW_FILL:
			BR->TrianglesBegin();

			BR->TrianglesSetColorFree( Color1, Color2, Color3 );
			BR->BatchTriangle( x1, y1, x2, y2, x3, y3 );

			break;
	}

	BR->Draw();
}

void cPrimitives::DrawQuad( const eeFloat& x1, const eeFloat& y1, const eeFloat& x2, const eeFloat& y2, const eeFloat& x3, const eeFloat& y3, const eeFloat& x4, const eeFloat& y4, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3, const eeColorA& Color4, const EE_FILLMODE& fillmode, const EE_RENDERALPHAS& blend, const eeFloat& lineWidth, const eeFloat& OffsetX, const eeFloat& OffsetY ) {
	BR->SetTexture( 0 );
	BR->SetBlendFunc( blend );

	switch(fillmode) {
		case DRAW_LINE:
			BR->SetLineWidth( lineWidth );

			BR->LineLoopBegin();
			BR->LineLoopSetColorFree( Color1, Color2 );
			BR->BatchLineLoop( OffsetX + x1, OffsetY + y1, OffsetX + x2, OffsetY + y2 );
			BR->LineLoopSetColorFree( Color2, Color3 );
			BR->BatchLineLoop( OffsetX + x3, OffsetY + y3, OffsetX + x4, OffsetY + y4 );
			break;
		case DRAW_FILL:
			BR->QuadsBegin();
			BR->QuadsSetColorFree( Color1, Color2, Color3, Color4 );
			BR->BatchQuadFree( OffsetX + x1, OffsetY + y1, OffsetX + x2, OffsetY + y2, OffsetX + x3, OffsetY + y3, OffsetX + x4, OffsetY + y4 );
			break;
	}

	BR->Draw();
}

void cPrimitives::DrawPolygon(const eePolygon2f& p, const EE_FILLMODE& fillmode, const EE_RENDERALPHAS& blend, const eeFloat& lineWidth) {
	BR->SetTexture( 0 );
	BR->SetBlendFunc( blend );

	switch(fillmode) {
		case DRAW_LINE:
			BR->SetLineWidth( lineWidth );

			BR->LineLoopBegin();
			BR->LineLoopSetColor( mColor );

			for ( Uint32 i = 0; i < p.Size(); i += 2 )
				BR->BatchLineLoop( p[i].x, p[i].y, p[i+1].x, p[i+1].y );

			break;
		case DRAW_FILL:
			BR->PolygonSetColor( mColor );
			BR->BatchPolygon( p );
			break;
	}

	BR->Draw();
}

void cPrimitives::DrawRectangle( const eeRectf& R, const eeColorA& TopLeft, const eeColorA& BottomLeft, const eeColorA& BottomRight, const eeColorA& TopRight, const eeFloat& Angle, const eeFloat& Scale, const EE_FILLMODE& fillmode, const EE_RENDERALPHAS& blend, const eeFloat& lineWidth, const eeUint& Corners) {
	DrawRectangle( R.Left, R.Top, R.Right - R.Left, R.Bottom - R.Top, TopLeft, BottomLeft, BottomRight, TopRight, Angle, Scale, fillmode, blend, lineWidth, Corners);
}

void cPrimitives::DrawRectangle( const eeRectf& R, const eeFloat& Angle, const eeFloat& Scale, const EE_FILLMODE& fillmode, const EE_RENDERALPHAS& blend, const eeFloat& lineWidth, const eeUint& Corners) {
	DrawRectangle( R.Left, R.Top, R.Right - R.Left, R.Bottom - R.Top, mColor, mColor, mColor, mColor, Angle, Scale, fillmode, blend, lineWidth, Corners);
}

void cPrimitives::DrawRectangle(const eeFloat& x, const eeFloat& y, const eeFloat& width, const eeFloat& height, const eeFloat& Angle, const eeFloat& Scale, const EE_FILLMODE& fillmode, const EE_RENDERALPHAS& blend, const eeFloat& lineWidth, const eeUint& Corners) {
	DrawRectangle(x, y, width, height, mColor, mColor, mColor, mColor, Angle, Scale, fillmode, blend, lineWidth, Corners);
}

void cPrimitives::DrawRoundedRectangle( const eeRectf& R, const eeColorA& TopLeft, const eeColorA& BottomLeft, const eeColorA& BottomRight, const eeColorA& TopRight, const eeFloat& Angle, const eeFloat& Scale, const EE_FILLMODE& fillmode, const EE_RENDERALPHAS& blend, const eeFloat& lineWidth, const eeUint& Corners ) {
	DrawRoundedRectangle( R.Left, R.Top, R.Right - R.Left, R.Bottom - R.Top, TopLeft, BottomLeft, BottomRight, TopRight, Angle, Scale, fillmode, blend, lineWidth, Corners);
}

void cPrimitives::DrawRoundedRectangle( const eeRectf& R, const eeFloat& Angle, const eeFloat& Scale, const EE_FILLMODE& fillmode, const EE_RENDERALPHAS& blend, const eeFloat& lineWidth, const eeUint& Corners) {
	DrawRoundedRectangle( R.Left, R.Top, R.Right - R.Left, R.Bottom - R.Top, mColor, mColor, mColor, mColor, Angle, Scale, fillmode, blend, lineWidth, Corners);
}

void cPrimitives::DrawRoundedRectangle(const eeFloat& x, const eeFloat& y, const eeFloat& width, const eeFloat& height, const eeFloat& Angle, const eeFloat& Scale, const EE_FILLMODE& fillmode, const EE_RENDERALPHAS& blend, const eeFloat& lineWidth, const eeUint& Corners ) {
	DrawRoundedRectangle(x, y, width, height, mColor, mColor, mColor, mColor, Angle, Scale, fillmode, blend, lineWidth, Corners);
}

void cPrimitives::DrawLine(const eeVector2f& p1, const eeVector2f& p2, const eeFloat& lineWidth) {
	DrawLine(p1.x, p1.y, p2.x, p2.y, lineWidth);
}

void cPrimitives::DrawPoint(const eeVector2f& p, const eeFloat& pointSize) {
	DrawPoint(p.x, p.y, pointSize);
}

void cPrimitives::DrawCircle(const eeVector2f& p, const eeFloat& radius, Uint32 points, const EE_FILLMODE& fillmode, const eeFloat& lineWidth) {
	DrawCircle(p.x, p.y, radius, points, fillmode, lineWidth);
}

void cPrimitives::SetColor( const eeColorA& Color ) {
	mColor = Color;
	glColor4ub( mColor.R(), mColor.G(), mColor.B(), mColor.A() );
}

void cPrimitives::DrawTriangle(const eeTriangle2f& t, const EE_FILLMODE& fillmode, const EE_RENDERALPHAS& blend, const eeFloat& lineWidth) {
	DrawTriangle( t.V[0], t.V[1], t.V[2], mColor, mColor, mColor, fillmode, blend, lineWidth );
}

void cPrimitives::DrawTriangle(const eeTriangle2f& t, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3, const EE_FILLMODE& fillmode, const EE_RENDERALPHAS& blend, const eeFloat& lineWidth) {
	DrawTriangle( t.V[0], t.V[1], t.V[2], Color1, Color2, Color3, fillmode, blend, lineWidth );
}

void cPrimitives::DrawTriangle(const eeVector2f& p1, const eeVector2f& p2, const eeVector2f& p3, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3, const EE_FILLMODE& fillmode, const EE_RENDERALPHAS& blend, const eeFloat& lineWidth) {
	DrawTriangle(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, Color1, Color2, Color3, fillmode, blend, lineWidth);
}

void cPrimitives::DrawTriangle(const eeFloat& x1, const eeFloat& y1, const eeFloat& x2, const eeFloat& y2, const eeFloat& x3, const eeFloat& y3, const EE_FILLMODE& fillmode, const EE_RENDERALPHAS& blend, const eeFloat& lineWidth) {
	DrawTriangle(x1, y1, x2, y2, x3, y3, mColor, mColor, mColor, fillmode, blend, lineWidth);
}

void cPrimitives::DrawTriangle(const eeVector2f& p1, const eeVector2f& p2, const eeVector2f& p3, const EE_FILLMODE& fillmode, const EE_RENDERALPHAS& blend, const eeFloat& lineWidth) {
	DrawTriangle(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, mColor, mColor, mColor, fillmode, blend, lineWidth);
}

void cPrimitives::DrawQuad(const eeFloat& x1, const eeFloat& y1, const eeFloat& x2, const eeFloat& y2, const eeFloat& x3, const eeFloat& y3, const eeFloat& x4, const eeFloat& y4, const EE_FILLMODE& fillmode, const EE_RENDERALPHAS& blend, const eeFloat& lineWidth, const eeFloat& OffsetX, const eeFloat& OffsetY ) {
	DrawQuad(x1, y1, x2, y2, x3, y3, x4, y4, mColor, mColor, mColor, mColor, fillmode, blend, lineWidth, OffsetX, OffsetY);
}

void cPrimitives::DrawQuad(const eeVector2f& p1, const eeVector2f& p2, const eeVector2f& p3, const eeVector2f& p4, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3, const eeColorA& Color4, const EE_FILLMODE& fillmode, const EE_RENDERALPHAS& blend, const eeFloat& lineWidth, const eeFloat& OffsetX, const eeFloat& OffsetY ) {
	DrawQuad(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, Color1, Color2, Color3, Color4, fillmode, blend, lineWidth, OffsetX, OffsetY);
}

void cPrimitives::DrawQuad(const eeVector2f& p1, const eeVector2f& p2, const eeVector2f& p3, const eeVector2f& p4, const EE_FILLMODE& fillmode, const EE_RENDERALPHAS& blend, const eeFloat& lineWidth, const eeFloat& OffsetX, const eeFloat& OffsetY ) {
	DrawQuad(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, mColor, mColor, mColor, mColor, fillmode, blend, lineWidth, OffsetX, OffsetY);
}

void cPrimitives::DrawQuad(const eeQuad2f& q, const EE_FILLMODE& fillmode, const EE_RENDERALPHAS& blend, const eeFloat& lineWidth, const eeFloat& OffsetX, const eeFloat& OffsetY ) {
	DrawQuad(q.V[0].x, q.V[0].y, q.V[1].x, q.V[1].y, q.V[2].x, q.V[2].y, q.V[3].x, q.V[3].y, mColor, mColor, mColor, mColor, fillmode, blend, lineWidth, OffsetX, OffsetY);
}

}}
