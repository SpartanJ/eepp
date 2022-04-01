#include "notificationcenter.hpp"

NotificationCenter::NotificationCenter( UILayout* layout ) : mLayout( layout ) {}

UITextView* NotificationCenter::addNotification( const String& text ) {
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
		{ Actions::FadeIn::New( Seconds( 0.125 ) ), Actions::Delay::New( Seconds( 2.5 ) ),
		  Actions::FadeOut::New( Seconds( 0.125 ) ), Actions::Close::New() } );
	tv->runAction( sequence );
	return tv;
}
