#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOS || \
	EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD || \
	EE_PLATFORM == EE_PLATFORM_HAIKU || EE_PLATFORM == EE_PLATFORM_IOS || \
	EE_PLATFORM == EE_PLATFORM_ANDROID
#include "utest.hpp"
#include <array>
#include <ctime>
#include <eepp/system/sys.hpp>

UTEST( SysProcess, inspectionReturnsCurrentProcessAndExactArgumentsWhenAvailable ) {
	using EE::System::Sys;
	const auto basic = Sys::getProcessInfo( Sys::getProcessID() );
	EXPECT_FALSE( basic.name.empty() );
	EXPECT_GT( basic.parentID, 0u );
	EXPECT_TRUE( basic.arguments.empty() );
	const auto full = Sys::getProcessInfo( Sys::getProcessID(), true );
	EXPECT_STREQ( basic.name.c_str(), full.name.c_str() );
	EXPECT_EQ( basic.parentID, full.parentID );
#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOS || \
	EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD || \
	EE_PLATFORM == EE_PLATFORM_ANDROID
	EXPECT_FALSE( full.arguments.empty() );
#endif
	EXPECT_TRUE( Sys::getProcessInfo( 0, true ).name.empty() );
	EXPECT_EQ( Sys::getProcessInfo( 0, true ).parentID, 0u );
	EXPECT_TRUE(
		Sys::getProcessInfo( static_cast<EE::System::ProcessID>( -1 ), true ).arguments.empty() );
}

// getProcessCreationTime() is implemented on these platforms only; elsewhere it returns -1.
#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOS || \
	EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD || \
	EE_PLATFORM == EE_PLATFORM_HAIKU
UTEST( SysProcess, creationTimeIsStableForTheSameProcess ) {
	using EE::System::Sys;
	// Process identity checks compare this value: it must not drift across a second boundary.
	const auto first = Sys::getProcessCreationTime( Sys::getProcessID() );
	ASSERT_GT( first, 0 );
#if EE_PLATFORM != EE_PLATFORM_HAIKU
	// Seconds since the epoch, no later than now (Haiku reports time since boot).
	EXPECT_LE( first, static_cast<EE::Int64>( std::time( nullptr ) ) );
#endif
	for ( int i = 0; i < 25; ++i ) {
		Sys::sleep( EE::System::Milliseconds( 50 ) );
		EXPECT_EQ( Sys::getProcessCreationTime( Sys::getProcessID() ), first );
	}
	EXPECT_EQ( Sys::getProcessCreationTime( static_cast<EE::System::ProcessID>( -1 ) ), -1 );
}
#endif

UTEST( SysProcess, cryptographicRandomnessUsesCallerStorageAndRejectsInvalidInput ) {
	using EE::System::Sys;
	std::array<unsigned char, 32> bytes{};
	ASSERT_TRUE( Sys::getRandomBytes( bytes.data(), bytes.size() ) );
	bool nonzero = false;
	for ( auto byte : bytes )
		nonzero |= byte != 0;
	EXPECT_TRUE( nonzero );
	EXPECT_TRUE( Sys::getRandomBytes( nullptr, 0 ) );
	EXPECT_FALSE( Sys::getRandomBytes( nullptr, 1 ) );
}

#endif
