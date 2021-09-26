#ifndef FORMATTERMODULE_HPP
#define FORMATTERMODULE_HPP

#include <eepp/config.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <set>
using namespace EE;
using namespace EE::System;
using namespace EE::UI;

class FormatterModule : public UICodeEditorModule {
  public:
	FormatterModule( const std::string& formatterPath, std::shared_ptr<ThreadPool> pool );

	virtual ~FormatterModule();

	void onRegister( UICodeEditor* );

	void onUnregister( UICodeEditor* );

  protected:
	enum class FormatterType { Inplace, Output };

	struct Formatter {
		std::vector<std::string> files;
		std::string command;
		FormatterType type{ FormatterType::Output };
	};

	std::shared_ptr<ThreadPool> mPool;
	std::vector<Formatter> mFormatters;
	std::set<UICodeEditor*> mEditors;
	bool mClosing{ false };

	bool mReady{ false };

	void load( const std::string& formatterPath );

	void formatDoc( UICodeEditor* editor );

	void runFormatter( UICodeEditor* editor, const Formatter& formatter, const std::string& path );

	FormatterModule::Formatter supportsFormatter( std::shared_ptr<TextDocument> doc );
};

#endif // FORMATTERMODULE_HPP
