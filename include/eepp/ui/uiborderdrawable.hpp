#ifndef EE_UI_UIBORDERDRAWABLE_HPP
#define EE_UI_UIBORDERDRAWABLE_HPP

#include <eepp/graphics/drawable.hpp>
#include <eepp/ui/border.hpp>

namespace EE { namespace UI {

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
	Borders mBorders;
	BorderType mBorderType;
	Sizef mSize;
	bool mNeedsUpdate;
	bool mColorNeedsUpdate;

	virtual void onAlphaChange();

	virtual void onColorFilterChange();

	virtual void onPositionChange();

	void update();
};

}} // namespace EE::UI

#endif // EE_UI_UIBORDERDRAWABLE_HPP
