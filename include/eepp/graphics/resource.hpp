#ifndef EE_GRAPHICS_RESOURCE_HPP
#define EE_GRAPHICS_RESOURCE_HPP

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include <eepp/config.hpp>
#include <eepp/core/memorymanager.hpp>
#include <eepp/thirdparty/unordered_dense.h>

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

/**
 * Strong 64-bit hash of a semantic resource name.
 *
 * This is a fast, process-local convenience key for trusted resource names. It is deliberately
 * distinct from String::HashType so legacy 32-bit hashes cannot enter resource-name APIs through
 * an implicit integer conversion. Complete ResourceKey values remain the authoritative identity
 * whenever collision safety or persistence is required.
 */
class ResourceNameHash {
  public:
	constexpr ResourceNameHash() = default;
	explicit constexpr ResourceNameHash( Uint64 value ) : mValue( value ) {}

	constexpr Uint64 value() const { return mValue; }
	explicit constexpr operator bool() const { return mValue != 0; }

	constexpr bool operator==( const ResourceNameHash& other ) const {
		return mValue == other.mValue;
	}
	constexpr bool operator!=( const ResourceNameHash& other ) const { return !( *this == other ); }
	constexpr bool operator<( const ResourceNameHash& other ) const {
		return mValue < other.mValue;
	}

  private:
	Uint64 mValue{ 0 };
};

/**
 * Hashes a semantic resource name with the vendored wyhash implementation used by
 * UnorderedMap. Do not serialize the result: use the complete ResourceKey in persistent formats.
 */
inline ResourceNameHash resourceNameHash( std::string_view name ) {
	return ResourceNameHash( ankerl::unordered_dense::hash<std::string_view>{}( name ) );
}

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

template <> struct hash<EE::Graphics::ResourceNameHash> {
	std::size_t operator()( const EE::Graphics::ResourceNameHash& hash ) const noexcept {
		return std::hash<EE::Uint64>{}( hash.value() );
	}
};

} // namespace std

#endif
