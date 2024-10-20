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
	if ( !mCurModel )
		mCurModel = mBaseModel;
}

const std::vector<std::vector<std::string>>& CommandPalette::getCommandPalette() const {
	return mCommandPalette;
}

const std::shared_ptr<CommandPaletteModel>& CommandPalette::getBaseModel() const {
	return mBaseModel;
}

void CommandPalette::setEditorCommandPalette( const std::vector<std::string>& commandList,
											  const UI::KeyBindings& keybindings ) {
	mCommandPaletteEditor = build( commandList, keybindings );
	mEditorModel = CommandPaletteModel::create( 3, mCommandPaletteEditor );
	if ( !mCurModel )
		mCurModel = mEditorModel;
}

const std::shared_ptr<CommandPaletteModel>& CommandPalette::getCurModel() const {
	return mCurModel;
}

const std::shared_ptr<CommandPaletteModel>& CommandPalette::getEditorModel() const {
	return mEditorModel;
}

void CommandPalette::setCommandPaletteEditor(
	const std::vector<std::vector<std::string>>& commandPaletteEditor ) {
	mCommandPaletteEditor = commandPaletteEditor;
}

void CommandPalette::setCurModel( const std::shared_ptr<CommandPaletteModel>& curModel ) {
	mCurModel = curModel;
}

std::shared_ptr<CommandPaletteModel>
CommandPalette::fuzzyMatch( const std::vector<std::vector<std::string>>& cmdPalette,
							const std::string& match, const size_t& max ) const {
	if ( cmdPalette.empty() )
		return {};

	Lock rl( mMatchingMutex );
	std::multimap<int, int, std::greater<int>> matchesMap;
	std::vector<std::vector<std::string>> ret;

	for ( size_t i = 0; i < cmdPalette.size(); i++ ) {
		int matchName = String::fuzzyMatch( cmdPalette[i][0], match );
		int matchKeybind = String::fuzzyMatch( cmdPalette[i][2], match );
		matchesMap.insert( { std::max( matchName, matchKeybind ), i } );
	}
	for ( auto& res : matchesMap ) {
		if ( ret.size() < max )
			ret.push_back( cmdPalette[res.second] );
	}
	return CommandPaletteModel::create( 3, ret );
}

void CommandPalette::asyncFuzzyMatch( const std::string& match, const size_t& max,
									  MatchResultCb res ) const {
	if ( !mCurModel )
		return;

	mPool->run( [this, match, max, res]() {
		const std::vector<std::vector<std::string>>& cmdPalette =
			mCurModel.get() == mBaseModel.get() ? mCommandPalette : mCommandPaletteEditor;
		res( fuzzyMatch( cmdPalette, match, max ) );
	} );
}

} // namespace ecode
