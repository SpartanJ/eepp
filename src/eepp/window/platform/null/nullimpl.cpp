#include <eepp/window/platform/null/nullimpl.hpp>

namespace EE { namespace Window { namespace Platform {

NullImpl::NullImpl( EE::Window::Window * window ) :
	PlatformImpl( window )
{
}

NullImpl::~NullImpl() {
}

void NullImpl::MinimizeWindow() {
}

void NullImpl::MaximizeWindow() {
}

bool NullImpl::IsWindowMaximized() {
	return false;
}

void NullImpl::HideWindow() {
}

void NullImpl::RaiseWindow() {
}

void NullImpl::ShowWindow() {
}

void NullImpl::MoveWindow( int left, int top ) {
}

void NullImpl::SetContext( eeWindowContex Context ) {
}

Vector2i NullImpl::Position() {
	return Vector2i(0,0);
}

void NullImpl::ShowMouseCursor() {
}

void NullImpl::HideMouseCursor() {
}

Cursor * NullImpl::CreateMouseCursor( Texture * tex, const Vector2i& hotspot, const std::string& name ) {
	return NULL;
}

Cursor * NullImpl::CreateMouseCursor( Image * img, const Vector2i& hotspot, const std::string& name ) {
	return NULL;
}

Cursor * NullImpl::CreateMouseCursor( const std::string& path, const Vector2i& hotspot, const std::string& name ) {
	return NULL;
}

void NullImpl::SetMouseCursor( Cursor * cursor ) {
}

void NullImpl::SetSystemMouseCursor( EE_SYSTEM_CURSOR syscursor ) {
}

void NullImpl::RestoreCursor() {
}

eeWindowContex NullImpl::GetWindowContext() {
	return 0;
}

}}}
