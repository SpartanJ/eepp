#ifndef EE_UI_UIBACKGROUNDDRAWABLE_HPP
#define EE_UI_UIBACKGROUNDDRAWABLE_HPP

#include <eepp/graphics/drawable.hpp>
#include <eepp/ui/border.hpp>

using namespace EE::Graphics;

namespace EE { namespace UI {

class EE_API UIBackgroundDrawable : public Drawable {
  public:
	static UIBackgroundDrawable* New();

	UIBackgroundDrawable();

	virtual ~UIBackgroundDrawable();

	void setSize( const Sizef& size );

	virtual Sizef getSize();

	virtual void draw();

	virtual void draw( const Vector2f& position );

	virtual void draw( const Vector2f& position, const Sizef& size );

	virtual bool isStateful();

	const BorderRadiuses& getRadiuses() const;

	void setRadiuses( const BorderRadiuses& radiuses );

	void setRadius( const Uint32& radius );

	Int32 getRadius() const;

  protected:
	VertexBuffer* mVertexBuffer;
	Sizef mSize;
	BorderRadiuses mRadiuses;
	bool mNeedsUpdate;
	bool mColorNeedsUpdate;

	virtual void onAlphaChange();

	virtual void onColorFilterChange();

	virtual void onPositionChange();

	void update();
};

}} // namespace EE::UI

#endif // EE_UI_UIBACKGROUNDDRAWABLE_HPP
