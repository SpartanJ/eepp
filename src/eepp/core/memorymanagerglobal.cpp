#include <cstddef>
#include <eepp/core/memorymanager.hpp>
#include <new>

#ifdef EE_MEMORY_MANAGER

void* operator new( std::size_t size ) {
	return EE::MemoryManager::allocateGlobal( size, alignof( std::max_align_t ) );
}

void* operator new[]( std::size_t size ) {
	return EE::MemoryManager::allocateGlobal( size, alignof( std::max_align_t ) );
}

void* operator new( std::size_t size, std::align_val_t alignment ) {
	return EE::MemoryManager::allocateGlobal( size, static_cast<std::size_t>( alignment ) );
}

void* operator new[]( std::size_t size, std::align_val_t alignment ) {
	return EE::MemoryManager::allocateGlobal( size, static_cast<std::size_t>( alignment ) );
}

void* operator new( std::size_t size, const std::nothrow_t& ) noexcept {
	try {
		return EE::MemoryManager::allocateGlobal( size, alignof( std::max_align_t ) );
	} catch ( ... ) {
		return nullptr;
	}
}

void* operator new[]( std::size_t size, const std::nothrow_t& ) noexcept {
	try {
		return EE::MemoryManager::allocateGlobal( size, alignof( std::max_align_t ) );
	} catch ( ... ) {
		return nullptr;
	}
}

void* operator new( std::size_t size, std::align_val_t alignment, const std::nothrow_t& ) noexcept {
	try {
		return EE::MemoryManager::allocateGlobal( size, static_cast<std::size_t>( alignment ) );
	} catch ( ... ) {
		return nullptr;
	}
}

void* operator new[]( std::size_t size, std::align_val_t alignment,
					  const std::nothrow_t& ) noexcept {
	try {
		return EE::MemoryManager::allocateGlobal( size, static_cast<std::size_t>( alignment ) );
	} catch ( ... ) {
		return nullptr;
	}
}

void operator delete( void* ptr ) noexcept {
	EE::MemoryManager::freeGlobal( ptr );
}
void operator delete[]( void* ptr ) noexcept {
	EE::MemoryManager::freeGlobal( ptr );
}
void operator delete( void* ptr, std::size_t ) noexcept {
	EE::MemoryManager::freeGlobal( ptr );
}
void operator delete[]( void* ptr, std::size_t ) noexcept {
	EE::MemoryManager::freeGlobal( ptr );
}
void operator delete( void* ptr, std::align_val_t ) noexcept {
	EE::MemoryManager::freeGlobal( ptr );
}
void operator delete[]( void* ptr, std::align_val_t ) noexcept {
	EE::MemoryManager::freeGlobal( ptr );
}
void operator delete( void* ptr, std::size_t, std::align_val_t ) noexcept {
	EE::MemoryManager::freeGlobal( ptr );
}
void operator delete[]( void* ptr, std::size_t, std::align_val_t ) noexcept {
	EE::MemoryManager::freeGlobal( ptr );
}
void operator delete( void* ptr, const std::nothrow_t& ) noexcept {
	EE::MemoryManager::freeGlobal( ptr );
}
void operator delete[]( void* ptr, const std::nothrow_t& ) noexcept {
	EE::MemoryManager::freeGlobal( ptr );
}
void operator delete( void* ptr, std::align_val_t, const std::nothrow_t& ) noexcept {
	EE::MemoryManager::freeGlobal( ptr );
}
void operator delete[]( void* ptr, std::align_val_t, const std::nothrow_t& ) noexcept {
	EE::MemoryManager::freeGlobal( ptr );
}

#endif
