#include "../../eepp/ui/accessibility/accessibilitytextindex.hpp"
#include "utest.h"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <future>
#include <random>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::Doc;

namespace {

/** Tests the internal index without native clients or a platform-specific exported API. */
class IndexClient : public TextDocument::Client {
  public:
	IndexClient( TextDocument& document, AccessibilityTextIndex& index ) :
		document( document ), index( index ) {
		document.registerClient( this );
	}

	~IndexClient() { document.unregisterClient( this ); }

	void onDocumentTextChanged( const DocumentContentChange& change ) override {
		if ( queryBeforeText ) {
			index.ensure( document );
			++earlyQueries;
		}
		if ( nested ) {
			auto edit = std::move( nested );
			edit();
		}
		index.changed( document, &change );
		if ( duplicate )
			index.changed( document, &change );
	}

	void onDocumentUndoRedo( const TextDocument::UndoRedo& ) override {}

	void onDocumentCursorChange( const TextPosition& ) override {
		if ( queryOnCursor ) {
			index.ensure( document );
			++cursorQueries;
		}
	}

	void onDocumentSelectionChange( const TextRange& ) override {}

	void onDocumentLineCountChange( const size_t&, const size_t& ) override {}

	void onDocumentLineChanged( const Int64& ) override {
		if ( queryBeforeText )
			index.ensure( document );
	}

	void onDocumentSaved( TextDocument* ) override {}

	void onDocumentClosed( TextDocument* ) override {}

	void onDocumentDirtyOnFileSystem( TextDocument* ) override {}

	void onDocumentMoved( TextDocument* ) override {}

	void onDocumentReset( TextDocument* ) override { index.changed( document, nullptr ); }

	Type getTextDocumentClientType() override { return Type::Auxiliary; }

	TextDocument& document;
	AccessibilityTextIndex& index;
	std::function<void()> nested;
	bool duplicate{ false };
	bool queryBeforeText{ false };
	bool queryOnCursor{ false };
	size_t earlyQueries{};
	size_t cursorQueries{};
};

void load( TextDocument& document, std::string_view text ) {
	document.loadFromMemory( reinterpret_cast<const Uint8*>( text.data() ), text.size() );
}

/** Reference includes each stored line's newline, but exposes no final sentinel newline. */
bool matchesReference( TextDocument& document, AccessibilityTextIndex& index ) {
	index.ensure( document );
	Int64 offset = 0;
	for ( size_t line = 0; line < document.linesCount(); ++line ) {
		for ( size_t column = 0; column < document.getLineLength( line ); ++column ) {
			if ( index.offset( { static_cast<Int64>( line ), static_cast<Int64>( column ) } ) !=
					 offset ||
				 index.position( static_cast<Int32>( offset ) ) != TextPosition( line, column ) )
				return false;
			++offset;
		}
	}
	return index.length() == offset - 1;
}

} // namespace

UTEST( AccessibilityTextIndex, ConversionsAndUnicodeMatchReference ) {
	for ( const std::string_view text : { "", "a", "a\n", "\n\n", "ab\ncd\nef", "a\r\nb\r\n",
										  "A😀é\n日本語\n🧑‍💻" } ) {
		TextDocument document;
		load( document, text );
		AccessibilityTextIndex index;
		EXPECT_TRUE( matchesReference( document, index ) );
		EXPECT_TRUE( index.position( -1 ) == TextPosition( 0, 0 ) );
		const size_t last = document.linesCount() - 1;
		EXPECT_TRUE( index.position( std::numeric_limits<Int32>::max() ) ==
					 TextPosition( last, document.getLineLength( last ) ) );
		EXPECT_EQ( AccessibilityTextIndex::clampOffset( std::numeric_limits<Int64>::max() ),
				   std::numeric_limits<Int32>::max() );
		EXPECT_EQ( AccessibilityTextIndex::clampOffset( -1 ), 0 );
	}
}

UTEST( AccessibilityTextIndex, IncrementalStructuralEditsAndUndo ) {
	TextDocument document;
	load( document, "ab\ncd\nef" );
	AccessibilityTextIndex index;
	index.ensure( document );
	IndexClient client( document, index );
	document.insert( 0, { 1, 1 }, "!" );
	EXPECT_TRUE( matchesReference( document, index ) );
	document.insert( 0, { 1, 1 }, "x\ny\n" );
	EXPECT_TRUE( matchesReference( document, index ) );
	document.remove( 0, { { 0, 1 }, { 3, 1 } } );
	EXPECT_TRUE( matchesReference( document, index ) );
	document.setSelection( { 0, 0 }, document.endOfDoc() );
	document.replaceSelection( "a\nβ\n😀\n" );
	EXPECT_TRUE( matchesReference( document, index ) );
	document.undo();
	EXPECT_TRUE( matchesReference( document, index ) );
	document.redo();
	EXPECT_TRUE( matchesReference( document, index ) );
	EXPECT_EQ( index.rebuilds(), 1u );
}

UTEST( AccessibilityTextIndex, GeneratedEditsMatchReference ) {
	TextDocument document;
	AccessibilityTextIndex index;
	index.ensure( document );
	IndexClient client( document, index );
	std::mt19937 random( 42 );
	const String inputs[] = { "a", "\n", "x\ny", String::fromUtf8( std::string_view{ "😀" } ),
							  String::fromUtf8( std::string_view{ "é" } ) };
	for ( size_t edit = 0; edit < 300; ++edit ) {
		const auto position = index.position( random() % ( index.length() + 1 ) );
		if ( edit % 3 && index.length() ) {
			const auto other = index.position( random() % ( index.length() + 1 ) );
			document.remove( 0, { position, other } );
		} else {
			document.insert( 0, position, inputs[random() % 5] );
		}
		EXPECT_TRUE( matchesReference( document, index ) );
	}
}

UTEST( AccessibilityTextIndex, OrdinaryEditReadsOneLineOfLargeDocument ) {
	TextDocument document;
	std::string text;
	static constexpr size_t Lines = 100000;
	text.reserve( Lines * 2 );
	for ( size_t line = 0; line < Lines; ++line )
		text += "a\n";
	load( document, text );
	AccessibilityTextIndex index;
	index.ensure( document );
	IndexClient client( document, index );
	const auto reads = index.lineReads();
	document.insert( 0, { Lines - 1, 0 }, "x" );
	index.ensure( document );
	EXPECT_EQ( index.lineReads(), reads + 1 );
	EXPECT_EQ( index.rebuilds(), 1u );
	EXPECT_EQ( index.offset( { Lines - 1, 0 } ), static_cast<Int32>( ( Lines - 1 ) * 2 ) );
	EXPECT_EQ( index.length(), static_cast<Int32>( text.size() + 1 ) );
	for ( Int64 line : { 0, 255, 256, 511, 512, 99999 } ) {
		const auto offset = static_cast<Int32>( line * 2 );
		EXPECT_TRUE( index.position( offset ) == TextPosition( line, 0 ) );
	}
}

UTEST( AccessibilityTextIndex, BlockBoundariesMatchReferenceAfterStructuralEdits ) {
	TextDocument document;
	std::string text;
	for ( size_t line = 0; line < 1024; ++line )
		text += std::string( line % 5, 'a' ) + '\n';
	load( document, text );
	AccessibilityTextIndex index;
	index.ensure( document );
	IndexClient client( document, index );
	EXPECT_TRUE( matchesReference( document, index ) );
	document.insert( 0, { 256, 0 }, "a\nb\nc\n" );
	EXPECT_TRUE( matchesReference( document, index ) );
	document.remove( 0, { { 254, 0 }, { 514, 0 } } );
	EXPECT_TRUE( matchesReference( document, index ) );
}

UTEST( AccessibilityTextIndex, DuplicateEarlyAndNestedCallbacksStayCorrect ) {
	TextDocument document;
	load( document, "ab\ncd" );
	AccessibilityTextIndex index;
	index.ensure( document );
	IndexClient client( document, index );
	client.duplicate = true;
	document.insert( 0, { 0, 1 }, "x\ny" );
	EXPECT_TRUE( matchesReference( document, index ) );
	EXPECT_EQ( index.rebuilds(), 1u );
	client.queryBeforeText = true;
	document.insert( 0, { 0, 0 }, "before\n" );
	EXPECT_TRUE( matchesReference( document, index ) );
	EXPECT_EQ( client.earlyQueries, 1u );
	client.nested = [&] { document.insert( 0, { 0, 0 }, "nested\n" ); };
	document.insert( 0, { 0, 0 }, "outer\n" );
	EXPECT_TRUE( matchesReference( document, index ) );
}

UTEST( AccessibilityTextIndex, MultipleCursorsAndLargePasteRecoverCorrectly ) {
	TextDocument document;
	load( document, "one\ntwo\nthree" );
	AccessibilityTextIndex index;
	index.ensure( document );
	IndexClient client( document, index );
	document.addSelection( TextPosition( 2, 1 ) );
	document.insert( 1, { 2, 1 }, "β\n" );
	document.insert( 0, { 0, 1 }, "x\ny" );
	EXPECT_TRUE( matchesReference( document, index ) );
	EXPECT_EQ( index.rebuilds(), 1u );

	// Large changes invalidate without reading any lines in the notification callback.
	const auto reads = index.lineReads();
	document.insert( 0, { 0, 0 }, String( 65537, 'x' ) );
	EXPECT_EQ( index.lineReads(), reads );
	EXPECT_EQ( index.rebuilds(), 1u );
	EXPECT_TRUE( matchesReference( document, index ) );
	EXPECT_EQ( index.rebuilds(), 2u );
}

UTEST( AccessibilityTextIndex, UndoCursorQueryBeforeMutationDoesNotCommitOldLengths ) {
	TextDocument document;
	load( document, "one\ntwo\nthree" );
	AccessibilityTextIndex index;
	index.ensure( document );
	IndexClient client( document, index );
	document.addSelection( TextPosition( 2, 1 ) );
	document.insert( 1, { 2, 1 }, "x\ny" );
	document.setSelection( TextPosition( 0, 0 ) );
	client.queryOnCursor = true;
	// Undo recreates the missing cursor after incrementing the revision but before changing
	// lines. A native/query cache must not treat that revision as completed structural work.
	document.undo();
	EXPECT_GT( client.cursorQueries, 0u );
	EXPECT_TRUE( matchesReference( document, index ) );
	document.redo();
	EXPECT_TRUE( matchesReference( document, index ) );
}

UTEST( AccessibilityTextIndex, MissedChangesResetsAndWorkerLoadsRecoverLazily ) {
	TextDocument document;
	load( document, "ab\ncd" );
	AccessibilityTextIndex index;
	index.ensure( document );
	document.insert( 0, { 0, 0 }, "a" );
	document.insert( 0, { 0, 0 }, "b\n" );
	EXPECT_TRUE( matchesReference( document, index ) );
	EXPECT_EQ( index.rebuilds(), 2u );
	document.reset();
	EXPECT_TRUE( matchesReference( document, index ) );
	auto worker = std::async( std::launch::async, [&] { load( document, "loaded\n😀" ); } );
	worker.get();
	EXPECT_TRUE( matchesReference( document, index ) );
	EXPECT_EQ( index.rebuilds(), 4u );
}

UTEST( AccessibilityTextIndex, BindingsAreLazySharedAndReleased ) {
	using namespace EE::System;
	using namespace EE::Window;
	UIApplication::Settings settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 );
	settings.accessibilityPolicy = AccessibilityPolicy::Disabled;
	UIApplication app( WindowSettings( 320, 240, "Text index test",
									   WindowStyle::Default | WindowStyle::Hidden,
									   WindowBackend::Default, 32, {}, 1, false, true ),
					   settings );
	ASSERT_NE( app.getUI(), nullptr );
	auto* first = UICodeEditor::New();
	auto* second = UICodeEditor::New();
	TextDocument document;
	load( document, "shared\ntext" );
	AccessibilityTextIndexes indexes;
	indexes.changed( first, document, nullptr );
	EXPECT_EQ( indexes.size(), 0u );
	auto* shared = &indexes.forWidget( first, document );
	EXPECT_TRUE( shared == &indexes.forWidget( second, document ) );
	EXPECT_EQ( indexes.size(), 1u );
	indexes.detach( first );
	EXPECT_EQ( indexes.size(), 1u );
	TextDocument other;
	load( other, "other" );
	indexes.changed( second, other, nullptr );
	EXPECT_EQ( indexes.size(), 0u );
	indexes.forWidget( second, other );
	EXPECT_EQ( indexes.size(), 1u );
	indexes.detach( second );
	EXPECT_EQ( indexes.size(), 0u );
}
