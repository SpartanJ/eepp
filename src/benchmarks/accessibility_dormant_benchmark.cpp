#include "../tests/unit_tests/utest.hpp"

#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/ui/models/model.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uiprogressbar.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uitextview.hpp>

// Define EE_BENCH_NO_ACCESSIBILITY when building against a tree without accessibility support.
#if !defined( EE_BENCH_NO_ACCESSIBILITY ) && \
	__has_include( <eepp/ui/accessibility/accessibilitymanager.hpp> )
#define EE_BENCH_HAS_ACCESSIBILITY 1
#include "../eepp/ui/accessibility/accessibilitybackend.hpp"
#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#endif

#include <cstdio>
#include <cstdlib>
#include <deque>
#include <new>

using namespace EE;
using namespace EE::Scene;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Doc;
using namespace EE::UI::Models;
using namespace EE::Window;

// Dormant-path benchmarks: no accessibility client is attached. The same source builds against a
// tree without accessibility support (for a baseline comparison) and against the branch.
//
// Allocations are counted by replacing the global operator new in this benchmark binary only.
// Only the measuring thread counts: background threads (native accessibility I/O, worker pools)
// would otherwise add nondeterministic noise, and the UI thread's cost is what is measured.

namespace {

thread_local bool tCountAllocations{ false };
thread_local Uint64 tAllocations{ 0 };
thread_local Uint64 tAllocatedBytes{ 0 };

void* countedAllocate( std::size_t size ) {
	if ( tCountAllocations ) {
		++tAllocations;
		tAllocatedBytes += size;
	}
	if ( void* memory = std::malloc( size ? size : 1 ) )
		return memory;
	throw std::bad_alloc();
}

} // namespace

void* operator new( std::size_t size ) {
	return countedAllocate( size );
}

void* operator new[]( std::size_t size ) {
	return countedAllocate( size );
}

void operator delete( void* memory ) noexcept {
	std::free( memory );
}

void operator delete[]( void* memory ) noexcept {
	std::free( memory );
}

void operator delete( void* memory, std::size_t ) noexcept {
	std::free( memory );
}

void operator delete[]( void* memory, std::size_t ) noexcept {
	std::free( memory );
}

namespace {

struct Measurement {
	Uint64 allocations{ 0 };
	Uint64 bytes{ 0 };
	double milliseconds{ 0 };
};

/** Counts the operator new calls and bytes `work` makes on this thread, and its wall time. */
template <typename Work> Measurement measure( const Work& work ) {
	tAllocations = 0;
	tAllocatedBytes = 0;
	tCountAllocations = true;
	Clock clock;
	work();
	Measurement result;
	result.milliseconds = clock.getElapsedTime().asMilliseconds();
	tCountAllocations = false;
	result.allocations = tAllocations;
	result.bytes = tAllocatedBytes;
	return result;
}

void report( const char* name, const Measurement& measurement, Uint64 operations ) {
	std::printf( "ACCESSIBILITY_DORMANT %s allocations=%llu bytes=%llu ms=%.3f "
				 "allocations_per_op=%.2f bytes_per_op=%.1f\n",
				 name, static_cast<unsigned long long>( measurement.allocations ),
				 static_cast<unsigned long long>( measurement.bytes ), measurement.milliseconds,
				 operations ? static_cast<double>( measurement.allocations ) / operations : 0.0,
				 operations ? static_cast<double>( measurement.bytes ) / operations : 0.0 );
	std::fflush( stdout );
}

UIApplication::Settings benchmarkSettings() {
	// The default policy: native accessibility is available, but no client is attached.
	return UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 );
}

WindowSettings benchmarkWindow() {
	return WindowSettings( 640, 480, "Accessibility dormant benchmark",
						   WindowStyle::Default | WindowStyle::Hidden, WindowBackend::Default, 32,
						   {}, 1, false, true );
}

/** Retains every change record the way an asynchronous consumer (such as an LSP client) does:
 * wraps it in a vector, queues a copy and copies it out again when draining. */
class RetainingConsumer : public TextDocument::Client {
  public:
	struct Record {
		Uint64 version;
		std::vector<DocumentContentChange> changes;
	};

	void onDocumentTextChanged( const DocumentContentChange& change ) override {
		std::vector<DocumentContentChange> changes{ change };
		mQueue.push_back( { ++mVersion, changes } );
	}

	void drain() {
		while ( !mQueue.empty() ) {
			Record record = mQueue.front();
			mQueue.pop_front();
			mDrained += record.changes.size();
		}
	}

	void onDocumentUndoRedo( const TextDocument::UndoRedo& ) override {}

	void onDocumentCursorChange( const TextPosition& ) override {}

	void onDocumentSelectionChange( const TextRange& ) override {}

	void onDocumentLineCountChange( const size_t&, const size_t& ) override {}

	void onDocumentLineChanged( const Int64& ) override {}

	void onDocumentSaved( TextDocument* ) override {}

	void onDocumentClosed( TextDocument* ) override {}

	void onDocumentDirtyOnFileSystem( TextDocument* ) override {}

	void onDocumentMoved( TextDocument* ) override {}

	void onDocumentReset( TextDocument* ) override {}

	Type getTextDocumentClientType() override { return Type::Auxiliary; }

	Uint64 drained() const { return mDrained; }

  private:
	std::deque<Record> mQueue;
	Uint64 mVersion{ 0 };
	Uint64 mDrained{ 0 };
};

class RowsModel : public Model {
  public:
	explicit RowsModel( size_t rows ) : mRows( rows ) {}

	size_t rowCount( const ModelIndex& ) const override { return mRows; }

	size_t columnCount( const ModelIndex& ) const override { return 1; }

	Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const override {
		return role == ModelRole::Display ? Variant( String::format( "Row %d", index.row() ) )
										  : Variant();
	}

	void insertRow( int row ) {
		beginInsertRows( {}, row, row );
		++mRows;
		endInsertRows();
	}

	void removeRow( int row ) {
		if ( beginDeleteRows( {}, row, row ) ) {
			--mRows;
			endDeleteRows();
		}
	}

  private:
	size_t mRows;
};

} // namespace

UTEST( AccessibilityDormant, LibraryAllocationsAreCounted ) {
	// Global operator new interposition must reach allocations made inside libeepp.
	const auto measurement = measure( [] {
		volatile size_t size = String::toLower( std::string( 256, 'A' ) ).size();
		(void)size;
	} );
	EXPECT_GT( measurement.allocations, 0u );
}

UTEST( AccessibilityDormant, TypingWithRetainingConsumer ) {
	UIApplication app( benchmarkWindow(), benchmarkSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* editor = UICodeEditor::New();
	editor->setParent( app.getUI()->getRoot() );
	RetainingConsumer consumer;
	auto& document = editor->getDocument();
	document.registerClient( &consumer );
	app.getUI()->update( Time::Zero );
	static constexpr int Characters = 5000;
	const auto typing = measure( [&] {
		for ( int i = 0; i < Characters; ++i )
			document.textInput( "a" );
		consumer.drain();
	} );
	report( "typing", typing, Characters );
	const auto deleting = measure( [&] {
		for ( int i = 0; i < Characters; ++i )
			document.deleteToPreviousChar();
		consumer.drain();
	} );
	report( "deleting_characters", deleting, Characters );
	document.unregisterClient( &consumer );
	EXPECT_EQ( consumer.drained(), static_cast<Uint64>( Characters * 2 ) );
}

UTEST( AccessibilityDormant, LargeDeletionWithRetainingConsumer ) {
	UIApplication app( benchmarkWindow(), benchmarkSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* editor = UICodeEditor::New();
	editor->setParent( app.getUI()->getRoot() );
	auto& document = editor->getDocument();
	std::string contents;
	static constexpr int Lines = 100000;
	contents.reserve( Lines * 32 );
	for ( int line = 0; line < Lines; ++line )
		contents += "line " + std::to_string( line ) + " with some text\n";
	document.textInput( String::fromUtf8( contents ) );
	app.getUI()->update( Time::Zero );
	RetainingConsumer consumer;
	document.registerClient( &consumer );
	const Uint64 removedBytes = contents.size() * sizeof( String::StringBaseType );
	const auto deletion = measure( [&] {
		document.selectAll();
		document.deleteSelection();
		consumer.drain();
	} );
	report( "large_deletion", deletion, 1 );
	std::printf( "ACCESSIBILITY_DORMANT large_deletion_removed_text_bytes=%llu "
				 "allocated_bytes_per_removed_byte=%.2f\n",
				 static_cast<unsigned long long>( removedBytes ),
				 static_cast<double>( deletion.bytes ) / removedBytes );
	document.unregisterClient( &consumer );
	EXPECT_TRUE( document.isEmpty() );
	// The removed text is extracted once for undo; change records must not carry it into
	// consumers that queue them (6 bytes per removed byte when they did, 2 without).
	EXPECT_LT( static_cast<double>( deletion.bytes ) / removedBytes, 3.0 );
}

UTEST( AccessibilityDormant, SpinBoxConstruction ) {
	UIApplication app( benchmarkWindow(), benchmarkSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	static constexpr int SpinBoxes = 1000;
	std::vector<UISpinBox*> spinBoxes;
	spinBoxes.reserve( SpinBoxes );
	const auto construction = measure( [&] {
		for ( int i = 0; i < SpinBoxes; ++i ) {
			auto* spinBox = UISpinBox::New();
			spinBox->setParent( app.getUI()->getRoot() );
			spinBoxes.push_back( spinBox );
		}
	} );
	report( "spinbox_construction", construction, SpinBoxes );
	const auto destruction = measure( [&] {
		for ( auto* spinBox : spinBoxes )
			eeDelete( spinBox );
	} );
	report( "spinbox_destruction", destruction, SpinBoxes );
}

UTEST( AccessibilityDormant, DeepHierarchyLifecycle ) {
	UIApplication app( benchmarkWindow(), benchmarkSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	app.getUI()->update( Time::Zero );
	static constexpr int Chains = 200;
	static constexpr int Depth = 64;
	const auto lifecycle = measure( [&] {
		for ( int chain = 0; chain < Chains; ++chain ) {
			UIWidget* top = UIWidget::New();
			top->setParent( app.getUI()->getRoot() );
			UIWidget* parent = top;
			for ( int depth = 1; depth < Depth; ++depth ) {
				auto* child = UIWidget::New();
				child->setParent( parent );
				parent = child;
			}
			// Delete from the deepest widget up: each deletion is a top-level one.
			while ( parent != top ) {
				auto* next = parent->getParent()->asType<UIWidget>();
				eeDelete( parent );
				parent = next;
			}
			eeDelete( top );
		}
	} );
	report( "deep_hierarchy_lifecycle", lifecycle, Chains * Depth );
}

UTEST( AccessibilityDormant, NestedSceneLifecycle ) {
	UIApplication app( benchmarkWindow(), benchmarkSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	app.getUI()->update( Time::Zero );
	static constexpr int Scenes = 100;
	const auto lifecycle = measure( [&] {
		for ( int i = 0; i < Scenes; ++i ) {
			auto* nested = UISceneNode::New( app.getWindow() );
			nested->setParent( app.getUI()->getRoot() );
			for ( int child = 0; child < 20; ++child )
				UIWidget::New()->setParent( nested->getRoot() );
			eeDelete( nested );
		}
	} );
	report( "nested_scene_lifecycle", lifecycle, Scenes );
}

UTEST( AccessibilityDormant, RecycledListCells ) {
	UIApplication app( benchmarkWindow(), benchmarkSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* list = UIListView::New();
	list->setParent( app.getUI()->getRoot() );
	list->setPixelsSize( 400, 400 );
	auto model = std::make_shared<RowsModel>( 2000 );
	list->setModel( model );
	app.getUI()->update( Time::Zero );
	static constexpr int Updates = 2000;
	const auto scrolling = measure( [&] {
		for ( int i = 0; i < Updates; ++i ) {
			list->getVerticalScrollBar()->setValue( static_cast<Float>( ( i * 37 ) % 1000 ) /
													1000.f );
			app.getUI()->update( Time::Zero );
		}
	} );
	report( "list_scroll_updates", scrolling, Updates );
	const auto mutations = measure( [&] {
		for ( int i = 0; i < Updates; ++i ) {
			model->insertRow( i % 100 );
			model->removeRow( ( i * 7 ) % 100 );
		}
	} );
	report( "list_model_mutations", mutations, Updates );
}

UTEST( AccessibilityDormant, WidgetNotifications ) {
	UIApplication app( benchmarkWindow(), benchmarkSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	static constexpr int Widgets = 200;
	static constexpr int Rounds = 50;
	std::vector<UITextView*> labels;
	std::vector<UIProgressBar*> bars;
	for ( int i = 0; i < Widgets; ++i ) {
		auto* label = UITextView::New();
		label->setParent( app.getUI()->getRoot() );
		labels.push_back( label );
		auto* bar = UIProgressBar::New();
		bar->setParent( app.getUI()->getRoot() );
		bars.push_back( bar );
	}
	app.getUI()->update( Time::Zero );
	const String first( "First label" );
	const String second( "Second label" );
	const auto notifications = measure( [&] {
		for ( int round = 0; round < Rounds; ++round ) {
			for ( int i = 0; i < Widgets; ++i ) {
				labels[i]->setText( round % 2 ? first : second );
				bars[i]->setProgress( static_cast<Float>( round % 100 ) );
				labels[i]->setVisible( round % 2 == 0 );
				labels[i]->setEnabled( round % 2 == 0 );
			}
		}
	} );
	report( "widget_notifications", notifications, Widgets * Rounds );
}

#ifdef EE_BENCH_HAS_ACCESSIBILITY

namespace {

/** A backend whose client can attach and disconnect on demand. */
class SwitchableBackend : public AccessibilityBackend {
  public:
	bool active{ false };

	bool isAvailable() const override { return true; }

	bool hasActiveClients() const override { return active; }

	void onEvent( const AccessibilityPendingEvent& ) override {}
};

} // namespace

UTEST( AccessibilityDormant, BackendsWithoutExactTextChangesAllocateNoEditRecords ) {
	UIApplication app( benchmarkWindow(), benchmarkSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	auto backend = std::make_unique<SwitchableBackend>();
	backend->active = true;
	manager->setBackend( std::move( backend ) );
	scene->update( Time::Zero );
	ASSERT_TRUE( scene->hasActiveAccessibilityClients() );
	auto* input = UITextInput::New();
	input->setParent( scene->getRoot() );
	input->setText( String( std::string( 10000, 'x' ) ) );
	// Model the UIA/macOS path: an actual client is active, but the backend uses ValueChanged
	// instead of document edit records. Only record construction is measured, not the edit or
	// its ordinary UI events. A long insertion would force a heap copy if the preflight regressed.
	DocumentContentChange change{ { { 0, 0 }, { 0, 0 } }, String( std::string( 10000, 'y' ) ) };
	static constexpr int Edits = 2000;
	const auto records = measure( [&] {
		for ( int i = 0; i < Edits; ++i )
			manager->onTextChanged( input, &change );
	} );
	report( "unused_text_edit_records", records, Edits );
	EXPECT_EQ( records.allocations, 0u );
	EXPECT_EQ( records.bytes, 0u );
}

UTEST( AccessibilityDormant, InitializationWithoutThreadPool ) {
	// Without a scene thread pool the first update still schedules native initialization off
	// thread. Measure that UI-thread scheduling against a later update; readiness is separate.
	UIApplication app( benchmarkWindow(), benchmarkSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	ASSERT_FALSE( app.getUI()->hasThreadPool() );
	const auto first = measure( [&] { app.getUI()->update( Time::Zero ); } );
	report( "first_update_without_thread_pool", first, 1 );
	const auto later = measure( [&] { app.getUI()->update( Time::Zero ); } );
	report( "later_update_without_thread_pool", later, 1 );
	// Readiness (the backend finished connecting) is separate from UI-thread cost.
	auto* manager = app.getUI()->getAccessibilityManager();
	Clock readiness;
	while ( !manager->isBackendInitializationComplete() &&
			readiness.getElapsedTime() < Seconds( 5 ) )
		Sys::sleep( Microseconds( 100 ) );
	std::printf( "ACCESSIBILITY_DORMANT readiness_ms=%.3f backend_available=%d\n",
				 readiness.getElapsedTime().asMilliseconds(),
				 manager->isBackendAvailable() ? 1 : 0 );
}

UTEST( AccessibilityDormant, ModelUpdatesAfterClientDisconnects ) {
	UIApplication app( benchmarkWindow(), benchmarkSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* manager = app.getUI()->getAccessibilityManager();
	auto backend = std::make_unique<SwitchableBackend>();
	auto* switchable = backend.get();
	manager->setBackend( std::move( backend ) );
	auto* list = UIListView::New();
	list->setParent( app.getUI()->getRoot() );
	list->setPixelsSize( 400, 400 );
	auto model = std::make_shared<RowsModel>( 2000 );
	list->setModel( model );
	app.getUI()->update( Time::Zero );
	static constexpr int Updates = 2000;
	auto mutate = [&] {
		for ( int i = 0; i < Updates; ++i ) {
			model->insertRow( i % 100 );
			model->removeRow( ( i * 7 ) % 100 );
		}
	};
	const auto neverQueried = measure( mutate );
	report( "model_mutations_never_queried", neverQueried, Updates );

	// A client reads every row, then disconnects.
	switchable->active = true;
	app.getUI()->update( Time::Zero );
	const auto listRef = manager->getNodeRef( list );
	const auto rows = manager->getChildCount( listRef );
	for ( size_t row = 0; row < rows; ++row )
		manager->getNodeInfo( manager->getChild( listRef, row ) );
	switchable->active = false;
	app.getUI()->update( Time::Zero );
	const auto afterDisconnect = measure( mutate );
	report( "model_mutations_after_disconnect", afterDisconnect, Updates );
	// A disconnected client's rows must not keep the model updating persistent handles.
	EXPECT_LE( afterDisconnect.allocations, neverQueried.allocations );
}

UTEST( AccessibilityDormant, IdChangesWithAnActiveClient ) {
	UIApplication app( benchmarkWindow(), benchmarkSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	auto backend = std::make_unique<SwitchableBackend>();
	auto* switchable = backend.get();
	manager->setBackend( std::move( backend ) );
	// Equal lengths past any small-string buffer: renaming reuses the id's capacity, so only a
	// copy of the previous id would allocate.
	const std::string first( 64, 'a' );
	const std::string second( 64, 'b' );
	const std::string target( 64, 't' );
	const std::string moved( 64, 'm' );
	auto* label = UITextView::New();
	label->setParent( scene->getRoot() );
	label->setId( target );
	label->setText( "Label" );
	auto* input = UITextInput::New();
	input->setParent( scene->getRoot() );
	input->setAccessibilityLabelledBy( target );
	auto* renamed = UIWidget::New();
	renamed->setParent( scene->getRoot() );
	renamed->setId( first );
	scene->update( Time::Zero );
	static constexpr int Renames = 10000;
	auto rename = [&]( UIWidget* widget, const std::string& a, const std::string& b ) {
		for ( int i = 0; i < Renames; ++i )
			widget->setId( i % 2 ? a : b );
	};
	rename( renamed, first, second );
	const auto noClient = measure( [&] { rename( renamed, first, second ); } );
	report( "set_id_no_client", noClient, Renames );

	// A client reads the input, so the manager remembers the label's id as a relation target.
	switchable->active = true;
	scene->update( Time::Zero );
	manager->getNodeInfo( manager->getNodeRef( input ) );
	const auto unreferenced = measure( [&] { rename( renamed, first, second ); } );
	report( "set_id_active_unreferenced", unreferenced, Renames );
	// An id no client depends on is never copied.
	EXPECT_LE( unreferenced.allocations, noClient.allocations );

	// Renaming the target itself keeps the old id to notify the input; reported, not guarded.
	const auto referenced = measure( [&] {
		for ( int i = 0; i < Renames; ++i ) {
			label->setId( i % 2 ? target : moved );
			manager->clearPendingEvents();
		}
	} );
	report( "set_id_active_referenced", referenced, Renames );
}

#endif
