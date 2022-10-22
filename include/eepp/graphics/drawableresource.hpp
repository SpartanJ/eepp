#ifndef EE_GRAPHICS_DRAWABLERESOURCE_HPP
#define EE_GRAPHICS_DRAWABLERESOURCE_HPP

#include <eepp/core.hpp>
#include <eepp/graphics/drawable.hpp>

namespace EE { namespace Graphics {

class EE_API DrawableResource : public Drawable {
  public:
	enum Event { Load, Change, Unload };

	virtual ~DrawableResource();

	typedef std::function<void( Event, DrawableResource* )> OnResourceChangeCallback;

	/** @return The DrawableResource Id. The Id is the String::hash of the name. */
	const String::HashType& getId() const;

	/** @return The DrawableResource Name. */
	const std::string getName() const;

	/** Sets the DrawableResource Name, it will also change the Id. */
	void setName( const std::string& name );

	/** Always true */
	bool isDrawableResource() const;

	/** Push a new on resource change callback.
	 * @return The Callback Id
	 */
	Uint32 pushResourceChangeCallback( const OnResourceChangeCallback& cb );

	/** Pop the on resource change callback id indicated. */
	void popResourceChangeCallback( const Uint32& callbackId );

  protected:
	std::string mName;
	String::HashType mId;
	Uint32 mNumCallBacks;
	std::map<Uint32, OnResourceChangeCallback> mCallbacks;

	explicit DrawableResource( Type drawableType );

	DrawableResource( Type drawableType, const std::string& name );

	void createUnnamed();

	virtual void onResourceChange();

	void sendEvent( const Event& event );
};

}} // namespace EE::Graphics

#endif
