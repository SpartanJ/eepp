#ifndef EE_UIACTION_HPP
#define EE_UIACTION_HPP

#include <cstdlib>
#include <eepp/core.hpp>
#include <eepp/system/time.hpp>
using namespace EE::System;

namespace EE { namespace UI {

class UINode;

class EE_API UIAction {
	public:
		enum ActionType
		{
			OnStart,
			OnStop,
			OnDone,
			OnStep
		};

		typedef cb::Callback2<void,UIAction*,const ActionType&> ActionCallback;

		UIAction();

		virtual ~UIAction();

		virtual void start() = 0;

		virtual void stop() = 0;

		virtual void update( const Time& time ) = 0;

		virtual bool isDone() = 0;

		virtual UIAction * clone() const;

		virtual UIAction * reverse() const;

		Uint32 getFlags() const;

		void setFlags( const Uint32 & flags );

		Uint32 getTag() const;

		void setTag(const Uint32 & tag);

		UINode * getTarget() const;

		Uint32 addEventListener( const ActionType & actionType, const ActionCallback & callback );

		void removeEventListener( const Uint32 & callbackId );

		void sendEvent( const ActionType & actionType );

		void setTarget( UINode * target );
	protected:
		friend class UINode;
		typedef std::map< ActionType, std::map<Uint32, ActionCallback> > ActionCallbackMap;

		UINode * mNode;
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
