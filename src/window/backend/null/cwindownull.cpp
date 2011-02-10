#include "cwindownull.hpp"
#include "cclipboardnull.hpp"
#include "cinputnull.hpp"

namespace EE { namespace Window { namespace Backend { namespace Null {

cWindowNull::cWindowNull( WindowSettings Settings, ContextSettings Context ) :
	cWindow( Settings, Context, eeNew( cClipboardNull, ( this ) ), eeNew( cInputNull, ( this ) ) )
{
	Create( Settings, Context );
}

cWindowNull::~cWindowNull() {
}

bool cWindowNull::Create( WindowSettings Settings, ContextSettings Context ) {
	return false;
}

void cWindowNull::ToggleFullscreen() {
}

void cWindowNull::Caption( const std::string& Caption ) {
}

std::string cWindowNull::Caption() {
	return std::string();
}

bool cWindowNull::Icon( const std::string& Path ) {
	return false;
}

void cWindowNull::Minimize() {
}

void cWindowNull::Maximize() {
}

void cWindowNull::Hide() {
}

void cWindowNull::Raise() {
}

void cWindowNull::Show() {
}

void cWindowNull::Position( Int16 Left, Int16 Top ) {
}

bool cWindowNull::Active() {
	return true;
}

bool cWindowNull::Visible() {
	return true;
}

eeVector2i cWindowNull::Position() {
	return eeVector2i();
}

void cWindowNull::Size( const Uint32& Width, const Uint32& Height ) {
}

void cWindowNull::Size( const Uint16& Width, const Uint16& Height, const bool& Windowed ) {
}

void cWindowNull::ShowCursor( const bool& showcursor ) {
}

void cWindowNull::SwapBuffers() {
}

std::vector< std::pair<unsigned int, unsigned int> > cWindowNull::GetPossibleResolutions() const {
	return std::vector< std::pair<unsigned int, unsigned int> >();
}

void cWindowNull::SetGamma( eeFloat Red, eeFloat Green, eeFloat Blue ) {
}

void cWindowNull::SetCurrentContext( eeWindowContex Context ) {
}

eeWindowContex cWindowNull::GetContext() const {
	return 0;
}

void cWindowNull::GetMainContext() {
}

eeWindowHandler	cWindowNull::GetWindowHandler() {
	return 0;
}

void cWindowNull::SetDefaultContext() {
}

}}}}
