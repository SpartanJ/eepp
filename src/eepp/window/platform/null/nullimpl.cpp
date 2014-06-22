#include <eepp/window/platform/null/nullimpl.hpp>

namespace EE { namespace Window { namespace Platform {

cNullImpl::cNullImpl( EE::Window::Window * window ) :
	PlatformImpl( window )
{
}

cNullImpl::~cNullImpl() {
}

void cNullImpl::MinimizeWindow() {
}

void cNullImpl::MaximizeWindow() {
}

bool cNullImpl::IsWindowMaximized() {
	return false;
}

void cNullImpl::HideWindow() {
}

void cNullImpl::RaiseWindow() {
}

void cNullImpl::ShowWindow() {
}

void cNullImpl::MoveWindow( int left, int top ) {
}

void cNullImpl::SetContext( eeWindowContex Context ) {
}

Vector2i cNullImpl::Position() {
	return Vector2i(0,0);
}

void cNullImpl::ShowMouseCursor() {
}

void cNullImpl::HideMouseCursor() {
}

Cursor * cNullImpl::CreateMouseCursor( Texture * tex, const Vector2i& hotspot, const std::string& name ) {
	return NULL;
}

Cursor * cNullImpl::CreateMouseCursor( Image * img, const Vector2i& hotspot, const std::string& name ) {
	return NULL;
}

Cursor * cNullImpl::CreateMouseCursor( const std::string& path, const Vector2i& hotspot, const std::string& name ) {
	return NULL;
}

void cNullImpl::SetMouseCursor( Cursor * cursor ) {
}

void cNullImpl::SetSystemMouseCursor( EE_SYSTEM_CURSOR syscursor ) {
}

void cNullImpl::RestoreCursor() {
}

eeWindowContex cNullImpl::GetWindowContext() {
	return 0;
}

}}}
