#include <eepp/ee.hpp>

class ResizeCircleCommand : public UndoCommand {
  public:
	ResizeCircleCommand( UIWidget* circle, Vector2f oldPos, Float oldSize ) :
		circle( circle ),
		pos( circle->getPixelsPosition() ),
		size( circle->getPixelsSize().x ),
		oldPos( oldPos ),
		oldSize( oldSize ) {}

	void undo() override {
		circle->setPixelsPosition( oldPos )
			->setPixelsSize( { oldSize, oldSize } )
			->setBorderRadius( oldSize / 2.f );
	}

	void redo() override {
		circle->setPixelsPosition( pos )
			->setPixelsSize( { size, size } )
			->setBorderRadius( size / 2.f );
	}

  protected:
	UIWidget* circle;
	Vector2f pos;
	Float size;
	Vector2f oldPos;
	Float oldSize;
};

class AddCircleCommand : public UndoCommand {
  public:
	AddCircleCommand( Node* parent, Vector2f pos, Float size, UndoStack& undoStack ) :
		pos( pos ), size( size ), parent( parent ), undoStack( undoStack ) {
		create();
	}

	virtual ~AddCircleCommand() {
		if ( SceneManager::existsSingleton() && !SceneManager::instance()->isShuttingDown() )
			circle->close();
	}

	void create() {
		circle = UIWidget::New();
		circle->addClass( "circle" )->setParent( parent );
		circle->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed )
			->setPixelsSize( { size, size } )
			->setBorderRadius( size / 2.f )
			->setPixelsPosition( pos );
		circle->onClick(
			[this]( auto ) {
				auto adjustMsgBox = UIMessageBox::New( UIMessageBox::OK, "Adjust diameter..." );
				adjustMsgBox->on( Event::OnConfirm, [this]( auto ) {
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
					Vector2f oldPos = circle->getPixelsPosition();
					Float oldSize = circle->getPixelsSize().x;
					slider->on( Event::OnValueChange, [this, slider]( auto ) {
						Float size = std::floor( slider->getValue() );
						Sizef newSize = { size, size };
						circle->setPixelsPosition(
							( circle->getPixelsPosition() + ( circle->getPixelsSize() / 2.f ) ) -
							newSize / 2.f );
						circle->setPixelsSize( newSize )->setBorderRadius( size / 2 );
					} );
					msgBox->on( Event::OnWindowClose, [this, oldPos, oldSize]( auto ) {
						if ( oldPos != circle->getPixelsPosition() ||
							 oldSize != circle->getPixelsSize().x )
							undoStack.push( new ResizeCircleCommand( circle, oldPos, oldSize ) );
					} );
					msgBox->showWhenReady();
				} );
				adjustMsgBox->showWhenReady();
			},
			MouseButton::EE_BUTTON_RIGHT );
	}

	void undo() override { circle->setVisible( false )->setEnabled( false ); }

	void redo() override { circle->setVisible( true )->setEnabled( true ); }

  protected:
	UIWidget* circle{ nullptr };
	Vector2f pos;
	Float size;
	Node* parent;
	UndoStack& undoStack;
};

// Reference https://eugenkiss.github.io/7guis/tasks#circle
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
			<PushButton id="undo" text="Undo" marginRight="8dp" enabled="false" />
			<PushButton id="redo" text="Redo" marginLeft="8dp" enabled="false" />
		</hbox>
		<Widget id="canvas" layout_width="match_parent" layout_height="0" layout_weight="1" />
	</vbox>
	)xml" );
	UndoStack undoStack;
	UIWidget* canvas = cont->find<UIWidget>( "canvas" );
	canvas->onClick( [&]( const MouseEvent* event ) {
		Float size = Math::randi( 20, 80 );
		Vector2f mousePos( event->getPosition().asFloat() - size / 2 );
		canvas->worldToNodeTranslation( mousePos );
		undoStack.push( new AddCircleCommand( canvas, mousePos, size, undoStack ) );
	} );
	auto undoBut = cont->find( "undo" );
	auto redoBut = cont->find( "redo" );
	undoBut->onClick( [&undoStack]( auto ) { undoStack.undo(); } );
	redoBut->onClick( [&undoStack]( auto ) { undoStack.redo(); } );
	undoStack.on( UndoStack::CanUndoChanged, [undoBut]( const UndoStack::Event* event ) {
		undoBut->setEnabled( event->asCanUndoChanged()->canUndo() );
	} );
	undoStack.on( UndoStack::CanRedoChanged, [redoBut]( const UndoStack::Event* event ) {
		redoBut->setEnabled( event->asCanRedoChanged()->canRedo() );
	} );
	return app.run();
}
