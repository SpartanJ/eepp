#include <eepp/ui/tools/fileassociationssettings.hpp>

#include <algorithm>
#include <eepp/core/containers.hpp>
#include <eepp/scene/mainthreadlifetime.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitablecell.hpp>
#include <eepp/ui/uitableview.hpp>

using namespace EE::System;
using namespace EE::Scene;
using namespace EE::UI::Models;

namespace EE::UI::Tools {

namespace {

class FileAssociationsModel final : public Model {
  public:
	enum Columns { Registered, Extension, Count };

	struct Entry {
		std::string extension;
		bool registered{ false };
	};

	FileAssociationsModel( std::vector<std::string> extensions,
						   const std::vector<std::string>& registered, String registeredColumn,
						   String extensionColumn ) :
		mRegisteredColumn( std::move( registeredColumn ) ),
		mExtensionColumn( std::move( extensionColumn ) ) {
		UnorderedSet<std::string> registeredSet( registered.begin(), registered.end() );
		mEntries.reserve( extensions.size() );
		for ( auto& extension : extensions ) {
			const bool selected = registeredSet.contains( extension );
			mEntries.push_back( { std::move( extension ), selected } );
		}
	}

	size_t rowCount( const ModelIndex& = {} ) const { return mEntries.size(); }

	size_t columnCount( const ModelIndex& = {} ) const { return Count; }

	std::string columnName( const size_t& column ) const {
		return column == Registered ? mRegisteredColumn.toUtf8() : mExtensionColumn.toUtf8();
	}

	Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const {
		if ( !index.isValid() || static_cast<size_t>( index.row() ) >= mEntries.size() )
			return {};
		const auto& entry = mEntries[index.row()];
		if ( role == ModelRole::Data && index.column() == Registered )
			return Variant( entry.registered );
		if ( role == ModelRole::Display && index.column() == Extension )
			return Variant( '.' + entry.extension );
		return {};
	}

	void setRegistered( size_t row, bool registered ) {
		if ( row < mEntries.size() )
			mEntries[row].registered = registered;
	}

	void setAllRegistered( bool registered ) {
		for ( auto& entry : mEntries )
			entry.registered = registered;
		invalidate( Model::UpdateFlag::DontInvalidateIndexes );
	}

	std::vector<std::string> extensions() const {
		std::vector<std::string> extensions;
		extensions.reserve( mEntries.size() );
		for ( const auto& entry : mEntries )
			extensions.emplace_back( entry.extension );
		return extensions;
	}

	std::vector<std::string> registeredExtensions() const {
		std::vector<std::string> extensions;
		for ( const auto& entry : mEntries ) {
			if ( entry.registered )
				extensions.emplace_back( entry.extension );
		}
		return extensions;
	}

  private:
	std::vector<Entry> mEntries;
	String mRegisteredColumn;
	String mExtensionColumn;
};

class FileAssociationTableCell final : public UITableCell {
  public:
	static FileAssociationTableCell* New( const std::string& tag, FileAssociationsModel* model,
										  ModelIndex index ) {
		return eeNew( FileAssociationTableCell, ( tag, model, index ) );
	}

	FileAssociationTableCell( const std::string& tag, FileAssociationsModel* model,
							  ModelIndex index ) :
		UITableCell( tag,
					 [model, index]( UIPushButton* ) -> UITextView* {
						 auto* check = UICheckBox::New();
						 check->setCheckMode( UICheckBox::Button );
						 check->setChecked( model->data( index, ModelRole::Data ).asBool() );
						 return check;
					 } ),
		mModel( model ) {}

	void updateCell( Model* model ) {
		if ( mTextBox->isType( UI_TYPE_CHECKBOX ) ) {
			auto* check = mTextBox->asType<UICheckBox>();
			mUpdating = true;
			check->setChecked( model->data( getCurIndex(), ModelRole::Data ).asBool() );
			mUpdating = false;
			if ( !mListening ) {
				check->on( Event::OnValueChange, [this, check]( const Event* ) {
					if ( !mUpdating )
						mModel->setRegistered( getCurIndex().row(), check->isChecked() );
				} );
				mListening = true;
			}
		}
	}

  private:
	FileAssociationsModel* mModel{ nullptr };
	bool mUpdating{ false };
	bool mListening{ false };
};

class UIFileAssociationsTableView final : public UITableView {
  public:
	static UIFileAssociationsTableView* New() { return eeNew( UIFileAssociationsTableView, () ); }

	UIWidget* createCell( UIWidget* rowWidget, const ModelIndex& index ) {
		if ( index.column() == FileAssociationsModel::Registered ) {
			auto* cell = FileAssociationTableCell::New(
				mTag + "::cell", static_cast<FileAssociationsModel*>( getModel() ), index );
			cell->getTextView()->setEnabled( true );
			cell->setDontAutoHideEmptyTextBox( true );
			return setupCell( cell, rowWidget, index );
		}
		return UITableView::createCell( rowWidget, index );
	}

  private:
	UIFileAssociationsTableView() : UITableView() {}
};

struct FileAssociationsViewState {
	std::shared_ptr<FileAssociationsModel> model;
	MainThreadLifetime<FileAssociationsViewState> lifetime;
	FileAssociationsSettings::Completion completion;
	UIWidget* layout{ nullptr };
	UICheckBox* desktopEntry{ nullptr };
	UIFileAssociationsTableView* table{ nullptr };
	UIPushButton* selectAll{ nullptr };
	UIPushButton* clear{ nullptr };
	UIPushButton* apply{ nullptr };
	bool applying{ false };
	bool desktopEntryInstalled{ false };

	explicit FileAssociationsViewState( UISettingsPanel* panel ) :
		lifetime( this, panel->getUISceneNode() ) {}

	void setControlsEnabled( bool enabled ) {
		if ( desktopEntry )
			desktopEntry->setEnabled( enabled );
		if ( table )
			table->setEnabled( enabled );
		if ( selectAll )
			selectAll->setEnabled( enabled );
		if ( clear )
			clear->setEnabled( enabled );
		if ( apply )
			apply->setEnabled( enabled );
	}

	void clearControls() {
		layout = nullptr;
		desktopEntry = nullptr;
		table = nullptr;
		selectAll = nullptr;
		clear = nullptr;
		apply = nullptr;
	}
};

} // namespace

FileAssociationApplication
FileAssociationsSettings::applicationForCurrentProcess( const std::string& name,
														std::string iconPath ) {
	auto executable = Sys::getProcessFilePath();
#if EE_PLATFORM == EE_PLATFORM_WIN
	iconPath = "\"" + executable + "\",0";
#else
	if ( iconPath.empty() )
		iconPath = Sys::getResourcePath( name ) + "assets/icon/" + name + ".png";
#endif
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
	// Register the persistent launcher, not an AppImage mount or the bundled .bin file.
	auto appImage = Sys::getEnv( "APPIMAGE" );
	if ( !appImage.empty() ) {
		executable = std::move( appImage );
		iconPath = name;
	} else if ( String::endsWith( executable, ".bin" ) ) {
		auto launcher = executable.substr( 0, executable.size() - 4 );
		if ( FileSystem::fileExists( launcher ) )
			executable = std::move( launcher );
	}
#endif
	return { name, name, std::move( executable ), std::move( iconPath ) };
}

void FileAssociationsSettings::addLauncherTo( UISettingsPanel* panel,
											  const std::string& applicationName ) {
	if ( !FileAssociation::supportsDesktopEntries() )
		return;
	panel->addCategory( "general.desktop_integration", "General", "Desktop Integration" );
	addTo( panel,
		   { "desktopEntry", "general.desktop_integration", "Application Launcher",
			 "Show this application in the system applications menu." },
		   applicationForCurrentProcess( applicationName ), {},
		   panel->getUISceneNode()->getThreadPool(), {},
		   []( bool success, const std::string& error ) {
			   auto* message = UIMessageBox::New(
				   UIMessageBox::OK,
				   success ? String( "Application launcher updated." )
						   : String( "Could not update the application launcher.\n" ) +
								 String::fromUtf8( error ) );
			   message->setTitle( "Desktop Integration" );
			   message->center();
			   message->showWhenReady();
		   } );
}

void FileAssociationsSettings::addTo( UISettingsPanel* panel, SettingDescriptor descriptor,
									  FileAssociationApplication application,
									  std::vector<std::string> extensions,
									  std::shared_ptr<ThreadPool> threadPool, Translate translate,
									  Completion completion ) {
	if ( !translate )
		translate = []( const std::string&, const String& fallback ) { return fallback; };
	std::sort( extensions.begin(), extensions.end() );
	FileAssociation association( application );
	auto registered = extensions.empty() ? std::vector<std::string>{}
										 : association.getRegisteredExtensions( extensions );
	auto viewState = std::make_shared<FileAssociationsViewState>( panel );
	viewState->completion = std::move( completion );
	if ( !extensions.empty() ) {
		viewState->model = std::make_shared<FileAssociationsModel>(
			std::move( extensions ), registered, translate( "registered", "Registered" ),
			translate( "extension", "Extension" ) );
	}
	panel->on( Event::OnClose, [viewState]( const Event* ) { viewState->lifetime.invalidate(); } );
	viewState->desktopEntryInstalled = association.isDesktopEntryInstalled();
	panel->addCustomWidget(
		std::move( descriptor ), [application = std::move( application ), viewState, threadPool,
								  translate = std::move( translate )]( UIWidget* parent ) {
			static constexpr const char* layoutSource = R"xml(
<vbox id="file_associations_container" lw="mp" lh="wc">
	<CheckBox id="install_desktop_entry" lw="wc" lh="wc" margin-bottom="6dp" />
	<vbox id="file_associations_table_container" lw="mp" lh="320dp" />
	<hbox lw="mp" lh="wc" margin-top="8dp" gravity="right">
		<PushButton id="select_all_associations" lw="wc" lh="wc" margin-right="4dp" />
		<PushButton id="clear_associations" lw="wc" lh="wc" margin-right="4dp" />
		<PushButton id="apply_associations" lw="wc" lh="wc" />
	</hbox>
</vbox>
)xml";
			auto* layout = parent->getUISceneNode()->loadLayoutFromString( layoutSource, parent );
			auto* desktopEntry = layout->find<UICheckBox>( "install_desktop_entry" );
			viewState->layout = layout;
			viewState->desktopEntry = desktopEntry;
			layout->on( Event::OnClose, [viewState, layout]( const Event* ) {
				if ( viewState->layout == layout )
					viewState->clearControls();
			} );
			desktopEntry->setText(
				translate( "install_desktop_entry", "Install application launcher" ) );
			desktopEntry->setChecked( viewState->desktopEntryInstalled );
			desktopEntry->setVisible( FileAssociation::supportsDesktopEntries() );

			if ( viewState->model ) {
				auto* table = UIFileAssociationsTableView::New();
				viewState->table = table;
				table->setParent( layout->find<UIWidget>( "file_associations_table_container" ) );
				table->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
				table->setColumnWidthMode( UIAbstractTableView::ColumnWidthMode::Percentage );
				table->setColumnsWidthPercentage( { 0.2f, 0.8f } );
				table->setModel( viewState->model );
				table->setHeadersVisible( true );
			} else {
				layout->find<UIWidget>( "file_associations_table_container" )->setVisible( false );
			}

			auto* selectAll = layout->find<UIPushButton>( "select_all_associations" );
			viewState->selectAll = selectAll;
			selectAll->setVisible( viewState->model != nullptr );
			selectAll->setText( translate( "select_all", "Select All" ) );
			selectAll->onClick( [viewState]( const MouseEvent* ) {
				if ( viewState->model )
					viewState->model->setAllRegistered( true );
			} );
			auto* clear = layout->find<UIPushButton>( "clear_associations" );
			viewState->clear = clear;
			clear->setVisible( viewState->model != nullptr );
			clear->setText( translate( "clear", "Clear" ) );
			clear->onClick( [viewState]( const MouseEvent* ) {
				if ( viewState->model )
					viewState->model->setAllRegistered( false );
			} );
			auto* apply = layout->find<UIPushButton>( "apply_associations" );
			viewState->apply = apply;
			apply->setText( translate( "apply", "Apply" ) );
			apply->onClick( [application, viewState, threadPool]( const MouseEvent* ) {
				if ( viewState->applying )
					return;
				viewState->applying = true;
				viewState->setControlsEnabled( false );
				const bool hasAssociations = viewState->model != nullptr;
				auto selected = hasAssociations ? viewState->model->registeredExtensions()
												: std::vector<std::string>{};
				auto supported =
					hasAssociations ? viewState->model->extensions() : std::vector<std::string>{};
				const bool installDesktopEntry =
					( viewState->desktopEntry && viewState->desktopEntry->isChecked() ) ||
					( FileAssociation::supportsDesktopEntries() && !selected.empty() );
				auto lifetime = viewState->lifetime.weakHandle();
				threadPool->run( [application, selected = std::move( selected ),
								  supported = std::move( supported ), installDesktopEntry,
								  hasAssociations, lifetime]() mutable {
					FileAssociation fileAssociation( std::move( application ) );
					const bool success =
						hasAssociations
							? fileAssociation.setRegisteredExtensions( selected, supported,
																	   installDesktopEntry )
							: fileAssociation.setDesktopEntryInstalled( installDesktopEntry );
					auto error = fileAssociation.getLastError();
					lifetime.run( [success, installDesktopEntry,
								   error = std::move( error )]( FileAssociationsViewState* state ) {
						state->applying = false;
						state->setControlsEnabled( true );
						if ( success ) {
							state->desktopEntryInstalled = installDesktopEntry;
							if ( state->desktopEntry )
								state->desktopEntry->setChecked( installDesktopEntry );
						}
						if ( state->completion )
							state->completion( success, error );
					} );
				} );
			} );
			viewState->setControlsEnabled( !viewState->applying );
			return layout;
		} );
}

} // namespace EE::UI::Tools
