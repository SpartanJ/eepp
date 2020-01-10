#include <eepp/scene/actions/disable.hpp>
#include <eepp/scene/node.hpp>

namespace EE { namespace Scene { namespace Actions {

Disable* Disable::New( const Time& time ) {
	return eeNew( Disable, ( time ) );
}

Action* Disable::clone() const {
	return New( mTime );
}

Disable::Disable( const Time& time ) : Enable( false, time ) {}

}}} // namespace EE::Scene::Actions
