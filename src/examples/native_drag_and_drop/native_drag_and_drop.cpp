#include "native_drag_and_drop.hpp"
#include <eepp/ee.hpp>
#include <iostream>

namespace {
struct DragUi {
	UIWidget* card;
	UITextView* status;
};

struct DropUi {
	UITextView* count;
	UITextView* files;
	size_t received{ 0 };
};

struct DragInput {
	EE::Window::Window* window;
	DragOffer* offer;
	DragUi* ui;
	DragAndDrop drag;
	Vector2i press;
	bool armed{ false };
};

// Read only the widget's rectangle, once per gesture. No full-window image or per-frame readback.
Image snapshotWidget( EE::Window::Window* window, UIWidget* widget ) {
	const auto bounds = widget->getWorldBounds();
	const int left = std::clamp( static_cast<int>( std::floor( bounds.Left ) ), 0,
								 static_cast<int>( window->getWidth() ) );
	const int top = std::clamp( static_cast<int>( std::floor( bounds.Top ) ), 0,
								static_cast<int>( window->getHeight() ) );
	const int right = std::clamp( static_cast<int>( std::ceil( bounds.Right ) ), left,
								  static_cast<int>( window->getWidth() ) );
	const int bottom = std::clamp( static_cast<int>( std::ceil( bounds.Bottom ) ), top,
								   static_cast<int>( window->getHeight() ) );
	const int width = right - left, height = bottom - top;
	if ( !width || !height || width > 2048 || height > 2048 || width * height > 1048576 )
		return {};
	auto context = Engine::instance()->makeWindowCurrent( window );
	window->clear();
	SceneManager::instance()->draw( window );
	GlobalBatchRenderer::instance()->draw();
	Image image( width, height, 4, Color::Transparent, false );
	FrameReadback readback;
	readback.pixels = image.getPixels();
	readback.size = { width, height };
	readback.stride = static_cast<size_t>( width ) * 4;
	readback.position = { left, static_cast<int>( window->getHeight() ) - bottom };
	readback.format = FramePixelFormat::RGBA32;
	readback.origin = FrameOrigin::TopLeft;
	if ( !window->readFrameBuffer( readback ) )
		return {};
	if ( readback.origin == FrameOrigin::BottomLeft ) {
		for ( int y = 0; y < height / 2; ++y ) {
			auto* row = image.getPixels() + y * readback.stride;
			auto* opposite = image.getPixels() + ( height - 1 - y ) * readback.stride;
			std::swap_ranges( row, row + readback.stride, opposite );
		}
	}
	return image;
}
} // namespace

int EE::Examples::runNativeDragDemo( std::vector<std::string> arguments, bool withDropTarget ) {
	if ( arguments.empty() ) {
		std::cerr << "Usage: eepp-native-drag-and-drop <absolute-file-path> [more paths...]\n";
		return EXIT_FAILURE;
	}
	DragOffer offer;
	offer.files.reserve( arguments.size() );
	for ( auto& argument : arguments )
		offer.files.push_back( { std::move( argument ) } );
	// Validation's serialized buffer and the moved-from argument array are startup scratch,
	// not persistent copies of the session payload.
	std::vector<std::string>{}.swap( arguments );
	{
		std::string encoded, error;
		if ( !DragAndDrop::encodeFileUriList( offer, encoded, &error ) ) {
			std::cerr << error << '\n';
			return EXIT_FAILURE;
		}
	}
	// Declare callback endpoints before the application. UIApplication destroys its scenes,
	// inputs and Engine (including pending drag completion) before these stack objects expire.
	// Close listeners clear widget pointers; callbacks therefore need no shared ownership.
	DragUi sourceUi{};
	DropUi dropUi{};
	UIApplication app(
		{ 640, 260,
		  withDropTarget ? "eepp - Native DND manual source" : "eepp - Native file drag" },
		{}, ContextSettings( true ) );
	if ( !app.getWindow() || !app.getWindow()->isOpen() )
		return EXIT_FAILURE;
	auto* window = app.getWindow();
	auto drag = DragAndDrop::forWindow( *window );
	auto* ui = app.getUI();
	ui->loadLayoutFromString( R"xml(
		<LinearLayout layout_width="match_parent" layout_height="match_parent" orientation="vertical"
			padding="16dp">
			<TextView id="instructions" layout_width="match_parent" layout_height="wrap_content" margin-bottom="12dp"
				text="Drag the file card into a browser upload input or file manager. Escape cancels." />
			<TextView id="files" layout_width="match_parent" layout_height="100dp" cursor="hand"
				padding="12dp" background-color="#26364c" border="1dp solid #6b9bda" border-radius="6dp" />
			<TextView id="status" layout_width="match_parent" layout_height="wrap_content"
				margin-top="12dp" />
		</LinearLayout>
	)xml" );
	if ( withDropTarget ) {
		ui->find<UITextView>( "instructions" )
			->setText( "Drag the card into the receiving window, a browser upload input or a file "
					   "manager. "
					   "Escape cancels." );
		auto* destination = app.createWindow( { 520, 340, "eepp - Native DND manual target" },
											  ContextSettings( true ) );
		if ( !destination ) {
			std::cerr << "Could not create the receiving window\n";
			return EXIT_FAILURE;
		}
		window->setPosition( 40, 60 );
		destination->getWindow()->setPosition( 720, 60 );
		destination->loadLayoutFromString( R"xml(
			<LinearLayout layout_width="match_parent" layout_height="match_parent"
				orientation="vertical" padding="16dp">
				<TextView layout_width="match_parent" layout_height="wrap_content" word-wrap="true"
					text="Drop the source card here, or drop files from your file manager." />
				<TextView id="received_count" layout_width="match_parent" layout_height="wrap_content" word-wrap="true"
					margin-top="12dp" text="Received files: 0" />
				<TextView id="received_files" layout_width="match_parent" layout_height="0dp" layout_weight="1"
					margin-top="12dp" padding="12dp" background-color="#26364c"
					border="1dp solid #6b9bda" text="Waiting for a drop. You control the mouse."
					text-overflow="ellipsis" />
			</LinearLayout>
		)xml" );
		dropUi = { destination->find<UITextView>( "received_count" ),
				   destination->find<UITextView>( "received_files" ) };
		auto invalidate = [&dropUi]( const Event* ) { dropUi.count = dropUi.files = nullptr; };
		dropUi.count->addEventListener( Event::OnClose, invalidate );
		dropUi.files->addEventListener( Event::OnClose, invalidate );
		destination->getWindow()->getInput()->pushCallback( [&dropUi]( InputEvent* event ) {
			if ( event->Type != InputEvent::FileDropped || !dropUi.count || !dropUi.files )
				return;
			// SDL releases the path after dispatch. The text view owns the bounded display copy;
			// retain at most eight paths so repeated manual drops cannot grow the UI indefinitely.
			String files = dropUi.received % 8 == 0 ? String{} : dropUi.files->getText();
			if ( !files.empty() )
				files += '\n';
			files += String::fromUtf8( std::string_view( event->file.file ) ).substr( 0, 2048 );
			++dropUi.received;
			dropUi.files->setText( std::move( files ) );
			dropUi.count->setText(
				"Received files: " + std::to_string( dropUi.received ) +
				( dropUi.received > 8 ? " (showing the latest group of up to 8)" : "" ) );
		} );
	}
	std::string summary =
		std::to_string( offer.files.size() ) +
		( offer.files.size() == 1 ? " file — drag to copy\n" : " files — drag to copy\n" );
	for ( size_t i = 0; i < std::min<size_t>( offer.files.size(), 2 ); ++i ) {
		if ( i )
			summary += '\n';
		summary += FileSystem::fileNameFromPath( offer.files[i].path );
	}
	if ( offer.files.size() > 2 )
		summary += " (+" + std::to_string( offer.files.size() - 2 ) + " more)";
	auto* card = ui->find<UITextView>( "files" );
	card->setText( std::move( summary ) );
	auto* status = ui->find<UITextView>( "status" );
	status->setText( drag.getCapabilities().fileSource
						 ? "Ready. Hold and drag the file card."
						 : "Native file dragging is unavailable on this display backend." );
	sourceUi = { card, status };
	auto invalidate = [&sourceUi]( const Event* ) { sourceUi = { nullptr, nullptr }; };
	card->addEventListener( Event::OnClose, invalidate );
	status->addEventListener( Event::OnClose, invalidate );
	DragInput input{ window, &offer, &sourceUi, drag, {} };
	// One pointer fits std::function's inline storage. Remove this callback before the stack
	// controller expires; completion/close callbacks instead borrow the longer-lived endpoints.
	const auto callbackId = window->getInput()->pushCallback( [&input]( InputEvent* event ) {
		auto* card = input.ui->card;
		auto* status = input.ui->status;
		if ( !card || !status )
			return;
		auto* window = input.window;
		auto& offer = *input.offer;
		auto& drag = input.drag;
		auto& press = input.press;
		auto& armed = input.armed;
		if ( event->Type == InputEvent::MouseButtonDown &&
			 event->button.button == EE_BUTTON_LEFT ) {
			press = { event->button.x, event->button.y };
			armed = card->getWorldBounds().contains( Vector2f( press.x, press.y ) );
		} else if ( event->Type == InputEvent::MouseButtonUp ) {
			armed = false;
		} else if ( event->Type == InputEvent::MouseMotion && armed && !drag.isDragging() ) {
			const Vector2i delta = Vector2i( event->motion.x, event->motion.y ) - press;
			if ( delta.x * delta.x + delta.y * delta.y >= 64 ) {
				armed = false;
				auto snapshot = snapshotWidget( window, card );
				offer.preview.image = snapshot.getPixels() ? &snapshot : nullptr;
				// Offset the preview just beyond the pointer so it does not obscure the target.
				offer.preview.hotspot = { -12, -16 };
				auto result = drag.startDrag( offer, [ui = input.ui]( DragResult result ) {
					auto& sourceUi = *ui;
					std::cout << "Drag completion: " << static_cast<int>( result.reason ) << '\n';
					if ( sourceUi.card && sourceUi.status ) {
						sourceUi.card->setAlpha( 255 );
						switch ( result.reason ) {
							case DragEndReason::Dropped:
								sourceUi.status->setText(
									"Dropped successfully. Drag again to copy." );
								break;
							case DragEndReason::Cancelled:
								sourceUi.status->setText( "Drag cancelled. Drag again to retry." );
								break;
							default:
								sourceUi.status->setText( result.error.empty() ? "Drag failed."
																			   : result.error );
								break;
						}
					}
				} );
				offer.preview.image = nullptr; // The session has already copied the snapshot.
				if ( result.status == DragStartStatus::Started ) {
					card->setAlpha( 150 );
					status->setText(
						"Dragging — release over an accepting target. Escape cancels." );
					// Present before Windows enters its native modal drag loop. This is one
					// ordinary UI frame, not continuous widget rendering for the preview.
					window->clear();
					SceneManager::instance()->draw( window );
					window->display();
				} else {
					status->setText( result.error );
				}
			}
		}
	} );
	const int result = app.run();
	if ( Engine::instance()->existsWindow( window ) ) {
		drag.cancelDrag();
		window->getInput()->popCallback( callbackId );
	}
	Engine::instance()->updateInput();
	return result;
}
