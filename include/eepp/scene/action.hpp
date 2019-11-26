#ifndef EE_SCENEACTION_HPP
#define EE_SCENEACTION_HPP

#include <cstdlib>
#include <eepp/core.hpp>
#include <eepp/system/time.hpp>
using namespace EE::System;

namespace EE { namespace Scene {

class Node;

class EE_API Action {
	public:
		enum ActionType
		{
			OnStart,
			OnStop,
			OnDone,
			OnStep,
			OnDelete
		};

		typedef std::function<void(Action*,const ActionType&)> ActionCallback;

		Action();

		virtual ~Action();

		virtual void start() = 0;

		virtual void stop() = 0;

		virtual void update( const Time& time ) = 0;

		virtual bool isDone() = 0;

		virtual Float getCurrentProgress() = 0;

		virtual Action * clone() const;

		virtual Action * reverse() const;

		Uint32 getFlags() const;

		void setFlags( const Uint32 & flags );

		Uint32 getTag() const;

		void setTag(const Uint32 & tag);

		Node * getTarget() const;

		Uint32 addEventListener( const ActionType & actionType, const ActionCallback & callback );

		Action * on( const ActionType & actionType, const ActionCallback & callback );

		void removeEventListener( const Uint32 & callbackId );

		void sendEvent( const ActionType & actionType );

		void setTarget( Node * target );
	protected:
		friend class Node;
		typedef std::map< ActionType, std::map<Uint32, ActionCallback> > ActionCallbackMap;

		Node * mNode;
		Uint32 mFlags;
		Uint32 mTag;
		Uint32 mNumCallBacks;
		ActionCallbackMap mCallbacks;

		virtual void onStart();

		virtual void onStop();

		virtual void onUpdate( const Time& time );
};

}}

#endif
