#include <eepp/scene/actions/sequence.hpp>

namespace EE { namespace Scene { namespace Actions {

Sequence* Sequence::New( const std::vector<Action*> sequence ) {
	return eeNew( Sequence, ( sequence ) );
}

Sequence* Sequence::New( Action* action, Action* action2 ) {
	return eeNew( Sequence, ( ActionContainer{ action, action2 } ) );
}

Sequence* Sequence::New( Action* action, Action* action2, Action* action3 ) {
	return eeNew( Sequence, ( ActionContainer{ action, action2, action3 } ) );
}

Sequence* Sequence::New( Action* action, Action* action2, Action* action3, Action* action4 ) {
	return eeNew( Sequence, ( ActionContainer{ action, action2, action3, action4 } ) );
}

Sequence* Sequence::New( Action* action, Action* action2, Action* action3, Action* action4,
						 Action* action5 ) {
	return eeNew( Sequence, ( ActionContainer{ action, action2, action3, action4, action5 } ) );
}

Sequence* Sequence::New( Action* action, Action* action2, Action* action3, Action* action4,
						 Action* action5, Action* action6 ) {
	return eeNew( Sequence,
				  ( ActionContainer{ action, action2, action3, action4, action5, action6 } ) );
}

Sequence* Sequence::New( Action* action, Action* action2, Action* action3, Action* action4,
						 Action* action5, Action* action6, Action* action7 ) {
	return eeNew( Sequence, ( ActionContainer{ action, action2, action3, action4, action5, action6,
											   action7 } ) );
}

Sequence* Sequence::New( Action* action, Action* action2, Action* action3, Action* action4,
						 Action* action5, Action* action6, Action* action7, Action* action8 ) {
	return eeNew( Sequence, ( ActionContainer{ action, action2, action3, action4, action5, action6,
											   action7, action8 } ) );
}

Sequence* Sequence::New( Action* action, Action* action2, Action* action3, Action* action4,
						 Action* action5, Action* action6, Action* action7, Action* action8,
						 Action* action9 ) {
	return eeNew( Sequence, ( ActionContainer{ action, action2, action3, action4, action5, action6,
											   action7, action8, action9 } ) );
}

void Sequence::start() {
	for ( size_t i = 0; i < mSequence.size(); i++ ) {
		mSequence[i]->setTarget( getTarget() );
	}

	mDuration = Time::Zero;
	for ( auto& seq : mSequence ) {
		mDuration += seq->getTotalTime();
	}

	mSequence[mCurPos]->start();

	sendEvent( ActionType::OnStart );
}

void Sequence::stop() {
	mSequence[mCurPos]->stop();
	sendEvent( ActionType::OnStop );
}

void Sequence::update( const Time& time ) {
	if ( isDone() )
		return;

	mSequence[mCurPos]->update( time );

	if ( mSequence[mCurPos]->isDone() && mCurPos + 1 < mSequence.size() ) {
		mCurPos++;
		mSequence[mCurPos]->start();
		sendEvent( ActionType::OnStep );
	}
}

bool Sequence::isDone() {
	return mCurPos == mSequence.size() - 1 && mSequence[mCurPos]->isDone();
}

Float Sequence::getCurrentProgress() {
	Float progress = 0.f;
	for ( Uint32 i = 0; i <= mCurPos; i++ ) {
		if ( i < mSequence.size() ) {
			Float partialProgress =
				mSequence[i]->getTotalTime().asMilliseconds() / getTotalTime().asMilliseconds();
			if ( i != mCurPos ) {
				progress += partialProgress;
			} else {
				progress += partialProgress * mSequence[i]->getCurrentProgress();
			}
		}
	}
	return progress;
}

Time Sequence::getTotalTime() {
	return mDuration;
}

Action* Sequence::clone() const {
	return eeNew( Sequence, ( mSequence ) );
}

Action* Sequence::reverse() const {
	ActionContainer reversed;

	for ( auto it = mSequence.rbegin(); it != mSequence.rend(); ++it ) {
		reversed.push_back( *it );
	}

	return eeNew( Sequence, ( std::move( reversed ) ) );
}

Sequence::~Sequence() {
	for ( size_t i = 0; i < mSequence.size(); i++ ) {
		Action* action = mSequence[i];
		eeSAFE_DELETE( action );
	}
}

Sequence::Sequence( const std::vector<Action*> sequence ) :
	mSequence( sequence.begin(), sequence.end() ), mCurPos( 0 ) {}

Sequence::Sequence( ActionContainer sequence ) : mSequence( std::move( sequence ) ), mCurPos( 0 ) {}

}}} // namespace EE::Scene::Actions
