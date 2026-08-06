#include <array>
#include <cstdlib>
#include <eepp/scene/action.hpp>
#include <eepp/scene/node.hpp>
#include <mutex>
#include <new>

namespace EE { namespace Scene {

namespace {

class ActionPool {
  public:
	static ActionPool& instance();

	void* allocate( std::size_t size ) {
		const std::size_t classIndex = findClass( size );
		if ( classIndex == NumClasses )
			return allocateLarge( size );

		std::lock_guard<std::mutex> lock( mMutex );
		SizeClass& sizeClass = mClasses[classIndex];
		if ( nullptr == sizeClass.freeList )
			allocatePage( classIndex );

		BlockHeader* block = sizeClass.freeList;
		sizeClass.freeList = block->next;
		block->classIndex = static_cast<Uint16>( classIndex );
		return block + 1;
	}

	void deallocate( void* ptr ) noexcept {
		if ( nullptr == ptr )
			return;

		BlockHeader* block = static_cast<BlockHeader*>( ptr ) - 1;
		if ( block->classIndex == LargeAllocation ) {
			std::free( block );
			return;
		}

		std::lock_guard<std::mutex> lock( mMutex );
		SizeClass& sizeClass = mClasses[block->classIndex];
		block->next = sizeClass.freeList;
		sizeClass.freeList = block;
	}

  private:
	static constexpr std::array<std::size_t, 9> ClassSizes = { 64,	96,	 128, 192, 256,
															   384, 512, 768, 1024 };
	static constexpr std::size_t NumClasses = ClassSizes.size();
	static constexpr Uint16 LargeAllocation = static_cast<Uint16>( -1 );
	static constexpr std::size_t BlocksPerPage = 64;

	struct alignas( std::max_align_t ) BlockHeader {
		BlockHeader* next;
		Uint16 classIndex;
	};

	struct Page {
		Page* next;
	};

	struct SizeClass {
		BlockHeader* freeList{ nullptr };
		Page* pages{ nullptr };
	};

	std::array<SizeClass, NumClasses> mClasses;
	std::mutex mMutex;

	static constexpr std::size_t alignUp( std::size_t size ) {
		return ( size + alignof( std::max_align_t ) - 1 ) & ~( alignof( std::max_align_t ) - 1 );
	}

	static std::size_t findClass( std::size_t size ) {
		for ( std::size_t i = 0; i < NumClasses; ++i ) {
			if ( size <= ClassSizes[i] )
				return i;
		}
		return NumClasses;
	}

	void allocatePage( std::size_t classIndex ) {
		SizeClass& sizeClass = mClasses[classIndex];
		const std::size_t stride = alignUp( sizeof( BlockHeader ) + ClassSizes[classIndex] );
		const std::size_t bytes = alignUp( sizeof( Page ) ) + stride * BlocksPerPage;
		Page* page = static_cast<Page*>( std::malloc( bytes ) );
		if ( nullptr == page )
			throw std::bad_alloc();

		page->next = sizeClass.pages;
		sizeClass.pages = page;
		char* storage = reinterpret_cast<char*>( page ) + alignUp( sizeof( Page ) );
		for ( std::size_t i = 0; i < BlocksPerPage; ++i ) {
			BlockHeader* block = reinterpret_cast<BlockHeader*>( storage + stride * i );
			block->classIndex = static_cast<Uint16>( classIndex );
			block->next = sizeClass.freeList;
			sizeClass.freeList = block;
		}
	}

	static void* allocateLarge( std::size_t size ) {
		BlockHeader* block =
			static_cast<BlockHeader*>( std::malloc( sizeof( BlockHeader ) + size ) );
		if ( nullptr == block )
			throw std::bad_alloc();
		block->classIndex = LargeAllocation;
		block->next = nullptr;
		return block + 1;
	}
};

ActionPool& ActionPool::instance() {
	// Actions can be owned by globals initialized before the first action allocation. Keeping the
	// pool alive until process exit avoids a static-destruction-order race with those owners.
	alignas( ActionPool ) static unsigned char storage[sizeof( ActionPool )];
	static ActionPool* pool = new ( storage ) ActionPool;
	return *pool;
}

} // namespace

void* Action::operator new( std::size_t size ) {
	return ActionPool::instance().allocate( size );
}

void Action::operator delete( void* ptr ) noexcept {
	ActionPool::instance().deallocate( ptr );
}

void Action::operator delete( void* ptr, std::size_t ) noexcept {
	ActionPool::instance().deallocate( ptr );
}

Action::~Action() {
	sendEvent( ActionType::OnDelete );
}

Uint32 Action::getFlags() const {
	return mFlags;
}

void Action::setFlags( const Uint32& flags ) {
	mFlags = flags;
}

Action::UniqueID Action::getTag() const {
	return mTag;
}

void Action::setTag( const Action::UniqueID& tag ) {
	mTag = tag;
}

void Action::setTarget( Node* target ) {
	if ( mNode != target ) {
		mNode = target;
		onTargetChange();
	}
}

void Action::setId( const UniqueID& id ) {
	mId = id;
}

const Action::UniqueID& Action::getId() {
	return mId;
}

Node* Action::getTarget() const {
	return mNode;
}

Action* Action::clone() const {
	return NULL;
}

Action* Action::reverse() const {
	return NULL;
}

Uint32 Action::addEventListener( const ActionType& actionType, const ActionCallback& callback ) {
	mNumCallBacks++;

	mCallbacks.emplace_back( ActionCallbackEntry{ actionType, mNumCallBacks, callback } );

	return mNumCallBacks;
}

Action* Action::on( const Action::ActionType& actionType, const Action::ActionCallback& callback ) {
	addEventListener( actionType, callback );
	return this;
}

void Action::removeEventListener( const Uint32& callbackId ) {
	for ( auto it = mCallbacks.begin(); it != mCallbacks.end(); ++it ) {
		if ( it->id == callbackId ) {
			mCallbacks.erase( it );
			break;
		}
	}
}

void Action::sendEvent( const ActionType& actionType ) {
	// Callbacks are allowed to mutate listener registration. Preserve the previous snapshot
	// semantics while keeping the common callback count in inline storage.
	SmallVector<ActionCallback, 4> callbacks;
	for ( const auto& callback : mCallbacks ) {
		if ( callback.type == actionType )
			callbacks.emplace_back( callback.callback );
	}
	for ( auto& callback : callbacks )
		callback( this, actionType );
}

void Action::onStart() {}

void Action::onStop() {}

void Action::onUpdate( const Time& ) {}

void Action::onTargetChange() {}

}} // namespace EE::Scene
