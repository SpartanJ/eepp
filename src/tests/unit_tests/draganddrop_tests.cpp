#include "../../eepp/window/draganddrop/dragsource.hpp"
#include "utest.hpp"
#include <eepp/window/draganddrop.hpp>

using namespace EE;
using namespace EE::Window;

namespace {
class TestDragSource : public Private::DragSource {
  public:
	using DragSource::DragSource;

	void tick() override {}

	bool handleEvent( void* ) override { return false; }

	void cancel() override { finish( DragEndReason::Cancelled ); }
};

std::string absolutePath( const std::string& suffix ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return "C:\\" + suffix;
#else
	return "/" + suffix;
#endif
}

std::string uriPrefix() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return "file:///C:/";
#else
	return "file:///";
#endif
}
} // namespace

UTEST( NativeDrag, EncodesReservedBytesUnicodeAndCrLf ) {
	DragOffer offer{ { { absolutePath( "a b#%?\r\n-_.~é.txt" ) } } };
	std::string payload;
	ASSERT_TRUE( DragAndDrop::encodeFileUriList( offer, payload ) );
	EXPECT_STDSTREQ( payload, uriPrefix() + "a%20b%23%25%3F%0D%0A-_.~%C3%A9.txt\r\n" );
}

UTEST( NativeDrag, PreservesOrderAndDuplicates ) {
	DragOffer offer{
		{ { absolutePath( "one" ) }, { absolutePath( "two" ) }, { absolutePath( "one" ) } } };
	std::string payload;
	ASSERT_TRUE( DragAndDrop::encodeFileUriList( offer, payload ) );
	EXPECT_STDSTREQ( payload,
					 uriPrefix() + "one\r\n" + uriPrefix() + "two\r\n" + uriPrefix() + "one\r\n" );
}

UTEST( NativeDrag, KeepsLiteralPercentEscapesAndPathPunctuation ) {
	DragOffer offer{ { { absolutePath( "literal%2F-%41-[]!$&'()+,;=@.txt" ) } } };
	std::string payload;
	ASSERT_TRUE( DragAndDrop::encodeFileUriList( offer, payload ) );
	EXPECT_STDSTREQ( payload, uriPrefix() + "literal%252F-%2541-%5B%5D!$&'()+,;=@.txt\r\n" );
}

UTEST( NativeDrag, SingleFileOfferUsesInlineStorageAndCanGrow ) {
	DragOffer offer{ { { absolutePath( "one" ) } } };
	EXPECT_TRUE( offer.files.is_small() );
	offer.files.push_back( { absolutePath( "two" ) } );
	EXPECT_FALSE( offer.files.is_small() );
	EXPECT_EQ( offer.files.size(), 2u );
	EXPECT_STDSTREQ( offer.files.front().path, absolutePath( "one" ) );
	EXPECT_STDSTREQ( offer.files.back().path, absolutePath( "two" ) );
}

UTEST( NativeDrag, RejectsEmptyRelativeAndMalformedPathsAtomically ) {
	std::string payload = "previous";
	std::string error;
	EXPECT_FALSE( DragAndDrop::encodeFileUriList( {}, payload, &error ) );
	EXPECT_TRUE( payload.empty() );
	EXPECT_FALSE( error.empty() );
	for ( const auto& invalid :
		  { std::string{}, std::string( "relative.txt" ), std::string( "C:relative" ),
			absolutePath( std::string( "x\0y", 3 ) ),
			absolutePath( std::string( "\xC0\xAF", 2 ) ) } ) {
		DragOffer offer{ { { absolutePath( "valid" ) }, { invalid } } };
		EXPECT_FALSE( DragAndDrop::encodeFileUriList( offer, payload ) );
		EXPECT_TRUE( payload.empty() );
	}
}

UTEST( NativeDrag, RejectsUnsupportedActions ) {
	DragOffer offer{ { { absolutePath( "one" ) } } };
	std::string payload;
	for ( DragActions actions : { 0u, 2u, 3u, 4u, 8u, 16u } ) {
		offer.actions = actions;
		EXPECT_FALSE( DragAndDrop::encodeFileUriList( offer, payload ) );
	}
}

UTEST( NativeDrag, CompletionIsOneShotAndOwnsItsCallback ) {
	int completions = 0;
	TestDragSource session(
		nullptr, [owned = std::string( "owned" ), &completions, utest_result]( DragResult result ) {
			EXPECT_STDSTREQ( owned, "owned" );
			EXPECT_EQ( result.reason, DragEndReason::Cancelled );
			EXPECT_EQ( result.performedAction, DragAction::None );
			++completions;
		} );
	EXPECT_TRUE( session.finish( DragEndReason::Cancelled, DragAction::Copy ) );
	EXPECT_FALSE( session.finish( DragEndReason::Dropped, DragAction::Copy ) );
	auto callback = std::move( session.completion );
	callback( std::move( session.result ) );
	EXPECT_EQ( completions, 1 );
}

#if EE_PLATFORM == EE_PLATFORM_WIN
UTEST( NativeDrag, EncodesUncAndRejectsDevicePaths ) {
	std::string payload;
	EXPECT_TRUE(
		DragAndDrop::encodeFileUriList( { { { "\\\\server\\share\\a b.txt" } } }, payload ) );
	EXPECT_STDSTREQ( payload, "file://server/share/a%20b.txt\r\n" );
	EXPECT_TRUE( DragAndDrop::encodeFileUriList( { { { "C:/mixed\\literal%2F.txt" },
													 { "D:/forward/slashes.txt" },
													 { "//server/share/a#b.txt" },
													 { "\\\\server@SSL\\share\\file.txt" },
													 { "//server:port/share/file.txt" } } },
												 payload ) );
	EXPECT_STDSTREQ( payload, "file:///C:/mixed/literal%252F.txt\r\n"
							  "file:///D:/forward/slashes.txt\r\n"
							  "file://server/share/a%23b.txt\r\n"
							  "file://server%40SSL/share/file.txt\r\n"
							  "file://server%3Aport/share/file.txt\r\n" );
	EXPECT_FALSE( DragAndDrop::encodeFileUriList( { { { "\\\\?\\C:\\file" } } }, payload ) );
	EXPECT_FALSE( DragAndDrop::encodeFileUriList( { { { "\\\\.\\device" } } }, payload ) );
	EXPECT_FALSE( DragAndDrop::encodeFileUriList( { { { "\\\\server\\\\file" } } }, payload ) );
	EXPECT_FALSE( DragAndDrop::encodeFileUriList( { { { "//server//file" } } }, payload ) );
}
#endif
