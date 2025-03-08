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
	explicit CommandPalette( const std::shared_ptr<ThreadPool>& pool );

	typedef std::function<void( std::shared_ptr<CommandPaletteModel> )> MatchResultCb;

	static std::vector<std::vector<std::string>> build( const std::vector<std::string>& commandList,
														const EE::UI::KeyBindings& keybindings );

	static std::shared_ptr<CommandPaletteModel>
	asModel( const std::vector<std::string>& commandList, const EE::UI::KeyBindings& keybindings );

	void asyncFuzzyMatch( const std::string& pattern, const size_t& max, MatchResultCb res ) const;

	std::shared_ptr<CommandPaletteModel>
	fuzzyMatch( const std::vector<std::vector<std::string>>& cmdPalette, const std::string& pattern,
				const size_t& max ) const;

	void setCommandPalette( const std::vector<std::string>& commandList,
							const EE::UI::KeyBindings& keybindings );

	const std::vector<std::vector<std::string>>& getCommandPalette() const;

	bool isSet() const { return !mCommandPalette.empty(); }

	const std::shared_ptr<CommandPaletteModel>& getBaseModel() const;

	bool isEditorSet() const { return !mCommandPaletteEditor.empty(); }

	void setEditorCommandPalette( const std::vector<std::string>& commandList,
								  const EE::UI::KeyBindings& keybindings );

	const std::shared_ptr<CommandPaletteModel>& getCurModel() const;

	const std::shared_ptr<CommandPaletteModel>& getEditorModel() const;

	void
	setCommandPaletteEditor( const std::vector<std::vector<std::string>>& commandPaletteEditor );

	void setCurModel( const std::shared_ptr<CommandPaletteModel>& curModel );

  protected:
	mutable Mutex mMatchingMutex;
	std::shared_ptr<ThreadPool> mPool;
	std::vector<std::vector<std::string>> mCommandPalette;
	std::vector<std::vector<std::string>> mCommandPaletteEditor;
	std::shared_ptr<CommandPaletteModel> mCurModel;
	std::shared_ptr<CommandPaletteModel> mBaseModel;
	std::shared_ptr<CommandPaletteModel> mEditorModel;
};

} // namespace ecode

#endif // ECODE_COMMANDPALETTE_HPP
