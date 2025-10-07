#ifndef EE_GRAPHICSCPRIMITIVES_H
#define EE_GRAPHICSCPRIMITIVES_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/blendmode.hpp>
#include <eepp/graphics/primitivetype.hpp>
#include <eepp/math/polygon2.hpp>

#include <eepp/system/color.hpp>
using namespace EE::System;

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
	 * @param segmentsCount Number of segments to represent the circle
	 * @param arcAngle The arc angle
	 * @param arcStartAngle The arc starting point angle
	 */
	void drawArc( const Vector2f& p, const Float& radius, Uint32 segmentsCount = 0,
				  const Float& arcAngle = 360.f, const Float& arcStartAngle = 0.f );

	/** Draw a circle on the screen
	 * @param p The coordinates ( x and y represents the center of the circle )
	 * @param radius The Circle Radius
	 * @param segmentsCount Number of segments to represent the circle. If segmentsCount is equal to
	 * 0 by default will use an optimized circle rendering ( precached coordinates ).
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
	void drawTriangle( const Triangle2f& t, const Color& Color1, const Color& Color2,
					   const Color& Color3 );

	/** Draw a rectangle on the screen
	 * @param R The Rectangle Rectf
	 * @param Angle Rectangle Angle
	 * @param Scale Rectangle Scale ( default 1.0f )
	 */
	void drawRectangle( const Rectf& R, const Float& Angle, const Vector2f& Scale = Vector2f::One );

	/** Draw a rectangle on the screen
	 * @param R The Rectangle Rectf
	 */
	void drawRectangle( const Rectf& R );

	/** Draw a rounded rectangle on the screen
	 * @param R The Rectangle Rectf
	 * @param Angle Rectangle Angle
	 * @param Scale Rectangle Scale ( default 1.0f )
	 * @param Corners Number of vertices per corner ( how rounded is each corner )
	 */
	void drawRoundedRectangle( const Rectf& R, const Float& Angle = 0,
							   const Vector2f& Scale = Vector2f::One,
							   const unsigned int& Corners = 8 );

	/** Draw a rectangle on the screen setting per vertex color
	 * @param R The Rectangle Rectf
	 * @param TopLeft The Top Left Rectangle Color
	 * @param BottomLeft The Bottom Left Rectangle Color
	 * @param BottomRight The Bottom Right Rectangle Color
	 * @param TopRight The Top Right Rectangle Color
	 * @param Angle Rectangle Angle
	 * @param Scale Rectangle Scale ( default 1.0f )
	 */
	void drawRectangle( const Rectf& R, const Color& TopLeft, const Color& BottomLeft,
						const Color& BottomRight, const Color& TopRight, const Float& Angle = 0,
						const Vector2f& Scale = Vector2f::One );

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
	void drawRoundedRectangle( const Rectf& R, const Color& TopLeft, const Color& BottomLeft,
							   const Color& BottomRight, const Color& TopRight,
							   const Float& Angle = 0, const Vector2f& Scale = Vector2f::One,
							   const unsigned int& Corners = 8 );

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
	void drawQuad( const Quad2f& q, const Color& Color1, const Color& Color2, const Color& Color3,
				   const Color& Color4, const Float& OffsetX = 0, const Float& OffsetY = 0 );

	/** Draw a polygon on screen
	 * @param p The Polygon
	 */
	void drawPolygon( const Polygon2f& p );

	/**
	 * @brief Draws a soft-edged shadow for a rectangular box using only primitives.
	 *
	 * This function improves upon basic rectangular shadows by rendering the corners
	 * with a radial gradient (using a triangle fan), which produces a much smoother look.
	 *
	 * @param boxRect The rectangle of the UI element casting the shadow (in screen coordinates).
	 * @param shadowOffset An offset to apply to the shadow's position relative to the box.
	 * @param shadowSize The distance the shadow extends and fades out from the box edges.
	 * @param shadowColor The base color of the shadow (alpha will be used for max opacity).
	 * @param cornerSegments The number of triangles to use for each corner fan. More segments
	 * result in a smoother corner.
	 */
	void drawSoftShadow( const Rectf& boxRect, const Vector2f& shadowOffset, Float shadowSize,
						 const Color& shadowColor, Uint32 cornerSegments = 8 );

	/** Set the current color for drawing primitives */
	void setColor( const Color& Color );

	/** @return The color used to draw the primitives */
	const Color& getColor();

	/** Forcing the draw, will force the batch renderer to draw the batched vertices immediately (
	 * active by default ). */
	void setForceDraw( const bool& force );

	const bool& getForceDraw() const;

	/** Force to draw the batched vertices. */
	void drawBatch();

	/** Set the fill mode used to draw primitives */
	void setFillMode( const PrimitiveFillMode& Mode );

	/** @return The fill mode used to draw primitives */
	const PrimitiveFillMode& getFillMode() const;

	/** Set the blend mode used to draw primitives */
	void setBlendMode( const BlendMode& Mode );

	/** @return The blend mode used to draw primitives */
	const BlendMode& getBlendMode() const;

	/** Set the line width to draw primitives */
	void setLineWidth( const Float& width );

	/** @return The line with to draw primitives */
	const Float& getLineWidth() const;

  private:
	Color mColor;
	PrimitiveFillMode mFillMode;
	BlendMode mBlendMode;
	Float mLineWidth;
	bool mForceDraw;
};

}} // namespace EE::Graphics

#endif
