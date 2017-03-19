#ifndef EE_GRAPHICSCPRIMITIVES_H
#define EE_GRAPHICSCPRIMITIVES_H

#include <eepp/graphics/base.hpp>
#include <eepp/math/polygon2.hpp>

namespace EE { namespace Graphics {

/** @brief Basic primitives rendering class */
class EE_API Primitives {
	public:
		Primitives();

		~Primitives();

		/** Draw a point on the screen
		* @param p The coordinates
		* @param pointSize Point Size (default 1.0f )
		*/
		void drawPoint( const Vector2f& p, const Float& pointSize = 1.0f );

		/** Draw a Line on screen
		* @param line The line
		*/
		void drawLine( const Line2f& line );

		/** Draw an arc on the screen
		* @param p The coordinates ( x and y represents the center of the circle )
		* @param radius The Circle Radius
		* @param segmentsCount Number of segments to represent the circle.
		*/
		void drawArc( const Vector2f& p, const Float& radius, Uint32 segmentsCount = 0, const Float& arcAngle = 360.f , const Float& arcStartAngle = 0.f );

		/** Draw a circle on the screen
		* @param p The coordinates ( x and y represents the center of the circle )
		* @param radius The Circle Radius
		* @param segmentsCount Number of segments to represent the circle. If segmentsCount is equal to 0 by default will use an optimized circle rendering ( precached coordinates ).
		*/
		void drawCircle( const Vector2f& p, const Float& radius, Uint32 segmentsCount = 0 );

		/** Draw a triangle on the screen
		* @param t The Triangle (Triangle2f)
		*/
		void drawTriangle( const Triangle2f& t );

		/** Draw a triangle on the screen setting per vertex color
		* @param t The Triangle (Triangle2f)
		* @param Color1 First Point Color
		* @param Color2 Second Point Color
		* @param Color3 Third Point Color
		*/
		void drawTriangle( const Triangle2f& t, const ColorA& Color1, const ColorA& Color2, const ColorA& Color3 );

		/** Draw a rectangle on the screen
		* @param R The Rectangle Rectf
		* @param Angle Rectangle Angle
		* @param Scale Rectangle Scale ( default 1.0f )
		*/
		void drawRectangle( const Rectf& R, const Float& Angle = 0, const Vector2f& Scale = Vector2f::One );

		/** Draw a rounded rectangle on the screen
		* @param R The Rectangle Rectf
		* @param Angle Rectangle Angle
		* @param Scale Rectangle Scale ( default 1.0f )
		* @param Corners Number of vertices per corner ( how rounded is each corner )
		*/
		void drawRoundedRectangle( const Rectf& R, const Float& Angle = 0, const Vector2f& Scale = Vector2f::One, const unsigned int& Corners = 8 );

		/** Draw a rectangle on the screen setting per vertex color
		* @param R The Rectangle Rectf
		* @param TopLeft The Top Left Rectangle Color
		* @param BottomLeft The Bottom Left Rectangle Color
		* @param BottomRight The Bottom Right Rectangle Color
		* @param TopRight The Top Right Rectangle Color
		* @param Angle Rectangle Angle
		* @param Scale Rectangle Scale ( default 1.0f )
		*/
		void drawRectangle( const Rectf& R, const ColorA& TopLeft, const ColorA& BottomLeft, const ColorA& BottomRight, const ColorA& TopRight, const Float& Angle = 0, const Vector2f& Scale = Vector2f::One );

		/** Draw a rounded rectangle on the screen setting per vertex color
		* @param R The Rectangle Rectf
		* @param TopLeft The Top Left Rectangle Color
		* @param BottomLeft The Bottom Left Rectangle Color
		* @param BottomRight The Bottom Right Rectangle Color
		* @param TopRight The Top Right Rectangle Color
		* @param Angle Rectangle Angle
		* @param Scale Rectangle Scale ( default 1.0f )
		* @param Corners Number of vertices per corner ( how rounded is each corner )
		*/
		void drawRoundedRectangle( const Rectf& R, const ColorA& TopLeft, const ColorA& BottomLeft, const ColorA& BottomRight, const ColorA& TopRight, const Float& Angle = 0, const Vector2f& Scale = Vector2f::One, const unsigned int& Corners = 8 );

		/** Draw a four edges polygon on screen
		* @param q The Quad
		* @param OffsetX X offset for the quad
		* @param OffsetY Y offset for the quad
		*/
		void drawQuad( const Quad2f& q, const Float& OffsetX = 0, const Float& OffsetY = 0 );

		/** Draw a four edges polygon on screen
		* @param q The Quad
		* @param Color1 First Point Color
		* @param Color2 Second Point Color
		* @param Color3 Third Point Color
		* @param Color4 Fourth Point Color
		* @param OffsetX X offset for the quad
		* @param OffsetY Y offset for the quad
		*/
		void drawQuad( const Quad2f& q, const ColorA& Color1, const ColorA& Color2, const ColorA& Color3, const ColorA& Color4, const Float& OffsetX = 0, const Float& OffsetY = 0 );

		/** Draw a polygon on screen
		* @param p The Polygon
		*/
		void drawPolygon( const Polygon2f& p );

		/** Set the current color for drawing primitives */
		void setColor( const ColorA& Color );

		/** @return The color used to draw the primitives */
		const ColorA& getColor();

		/** Forcing the draw, will force the batch renderer to draw the batched vertexs immediately ( active by default ). */
		void setForceDraw( const bool& force );

		const bool& getForceDraw() const;

		/** Force to draw the batched vertexs. */
		void drawBatch();

		/** Set the fill mode used to draw primitives */
		void setFillMode( const EE_FILL_MODE& Mode );

		/** @return The fill mode used to draw primitives */
		const EE_FILL_MODE& getFillMode() const;

		/** Set the blend mode used to draw primitives */
		void setBlendMode( const EE_BLEND_MODE& Mode );

		/** @return The blend mode used to draw primitives */
		const EE_BLEND_MODE& getBlendMode() const;

		/** Set the line width to draw primitives */
		void setLineWidth( const Float& width );

		/** @return The line with to draw primitives */
		const Float& getLineWidth() const;
	private:
		ColorA					mColor;
		EE_FILL_MODE			mFillMode;
		EE_BLEND_MODE			mBlendMode;
		Float					mLineWidth;
		bool					mForceDraw;
};

}}

#endif
