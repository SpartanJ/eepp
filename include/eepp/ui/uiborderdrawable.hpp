#ifndef EE_UI_UIBORDERDRAWABLE_HPP
#define EE_UI_UIBORDERDRAWABLE_HPP

#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/rectangledrawable.hpp>

namespace EE { namespace Graphics {
class VertexBuffer;
}} // namespace EE::Graphics
using namespace EE::Graphics;

namespace EE { namespace UI {

enum class BorderType : Uint32 { Inside, Outside };

class EE_API UIBorderDrawable : public Drawable {
  public:
	static UIBorderDrawable* New();

	UIBorderDrawable();

	virtual ~UIBorderDrawable();

	virtual Sizef getSize();

	virtual void draw();

	virtual void draw( const Vector2f& position );

	virtual void draw( const Vector2f& position, const Sizef& size );

	virtual bool isStateful();

	/** Set the line width to draw primitives */
	virtual void setLineWidth( const Float& width );

	/** @return The line with to draw primitives */
	Float getLineWidth() const;

	Int32 getRadius() const;

	void setRadius( const Int32& radius );

	Color getColorLeft() const;

	void setColorLeft( const Color& colorLeft );

	Color getColorRight() const;

	void setColorRight( const Color& colorRight );

	Color getColorTop() const;

	void setColorTop( const Color& colorTop );

	Color getColorBottom() const;

	void setColorBottom( const Color& colorBottom );

	const BorderType& getBorderType() const;

	void setBorderType( const BorderType& borderType );

  protected:
	VertexBuffer* mVertexBuffer;

	struct Border {
		int width = 0;
		Color color;
	};

	struct BorderRadiuses {
		Float topLeftX = 0;
		Float topLeftY = 0;
		Float topRightX = 0;
		Float topRightY = 0;
		Float bottomRightX = 0;
		Float bottomRightY = 0;
		Float bottomLeftX = 0;
		Float bottomLeftY = 0;
	};

	struct Borders {
		Border left;
		Border top;
		Border right;
		Border bottom;
		BorderRadiuses radius;
	};

	Borders mBorders;
	BorderType mBorderType;
	Sizef mSize;
	bool mNeedsUpdate;
	bool mColorNeedsUpdate;

	virtual void onAlphaChange();

	virtual void onColorFilterChange();

	virtual void onPositionChange();

	void update();

	void updateColor();

	void createBorders( VertexBuffer* vbo, const UIBorderDrawable::Borders& borders,
						const Vector2f& pos, const Sizef& size );
};

}} // namespace EE::UI

#endif // EE_UI_UIBORDERDRAWABLE_HPP
