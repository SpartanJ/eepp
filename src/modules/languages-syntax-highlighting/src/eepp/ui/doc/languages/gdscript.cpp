#include <eepp/ui/doc/languages/gdscript.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addGDScript() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "GDScript",
		  { "%.gd$" },
		  {
			  { { "#.-\n" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "-?0x%x*" }, "number" },
			  { { "-?%d+[%d%.e]*" }, "number" },
			  { { "-?%.?%d+" }, "number" },
			  { { "[%+%:%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "Array", "type" },
			  { "RID", "type" },
			  { "NodePath", "type" },
			  { "Basis", "type" },
			  { "Transform", "type" },
			  { "Quat", "type" },
			  { "void", "type" },
			  { "Transform2D", "type" },
			  { "AABB", "type" },
			  { "true", "literal" },
			  { "Vector3", "type" },
			  { "int", "type" },
			  { "mastersync", "keyword" },
			  { "is", "keyword" },
			  { "signal", "keyword" },
			  { "class", "keyword" },
			  { "Rect2", "type" },
			  { "pass", "keyword" },
			  { "elif", "keyword" },
			  { "PoolStringArray", "type" },
			  { "as", "keyword" },
			  { "TAU", "literal" },
			  { "INF", "literal" },
			  { "and", "keyword" },
			  { "PoolVector2Array", "type" },
			  { "for", "keyword" },
			  { "Color", "type" },
			  { "PoolIntArray", "type" },
			  { "tool", "keyword" },
			  { "bool", "type" },
			  { "continue", "keyword" },
			  { "Object", "type" },
			  { "NAN", "literal" },
			  { "Dictionary", "type" },
			  { "self", "keyword" },
			  { "const", "keyword" },
			  { "or", "keyword" },
			  { "false", "literal" },
			  { "PoolByteArray", "type" },
			  { "PI", "literal" },
			  { "var", "keyword" },
			  { "while", "keyword" },
			  { "PoolVector3Array", "type" },
			  { "func", "keyword" },
			  { "if", "keyword" },
			  { "setget", "keyword" },
			  { "String", "type" },
			  { "else", "keyword" },
			  { "in", "keyword" },
			  { "onready", "keyword" },
			  { "break", "keyword" },
			  { "not", "keyword" },
			  { "match", "keyword" },
			  { "static", "keyword" },
			  { "extends", "keyword" },
			  { "PoolColorArray", "type" },
			  { "float", "type" },
			  { "Plane", "type" },
			  { "puppet", "keyword" },
			  { "remotesync", "keyword" },
			  { "return", "keyword" },
			  { "class_name", "keyword" },
			  { "enum", "keyword" },
			  { "breakpoint", "keyword" },
			  { "null", "literal" },
			  { "Vector2", "type" },
			  { "preload", "keyword" },
			  { "export", "keyword" },
			  { "yield", "keyword" },
			  { "puppetsync", "keyword" },
			  { "assert", "keyword" },
			  { "remote", "keyword" },
			  { "master", "keyword" },
			  { "PoolRealArray", "type" },

		  },
		  "#",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Indentation );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
