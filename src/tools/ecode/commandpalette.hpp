#ifndef ECODE_COMMANDPALETTE_HPP
#define ECODE_COMMANDPALETTE_HPP

#include <eepp/core.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/keyboardshortcut.hpp>
#include <eepp/ui/models/itemlistmodel.hpp>

using namespace EE;
using namespace EE::UI::Models;
using namespace EE::System;

namespace ecode {

using CommandPaletteModel = ItemVectorListOwnerModel<std::string>;

class CommandPalette {
  public:
	CommandPalette( const std::shared_ptr<ThreadPool>& pool );

	typedef std::function<void( std::shared_ptr<CommandPaletteModel> )> MatchResultCb;

	static std::vector<std::vector<std::string>> build( const std::vector<std::string>& commandList,
														const EE::UI::KeyBindings& keybindings );

	static std::shared_ptr<CommandPaletteModel>
	asModel( const std::vector<std::string>& commandList, const EE::UI::KeyBindings& keybindings );

	void asyncFuzzyMatch( const std::string& match, const size_t& max, MatchResultCb res ) const;

	std::shared_ptr<CommandPaletteModel> fuzzyMatch( const std::string& match,
													 const size_t& max ) const;

	void setCommandPalette( const std::vector<std::string>& commandList,
							const EE::UI::KeyBindings& keybindings );

	const std::vector<std::vector<std::string>>& getCommandPalette() const;

	bool isSet() const { return !mCommandPalette.empty(); }

	const std::shared_ptr<CommandPaletteModel>& getBaseModel() const;

  protected:
	mutable Mutex mMatchingMutex;
	std::shared_ptr<ThreadPool> mPool;
	std::vector<std::vector<std::string>> mCommandPalette;
	std::shared_ptr<CommandPaletteModel> mBaseModel;
};

} // namespace ecode

#endif // ECODE_COMMANDPALETTE_HPP
