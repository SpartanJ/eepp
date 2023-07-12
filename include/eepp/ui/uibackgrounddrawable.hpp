#ifndef EE_UI_UIBACKGROUNDDRAWABLE_HPP
#define EE_UI_UIBACKGROUNDDRAWABLE_HPP

#include <eepp/graphics/drawable.hpp>
#include <eepp/ui/border.hpp>

using namespace EE::Graphics;

namespace EE { namespace UI {

class UINode;

class EE_API UIBackgroundDrawable : public Drawable {
  public:
	static UIBackgroundDrawable* New( const UINode* owner );

	UIBackgroundDrawable( const UINode* owner );

	virtual ~UIBackgroundDrawable();

	void setSize( const Sizef& size );

	virtual Sizef getSize();

	virtual Sizef getPixelsSize();

	virtual void draw();

	virtual void draw( const Vector2f& position );

	virtual void draw( const Vector2f& position, const Sizef& size );

	virtual bool isStateful();

	const BorderRadiuses& getRadiuses() const;

	bool hasRadius() const;

	void setRadiuses( const BorderRadiuses& radiuses );

	void setRadius( const Uint32& radius );

	Int32 getRadius() const;

	void invalidate();

	void setTopWidth( const std::string& topWidth );

	void setBottomWidth( const std::string& bottomWidth );

	void setTopLeftRadius( const std::string& radius );

	void setTopRightRadius( const std::string& radius );

	void setBottomLeftRadius( const std::string& radius );

	void setBottomRightRadius( const std::string& radius );

	bool isSmooth() const;

	void setSmooth( bool smooth );

  protected:
	const UINode* mOwner;
	BorderRadiuseStr mRadiusesStr;
	VertexBuffer* mVertexBuffer;
	Sizef mSize;
	BorderRadiuses mRadiuses;
	bool mNeedsUpdate;
	bool mNeedsRadiusUpdate;
	bool mColorNeedsUpdate;
	bool mSmooth{ false };

	virtual void onAlphaChange();

	virtual void onColorFilterChange();

	virtual void onPositionChange();

	void update();

	void updateRadiuses();
};

}} // namespace EE::UI

#endif // EE_UI_UIBACKGROUNDDRAWABLE_HPP
