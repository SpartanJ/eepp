#ifndef EE_GRAPHICS_STATELISTDRAWABLE_HPP
#define EE_GRAPHICS_STATELISTDRAWABLE_HPP

#include <eepp/graphics/statefuldrawable.hpp>
#include <map>

namespace EE { namespace Graphics {

class EE_API StateListDrawable : public StatefulDrawable {
  public:
	static StateListDrawable* New( const std::string& name = "" );

	explicit StateListDrawable( const std::string& name = "" );

	virtual ~StateListDrawable();

	virtual Sizef getSize();

	virtual Sizef getPixelsSize();

	virtual Sizef getSize( const Uint32& state );

	virtual Sizef getPixelsSize( const Uint32& state );

	virtual void draw();

	virtual void draw( const Vector2f& position );

	virtual void draw( const Vector2f& position, const Sizef& size );

	virtual bool isStateful();

	virtual StatefulDrawable* setState( Uint32 state );

	virtual const Uint32& getState() const;

	virtual Drawable* getStateDrawable( const Uint32& state );

	virtual StateListDrawable* setStateDrawable( const Uint32& state, Drawable* drawable,
												 bool ownIt = false );

	virtual Sizef getStateSize( const Uint32& state );

	virtual StateListDrawable* setStateColor( const Uint32& state, const Color& color );

	virtual Color getStateColor( const Uint32& state );

	virtual StateListDrawable* setStateAlpha( const Uint32& state, const Uint8& alpha );

	virtual Uint8 getStateAlpha( const Uint32& state );

	bool hasDrawableState( const Uint32& state ) const;

	bool hasDrawableStateColor( const Uint32& state ) const;

	void clearDrawables();

  protected:
	Uint32 mCurrentState;
	Drawable* mCurrentDrawable;
	std::map<Uint32, Drawable*> mDrawables;
	std::map<Drawable*, bool> mDrawablesOwnership;
	std::map<Uint32, Color> mDrawableColors;

	StateListDrawable( Type type, const std::string& name = "" );

	virtual void onColorFilterChange();
};

}} // namespace EE::Graphics

#endif
