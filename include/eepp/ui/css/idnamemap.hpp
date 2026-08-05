#ifndef EE_UI_CSS_IDNAMEMAP_HPP
#define EE_UI_CSS_IDNAMEMAP_HPP

#include <eepp/core/containers.hpp>
#include <eepp/system/log.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace EE { namespace UI { namespace CSS {

/**
 * @brief Maps dense process-local IDs to canonical names and back.
 *
 * The map is the full-string reverse lookup for a dense ID space. It owns no
 * definitions: it only tracks which ID belongs to which canonical (or alias)
 * name. Built-in IDs must be registered with addBuiltin() before finalizeBuiltins()
 * seals the built-in range; runtime names then receive IDs via getOrCreateId().
 *
 * Numeric IDs are unstable internal identifiers. Names are the stable external
 * representation. All names passed into the map are expected to already be
 * lowercase and trimmed; the map performs no hidden normalization and no
 * hash-only equality.
 */
template <typename Id, std::size_t MaxIds> class IdNameMap {
  public:
	IdNameMap() {
		mNames.emplace_back(); // index zero is Invalid with an empty name
		mNextCustomId = static_cast<Id>( 0 );
	}

	/**
	 * @brief Registers a built-in ID with its canonical name.
	 *
	 * Valid only before custom allocation begins. Rejects Invalid, IDs at or
	 * beyond MaxIds, duplicate names, and assigning a different name to an
	 * occupied ID.
	 *
	 * @return true on success.
	 */
	bool addBuiltin( Id id, const std::string& canonicalName ) {
		if ( mBuiltinsFinalized ) {
			EE::System::Log::error(
				"IdNameMap: cannot register built-in \"%s\" after finalization.",
				canonicalName.c_str() );
			return false;
		}
		const auto underlying = static_cast<std::underlying_type_t<Id>>( id );
		const auto maxUnderlying = static_cast<std::underlying_type_t<Id>>( MaxIds );
		if ( underlying == 0 ) {
			EE::System::Log::error( "IdNameMap: cannot register Invalid built-in \"%s\".",
									canonicalName.c_str() );
			return false;
		}
		if ( underlying >= maxUnderlying ) {
			EE::System::Log::error( "IdNameMap: built-in ID %d for \"%s\" is out of range.",
									underlying, canonicalName.c_str() );
			return false;
		}
		if ( mIdsByName.find( canonicalName ) != mIdsByName.end() ) {
			EE::System::Log::error( "IdNameMap: duplicate built-in name \"%s\".",
									canonicalName.c_str() );
			return false;
		}
		if ( underlying < mNames.size() && !mNames[underlying].empty() ) {
			EE::System::Log::error(
				"IdNameMap: built-in ID %d already bound to \"%s\", not \"%s\".", underlying,
				mNames[underlying].c_str(), canonicalName.c_str() );
			return false;
		}
		if ( mNames.size() <= underlying )
			mNames.resize( underlying + 1 );
		mNames[underlying] = canonicalName;
		mIdsByName.emplace( canonicalName, id );
		return true;
	}

	/**
	 * @brief Adds an alias that resolves to an existing ID.
	 *
	 * Only the reverse entry is added; no ID is occupied and the canonical name
	 * is never replaced.
	 */
	bool addAlias( const std::string& alias, Id target ) {
		if ( !contains( target ) ) {
			EE::System::Log::error( "IdNameMap: alias \"%s\" targets an unknown ID.",
									alias.c_str() );
			return false;
		}
		const auto existing = mIdsByName.find( alias );
		if ( existing != mIdsByName.end() ) {
			if ( existing->second == target )
				return true;
			EE::System::Log::error( "IdNameMap: alias \"%s\" is already bound to another ID.",
									alias.c_str() );
			return false;
		}
		mIdsByName.emplace( alias, target );
		return true;
	}

	/**
	 * @brief Seals the built-in range and enables runtime ID allocation.
	 *
	 * Verifies every slot in [1, FirstCustomId) is populated. After this call
	 * addBuiltin() is permanently rejected.
	 *
	 * @return true when all built-in slots are populated.
	 */
	bool finalizeBuiltins() {
		const auto firstCustom = static_cast<std::underlying_type_t<Id>>( Id::FirstCustomId );
		bool complete = true;
		for ( std::size_t i = 1; i < firstCustom; ++i ) {
			if ( i >= mNames.size() || mNames[i].empty() ) {
				EE::System::Log::error( "IdNameMap: built-in ID %zu was never registered.", i );
				complete = false;
			}
		}
		if ( mNames.size() > firstCustom ) {
			EE::System::Log::error(
				"IdNameMap: built-in slots beyond FirstCustomId are populated." );
			complete = false;
		}
		if ( complete ) {
			mBuiltinsFinalized = true;
			mNextCustomId = Id::FirstCustomId;
		}
		return complete;
	}

	/**
	 * @brief Returns the ID for a canonical or alias name.
	 *
	 * @return Invalid when the name is unknown.
	 */
	Id getId( std::string_view name ) const {
		const auto it = mIdsByName.find( std::string( name ) );
		return it != mIdsByName.end() ? it->second : static_cast<Id>( 0 );
	}

	Id getId( const std::string& name ) const {
		const auto it = mIdsByName.find( name );
		return it != mIdsByName.end() ? it->second : static_cast<Id>( 0 );
	}

	Id getId( const char* name ) const { return getId( std::string_view( name ) ); }

	/**
	 * @brief Returns the canonical name for an ID.
	 *
	 * @return The empty invalid name for invalid, out-of-range, or unassigned IDs.
	 */
	const std::string& getName( Id id ) const {
		const auto underlying = static_cast<std::underlying_type_t<Id>>( id );
		if ( underlying == 0 || underlying >= mNames.size() )
			return mNames[0];
		return mNames[underlying];
	}

	/**
	 * @brief Returns the existing ID for a canonical name, or allocates a new custom ID.
	 *
	 * Only valid after finalizeBuiltins(). Reaching the capacity returns Invalid
	 * and logs a clear error containing the rejected name.
	 */
	Id getOrCreateId( const std::string& canonicalName ) {
		if ( !mBuiltinsFinalized ) {
			EE::System::Log::error(
				"IdNameMap: getOrCreateId(\"%s\") called before finalizeBuiltins().",
				canonicalName.c_str() );
			return static_cast<Id>( 0 );
		}
		const auto existing = getId( canonicalName );
		if ( existing != static_cast<Id>( 0 ) )
			return existing;
		const auto next = static_cast<std::underlying_type_t<Id>>( mNextCustomId );
		if ( next >= static_cast<std::underlying_type_t<Id>>( MaxIds ) ) {
			EE::System::Log::error( "IdNameMap: ID capacity exhausted (max %zu), rejecting \"%s\".",
									MaxIds, canonicalName.c_str() );
			return static_cast<Id>( 0 );
		}
		if ( mNames.size() <= next )
			mNames.resize( next + 1 );
		mNames[next] = canonicalName;
		mIdsByName.emplace( canonicalName, static_cast<Id>( next ) );
		mNextCustomId = static_cast<Id>( next + 1 );
		return static_cast<Id>( next );
	}

	bool contains( Id id ) const {
		const auto underlying = static_cast<std::underlying_type_t<Id>>( id );
		return underlying > 0 && underlying < mNames.size() && !mNames[underlying].empty();
	}

	bool contains( std::string_view name ) const {
		return mIdsByName.find( std::string( name ) ) != mIdsByName.end();
	}

	bool contains( const std::string& name ) const {
		return mIdsByName.find( name ) != mIdsByName.end();
	}

	bool contains( const char* name ) const { return contains( std::string_view( name ) ); }

	std::size_t size() const { return mIdsByName.size(); }

  private:
	std::vector<std::string> mNames;
	UnorderedMap<std::string, Id> mIdsByName;
	Id mNextCustomId;
	bool mBuiltinsFinalized{ false };
};

}}} // namespace EE::UI::CSS

#endif // EE_UI_CSS_IDNAMEMAP_HPP
