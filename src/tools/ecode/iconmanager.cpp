#include "iconmanager.hpp"
#include <eepp/ui/uiicontheme.hpp>
#include <eepp/ui/uiiconthememanager.hpp>

namespace ecode {

void IconManager::init( UISceneNode* sceneNode, FontTrueType* iconFont, FontTrueType* mimeIconFont,
						FontTrueType* codIconFont ) {

	UIIconTheme* iconTheme = UIIconTheme::New( "ecode" );
	std::unordered_map<std::string, Uint32> icons = {

		{ "document-new", 0xecc3 },
		{ "document-open", 0xed70 },
		{ "document-save", 0xf0b3 },
		{ "document-save-as", 0xf0b3 },
		{ "document-close", 0xeb99 },
		{ "quit", 0xeb97 },
		{ "undo", 0xea58 },
		{ "redo", 0xea5a },
		{ "cut", 0xf0c1 },
		{ "copy", 0xecd5 },
		{ "paste", 0xeb91 },
		{ "edit", 0xec86 },
		{ "split-horizontal", 0xf17a },
		{ "split-vertical", 0xf17b },
		{ "find-replace", 0xed2b },
		{ "folder-add", 0xed5a },
		{ "file-add", 0xecc9 },
		{ "file-copy", 0xecd3 },
		{ "file-code", 0xecd1 },
		{ "file-edit", 0xecdb },
		{ "font-size", 0xed8d },
		{ "delete-bin", 0xec1e },
		{ "delete-text", 0xec1e },
		{ "zoom-in", 0xf2db },
		{ "zoom-out", 0xf2dd },
		{ "zoom-reset", 0xeb47 },
		{ "fullscreen", 0xed9c },
		{ "keybindings", 0xee75 },
		{ "search", 0xf0d1 },
		{ "go-up", 0xea78 },
		{ "ok", 0xeb7a },
		{ "cancel", 0xeb98 },
		{ "color-picker", 0xf13d },
		{ "pixel-density", 0xed8c },
		{ "go-to-line", 0xf1f8 },
		{ "table-view", 0xf1de },
		{ "list-view", 0xecf1 },
		{ "menu-unfold", 0xef40 },
		{ "menu-fold", 0xef3d },
		{ "download-cloud", 0xec58 },
		{ "layout-left", 0xee94 },
		{ "layout-right", 0xee9b },
		{ "color-scheme", 0xebd4 },
		{ "global-settings", 0xedcf },
		{ "folder-user", 0xed84 },
		{ "help", 0xf045 },
		{ "terminal", 0xf1f6 },
		{ "earth", 0xec7a },
		{ "arrow-down", 0xea4c },
		{ "arrow-up", 0xea76 },
		{ "arrow-down-s", 0xea4e },
		{ "arrow-up-s", 0xea78 },
		{ "arrow-right-s", 0xea6e },
		{ "match-case", 0xed8d },
		{ "palette", 0xefc5 },
		{ "file-code", 0xecd1 },
		{ "cursor-pointer", 0xec09 },
		{ "drive", 0xedf8 },
		{ "refresh", 0xf064 },
		{ "hearth-pulse", 0xee10 },
		{ "add", 0xea12 },
		{ "hammer", 0xedee },
		{ "eraser", 0xec9e },
		{ "file-search", 0xed05 },
		{ "window", 0xf2c4 },
		{ "file-lock-fill", 0xecf2 } };

	for ( const auto& icon : icons )
		iconTheme->add( UIGlyphIcon::New( icon.first, iconFont, icon.second ) );

	if ( mimeIconFont && mimeIconFont->loaded() ) {
		std::unordered_map<std::string, Uint32> mimeIcons =

			{ { "filetype-lua", 61826 },
			  { "filetype-c", 61718 },
			  { "filetype-h", 61792 },
			  { "filetype-cs", 61720 },
			  { "filetype-cpp", 61719 },
			  { "filetype-hpp", 61719 },
			  { "filetype-css", 61743 },
			  { "filetype-conf", 61781 },
			  { "filetype-cfg", 61781 },
			  { "filetype-desktop", 61781 },
			  { "filetype-service", 61781 },
			  { "filetype-env", 61781 },
			  { "filetype-properties", 61781 },
			  { "filetype-ini", 61781 },
			  { "filetype-dart", 61744 },
			  { "filetype-diff", 61752 },
			  { "filetype-zip", 61775 },
			  { "filetype-go", 61789 },
			  { "filetype-htm", 61799 },
			  { "filetype-html", 61799 },
			  { "filetype-java", 61809 },
			  { "filetype-js", 61810 },
			  { "filetype-json", 61811 },
			  { "filetype-kt", 61814 },
			  { "filetype-md", 61829 },
			  { "filetype-perl", 61853 },
			  { "filetype-php", 61855 },
			  { "filetype-py", 61863 },
			  { "filetype-pyc", 61863 },
			  { "filetype-pyd", 61863 },
			  { "filetype-swift", 61906 },
			  { "filetype-rb", 61880 },
			  { "filetype-rs", 61881 },
			  { "filetype-ts", 61923 },
			  { "filetype-jsx", 0xf1ab },
			  { "filetype-tsx", 0xf1ab },
			  { "filetype-yaml", 61945 },
			  { "filetype-yml", 61945 },
			  { "filetype-jpg", 61801 },
			  { "filetype-png", 61801 },
			  { "filetype-jpeg", 61801 },
			  { "filetype-bmp", 61801 },
			  { "filetype-tga", 61801 },
			  { "filetype-sh", 61911 },
			  { "filetype-bash", 61911 },
			  { "filetype-fish", 61911 },
			  { "filetype-scala", 61882 },
			  { "filetype-r", 61866 },
			  { "filetype-rake", 61880 },
			  { "filetype-rss", 61879 },
			  { "filetype-sql", 61746 },
			  { "filetype-elm", 61763 },
			  { "filetype-ex", 61971 },
			  { "filetype-exs", 61971 },
			  { "filetype-awk", 61971 },
			  { "filetype-nim", 61734 },
			  { "filetype-xml", 61769 },
			  { "filetype-dockerfile", 61758 },
			  { "filetype-scala", 61882 },
			  { "filetype-sc", 61882 },
			  { "filetype-perl", 61853 },
			  { "filetype-vue", 0xf1f4 },
			  { "file", 61766 },
			  { "file-symlink", 61774 },
			  { "folder", 0xF23B },
			  { "folder-open", 0xF23C },
			  { "tree-expanded", 0xF11E },
			  { "tree-contracted", 0xF120 },
			  { "github", 0xF184 },
			  { "package", 61846 },
			  { "tab-close", 61944 } };

		for ( const auto& icon : mimeIcons )
			iconTheme->add( UIGlyphIcon::New( icon.first, mimeIconFont, icon.second ) );
	}

	if ( codIconFont && codIconFont->loaded() ) {
		std::unordered_map<std::string, Uint32> codIcons = {

			{ "symbol-text", 0xea93 },
			{ "symbol-method", 0xea8c },
			{ "symbol-function", 0xea8c },
			{ "symbol-constructor", 0xea8c },
			{ "symbol-field", 0xeb5f },
			{ "symbol-variable", 0xea88 },
			{ "symbol-class", 0xeb5b },
			{ "symbol-interface", 0xeb61 },
			{ "symbol-module", 0xea8b },
			{ "symbol-property", 0xeb65 },
			{ "symbol-unit", 0xea96 },
			{ "symbol-value", 0xea95 },
			{ "symbol-enum", 0xea95 },
			{ "symbol-keyword", 0xeb62 },
			{ "symbol-snippet", 0xeb66 },
			{ "symbol-color", 0xeb5c },
			{ "symbol-file", 0xeb60 },
			{ "symbol-reference", 0xea94 },
			{ "symbol-folder", 0xea83 },
			{ "symbol-enum-member", 0xeb5e },
			{ "symbol-constant", 0xeb5d },
			{ "symbol-struct", 0xea91 },
			{ "symbol-event", 0xea86 },
			{ "symbol-operator", 0xeb64 },
			{ "symbol-type-parameter", 0xea92 },
			{ "expand-all", 0xeb95 },
			{ "symbol-namespace", 0xea8b },
			{ "symbol-package", 0xea8b },
			{ "symbol-string", 0xeb8d },
			{ "symbol-number", 0xea90 },
			{ "symbol-boolean", 0xea8f },
			{ "symbol-array", 0xea8a },
			{ "symbol-object", 0xea8b },
			{ "symbol-key", 0xea93 },
			{ "symbol-null", 0xea8f },
			{ "collapse-all", 0xeac5 },
			{ "chevron-right", 0xeab6 },
			{ "lightbulb-autofix", 0xeb13 },
			{ "layout-sidebar-left-off", 0xec02 },
			{ "layout-sidebar-left", 0xebf3 },
			{ "warning", 0xea6c },
			{ "error", 0xea87 },
			{ "search-fuzzy", 0xec0d },
			{ "source-control", 0xea68 },
			{ "repo", 0xea62 },
			{ "repo-pull", 0xeb40 },
			{ "repo-push", 0xeb41 },
			{ "repo-forked", 0xea63 },
			{ "repo-fetch", 0xec1d },
			{ "git-fetch", 0xf101 },
			{ "git-commit", 0xeafc },
			{ "git-stash", 0xec26 },
			{ "git-stash-apply", 0xec27 },
			{ "git-stash-pop", 0xec28 },
			{ "git-merge", 0xeafe },
			{ "diff-single", 0xec22 },
			{ "remove", 0xeb3b },
			{ "tag", 0xea66 },
			{ "globe", 0xeb01 },
			{ "circle-filled", 0xea71 },
			{ "circle", 0xeabc },
			{ "diff-multiple", 0xec23 },
		};

		for ( const auto& icon : codIcons )
			iconTheme->add( UIGlyphIcon::New( icon.first, codIconFont, icon.second ) );
	}

	iconTheme->add( UISVGIcon::New(
		"ecode",
		"<svg width='256' height='256' version='1.1' viewBox='0 0 12.7 12.7' "
		"xmlns='http://www.w3.org/2000/svg'><g transform='matrix(.80588 0 0 .80588 1.3641 "
		"1.2538)' fill='none' stroke='#fff' stroke-linecap='round' stroke-width='.8px'><path "
		"transform='matrix(1 0 0 -1 .19808 11.984)' d='m6.0943 5.9899c0.16457 0.1173 0.0098785 "
		"0.35997-0.11145 0.43612-0.40848 0.25638-0.89022-0.11622-1.039-0.4965-0.32983-0.84299 "
		"0.37932-1.6794 1.1646-1.8967 1.4211-0.39338 2.7295 0.77904 3.0093 2.1158 0.36822 "
		"1.7593-0.79537 3.4576-2.3989 4.1096'/><path transform='matrix(1 0 0 -1 .030284 "
		"12.252)' d='m6.3163 6.3608c-0.14689-0.18552 0.10611-0.40806 0.26737-0.45419 "
		"0.54289-0.15532 0.96335 0.42061 1.0068 0.89674 0.096424 1.0555-0.97349 1.7652-1.9275 "
		"1.7539-1.7263-0.020368-2.8161-1.765-2.6954-3.3595 0.15881-2.0986 2.0205-3.6297 "
		"4.0363-3.8406'/></g></svg>" ) );

	sceneNode->getUIIconThemeManager()->setCurrentTheme( iconTheme );
}

} // namespace ecode
