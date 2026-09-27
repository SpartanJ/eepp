#include <eepp/ee.hpp>
#include <eepp/ui/uitextselectioncontroller.hpp>

class SelectionDocument : public UILinearLayout {
  public:
	SelectionDocument() : UILinearLayout( "selectiondocument", UIOrientation::Vertical ) {
		mSelection.setHost( this );
		mSelection.setSelectionRoot( this );
		setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
		subscribeScheduledUpdate();
	}

	virtual ~SelectionDocument() { mSelection.onDocumentWillChange(); }

	virtual UITextSelectionController* getTextSelectionController() { return &mSelection; }

	virtual const UITextSelectionController* getTextSelectionController() const {
		return &mSelection;
	}

  protected:
	virtual Uint32 onKeyDown( const KeyEvent& event ) {
		if ( mSelection.onKeyDown( event ) )
			return 1;
		return UILinearLayout::onKeyDown( event );
	}

	virtual void scheduledUpdate( const Time& time ) {
		UILinearLayout::scheduledUpdate( time );
		mSelection.updateSelectionDrag();
	}

  private:
	UITextSelectionController mSelection;
};

EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 800, 600, "eepp - UIRichText Example" } );

	app.getUI()->loadLayoutFromString( R"xml(
	<ScrollView id="document_scroll" layout_width="match_parent" layout_height="match_parent" />
	)xml" );
	auto* document = eeNew( SelectionDocument, () );
	document->setParent( app.getUI()->find<UIScrollView>( "document_scroll" ) );
	app.getUI()->loadLayoutFromString( R"xml(
		<vbox id="main_container" layout_width="match_parent" layout_height="wrap_content" padding="8dp">
			<RichText font-size="12dp"
				color="white">Welcome to the <span color="#FFD700" font-style="bold">UIRichText</span> example!
				This component supports <span color="#00FF00" font-style="italic">styled text</span>,
				<span color="#00BFFF" font-style="shadow">shadows</span>,
				and <span color="#FF4500" text-stroke-width="1dp" text-stroke-color="black">outlines</span> using <span font-family="monospace" color="#A9A9A9">HTML-like tags</span>.
			</RichText>
			<Image src="file://assets/icon/ee.png" margin="4dp" layout-gravity="center_horizontal" />
			<RichText font-size="12dp"
			color="#fefefe">We can also mix <span color="#FFD700" font-style="bold">contents</span> with more <span color="#00FF00" font-style="italic">text</span>!
			</RichText>
		</vbox>
	)xml",
									   document );

	auto mainContainer = app.getUI()->find<UILinearLayout>( "main_container" );

	app.getWindow()->getInput()->pushCallback(
		[mainContainer, document, &app]( InputEvent* event ) {
			switch ( event->Type ) {
				case InputEvent::FileDropped: {
					std::string file( event->file.file );
					std::string data;
					FileSystem::fileGet( file, data );
					document->getTextSelectionController()->onDocumentWillChange();
					mainContainer->closeAllChildren();
					std::string uri( "file://" + FileSystem::fileRemoveFileName( file ) );
					FileSystem::dirAddSlashAtEnd( uri );
					app.getUI()->setURI( URI( uri ) );
					app.getUI()->loadLayoutFromString( data, mainContainer );
					document->getTextSelectionController()->onDocumentChanged();
					break;
				}
				case InputEvent::TextDropped: {
					document->getTextSelectionController()->onDocumentWillChange();
					mainContainer->closeAllChildren();
					app.getUI()->loadLayoutFromString( event->textdrop.text, mainContainer );
					document->getTextSelectionController()->onDocumentChanged();
					break;
				}
				default:
					break;
			}
		} );

	app.getUI()->on( Event::KeyUp, [&app]( const Event* event ) {
		if ( event->asKeyEvent()->getKeyCode() == KEY_F11 ) {
			UIWidgetInspector::create( app.getUI() );
		}
	} );

	return app.run();
}
