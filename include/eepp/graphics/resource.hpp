#ifndef EE_GRAPHICS_RESOURCE_HPP
#define EE_GRAPHICS_RESOURCE_HPP

#include <atomic>
#include <cstddef>
#include <memory>

#include <eepp/core.hpp>

namespace EE { namespace Graphics {

class Texture;

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

template <typename T> using ResourcePtr = std::shared_ptr<T>;
template <typename T> using ResourceWeakPtr = std::weak_ptr<T>;

/** Shared accounting state kept by both a resource and its live-registry record. */
class ResourceMetrics {
  public:
	std::size_t getMemoryBytes() const { return mMemoryBytes.load( std::memory_order_relaxed ); }

  private:
	friend class Texture;

	void setMemoryBytes( std::size_t bytes ) {
		mMemoryBytes.store( bytes, std::memory_order_relaxed );
	}

	std::atomic<std::size_t> mMemoryBytes{ 0 };
};

/** Deleter used by eepp resource control blocks so EE_MEMORY_MANAGER sees the matching eeDelete. */
template <typename T> struct ResourceDeleter {
	void operator()( T* resource ) const noexcept { eeDelete( resource ); }
};

}} // namespace EE::Graphics

#endif
