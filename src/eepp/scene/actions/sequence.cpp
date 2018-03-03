#include <eepp/scene/actions/sequence.hpp>

namespace EE { namespace Scene { namespace Actions {

Sequence * Sequence::New( const std::vector<Action *> sequence ) {
	return eeNew( Sequence, ( sequence ) );
}

Sequence * Sequence::New( Action * action, Action * action2 ) {
	return New( { action, action2 } );
}

Sequence * Sequence::New( Action * action, Action * action2, Action * action3 ) {
	return New( { action, action2, action3 } );
}

Sequence * Sequence::New( Action * action, Action * action2, Action * action3, Action * action4 ) {
	return New( { action, action2, action3, action4 } );
}

Sequence * Sequence::New( Action * action, Action * action2, Action * action3, Action * action4, Action * action5 ) {
	return New( { action, action2, action3, action4, action5 } );
}

Sequence * Sequence::New( Action * action, Action * action2, Action * action3, Action * action4, Action * action5, Action * action6 ) {
	return New( { action, action2, action3, action4, action5, action6 } );
}

Sequence * Sequence::New( Action * action, Action * action2, Action * action3, Action * action4, Action * action5, Action * action6, Action * action7 ) {
	return New( { action, action2, action3, action4, action5, action6, action7 } );
}

Sequence * Sequence::New( Action * action, Action * action2, Action * action3, Action * action4, Action * action5, Action * action6, Action * action7, Action * action8 ) {
	return New( { action, action2, action3, action4, action5, action6, action7, action8 } );
}

Sequence * Sequence::New( Action * action, Action * action2, Action * action3, Action * action4, Action * action5, Action * action6, Action * action7, Action * action8, Action * action9 ) {
	return New( { action, action2, action3, action4, action5, action6, action7, action8, action9 } );
}

void Sequence::start() {
	for ( size_t i = 0; i < mSequence.size(); i++ ) {
		mSequence[ i ]->setTarget( getTarget() );
	}

	mSequence[ mCurPos ]->start();

	sendEvent( ActionType::OnStart );
}

void Sequence::stop() {
	mSequence[ mCurPos ]->stop();
	sendEvent( ActionType::OnStop );
}

void Sequence::update( const Time & time ) {
	if ( isDone() )
		return;

	mSequence[ mCurPos ]->update( time );

	if ( mSequence[ mCurPos ]->isDone() && mCurPos + 1 < mSequence.size() ) {
		mCurPos++;
		mSequence[ mCurPos ]->start();
		sendEvent( ActionType::OnStep );
	}
}

bool Sequence::isDone() {
	return mCurPos == mSequence.size() - 1 && mSequence[ mCurPos ]->isDone();
}

Action * Sequence::clone() const {
	return Sequence::New( mSequence );
}

Action * Sequence::reverse() const {
	std::vector<Action*> reversed;

	for ( auto it = mSequence.rbegin(); it != mSequence.rend(); ++it ) {
		reversed.push_back( *it );
	}

	return Sequence::New( reversed );
}

Sequence::~Sequence() {
	for ( size_t i = 0; i < mSequence.size(); i++ ) {
		Action * action = mSequence[ i ];
		eeSAFE_DELETE( action );
	}
}

Sequence::Sequence( const std::vector<Action*> sequence ) :
	mSequence( sequence ),
	mCurPos( 0 )
{}

}}} 
