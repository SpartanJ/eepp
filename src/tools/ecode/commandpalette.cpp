#include "commandpalette.hpp"

namespace ecode {

std::vector<std::vector<std::string>>
CommandPalette::build( const std::vector<std::string>& commandList,
					   const EE::UI::KeyBindings& keybindings ) {
	std::vector<std::vector<std::string>> ret;
	for ( const auto& cmd : commandList ) {
		std::string cmdName( cmd );
		String::capitalizeInPlace( cmdName );
		String::replaceAll( cmdName, "-", " " );
		ret.push_back( { cmdName, keybindings.getCommandKeybindString( cmd ), cmd } );
	}
	return ret;
}

std::shared_ptr<CommandPaletteModel>
CommandPalette::asModel( const std::vector<std::string>& commandList,
						 const EE::UI::KeyBindings& keybindings ) {
	return CommandPaletteModel::create( 3, build( commandList, keybindings ) );
}

CommandPalette::CommandPalette( const std::shared_ptr<ThreadPool>& pool ) : mPool( pool ) {}

void CommandPalette::setCommandPalette( const std::vector<std::string>& commandList,
										const UI::KeyBindings& keybindings ) {
	mCommandPalette = build( commandList, keybindings );
	mBaseModel = CommandPaletteModel::create( 3, mCommandPalette );
}

const std::vector<std::vector<std::string>>& CommandPalette::getCommandPalette() const {
	return mCommandPalette;
}

const std::shared_ptr<CommandPaletteModel>& CommandPalette::getBaseModel() const {
	return mBaseModel;
}

std::shared_ptr<CommandPaletteModel> CommandPalette::fuzzyMatch( const std::string& match,
																 const size_t& max ) const {
	if ( mCommandPalette.empty() )
		return {};

	Lock rl( mMatchingMutex );
	std::multimap<int, int, std::greater<int>> matchesMap;
	std::vector<std::vector<std::string>> ret;

	for ( size_t i = 0; i < mCommandPalette.size(); i++ ) {
		int matchName = String::fuzzyMatch( mCommandPalette[i][0], match );
		int matchKeybind = String::fuzzyMatch( mCommandPalette[i][1], match );
		matchesMap.insert( { std::max( matchName, matchKeybind ), i } );
	}
	for ( auto& res : matchesMap ) {
		if ( ret.size() < max ) {
			ret.push_back( { mCommandPalette[res.second][0], mCommandPalette[res.second][1],
							 mCommandPalette[res.second][2] } );
		}
	}
	return CommandPaletteModel::create( 3, ret );
}

void CommandPalette::asyncFuzzyMatch( const std::string& match, const size_t& max,
									  MatchResultCb res ) const {
	mPool->run( [&, match, max, res]() { res( fuzzyMatch( match, max ) ); } );
}

} // namespace ecode
