#include <eepp/ee.hpp>

// Reference https://eugenkiss.github.io/7guis/tasks#circle
// This is a work in progress, undo redo stack is pending
EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 800, 600, "eepp - 7GUIs - Circle Drawer" } );
	UIWidget* cont = app.getUI()->loadLayoutFromString( R"xml(
	<style>
		#canvas {
			background-color: var(--list-back);
			margin: 0dp 8dp 8dp 8dp;
			clip: true;
		}
		.circle {
			background-color: transparent;
			border: 1dp solid var(--primary);
			transition: all 0.1s;
		}
		.circle:hover {
			background-color: var(--primary);
		}
	</style>
	<vbox layout_width="match_parent" layout_height="match_parent">
		<hbox padding="4dp" layout_gravity="center">
			<PushButton id="undo" text="Undo" marginRight="8dp" />
			<PushButton id="redo" text="Redo" marginLeft="8dp" />
		</hbox>
		<Widget id="canvas" layout_width="match_parent" layout_height="0" layout_weight="1" />
	</vbox>
	)xml" );
	UIWidget* canvas = cont->find<UIWidget>( "canvas" );
	canvas->onClick( [&]( const MouseEvent* event ) {
		Float size = Math::randi( 20, 80 );
		Vector2f mousePos( event->getPosition().asFloat() - size / 2 );
		canvas->worldToNodeTranslation( mousePos );
		UIWidget* circle = UIWidget::New();
		circle->addClass( "circle" )->setParent( canvas );
		circle->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed )
			->setPixelsSize( { size, size } )
			->setBorderRadius( size / 2 )
			->setPixelsPosition( mousePos );
		// undoRedoStack->insert( circle );
		circle->onClick(
			[circle]( auto ) {
				auto adjustMsgBox = UIMessageBox::New( UIMessageBox::OK, "Adjust diameter..." );
				adjustMsgBox->on( Event::OnConfirm, [circle]( auto ) {
					auto pos( circle->getPixelsPosition().asInt() );
					auto msgBox = UIMessageBox::New(
						UIMessageBox::OK,
						String::format( "Adjust diameter of circle at (%d, %d)", pos.x, pos.y ) );
					msgBox->getButtonOK()->setVisible( false )->setEnabled( false );
					auto slider = UISlider::NewHorizontal();
					slider->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent )
						->setLayoutMargin( Rectf( 8, 8, 8, 8 ) )
						->setParent( msgBox->getLayoutCont()->getFirstChild() )
						->setFocus()
						->toPosition( 1 );
					slider->setMinValue( 20 );
					slider->setMaxValue( 80 );
					slider->setValue( circle->getPixelsSize().x );
					slider->on( Event::OnValueChange, [slider, circle]( auto ) {
						Float size = std::floor( slider->getValue() );
						Sizef newSize = { size, size };
						circle->setPixelsPosition(
							( circle->getPixelsPosition() + ( circle->getPixelsSize() / 2.f ) ) -
							newSize / 2.f );
						circle->setPixelsSize( newSize )->setBorderRadius( size / 2 );
					} );
					msgBox->on( Event::OnWindowClose, [/*circle*/]( auto ) {
						// undoRedoStack->changeSize( circle );
					} );
					msgBox->showWhenReady();
				} );
				adjustMsgBox->showWhenReady();
			},
			MouseButton::EE_BUTTON_RIGHT );
	} );
	cont->find( "undo" )->onClick( []( auto ) {
		// undoRedoStack->undo();
	} );
	cont->find( "redo" )->onClick( []( auto ) {
		// undoRedoStack->redo();
	} );
	return app.run();
}
