#include <eepp/ui/doc/languages/solidity.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addSolidity() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Solidity",
		  { "%.sol$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"\"\"", "\"\"\"" }, "string" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "interface", "keyword" }, { "bytes3", "type" },
			  { "blockhash", "type" },	  { "uint64", "type" },
			  { "ecrecover", "type" },	  { "new", "keyword" },
			  { "ripemd160", "type" },	  { "sha3", "type" },
			  { "byte", "type" },		  { "bytes32", "type" },
			  { "suicide", "type" },	  { "virtual", "type" },
			  { "revert", "type" },		  { "bytes30", "type" },
			  { "msg", "type" },		  { "bytes15", "type" },
			  { "enum", "keyword" },	  { "payable", "type" },
			  { "assembly", "keyword" },  { "super", "type" },
			  { "library", "keyword" },	  { "private", "type" },
			  { "is", "keyword" },		  { "now", "type" },
			  { "throw", "keyword" },	  { "bytes18", "type" },
			  { "contract", "keyword" },  { "view", "type" },
			  { "bytes14", "type" },	  { "let", "keyword" },
			  { "external", "type" },	  { "return", "keyword" },
			  { "mulmod", "type" },		  { "modifier", "keyword" },
			  { "bytes12", "type" },	  { "uint256", "type" },
			  { "assert", "type" },		  { "tx", "type" },
			  { "require", "keyword" },	  { "bytes8", "type" },
			  { "uint32", "type" },		  { "returns", "keyword" },
			  { "do", "keyword" },		  { "sha256", "type" },
			  { "while", "keyword" },	  { "bytes23", "type" },
			  { "function", "keyword" },  { "memory", "type" },
			  { "event", "keyword" },	  { "else", "keyword" },
			  { "break", "keyword" },	  { "for", "keyword" },
			  { "address", "type" },	  { "bytes4", "type" },
			  { "abstract", "keyword" },  { "gasleft", "type" },
			  { "keccak256", "type" },	  { "bytes25", "type" },
			  { "bytes28", "type" },	  { "this", "type" },
			  { "bytes10", "type" },	  { "protected", "keyword" },
			  { "uint8", "type" },		  { "delete", "keyword" },
			  { "indexed", "type" },	  { "bytes11", "type" },
			  { "bytes17", "type" },	  { "bytes21", "type" },
			  { "bytes5", "type" },		  { "bytes26", "type" },
			  { "bytes16", "type" },	  { "string", "type" },
			  { "bytes31", "type" },	  { "emit", "keyword" },
			  { "int32", "type" },		  { "bytes19", "type" },
			  { "bytes27", "type" },	  { "int16", "type" },
			  { "int8", "type" },		  { "bytes13", "type" },
			  { "bytes22", "type" },	  { "hash", "type" },
			  { "bytes7", "type" },		  { "pragma", "type" },
			  { "int128", "type" },		  { "bytes29", "type" },
			  { "int256", "type" },		  { "bytes", "type" },
			  { "int64", "type" },		  { "uint16", "type" },
			  { "int", "type" },		  { "uint", "type" },
			  { "bytes24", "type" },	  { "struct", "keyword" },
			  { "uint128", "type" },	  { "bytes2", "type" },
			  { "bytes6", "type" },		  { "bytes1", "type" },
			  { "override", "type" },	  { "constructor", "keyword" },
			  { "using", "keyword" },	  { "nonpayable", "type" },
			  { "mapping", "type" },	  { "inherited", "type" },
			  { "bytes9", "type" },		  { "pure", "type" },
			  { "selfdestruct", "type" }, { "immutable", "type" },
			  { "constant", "type" },	  { "bytes20", "type" },
			  { "addmod", "type" },		  { "internal", "type" },
			  { "bool", "type" },		  { "public", "type" },
			  { "block", "type" },		  { "storage", "type" },
			  { "calldata", "type" },	  { "if", "keyword" },
			  { "continue", "keyword" },

		  },
		  "",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
	sd.setBlockComment( { "/*", "*/" } );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
