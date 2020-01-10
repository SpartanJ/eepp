#include <eepp/scene/actions/spawn.hpp>

namespace EE { namespace Scene { namespace Actions {

Spawn* Spawn::New( const std::vector<Action*> spawn ) {
	return eeNew( Spawn, ( spawn ) );
}

Spawn* Spawn::New( Action* action, Action* action2 ) {
	return New( {action, action2} );
}

Spawn* Spawn::New( Action* action, Action* action2, Action* action3 ) {
	return New( {action, action2, action3} );
}

Spawn* Spawn::New( Action* action, Action* action2, Action* action3, Action* action4 ) {
	return New( {action, action2, action3, action4} );
}

Spawn* Spawn::New( Action* action, Action* action2, Action* action3, Action* action4,
				   Action* action5 ) {
	return New( {action, action2, action3, action4, action5} );
}

Spawn* Spawn::New( Action* action, Action* action2, Action* action3, Action* action4,
				   Action* action5, Action* action6 ) {
	return New( {action, action2, action3, action4, action5, action6} );
}

Spawn* Spawn::New( Action* action, Action* action2, Action* action3, Action* action4,
				   Action* action5, Action* action6, Action* action7 ) {
	return New( {action, action2, action3, action4, action5, action6, action7} );
}

Spawn* Spawn::New( Action* action, Action* action2, Action* action3, Action* action4,
				   Action* action5, Action* action6, Action* action7, Action* action8 ) {
	return New( {action, action2, action3, action4, action5, action6, action7, action8} );
}

Spawn* Spawn::New( Action* action, Action* action2, Action* action3, Action* action4,
				   Action* action5, Action* action6, Action* action7, Action* action8,
				   Action* action9 ) {
	return New( {action, action2, action3, action4, action5, action6, action7, action8, action9} );
}

void Spawn::start() {
	for ( size_t i = 0; i < mSpawn.size(); i++ ) {
		mSpawn[i]->setTarget( getTarget() );
		mSpawn[i]->start();
	}

	sendEvent( ActionType::OnStart );
}

void Spawn::stop() {
	for ( size_t i = 0; i < mSpawn.size(); i++ )
		mSpawn[i]->stop();

	sendEvent( ActionType::OnStop );
}

void Spawn::update( const Time& time ) {
	if ( isDone() )
		return;

	bool allDone = true;

	for ( size_t i = 0; i < mSpawn.size(); i++ ) {
		mSpawn[i]->update( time );

		if ( !mSpawn[i]->isDone() ) {
			allDone = false;
		}
	}

	mAllDone = allDone;
}

bool Spawn::isDone() {
	return mAllDone;
}

Float Spawn::getCurrentProgress() {
	Float min = 1.f;

	for ( size_t i = 0; i < mSpawn.size(); i++ ) {
		min = eemin( min, mSpawn[i]->getCurrentProgress() );
	}

	return min;
}

Action* Spawn::clone() const {
	return Spawn::New( mSpawn );
}

Action* Spawn::reverse() const {
	std::vector<Action*> reversed;

	for ( auto it = mSpawn.rbegin(); it != mSpawn.rend(); ++it ) {
		reversed.push_back( *it );
	}

	return Spawn::New( reversed );
}

Spawn::~Spawn() {
	for ( size_t i = 0; i < mSpawn.size(); i++ ) {
		Action* action = mSpawn[i];
		eeSAFE_DELETE( action );
	}
}

Spawn::Spawn( const std::vector<Action*> spawn ) : mSpawn( spawn ), mAllDone( false ) {}

}}} // namespace EE::Scene::Actions
