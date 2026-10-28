#include <eepp/ui/tools/keybindingsconfigdata.hpp>

namespace EE::UI::Tools {

KeyBindings::Shortcut KeybindingsConfigData::resolve( const Input* input,
													  std::string expression ) const {
	String::toLowerInPlace( expression );
	// Substitute complete modifier tokens. Literal key names such as ModeSwitch must
	// retain their spelling instead of having a "mod" substring rewritten.
	for ( size_t start = 0; start < expression.size(); ) {
		auto end = expression.find( '+', start );
		auto token =
			std::string_view( expression )
				.substr( start, end == std::string::npos ? std::string::npos : end - start );
		const std::string* replacement = nullptr;
		if ( token == "mod" || token == "modifier" )
			replacement = &modifier;
		else if ( token == "mod2" || token == "modifier2" )
			replacement = &secondaryModifier;
		if ( replacement ) {
			expression.replace( start, token.size(), *replacement );
			if ( end != std::string::npos )
				end = start + replacement->size();
		}
		if ( end == std::string::npos )
			break;
		start = end + 1;
	}
	return KeyBindings::sanitizeShortcut( KeyBindings::toShortcut( input, expression ) );
}

KeyBindings::ShortcutMap KeybindingsConfigData::shortcuts( const Input* input,
														   std::string_view context ) const {
	KeyBindings::ShortcutMap result;
	for ( const auto& command : commands ) {
		if ( command.context != context || command.unbound )
			continue;
		for ( const auto& expression : command.bindings )
			result[resolve( input, expression )] = command.command;
	}
	return result;
}

} // namespace EE::UI::Tools
