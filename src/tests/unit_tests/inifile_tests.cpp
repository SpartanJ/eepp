#include "utest.hpp"

#include <eepp/system/inifile.hpp>
#include <eepp/system/iostreamstring.hpp>

using namespace EE::System;

UTEST( IniFile, preservesInsertionOrderAndSerializes ) {
	IniFile ini( "", false );
	ini.addHeaderComment( "header" );
	ini.setValue( std::string_view{ "project" }, std::string_view{ "path" },
				  std::string_view{ "/tmp/project" } );
	ini.setValue( std::string_view{ "project" }, std::string_view{ "session" },
				  std::string_view{ "open" } );
	ini.addKeyComment( std::string_view{ "project" }, std::string_view{ "state" } );
	ini.setValueI( "editor", "tab_width", 4 );

	IOStreamString stream;
	EXPECT_TRUE( ini.writeStream( stream ) );
	EXPECT_STDSTREQ( ";header\n\n[project]\n;state\npath=/tmp/project\nsession=open\n\n"
					 "[editor]\ntab_width=4\n\n",
					 stream.getStream() );

	EXPECT_EQ( 2u, ini.getNumKeys() );
	EXPECT_STDSTREQ( "project", ini.getKeyName( 0 ) );
	EXPECT_STDSTREQ( "session", ini.getValueName( 0, 1 ) );
	EXPECT_STDSTREQ( "open", ini.getValue( 0, 1 ) );
}

UTEST( IniFile, parsesUpdatesAndDeletesValues ) {
	const std::string source =
		"[project]\npath=/tmp/project\nsession=open\n\n[editor]\ntab_width=4\n";
	IniFile ini( reinterpret_cast<const EE::Uint8*>( source.data() ),
				 static_cast<EE::Uint32>( source.size() ) );

	EXPECT_TRUE( ini.iniParsed() );
	EXPECT_TRUE( ini.setValue( std::string_view{ "project" }, std::string_view{ "session" },
							   std::string_view{ "closed" } ) );
	EXPECT_TRUE( ini.deleteValue( "project", "path" ) );
	EXPECT_EQ( 1u, ini.getNumValues( "project" ) );
	EXPECT_STDSTREQ( "closed", ini.getValue( "project", "session" ) );
	EXPECT_TRUE( ini.deleteKey( "editor" ) );
	EXPECT_EQ( 1u, ini.getNumKeys() );
}

UTEST( IniFile, ecodeProjectStateRoundTripsByteForByte ) {
	IniFile project( "/tmp/project.cfg", false );
	auto set = [&project]( std::string_view key, std::string_view name, std::string_view data ) {
		return project.setValue( key, name, data );
	};

	EXPECT_TRUE( set( "path", "folder_path", "/home/user/projects/eepp/" ) );
	project.setValueB( "project_tree", "show_hidden_files", true );
	project.setValueB( "document", "use_global_settings", false );
	EXPECT_TRUE( set( "document", "h_ext_language_type", "cpp" ) );
	project.setValueB( "document", "trim_trailing_whitespaces", true );
	project.setValueB( "document", "force_new_line_at_end_of_file", true );
	project.setValueB( "document", "auto_detect_indent_type", false );
	EXPECT_TRUE( set( "document", "auto_indent", "keep-indentation" ) );
	project.setValueB( "document", "write_bom", false );
	project.setValueI( "document", "indent_width", 4 );
	project.setValueB( "document", "indent_spaces", false );
	EXPECT_TRUE( set( "document", "line_endings", "LF" ) );
	project.setValueI( "document", "tab_width", 4 );
	project.setValueI( "document", "line_breaking_column", 100 );
	EXPECT_TRUE( set( "build", "build_name", "eepp-linux-ninja" ) );
	EXPECT_TRUE( set( "build", "build_type", "release" ) );
	EXPECT_TRUE( set( "build", "run_name", "ecode" ) );
	EXPECT_TRUE( set(
		"nodes", "documents",
		R"({"type":"splitter","first":{"type":"tabwidget","files":[{"type":"editor","path":"/home/user/projects/eepp/src/tools/ecode/appconfig.cpp"}]},"last":{"type":"tabwidget","files":[]}})" ) );
	EXPECT_TRUE( set( "languages_extensions", ".h", "cpp" ) );
	EXPECT_TRUE( set( "languages_extensions", ".inl", "cpp" ) );

	const std::string expected =
		"[path]\nfolder_path=/home/user/projects/eepp/\n\n"
		"[project_tree]\nshow_hidden_files=1\n\n"
		"[document]\nuse_global_settings=0\nh_ext_language_type=cpp\n"
		"trim_trailing_whitespaces=1\nforce_new_line_at_end_of_file=1\n"
		"auto_detect_indent_type=0\nauto_indent=keep-indentation\nwrite_bom=0\n"
		"indent_width=4\nindent_spaces=0\nline_endings=LF\ntab_width=4\n"
		"line_breaking_column=100\n\n"
		"[build]\nbuild_name=eepp-linux-ninja\nbuild_type=release\nrun_name=ecode\n\n"
		"[nodes]\ndocuments={\"type\":\"splitter\",\"first\":{\"type\":\"tabwidget\","
		"\"files\":[{\"type\":\"editor\",\"path\":\"/home/user/projects/eepp/src/tools/"
		"ecode/appconfig.cpp\"}]},\"last\":{\"type\":\"tabwidget\",\"files\":[]}}\n\n"
		"[languages_extensions]\n.h=cpp\n.inl=cpp\n\n";

	IOStreamString firstWrite;
	EXPECT_TRUE( project.writeStream( firstWrite ) );
	EXPECT_STDSTREQ( expected, firstWrite.getStream() );

	IniFile reloaded( reinterpret_cast<const EE::Uint8*>( expected.data() ),
					  static_cast<EE::Uint32>( expected.size() ) );
	EXPECT_TRUE( reloaded.iniParsed() );
	EXPECT_STDSTREQ( "/home/user/projects/eepp/", reloaded.getValue( "path", "folder_path" ) );
	EXPECT_EQ( 100, reloaded.getValueI( "document", "line_breaking_column" ) );
	EXPECT_TRUE( reloaded.getValueB( "project_tree", "show_hidden_files" ) );
	EXPECT_FALSE( reloaded.setValue( std::string_view{ "missing" }, std::string_view{ "value" },
									 std::string_view{ "data" }, false ) );
	EXPECT_TRUE( reloaded.setValue( std::string_view{ "build" }, std::string_view{ "run_name" },
									std::string_view{ "ecode" }, false ) );

	IOStreamString secondWrite;
	EXPECT_TRUE( reloaded.writeStream( secondWrite ) );
	EXPECT_STDSTREQ( expected, secondWrite.getStream() );
}
