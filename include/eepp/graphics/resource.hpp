#ifndef EE_GRAPHICS_RESOURCE_HPP
#define EE_GRAPHICS_RESOURCE_HPP

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include <eepp/core.hpp>

namespace EE { namespace Graphics {

/** Immutable identity assigned once when a resource is created. Resource IDs are process-wide and
 * are not reset when Engine state is recreated by tests. */
class ResourceId {
  public:
	constexpr ResourceId() = default;
	explicit constexpr ResourceId( Uint64 value ) : mValue( value ) {}

	constexpr Uint64 value() const { return mValue; }
	explicit constexpr operator bool() const { return mValue != 0; }

	constexpr bool operator==( const ResourceId& other ) const { return mValue == other.mValue; }
	constexpr bool operator!=( const ResourceId& other ) const { return !( *this == other ); }
	constexpr bool operator<( const ResourceId& other ) const { return mValue < other.mValue; }

  private:
	Uint64 mValue{ 0 };
};

/** Immutable semantic lookup key. Catalog equality always compares the complete key value. */
class ResourceKey {
  public:
	ResourceKey() = default;
	explicit ResourceKey( std::string value ) : mValue( std::move( value ) ) {}

	const std::string& value() const { return mValue; }
	bool empty() const { return mValue.empty(); }

	bool operator==( const ResourceKey& other ) const { return mValue == other.mValue; }
	bool operator!=( const ResourceKey& other ) const { return !( *this == other ); }

  private:
	std::string mValue;
};

template <typename T> using ResourcePtr = std::shared_ptr<T>;
template <typename T> using ResourceWeakPtr = std::weak_ptr<T>;

/** Deleter used by eepp resource control blocks so EE_MEMORY_MANAGER sees the matching eeDelete. */
template <typename T> struct ResourceDeleter {
	void operator()( T* resource ) const noexcept { eeDelete( resource ); }
};

template <typename T, typename... Args> ResourcePtr<T> makeResource( Args&&... args ) {
	return ResourcePtr<T>( eeNew( T, ( std::forward<Args>( args )... ) ), ResourceDeleter<T>() );
}

}} // namespace EE::Graphics

namespace std {

template <> struct hash<EE::Graphics::ResourceId> {
	std::size_t operator()( const EE::Graphics::ResourceId& id ) const noexcept {
		return std::hash<EE::Uint64>{}( id.value() );
	}
};

} // namespace std

#endif
