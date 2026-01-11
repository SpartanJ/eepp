#include <eepp/ee.hpp>

class UISineWaveWidget : public UIWidget {
  public:
	static UISineWaveWidget* New() { return eeNew( UISineWaveWidget, () ); }

	// Implement the widget draw method
	void draw() override {
		Sizef size = getPixelsSize().asFloat();
		Float centerY = size.getHeight() * 0.5f;
		Float amplitude = size.getHeight() / 2 * 0.5f;
		BatchRenderer br;
		br.lineStripSetColor( mLineColor );
		for ( Float x = 0; x < size.getWidth(); x++ )
			br.batchLineStrip( mPosition.x + x, amplitude * std::sin( mFrequency * x ) + centerY );
		br.draw();
	}

	void setFrequency( Float freq ) {
		if ( freq != mFrequency ) {
			mFrequency = freq;
			onValueChange();  // Trigger the on value change event
			invalidateDraw(); // Tell the scene that we need to redraw (UI only redraws when needed)
		}
	}

	Float frequency() { return mFrequency; }

	void setColor( Color color ) {
		if ( color != mLineColor ) {
			mLineColor = color;
			invalidateDraw();
		}
	}

  protected:
	Float mFrequency{ 0.f };
	Color mLineColor{ Color::Red };
	Float mDir{ 1 };

	UISineWaveWidget() :
		UIWidget(
			"sinewavewidget" ) // parameter is the element tag (for accessing the element from CSS)
	{}
};

EE_MAIN_FUNC int main( int, char** ) {
	// UIApplication allows to quickly initialize the application as an UI application
	UIApplication app( { 1280, 768, "eepp - Custom Widget Example" } );

	UISceneNode* ui = app.getUI(); // Get the UI Scene

	// Register the new widget type into the system, you must provide a name and the creation
	// callback. The name will be used as the XML Element name.
	// This must be done before loading any XML that refers to this XML element name.
	UIWidgetCreator::registerWidget( "SineWaveWidget", UISineWaveWidget::New );

	// Load the XML document with its styles
	ui->loadLayoutFromString( R"xml(
	<style>
		:root > * { background-color: white; }
		#frequency_display { text-color: black; font-size: 24dp; margin: 8dp 0 0 8dp; }
	</style>
	<RelativeLayout layout_width="match_parent" layout_height="match_parent">
		<SineWaveWidget id="wave" layout_width="match_parent" layout_height="match_parent" />
		<SineWaveWidget id="wave2" layout_width="match_parent" layout_height="match_parent" />
		<TextView id="frequency_display" layout_width="wrap_content" layout_height="wrap_content" />
	</RelativeLayout>
	)xml" );

	// Get the elements created, "find" will search by id, asType is just a cast to its type
	UISineWaveWidget* sineWaveWidget = ui->find( "wave" )->asType<UISineWaveWidget>();
	UISineWaveWidget* sineWaveWidget2 = ui->find( "wave2" )->asType<UISineWaveWidget>();
	UITextView* textView = ui->find( "frequency_display" )->asType<UITextView>();

	// Listen for the KeyDown event from the scene element (events are propagated upwards until
	// an element process the event (in this example it will propagate until the root element)
	ui->on( Event::KeyDown, [sineWaveWidget, sineWaveWidget2, ui]( const Event* event ) {
		Float diff = ui->getElapsed().asSeconds() * 0.1f;
		if ( event->asKeyEvent()->getKeyCode() == KEY_UP ) {
			sineWaveWidget->setFrequency( sineWaveWidget->frequency() + diff );
			sineWaveWidget2->setFrequency( sineWaveWidget2->frequency() - diff );
		} else if ( event->asKeyEvent()->getKeyCode() == KEY_DOWN ) {
			sineWaveWidget->setFrequency( sineWaveWidget->frequency() - diff );
			sineWaveWidget2->setFrequency( sineWaveWidget2->frequency() + diff );
		}
	} );

	// Register the on value change event to update the frequency label
	sineWaveWidget->on( Event::OnValueChange, [sineWaveWidget, textView]( const Event* ) {
		textView->setText( String::format( "frequency is %.2f", sineWaveWidget->frequency() ) );
	} );

	// Change the second sine wave color
	sineWaveWidget2->setColor( Color::Blue );

	// Set an initial frequency (this will trigger the first on value change)
	sineWaveWidget->setFrequency( 0.025f );
	sineWaveWidget2->setFrequency( -0.025f ); // Generate an inverted sine wave

	return app.run();
}
