#include "notificationcenter.hpp"

namespace ecode {

NotificationCenter::NotificationCenter( UILayout* layout, PluginManager* pluginManager ) :
	mLayout( layout ), mPluginManager( pluginManager ) {
	mPluginManager->subscribeMessages(
		"notificationcenter", [this]( const PluginMessage& msg ) -> PluginRequestHandle {
			if ( !msg.isBroadcast() )
				return {};
			if ( msg.type == PluginMessageType::ShowMessage ) {
				auto sm = msg.asShowMessage();
				if ( !sm.message.empty() )
					addNotification( sm.message, Seconds( 10 ) );
			} else if ( msg.type == PluginMessageType::ShowDocument ) {
				auto sd = msg.asShowDocument();
				if ( !sd.uri.empty() ) {
					addShowRequest( sd.uri.toString(),
									mLayout->getUISceneNode()->i18n( "open", "Open" ),
									Seconds( 10 ) );
				}
			}
			return {};
		} );
}

void NotificationCenter::addNotification( const String& text, const Time& delay ) {
	auto action = [this, text, delay]() {
		UITextView* tv = UITextView::New();
		tv->addEventListener( Event::MouseClick, [tv]( const Event* event ) {
			const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK )
				tv->close();
		} );
		tv->setParent( mLayout );
		tv->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
		tv->setFlags( UI_WORD_WRAP );
		tv->setText( text );
		tv->addClass( "notification" );
		Action* sequence = Actions::Sequence::New(
			{ Actions::FadeIn::New( Seconds( 0.125 ) ), Actions::Delay::New( delay ),
			  Actions::FadeOut::New( Seconds( 0.125 ) ), Actions::Close::New() } );
		tv->runAction( sequence );
	};

	if ( Engine::isRunninMainThread() )
		action();
	else
		mLayout->runOnMainThread( action );
}

void NotificationCenter::addShowRequest( const String& uri, const String& actionText,
										 const Time& delay ) {
	auto action = [this, uri, actionText, delay]() {
		static const auto layout = R"xml(
	<vbox lw="mp" class="notification">
		<TextView lw="mp" wordwrap="true" />
		<hbox lg="right">
			<PushButton />
		</hbox>
	</vbox>
	)xml";
		UILinearLayout* lay = mLayout->getUISceneNode()
								  ->loadLayoutFromString( layout, mLayout )
								  ->asType<UILinearLayout>();
		UITextView* tv = lay->findByType( UI_TYPE_TEXTVIEW )->asType<UITextView>();
		tv->setText( mLayout->getUISceneNode()->i18n(
			"open_url_question", String::format( "Open URL\n%s?", uri.toUtf8().c_str() ) ) );
		UIPushButton* pb = lay->findByType( UI_TYPE_PUSHBUTTON )->asType<UIPushButton>();
		pb->setText( actionText );
		pb->on( Event::MouseClick, [uri]( const Event* event ) {
			if ( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK )
				Engine::instance()->openURI( uri );
		} );
		Action* sequence = Actions::Sequence::New(
			{ Actions::FadeIn::New( Seconds( 0.125 ) ), Actions::Delay::New( delay ),
			  Actions::FadeOut::New( Seconds( 0.125 ) ), Actions::Close::New() } );
		lay->runAction( sequence );
	};

	if ( Engine::isRunninMainThread() )
		action();
	else
		mLayout->runOnMainThread( action );
}

} // namespace ecode
