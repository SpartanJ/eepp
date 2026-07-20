#ifndef EE_GRAPHICS_DRAWABLERESOURCE_HPP
#define EE_GRAPHICS_DRAWABLERESOURCE_HPP

#include <eepp/core.hpp>
#include <eepp/graphics/drawable.hpp>
#include <memory>

namespace EE { namespace Graphics {

class DrawableResource;

struct DrawableResourceCallbackState {
	using Callback = std::function<void( DrawableResource& )>;
	Uint32 nextId{ 0 };
	UnorderedMap<Uint32, Callback> callbacks;
};

class EE_API DrawableResourceConnection {
  public:
	DrawableResourceConnection() = default;
	~DrawableResourceConnection();
	DrawableResourceConnection( DrawableResourceConnection&& other ) noexcept;
	DrawableResourceConnection& operator=( DrawableResourceConnection&& other ) noexcept;
	DrawableResourceConnection( const DrawableResourceConnection& ) = delete;
	DrawableResourceConnection& operator=( const DrawableResourceConnection& ) = delete;

	void disconnect();
	explicit operator bool() const;

  private:
	friend class DrawableResource;
	DrawableResourceConnection( std::weak_ptr<DrawableResourceCallbackState> state, Uint32 id );

	std::weak_ptr<DrawableResourceCallbackState> mState;
	Uint32 mId{ 0 };
};

class EE_API DrawableResource : public Drawable {
  public:
	virtual ~DrawableResource();

	using OnResourceChangeCallback = DrawableResourceCallbackState::Callback;

	/** @return The DrawableResource Id. The Id is the String::hash of the name. */
	const String::HashType& getId() const;

	/** @return The DrawableResource Name. */
	const std::string getName() const;

	/** Sets the DrawableResource Name, it will also change the Id. */
	void setName( const std::string& name );

	/** Always true */
	bool isDrawableResource() const;

	/** Connects a callback for mutable resource data changes. */
	DrawableResourceConnection connectResourceChange( OnResourceChangeCallback cb );

  protected:
	std::string mName;
	String::HashType mId;
	std::shared_ptr<DrawableResourceCallbackState> mCallbackState;

	explicit DrawableResource( Type drawableType );

	DrawableResource( Type drawableType, const std::string& name );

	void createUnnamed();

	virtual void onResourceChange();

	void sendResourceChanged();
};

}} // namespace EE::Graphics

#endif
