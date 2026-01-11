#include <eepp/ui/doc/languages/gn.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addGn() {

	return SyntaxDefinitionManager::instance()
		->add(

			{ "GN",
			  { "%.gn$" },
			  {
				  { { "include", "#expression" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },

			  },
			  {

			  },
			  "",
			  {}

			} )
		.addRepositories( {

			{ "boolean",
			  {
				  { { "\\b(true|false)\\b" }, "literal", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "builtins",
			  {
				  { { "\\b(action|action_foreach|bundle_data|copy|create_bundle|executable|"
					  "generated_file|group|loadable_module|rust_library|rust_proc_macro|shared_"
					  "library|source_set|static_library|target)\\b" },
					"function",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "\\b(assert|config|declare_args|defined|exec_script|filter_exclude|filter_"
					  "include|filter_labels_exclude|filter_labels_include|foreach|forward_"
					  "variables_from|get_label_info|get_path_info|get_target_outputs|getenv|"
					  "import|label_matches|not_needed|pool|print|print_stack_trace|process_file_"
					  "template|read_file|rebase_path|set_default_toolchain|set_defaults|split_"
					  "list|string_join|string_replace|string_split|template|tool|toolchain|write_"
					  "file)\\b" },
					"function",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "\\b(current_cpu|current_os|current_toolchain|default_toolchain|gn_version|"
					  "host_cpu|host_os|invoker|python_path|root_build_dir|root_gen_dir|root_out_"
					  "dir|target_cpu|target_gen_dir|target_name|target_os|target_out_dir)\\b" },
					"literal",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "\\b(aliased_deps|all_dependent_configs|allow_circular_includes_from|arflags|"
					  "args|asmflags|assert_no_deps|bridge_header|bundle_contents_dir|bundle_deps_"
					  "filter|bundle_executable_dir|bundle_resources_dir|bundle_root_dir|cflags|"
					  "cflags_c|cflags_cc|cflags_objc|cflags_objcc|check_includes|code_signing_"
					  "args|code_signing_outputs|code_signing_script|code_signing_sources|complete_"
					  "static_lib|configs|contents|crate_name|crate_root|crate_type|data|data_deps|"
					  "data_keys|defines|depfile|deps|externs|framework_dirs|frameworks|friend|gen_"
					  "deps|include_dirs|inputs|ldflags|lib_dirs|libs|metadata|mnemonic|module_"
					  "name|output_conversion|output_dir|output_extension|output_name|output_"
					  "prefix_override|outputs|partial_info_plist|pool|post_processing_args|post_"
					  "processing_outputs|post_processing_script|post_processing_sources|"
					  "precompiled_header|precompiled_header_type|precompiled_source|product_type|"
					  "public|public_configs|public_deps|rebase|response_file_contents|rustflags|"
					  "script|sources|swiftflags|testonly|transparent|visibility|walk_keys|weak_"
					  "frameworks|write_runtime_deps|xcasset_compiler_flags|xcode_extra_attributes|"
					  "xcode_test_application_name)\\b" },
					"literal",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "keywords",
			  {
				  { { "\\b(if|else)\\b" }, "keyword", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "call",
			  {
				  { { "\\b([A-Za-z_][A-Za-z0-9_]*)\\s*\\(", "\\)" },
					{ "normal", "function" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#expression" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },

			  } },
			{ "number",
			  {
				  { { "\\b-?\\d+\\b" }, "number", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "comment",
			  {
				  { { "#", "$" }, { "comment" }, {}, "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "expression",
			  {
				  { { "include", "#keywords" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#builtins" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#call" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#identifier" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#operators" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#literals" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#comment" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },

			  } },
			{ "identifier",
			  {
				  { { "\\b[A-Za-z_][A-Za-z0-9_]*\\b" },
					"normal",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "literals",
			  {
				  { { "include", "#string" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#number" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#boolean" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },

			  } },
			{ "operators",
			  {
				  { { "\\b(\\+|\\+=|==|!=|-|-=|<|<=|!|=|>|>=|&&|\\|\\|\\.)\\b" },
					"operator",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "string",
			  {
				  { { "\"", "\"" },
					{ "string" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "\\\\[\"$\\\\]" }, "string", "", SyntaxPatternMatchType::RegEx },
						{ { "\\$0x[0-9A-Fa-f][0-9A-Fa-f]" },
						  "string",
						  "",
						  SyntaxPatternMatchType::RegEx },
						{ { "\\$\\{", "\\}" },
						  { "operator" },
						  { "operator" },
						  "",
						  SyntaxPatternMatchType::RegEx,
						  {
							  { { "include", "#expression" },
								{ "normal" },
								{},
								"",
								SyntaxPatternMatchType::LuaPattern },

						  } },
						{ { "(\\$)([A-Za-z_][A-Za-z0-9_]*)" },
						  { "normal", "operator", "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::RegEx },

					} },

			  } },
		} );
}

}}}} // namespace EE::UI::Doc::Language
