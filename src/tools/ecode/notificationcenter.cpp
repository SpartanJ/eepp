#include "notificationcenter.hpp"
#include <eepp/scene/action.hpp>
#include <eepp/scene/actions/actions.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/window/engine.hpp>

using namespace EE::Scene;

namespace ecode {

NotificationCenter* sInstance = nullptr;

NotificationCenter* NotificationCenter::instance() {
	return sInstance;
}

NotificationCenter::NotificationCenter( UILayout* layout, PluginManager* pluginManager ) :
	mLayout( layout ), mPluginManager( pluginManager ) {
	sInstance = this;
	mPluginManager->subscribeMessages(
		"notificationcenter", [this]( const PluginMessage& msg ) -> PluginRequestHandle {
			if ( !msg.isBroadcast() )
				return {};
			if ( msg.type == PluginMessageType::ShowMessage ) {
				auto sm = msg.asShowMessage();
				if ( !sm.message.empty() )
					addNotification( sm.message, Seconds( 10 ), sm.allowCopy );
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

void NotificationCenter::addNotification( const String& text, const Time& delay, bool allowCopy ) {
	auto action = [this, text, delay, allowCopy]() {
		UITextView* tv = UITextView::New();
		tv->on( Event::MouseClick, [tv, allowCopy]( const Event* event ) {
			const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
			if ( mouseEvent->getFlags() &
				 ( allowCopy ? EE_BUTTON_MMASK : ( EE_BUTTON_LMASK | EE_BUTTON_RMASK ) ) )
				tv->close();
		} );
		tv->setParent( mLayout );
		tv->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
		tv->setFlags( UI_WORD_WRAP );
		tv->setText( text );
		tv->addClass( "notification" );
		tv->setTextSelection( allowCopy );
		mLayout->toFront();
		Action* sequence = Actions::Sequence::New(
			{ Actions::FadeIn::New( Seconds( 0.125 ) ), Actions::Delay::New( delay ),
			  Actions::FadeOut::New( Seconds( 0.125 ) ), Actions::Close::New() } );
		tv->runAction( sequence );
		Log::info( "Displayed notification:\n%s", text.toUtf8() );
	};

	mLayout->ensureMainThread( action );
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
		mLayout->toFront();
		lay->runAction( sequence );
	};

	mLayout->ensureMainThread( action );
}

void NotificationCenter::addInteractiveNotification( String text, String actionText,
													 std::function<void()> onInteraction,
													 const Time& delay, bool allowCopy ) {
	auto action = [this, text = std::move( text ), actionText = std::move( actionText ), delay,
				   allowCopy, onInteraction = std::move( onInteraction )]() {
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
		tv->setText( text );
		tv->setTextSelection( allowCopy );
		tv->on( Event::MouseClick, [allowCopy, lay]( const Event* event ) {
			const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
			if ( mouseEvent->getFlags() &
				 ( allowCopy ? EE_BUTTON_MMASK : ( EE_BUTTON_LMASK | EE_BUTTON_RMASK ) ) )
				lay->close();
		} );
		UIPushButton* pb = lay->findByType( UI_TYPE_PUSHBUTTON )->asType<UIPushButton>();
		pb->setText( actionText );
		pb->onClick( [actionText, onInteraction = std::move( onInteraction )]( const MouseEvent* ) {
			if ( onInteraction )
				onInteraction();
		} );
		Action* sequence = Actions::Sequence::New(
			{ Actions::FadeIn::New( Seconds( 0.125 ) ), Actions::Delay::New( delay ),
			  Actions::FadeOut::New( Seconds( 0.125 ) ), Actions::Close::New() } );
		mLayout->toFront();
		lay->runAction( sequence );
	};

	mLayout->ensureMainThread( action );
}

} // namespace ecode
