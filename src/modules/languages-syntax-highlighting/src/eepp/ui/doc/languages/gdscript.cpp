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
			  { "Array", "keyword2" },
			  { "RID", "keyword2" },
			  { "NodePath", "keyword2" },
			  { "Basis", "keyword2" },
			  { "Transform", "keyword2" },
			  { "Quat", "keyword2" },
			  { "void", "keyword2" },
			  { "Transform2D", "keyword2" },
			  { "AABB", "keyword2" },
			  { "true", "literal" },
			  { "Vector3", "keyword2" },
			  { "int", "keyword2" },
			  { "mastersync", "keyword" },
			  { "is", "keyword" },
			  { "signal", "keyword" },
			  { "class", "keyword" },
			  { "Rect2", "keyword2" },
			  { "pass", "keyword" },
			  { "elif", "keyword" },
			  { "PoolStringArray", "keyword2" },
			  { "as", "keyword" },
			  { "TAU", "literal" },
			  { "INF", "literal" },
			  { "and", "keyword" },
			  { "PoolVector2Array", "keyword2" },
			  { "for", "keyword" },
			  { "Color", "keyword2" },
			  { "PoolIntArray", "keyword2" },
			  { "tool", "keyword" },
			  { "bool", "keyword2" },
			  { "continue", "keyword" },
			  { "Object", "keyword2" },
			  { "NAN", "literal" },
			  { "Dictionary", "keyword2" },
			  { "self", "keyword" },
			  { "const", "keyword" },
			  { "or", "keyword" },
			  { "false", "literal" },
			  { "PoolByteArray", "keyword2" },
			  { "PI", "literal" },
			  { "var", "keyword" },
			  { "while", "keyword" },
			  { "PoolVector3Array", "keyword2" },
			  { "func", "keyword" },
			  { "if", "keyword" },
			  { "setget", "keyword" },
			  { "String", "keyword2" },
			  { "else", "keyword" },
			  { "in", "keyword" },
			  { "onready", "keyword" },
			  { "break", "keyword" },
			  { "not", "keyword" },
			  { "match", "keyword" },
			  { "static", "keyword" },
			  { "extends", "keyword" },
			  { "PoolColorArray", "keyword2" },
			  { "float", "keyword2" },
			  { "Plane", "keyword2" },
			  { "puppet", "keyword" },
			  { "remotesync", "keyword" },
			  { "return", "keyword" },
			  { "class_name", "keyword" },
			  { "enum", "keyword" },
			  { "breakpoint", "keyword" },
			  { "null", "literal" },
			  { "Vector2", "keyword2" },
			  { "preload", "keyword" },
			  { "export", "keyword" },
			  { "yield", "keyword" },
			  { "puppetsync", "keyword" },
			  { "assert", "keyword" },
			  { "remote", "keyword" },
			  { "master", "keyword" },
			  { "PoolRealArray", "keyword2" },

		  },
		  "#",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Indentation );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
