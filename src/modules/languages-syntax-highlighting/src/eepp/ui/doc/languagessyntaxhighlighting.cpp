#include <eepp/ui/doc/languages/ada.hpp>
#include <eepp/ui/doc/languages/adept.hpp>
#include <eepp/ui/doc/languages/angelscript.hpp>
#include <eepp/ui/doc/languages/awkscript.hpp>
#include <eepp/ui/doc/languages/batchscript.hpp>
#include <eepp/ui/doc/languages/bazel.hpp>
#include <eepp/ui/doc/languages/bend.hpp>
#include <eepp/ui/doc/languages/blueprint.hpp>
#include <eepp/ui/doc/languages/brainfuck.hpp>
#include <eepp/ui/doc/languages/buzz.hpp>
#include <eepp/ui/doc/languages/carbon.hpp>
#include <eepp/ui/doc/languages/clojure.hpp>
#include <eepp/ui/doc/languages/cmake.hpp>
#include <eepp/ui/doc/languages/containerfile.hpp>
#include <eepp/ui/doc/languages/crystal.hpp>
#include <eepp/ui/doc/languages/csharp.hpp>
#include <eepp/ui/doc/languages/d.hpp>
#include <eepp/ui/doc/languages/dart.hpp>
#include <eepp/ui/doc/languages/difffile.hpp>
#include <eepp/ui/doc/languages/elixir.hpp>
#include <eepp/ui/doc/languages/elm.hpp>
#include <eepp/ui/doc/languages/environmentfile.hpp>
#include <eepp/ui/doc/languages/fantom.hpp>
#include <eepp/ui/doc/languages/fennel.hpp>
#include <eepp/ui/doc/languages/fortran.hpp>
#include <eepp/ui/doc/languages/fstab.hpp>
#include <eepp/ui/doc/languages/gdscript.hpp>
#include <eepp/ui/doc/languages/glsl.hpp>
#include <eepp/ui/doc/languages/go.hpp>
#include <eepp/ui/doc/languages/graphql.hpp>
#include <eepp/ui/doc/languages/groovy.hpp>
#include <eepp/ui/doc/languages/hare.hpp>
#include <eepp/ui/doc/languages/haskell.hpp>
#include <eepp/ui/doc/languages/haxe.hpp>
#include <eepp/ui/doc/languages/hlsl.hpp>
#include <eepp/ui/doc/languages/htaccess.hpp>
#include <eepp/ui/doc/languages/ignorefile.hpp>
#include <eepp/ui/doc/languages/jai.hpp>
#include <eepp/ui/doc/languages/java.hpp>
#include <eepp/ui/doc/languages/jsx.hpp>
#include <eepp/ui/doc/languages/julia.hpp>
#include <eepp/ui/doc/languages/kotlin.hpp>
#include <eepp/ui/doc/languages/latex.hpp>
#include <eepp/ui/doc/languages/lobster.hpp>
#include <eepp/ui/doc/languages/makefile.hpp>
#include <eepp/ui/doc/languages/meson.hpp>
#include <eepp/ui/doc/languages/moonscript.hpp>
#include <eepp/ui/doc/languages/nelua.hpp>
#include <eepp/ui/doc/languages/nim.hpp>
#include <eepp/ui/doc/languages/objeck.hpp>
#include <eepp/ui/doc/languages/objective-c.hpp>
#include <eepp/ui/doc/languages/ocaml.hpp>
#include <eepp/ui/doc/languages/odin.hpp>
#include <eepp/ui/doc/languages/openscad.hpp>
#include <eepp/ui/doc/languages/pascal.hpp>
#include <eepp/ui/doc/languages/perl.hpp>
#include <eepp/ui/doc/languages/php.hpp>
#include <eepp/ui/doc/languages/pico-8.hpp>
#include <eepp/ui/doc/languages/po.hpp>
#include <eepp/ui/doc/languages/pony.hpp>
#include <eepp/ui/doc/languages/postgresql.hpp>
#include <eepp/ui/doc/languages/powershell.hpp>
#include <eepp/ui/doc/languages/r.hpp>
#include <eepp/ui/doc/languages/ring.hpp>
#include <eepp/ui/doc/languages/ruby.hpp>
#include <eepp/ui/doc/languages/rust.hpp>
#include <eepp/ui/doc/languages/sass.hpp>
#include <eepp/ui/doc/languages/scala.hpp>
#include <eepp/ui/doc/languages/shellscript.hpp>
#include <eepp/ui/doc/languages/smallbasic.hpp>
#include <eepp/ui/doc/languages/solidity.hpp>
#include <eepp/ui/doc/languages/sql.hpp>
#include <eepp/ui/doc/languages/svelte.hpp>
#include <eepp/ui/doc/languages/swift.hpp>
#include <eepp/ui/doc/languages/tcl.hpp>
#include <eepp/ui/doc/languages/teal.hpp>
#include <eepp/ui/doc/languages/toml.hpp>
#include <eepp/ui/doc/languages/typescript.hpp>
#include <eepp/ui/doc/languages/v.hpp>
#include <eepp/ui/doc/languages/vala.hpp>
#include <eepp/ui/doc/languages/vb.hpp>
#include <eepp/ui/doc/languages/verilog.hpp>
#include <eepp/ui/doc/languages/vue.hpp>
#include <eepp/ui/doc/languages/wren.hpp>
#include <eepp/ui/doc/languages/x86assembly.hpp>
#include <eepp/ui/doc/languages/xit.hpp>
#include <eepp/ui/doc/languages/xtend.hpp>
#include <eepp/ui/doc/languages/yaml.hpp>
#include <eepp/ui/doc/languages/yuescript.hpp>
#include <eepp/ui/doc/languages/zig.hpp>
#include <eepp/ui/doc/languagessyntaxhighlighting.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void LanguagesSyntaxHighlighting::load() {
	addAda();
	addAdept();
	addAngelScript();
	addAwkScript();
	addBatchScript();
	addBazel();
	addBend();
	addBlueprint();
	addBrainfuck();
	addBuzz();
	addCarbon();
	addContainerFile();
	addClojure();
	addCMake();
	addCrystal();
	addCSharp();
	addOpenSCAD();
	addRing();
	addTcl();
	addD();
	addDart();
	addDiff();
	addElixir();
	addElm();
	addEnvironmentFile();
	addFantom();
	addFennel();
	addFortran();
	addFstab();
	addGDScript();
	addGLSL();
	addGo();
	addGraphQL();
	addGroovy();
	addHaskell();
	addHare();
	addHaxe();
	addHLSL();
	addHtaccessFile();
	addIgnoreFile();
	addJai();
	addJava();
	addJulia();
	addJSX();
	addKotlin();
	addLatex();
	addLobster();
	addMakefile();
	addMeson();
	addMoonscript();
	addNelua();
	addNim();
	addObjeck();
	addObjectiveC();
	addOCaml();
	addOdin();
	addPascal();
	addPerl();
	addPICO8();
	addPHP();
	addPO();
	addPony();
	addPostgreSQL();
	addPowerShell();
	addR();
	addRuby();
	addRust();
	addSass();
	addScala();
	addShellScript();
	addSmallBASIC();
	addSolidity();
	addSQL();
	addSvelte();
	addSwift();
	addTeal();
	addToml();
	addTypeScript();
	addV();
	addVala();
	addVerilog();
	addVisualBasic();
	addVue();
	addWren();
	addX86Assembly();
	addXit();
	addXtend();
	addYAML();
	addYueScript();
	addZig();
}

}}}} // namespace EE::UI::Doc::Language
