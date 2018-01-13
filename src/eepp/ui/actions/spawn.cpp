#include <eepp/ui/actions/spawn.hpp>

namespace EE { namespace UI { namespace Action {

Spawn * Spawn::New( const std::vector<UIAction *> spawn ) {
	return eeNew( Spawn, ( spawn ) );
}

Spawn * Spawn::New( UIAction * action, UIAction * action2 ) {
	return New( { action, action2 } );
}

Spawn * Spawn::New( UIAction * action, UIAction * action2, UIAction * action3 ) {
	return New( { action, action2, action3 } );
}

Spawn * Spawn::New( UIAction * action, UIAction * action2, UIAction * action3, UIAction * action4 ) {
	return New( { action, action2, action3, action4 } );
}

Spawn * Spawn::New( UIAction * action, UIAction * action2, UIAction * action3, UIAction * action4, UIAction * action5 ) {
	return New( { action, action2, action3, action4, action5 } );
}

Spawn * Spawn::New( UIAction * action, UIAction * action2, UIAction * action3, UIAction * action4, UIAction * action5, UIAction * action6 ) {
	return New( { action, action2, action3, action4, action5, action6 } );
}

Spawn * Spawn::New( UIAction * action, UIAction * action2, UIAction * action3, UIAction * action4, UIAction * action5, UIAction * action6, UIAction * action7 ) {
	return New( { action, action2, action3, action4, action5, action6, action7 } );
}

Spawn * Spawn::New( UIAction * action, UIAction * action2, UIAction * action3, UIAction * action4, UIAction * action5, UIAction * action6, UIAction * action7, UIAction * action8 ) {
	return New( { action, action2, action3, action4, action5, action6, action7, action8 } );
}

Spawn * Spawn::New( UIAction * action, UIAction * action2, UIAction * action3, UIAction * action4, UIAction * action5, UIAction * action6, UIAction * action7, UIAction * action8, UIAction * action9 ) {
	return New( { action, action2, action3, action4, action5, action6, action7, action8, action9 } );
}

void Spawn::start() {
	for ( size_t i = 0; i < mSpawn.size(); i++ ) {
		mSpawn[ i ]->setTarget( getTarget() );
		mSpawn[ i ]->start();
	}

	sendEvent( ActionType::OnStart );
}

void Spawn::stop() {
	for ( size_t i = 0; i < mSpawn.size(); i++ )
		mSpawn[ i ]->stop();

	sendEvent( ActionType::OnStop );
}

void Spawn::update( const Time & time ) {
	if ( isDone() )
		return;

	bool allDone = true;

	for ( size_t i = 0; i < mSpawn.size(); i++ ) {
		mSpawn[ i ]->update( time );

		if ( !mSpawn[i]->isDone() ) {
			allDone = false;
		}
	}

	mAllDone = allDone;
}

bool Spawn::isDone() {
	return mAllDone;
}

UIAction * Spawn::clone() const {
	return Spawn::New( mSpawn );
}

UIAction * Spawn::reverse() const {
	std::vector<UIAction*> reversed;

	for ( auto it = mSpawn.rbegin(); it != mSpawn.rend(); ++it ) {
		reversed.push_back( *it );
	}

	return Spawn::New( reversed );
}

Spawn::~Spawn() {
	for ( size_t i = 0; i < mSpawn.size(); i++ ) {
		UIAction * action = mSpawn[ i ];
		eeSAFE_DELETE( action );
	}
}

Spawn::Spawn( const std::vector<UIAction*> spawn ) :
	mSpawn( spawn )
{}

}}} 
