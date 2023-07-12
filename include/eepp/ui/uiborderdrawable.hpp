#ifndef EE_UI_UIBORDERDRAWABLE_HPP
#define EE_UI_UIBORDERDRAWABLE_HPP

#include <eepp/graphics/drawable.hpp>
#include <eepp/math/rect.hpp>
#include <eepp/ui/border.hpp>

namespace EE { namespace UI {

class UINode;

class EE_API UIBorderDrawable : public Drawable {
  public:
	static UIBorderDrawable* New( const UINode* owner );

	UIBorderDrawable( const UINode* owner );

	virtual ~UIBorderDrawable();

	virtual Sizef getSize();

	virtual Sizef getPixelsSize();

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

	void invalidate();

	void setLeftWidth( const std::string& leftWidth );

	void setRightWidth( const std::string& rightWidth );

	void setTopWidth( const std::string& topWidth );

	void setBottomWidth( const std::string& bottomWidth );

	void setTopLeftRadius( const std::string& radius );

	void setTopRightRadius( const std::string& radius );

	void setBottomLeftRadius( const std::string& radius );

	void setBottomRightRadius( const std::string& radius );

	const Borders& getBorders() const;

	Rectf getBorderBoxDiff() const;

	bool isSmooth() const;

	void setSmooth( bool smooth );

  protected:
	const UINode* mOwner;
	VertexBuffer* mVertexBuffer;
	Borders mBorders;
	BorderStr mBorderStr;
	BorderType mBorderType;
	Sizef mSize;
	bool mNeedsUpdate;
	bool mColorNeedsUpdate;
	bool mHasBorder;
	bool mSmooth{ false };

	virtual void onAlphaChange();

	virtual void onColorFilterChange();

	virtual void onPositionChange();

	void update();

	void updateBorders();
};

}} // namespace EE::UI

#endif // EE_UI_UIBORDERDRAWABLE_HPP
