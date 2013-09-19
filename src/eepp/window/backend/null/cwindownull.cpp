#include <eepp/window/backend/null/cwindownull.hpp>
#include <eepp/window/backend/null/cclipboardnull.hpp>
#include <eepp/window/backend/null/cinputnull.hpp>
#include <eepp/window/backend/null/ccursormanagernull.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

cWindowNull::cWindowNull( WindowSettings Settings, ContextSettings Context ) :
	cWindow( Settings, Context, eeNew( cClipboardNull, ( this ) ), eeNew( cInputNull, ( this ) ), eeNew( cCursorManagerNull, ( this ) ) )
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

void cWindowNull::Size( Uint32 Width, Uint32 Height, bool Windowed ) {
}

void cWindowNull::SwapBuffers() {
}

std::vector<DisplayMode> cWindowNull::GetDisplayModes() const {
	return std::vector<DisplayMode>();
}

void cWindowNull::SetGamma( eeFloat Red, eeFloat Green, eeFloat Blue ) {
}

eeWindowContex cWindowNull::GetContext() const {
	return 0;
}

void cWindowNull::GetMainContext() {
}

eeWindowHandle	cWindowNull::GetWindowHandler() {
	return 0;
}

void cWindowNull::SetDefaultContext() {
}

}}}}
