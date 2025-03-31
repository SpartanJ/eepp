#include <eepp/ui/doc/languages/lisp.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addLisp() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Lisp",
		  { "%.lisp$", "%.cl$", "%.el$" },
		  {
			  { { ";.-\n" }, "comment" },
			  { { "#|", "|#", "\\" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "#x[%da-fA-F]+" }, "number" },
			  { { "#o[0-7]+" }, "number" },
			  { { "#b[01]+" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "%(%s*([%a_][%w_-]*)%s+([%a_][%w_-]*)" }, { "operator", "symbol", "keyword2" } },
			  { { "'([%a_][%w_-]*)" }, { "operator", "keyword2", "symbol" } },
			  { { ":([%a_][%w_-]*)" }, { "operator", "keyword3", "symbol" } },
			  { { "[%<%>%=%*%/%+%-%`%@%%%(%)]" }, "operator" },

		  },
		  {

			  { "defclass", "keyword" },
			  { "defconstant", "keyword" },
			  { "defgeneric", "keyword" },
			  { "define-compiler-macro", "keyword" },
			  { "define-condition", "keyword" },
			  { "define-method-combination", "keyword" },
			  { "define-modify-macro", "keyword" },
			  { "define-setf-expander", "keyword" },
			  { "define-symbol-macro", "keyword" },
			  { "defmacro", "keyword" },
			  { "defmethod", "keyword" },
			  { "defpackage", "keyword" },
			  { "defparameter", "keyword" },
			  { "defsetf", "keyword" },
			  { "defstruct", "keyword" },
			  { "deftype", "keyword" },
			  { "defun", "keyword" },
			  { "defvar", "keyword" },
			  { "abort", "keyword" },
			  { "assert", "keyword" },
			  { "block", "keyword" },
			  { "break", "keyword" },
			  { "case", "keyword" },
			  { "catch", "keyword" },
			  { "ccase", "keyword" },
			  { "cerror", "keyword" },
			  { "cond", "keyword" },
			  { "ctypecase", "keyword" },
			  { "declaim", "keyword" },
			  { "declare", "keyword" },
			  { "do", "keyword" },
			  { "do*", "keyword" },
			  { "do-all-symbols", "keyword" },
			  { "do-external-symbols", "keyword" },
			  { "do-symbols", "keyword" },
			  { "dolist", "keyword" },
			  { "dotimes", "keyword" },
			  { "ecase", "keyword" },
			  { "error", "keyword" },
			  { "etypecase", "keyword" },
			  { "eval-when", "keyword" },
			  { "flet", "keyword" },
			  { "handler-bind", "keyword" },
			  { "handler-case", "keyword" },
			  { "if", "keyword" },
			  { "ignore-errors", "keyword" },
			  { "in-package", "keyword" },
			  { "labels", "keyword" },
			  { "lambda", "keyword" },
			  { "let", "keyword" },
			  { "let*", "keyword" },
			  { "locally", "keyword" },
			  { "loop", "keyword" },
			  { "map", "keyword" },
			  { "macrolet", "keyword" },
			  { "make-instance", "keyword" },
			  { "multiple-value-bind", "keyword" },
			  { "proclaim", "keyword" },
			  { "prog", "keyword" },
			  { "prog*", "keyword" },
			  { "prog1", "keyword" },
			  { "prog2", "keyword" },
			  { "progn", "keyword" },
			  { "progv", "keyword" },
			  { "provide", "keyword" },
			  { "require", "keyword" },
			  { "restart-bind", "keyword" },
			  { "restart-case", "keyword" },
			  { "restart-name", "keyword" },
			  { "return", "keyword" },
			  { "return-from", "keyword" },
			  { "signal", "keyword" },
			  { "symbol-macrolet", "keyword" },
			  { "tagbody", "keyword" },
			  { "the", "keyword" },
			  { "throw", "keyword" },
			  { "typecase", "keyword" },
			  { "unless", "keyword" },
			  { "unwind-protect", "keyword" },
			  { "when", "keyword" },
			  { "with-accessors", "keyword" },
			  { "with-compilation-unit", "keyword" },
			  { "with-condition-restarts", "keyword" },
			  { "with-hash-table-iterator", "keyword" },
			  { "with-input-from-string", "keyword" },
			  { "with-open-file", "keyword" },
			  { "with-open-stream", "keyword" },
			  { "with-output-to-string", "keyword" },
			  { "with-package-iterator", "keyword" },
			  { "with-simple-restart", "keyword" },
			  { "with-slots", "keyword" },
			  { "with-standard-io-syntax", "keyword" },
			  { "t", "keyword" },
			  { "nil", "literal" },

		  },
		  ";",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces )
		.setFoldBraces( { { '(', ')' }, { '{', '}' }, { '[', ']' } } );
}

}}}} // namespace EE::UI::Doc::Language
