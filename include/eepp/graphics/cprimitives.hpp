#ifndef EE_GRAPHICSCPRIMITIVES_H
#define EE_GRAPHICSCPRIMITIVES_H

#include <eepp/graphics/base.hpp>
#include <eepp/math/polygon2.hpp>

namespace EE { namespace Graphics {

/** @brief Basic primitives rendering class */
class EE_API cPrimitives {
	public:
		cPrimitives();

		~cPrimitives();

		/** Draw a point on the screen
		* @param p The coordinates
		* @param pointSize Point Size (default 1.0f )
		*/
		void DrawPoint( const eeVector2f& p, const Float& pointSize = 1.0f );

		/** Draw a Line on screen
		* @param line The line
		*/
		void DrawLine( const eeLine2f& line );

		/** Draw a circle on the screen
		* @param p The coordinates ( x and y represents the center of the circle )
		* @param radius The Circle Radius
		* @param points Number of points to represent the circle. If points is equal to 0 by default will use an optimized circle rendering ( precached coordinates ).
		*/
		void DrawCircle( const eeVector2f& p, const Float& radius, Uint32 points = 0 );

		/** Draw a triangle on the screen
		* @param t The Triangle (eeTriangle2f)
		*/
		void DrawTriangle( const eeTriangle2f& t );

		/** Draw a triangle on the screen setting per vertex color
		* @param t The Triangle (eeTriangle2f)
		* @param Color1 First Point Color
		* @param Color2 Second Point Color
		* @param Color3 Third Point Color
		*/
		void DrawTriangle( const eeTriangle2f& t, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3 );

		/** Draw a rectangle on the screen
		* @param R The Rectangle eeRectf
		* @param Angle Rectangle Angle
		* @param Scale Rectangle Scale ( default 1.0f )
		*/
		void DrawRectangle( const eeRectf& R, const Float& Angle = 0, const eeVector2f& Scale = eeVector2f::One );

		/** Draw a rounded rectangle on the screen
		* @param R The Rectangle eeRectf
		* @param Angle Rectangle Angle
		* @param Scale Rectangle Scale ( default 1.0f )
		* @param Corners Number of vertices per corner ( how rounded is each corner )
		*/
		void DrawRoundedRectangle( const eeRectf& R, const Float& Angle = 0, const eeVector2f& Scale = eeVector2f::One, const unsigned int& Corners = 8 );

		/** Draw a rectangle on the screen setting per vertex color
		* @param R The Rectangle eeRectf
		* @param TopLeft The Top Left Rectangle Color
		* @param BottomLeft The Bottom Left Rectangle Color
		* @param BottomRight The Bottom Right Rectangle Color
		* @param TopRight The Top Right Rectangle Color
		* @param Angle Rectangle Angle
		* @param Scale Rectangle Scale ( default 1.0f )
		*/
		void DrawRectangle( const eeRectf& R, const eeColorA& TopLeft, const eeColorA& BottomLeft, const eeColorA& BottomRight, const eeColorA& TopRight, const Float& Angle = 0, const eeVector2f& Scale = eeVector2f::One );

		/** Draw a rounded rectangle on the screen setting per vertex color
		* @param R The Rectangle eeRectf
		* @param TopLeft The Top Left Rectangle Color
		* @param BottomLeft The Bottom Left Rectangle Color
		* @param BottomRight The Bottom Right Rectangle Color
		* @param TopRight The Top Right Rectangle Color
		* @param Angle Rectangle Angle
		* @param Scale Rectangle Scale ( default 1.0f )
		* @param Corners Number of vertices per corner ( how rounded is each corner )
		*/
		void DrawRoundedRectangle( const eeRectf& R, const eeColorA& TopLeft, const eeColorA& BottomLeft, const eeColorA& BottomRight, const eeColorA& TopRight, const Float& Angle = 0, const eeVector2f& Scale = eeVector2f::One, const unsigned int& Corners = 8 );

		/** Draw a four edges polygon on screen
		* @param q The Quad
		* @param OffsetX X offset for the quad
		* @param OffsetY Y offset for the quad
		*/
		void DrawQuad( const eeQuad2f& q, const Float& OffsetX = 0, const Float& OffsetY = 0 );

		/** Draw a four edges polygon on screen
		* @param q The Quad
		* @param Color1 First Point Color
		* @param Color2 Second Point Color
		* @param Color3 Third Point Color
		* @param Color4 Fourth Point Color
		* @param OffsetX X offset for the quad
		* @param OffsetY Y offset for the quad
		*/
		void DrawQuad( const eeQuad2f& q, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3, const eeColorA& Color4, const Float& OffsetX = 0, const Float& OffsetY = 0 );

		/** Draw a polygon on screen
		* @param p The Polygon
		*/
		void DrawPolygon( const eePolygon2f& p );

		/** Set the current color for drawing primitives */
		void SetColor( const eeColorA& Color );

		/** Forcing the draw, will force the batch renderer to draw the batched vertexs immediately ( active by default ). */
		void ForceDraw( const bool& force );

		const bool& ForceDraw() const;

		/** Force to draw the batched vertexs. */
		void DrawBatch();

		/** Set the fill mode used to draw primitives */
		void FillMode( const EE_FILL_MODE& Mode );

		/** @return The fill mode used to draw primitives */
		const EE_FILL_MODE& FillMode() const;

		/** Set the blend mode used to draw primitives */
		void BlendMode( const EE_BLEND_MODE& Mode );

		/** @return The blend mode used to draw primitives */
		const EE_BLEND_MODE& BlendMode() const;

		/** Set the line width to draw primitives */
		void LineWidth( const Float& width );

		/** @return The line with to draw primitives */
		const Float& LineWidth() const;
	private:
		eeColorA				mColor;
		EE_FILL_MODE			mFillMode;
		EE_BLEND_MODE			mBlendMode;
		Float					mLineWidth;
		bool					mForceDraw;
};

}}

#endif
