#pragma once
#include <eepp/config.hpp>

#include <cstdint>
#include <optional>
#include <string>

namespace EE { namespace System {

class EE_API UUID {
  public:
	UUID();

	UUID( uint64_t high, uint64_t low );

	UUID( bool autocreate );

	UUID( const UUID& other ) = default;

	UUID( UUID&& other ) = default;

	UUID& operator=( const UUID& other ) = default;

	UUID& operator=( UUID&& other ) = default;

	static std::optional<UUID> fromString( const std::string_view& str );

	std::string toString() const;

	void refresh();

	bool isInitialized() const;

	bool operator==( const UUID& other ) const {
		return mHigh == other.mHigh && mLow == other.mLow;
	}

	bool operator!=( const UUID& other ) const { return !( *this == other ); }

	bool operator<( const UUID& other ) const {
		return mHigh < other.mHigh || ( mHigh == other.mHigh && mLow < other.mLow );
	}

	bool operator>( const UUID& other ) const { return other < *this; }

	bool operator<=( const UUID& other ) const { return !( *this > other ); }

	bool operator>=( const UUID& other ) const { return !( *this < other ); }

  protected:
	uint64_t mHigh{ 0 }; // Bits 0-63 of the UUID
	uint64_t mLow{ 0 };	 // Bits 64-127 of the UUID
};

}} // namespace EE::System
