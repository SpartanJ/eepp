#include <eepp/ui/doc/languages/armassembly.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addARMAssembly() {

	return SyntaxDefinitionManager::instance()
		->add(

			{ "ARM Assembly",
			  { "%.s$", "%.S$", "%.asm$", "%.sx$" },
			  {
				  { { "^\\s*[#\\.](define|include|(end|el|else)?if|if(def|ndef)?|else)(\\s+("
					  "defined\\(\\w+\\)|\\w+)(\\s+(&&|\\|\\|)\\s+defined\\(\\w+\\)|\\w]+)*)?\\b" },
					"normal",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "^\\s*\\.?\\w+:\\s*(?=$|;)" }, "normal", "", SyntaxPatternMatchType::RegEx },
				  { { "\\b(?i)(wf[ei]t?)(?-i)\\b" }, "keyword", "", SyntaxPatternMatchType::RegEx },
				  { { "^\\s*\\.\\w+\\b" }, "keyword", "", SyntaxPatternMatchType::RegEx },
				  { { "\\b\\=" }, "keyword", "", SyntaxPatternMatchType::RegEx },
				  { { "\\b(?i)(abs|adclb|adclt|adcs|adc|addg|addha|addhn2|addhnb|addhnt|addhn|"
					  "addpl|addp|addqv|addspl|addsvl|adds|addva|addvl|addv|add|adrp|adr|aesd|aese|"
					  "aesimc|aesmc|andqv|ands|andv|and|asrd|asrr|asrv|asr|at|autda|autdb|autdza|"
					  "autdzb|autia1716|autiasp|autiaz|autia|autib1716|autibsp|autibz|autib|autiza|"
					  "autizb|axflag|bcax|bc|bdep|bext|bfadd|bfclamp|bfcvtn2|bfcvtnt|bfcvtn|bfcvt|"
					  "bfc|bfdot|bfi|bfmaxnm|bfmax|bfminnm|bfmin|bfmlalb|bfmlalt|bfmlal|bfmla|"
					  "bfmlslb|bfmlslt|bfmlsl|bfmls|bfmmla|bfmopa|bfmops|bfmul|bfm|bfsub|bfvdot|"
					  "bfxil|bgrp|bics|bic|bif|bit|blraaz|blraa|blrabz|blrab|blr|bl|bmopa|bmops|"
					  "braaz|braa|brabz|brab|brb|brkas|brka|brkbs|brkb|brkns|brkn|brkpas|brkpa|"
					  "brkpbs|brkpb|brk|br|bsl1n|bsl2n|bsl|bti|b|cadd|casab|casah|casalb|casalh|"
					  "casal|casa|casb|cash|caslb|caslh|casl|caspal|caspa|caspl|casp|cas|cbnz|cbz|"
					  "ccmn|ccmp|cdot|cfinv|cfp|chkfeat|cinc|cinv|clasta|clastb|clrbhb|clrex|cls|"
					  "clz|cmeq|cmge|cmgt|cmhi|cmhs|cmla|cmle|cmlt|cmn|cmpal|cmpcc|cmpcs|cmpeq|"
					  "cmpge|cmpgt|cmphi|cmple|cmplo|cmpls|cmplt|cmpmi|cmpne|cmppl|cmpp|cmpvc|"
					  "cmpvs|cmp|cmtst|cneg|cnot|cntb|cntd|cnth|cntp|cntw|cnt|compact|cosp|cpp|"
					  "cpyen|cpyern|cpyertn|cpyertrn|cpyertwn|cpyert|cpyetn|cpyetrn|cpyetwn|cpyet|"
					  "cpyewn|cpyewtn|cpyewtrn|cpyewtwn|cpyewt|cpye|cpyfen|cpyfern|cpyfertn|"
					  "cpyfertrn|cpyfertwn|cpyfert|cpyfetn|cpyfetrn|cpyfetwn|cpyfet|cpyfewn|"
					  "cpyfewtn|cpyfewtrn|cpyfewtwn|cpyfewt|cpyfe|cpyfmn|cpyfmrn|cpyfmrtn|"
					  "cpyfmrtrn|cpyfmrtwn|cpyfmrt|cpyfmtn|cpyfmtrn|cpyfmtwn|cpyfmt|cpyfmwn|"
					  "cpyfmwtn|cpyfmwtrn|cpyfmwtwn|cpyfmwt|cpyfm|cpyfpn|cpyfprn|cpyfprtn|"
					  "cpyfprtrn|cpyfprtwn|cpyfprt|cpyfptn|cpyfptrn|cpyfptwn|cpyfpt|cpyfpwn|"
					  "cpyfpwtn|cpyfpwtrn|cpyfpwtwn|cpyfpwt|cpyfp|cpymn|cpymrn|cpymrtn|cpymrtrn|"
					  "cpymrtwn|cpymrt|cpymtn|cpymtrn|cpymtwn|cpymt|cpymwn|cpymwtn|cpymwtrn|"
					  "cpymwtwn|cpymwt|cpym|cpypn|cpyprn|cpyprtn|cpyprtrn|cpyprtwn|cpyprt|cpyptn|"
					  "cpyptrn|cpyptwn|cpypt|cpypwn|cpypwtn|cpypwtrn|cpypwtwn|cpypwt|cpyp|cpy|"
					  "crc32b|crc32cb|crc32ch|crc32cw|crc32cx|crc32h|crc32w|crc32x|csdb|csel|csetm|"
					  "cset|csinc|csinv|csneg|ctermeq|ctermne|ctz|dcps1|dcps2|dcps3|dc|decb|decd|"
					  "dech|decp|decw|dgh|dmb|dsb|dupm|dupq|dup|dvp|eon|eor3|eorbt|eorqv|eors|"
					  "eortb|eorv|eor|eretaa|eretab|eret|esb|extq|extr|ext|fabd|fabs|facal|faccc|"
					  "faccs|faceq|facge|facgt|fachi|facle|facls|faclt|facmi|facne|facpl|facvc|"
					  "facvs|fadda|faddp|faddqv|faddv|fadd|fcadd|fccmpe|fccmp|fclamp|fcmal|fcmcc|"
					  "fcmcs|fcmeq|fcmge|fcmgt|fcmhi|fcmla|fcmle|fcmls|fcmlt|fcmmi|fcmne|fcmpe|"
					  "fcmpl|fcmp|fcmvc|fcmvs|fcpy|fcsel|fcvtas|fcvtau|fcvtl2|fcvtlt|fcvtl|fcvtms|"
					  "fcvtmu|fcvtn2|fcvtns|fcvtnt|fcvtnu|fcvtn|fcvtps|fcvtpu|fcvtxn2|fcvtxnt|"
					  "fcvtxn|fcvtx|fcvtzs|fcvtzu|fcvt|fdivr|fdiv|fdot|fdup|fexpa|fjcvtzs|flogb|"
					  "fmadd|fmad|fmaxnmp|fmaxnmqv|fmaxnmv|fmaxnm|fmaxp|fmaxqv|fmaxv|fmax|fminnmp|"
					  "fminnmqv|fminnmv|fminnm|fminp|fminqv|fminv|fmin|fmlal2|fmlalb|fmlalt|fmlal|"
					  "fmla|fmlsl2|fmlslb|fmlslt|fmlsl|fmls|fmmla|fmopa|fmops|fmov|fmsb|fmsub|"
					  "fmulx|fmul|fneg|fnmadd|fnmad|fnmla|fnmls|fnmsb|fnmsub|fnmul|frecpe|frecps|"
					  "frecpx|frint32x|frint32z|frint64x|frint64z|frinta|frinti|frintm|frintn|"
					  "frintp|frintx|frintz|frsqrte|frsqrts|fscale|fsqrt|fsubr|fsub|ftmad|ftsmul|"
					  "ftssel|fvdot|gcsbdsync|gcspopcx|gcspopm|gcspopx|gcspushm|gcspushx|gcsss1|"
					  "gcsss2|gcsstr|gcssttr|gmi|hint|histcnt|histseg|hlt|hvc|ic|incb|incd|inch|"
					  "incp|incw|index|insr|ins|irg|isb|lasta|lastb|ld1b|ld1d|ld1h|ld1q|ld1rb|"
					  "ld1rd|ld1rh|ld1rob|ld1rod|ld1roh|ld1row|ld1rqb|ld1rqd|ld1rqh|ld1rqw|ld1rsb|"
					  "ld1rsh|ld1rsw|ld1rw|ld1r|ld1sb|ld1sh|ld1sw|ld1w|ld1|ld2b|ld2d|ld2h|ld2q|"
					  "ld2r|ld2w|ld2|ld3b|ld3d|ld3h|ld3q|ld3r|ld3w|ld3|ld4b|ld4d|ld4h|ld4q|ld4r|"
					  "ld4w|ld4|ld64b|ldaddab|ldaddah|ldaddalb|ldaddalh|ldaddal|ldadda|ldaddb|"
					  "ldaddh|ldaddlb|ldaddlh|ldaddl|ldadd|ldap1|ldaprb|ldaprh|ldapr|ldapurb|"
					  "ldapurh|ldapursb|ldapursh|ldapursw|ldapur|ldarb|ldarh|ldar|ldaxp|ldaxrb|"
					  "ldaxrh|ldaxr|ldclrab|ldclrah|ldclralb|ldclralh|ldclral|ldclra|ldclrb|ldclrh|"
					  "ldclrlb|ldclrlh|ldclrl|ldclrpal|ldclrpa|ldclrpl|ldclrp|ldclr|ldeorab|"
					  "ldeorah|ldeoralb|ldeoralh|ldeoral|ldeora|ldeorb|ldeorh|ldeorlb|ldeorlh|"
					  "ldeorl|ldeor|ldff1b|ldff1d|ldff1h|ldff1sb|ldff1sh|ldff1sw|ldff1w|ldgm|ldg|"
					  "ldiapp|ldlarb|ldlarh|ldlar|ldnf1b|ldnf1d|ldnf1h|ldnf1sb|ldnf1sh|ldnf1sw|"
					  "ldnf1w|ldnp|ldnt1b|ldnt1d|ldnt1h|ldnt1sb|ldnt1sh|ldnt1sw|ldnt1w|ldpsw|ldp|"
					  "ldraa|ldrab|ldrb|ldrh|ldrsb|ldrsh|ldrsw|ldr|ldsetab|ldsetah|ldsetalb|"
					  "ldsetalh|ldsetal|ldseta|ldsetb|ldseth|ldsetlb|ldsetlh|ldsetl|ldsetpal|"
					  "ldsetpa|ldsetpl|ldsetp|ldset|ldsmaxab|ldsmaxah|ldsmaxalb|ldsmaxalh|ldsmaxal|"
					  "ldsmaxa|ldsmaxb|ldsmaxh|ldsmaxlb|ldsmaxlh|ldsmaxl|ldsmax|ldsminab|ldsminah|"
					  "ldsminalb|ldsminalh|ldsminal|ldsmina|ldsminb|ldsminh|ldsminlb|ldsminlh|"
					  "ldsminl|ldsmin|ldtrb|ldtrh|ldtrsb|ldtrsh|ldtrsw|ldtr|ldumaxab|ldumaxah|"
					  "ldumaxalb|ldumaxalh|ldumaxal|ldumaxa|ldumaxb|ldumaxh|ldumaxlb|ldumaxlh|"
					  "ldumaxl|ldumax|lduminab|lduminah|lduminalb|lduminalh|lduminal|ldumina|"
					  "lduminb|lduminh|lduminlb|lduminlh|lduminl|ldumin|ldurb|ldurh|ldursb|ldursh|"
					  "ldursw|ldur|ldxp|ldxrb|ldxrh|ldxr|lslr|lslv|lsl|lsrr|lsrv|lsr|luti2|luti4|"
					  "madd|mad|match|mla|mls|mneg|movaz|mova|movi|movk|movn|movprfx|movs|movt|"
					  "movz|mov|mrrs|mrs|msb|msrr|msr|msub|mul|mvni|mvn|nands|nand|nbsl|negs|neg|"
					  "ngcs|ngc|nmatch|nors|nor|nots|not|orns|orn|orqv|orrs|orr|orv|pacda|pacdb|"
					  "pacdza|pacdzb|pacga|pacia1716|paciasp|paciaz|pacia|pacib1716|pacibsp|pacibz|"
					  "pacib|paciza|pacizb|pext|pfalse|pfirst|pmov|pmull2|pmullb|pmullt|pmull|pmul|"
					  "pnext|prfb|prfd|prfh|prfm|prfum|prfw|psbcsync|psel|pssbb|ptest|ptrues|ptrue|"
					  "punpkhi|punpklo|raddhn2|raddhnb|raddhnt|raddhn|rax1|rbit|rcwcasal|rcwcasa|"
					  "rcwcasl|rcwcaspal|rcwcaspa|rcwcaspl|rcwcasp|rcwcas|rcwclral|rcwclra|rcwclrl|"
					  "rcwclrpal|rcwclrpa|rcwclrpl|rcwclrp|rcwclr|rcwscasal|rcwscasa|rcwscasl|"
					  "rcwscaspal|rcwscaspa|rcwscaspl|rcwscasp|rcwscas|rcwsclral|rcwsclra|rcwsclrl|"
					  "rcwsclrpal|rcwsclrpa|rcwsclrpl|rcwsclrp|rcwsclr|rcwsetal|rcwseta|rcwsetl|"
					  "rcwsetpal|rcwsetpa|rcwsetpl|rcwsetp|rcwset|rcwssetal|rcwsseta|rcwssetl|"
					  "rcwssetpal|rcwssetpa|rcwssetpl|rcwssetp|rcwsset|rcwsswpal|rcwsswpa|rcwsswpl|"
					  "rcwsswppal|rcwsswppa|rcwsswppl|rcwsswpp|rcwsswp|rcwswpal|rcwswpa|rcwswpl|"
					  "rcwswppal|rcwswppa|rcwswppl|rcwswpp|rcwswp|rdffrs|rdffr|rdsvl|rdvl|retaa|"
					  "retab|ret|rev16|rev32|rev64|revb|revd|revh|revw|rev|rmif|rorv|ror|rprfm|"
					  "rshrn2|rshrnb|rshrnt|rshrn|rsubhn2|rsubhnb|rsubhnt|rsubhn|sabal2|sabalb|"
					  "sabalt|sabal|saba|sabdl2|sabdlb|sabdlt|sabdl|sabd|sadalp|saddl2|saddlbt|"
					  "saddlb|saddlp|saddlt|saddlv|saddl|saddv|saddw2|saddwb|saddwt|saddw|sbclb|"
					  "sbclt|sbcs|sbc|sbfiz|sbfm|sbfx|sb|sclamp|scvtf|sdivr|sdiv|sdot|sel|seten|"
					  "setetn|setet|sete|setf16|setf8|setffr|setgen|setgetn|setget|setge|setgmn|"
					  "setgmtn|setgmt|setgm|setgpn|setgptn|setgpt|setgp|setmn|setmtn|setmt|setm|"
					  "setpn|setptn|setpt|setp|sevl|sev|sha1c|sha1h|sha1m|sha1p|sha1su0|sha1su1|"
					  "sha256h2|sha256h|sha256su0|sha256su1|sha512h2|sha512h|sha512su0|sha512su1|"
					  "shadd|shll2|shll|shl|shrn2|shrnb|shrnt|shrn|shsubr|shsub|sli|sm3partw1|"
					  "sm3partw2|sm3ss1|sm3tt1a|sm3tt1b|sm3tt2a|sm3tt2b|sm4ekey|sm4e|smaddl|smaxp|"
					  "smaxqv|smaxv|smax|smc|sminp|sminqv|sminv|smin|smlal2|smlalb|smlall|smlalt|"
					  "smlal|smlsl2|smlslb|smlsll|smlslt|smlsl|smmla|smnegl|smopa|smops|smov|"
					  "smstart|smstop|smsubl|smulh|smull2|smullb|smullt|smull|splice|sqabs|sqadd|"
					  "sqcadd|sqcvtn|sqcvtun|sqcvtu|sqcvt|sqdecb|sqdecd|sqdech|sqdecp|sqdecw|"
					  "sqdmlal2|sqdmlalbt|sqdmlalb|sqdmlalt|sqdmlal|sqdmlsl2|sqdmlslbt|sqdmlslb|"
					  "sqdmlslt|sqdmlsl|sqdmulh|sqdmull2|sqdmullb|sqdmullt|sqdmull|sqincb|sqincd|"
					  "sqinch|sqincp|sqincw|sqneg|sqrdcmlah|sqrdmlah|sqrdmlsh|sqrdmulh|sqrshlr|"
					  "sqrshl|sqrshrn2|sqrshrnb|sqrshrnt|sqrshrn|sqrshrun2|sqrshrunb|sqrshrunt|"
					  "sqrshrun|sqrshru|sqrshr|sqshlr|sqshlu|sqshl|sqshrn2|sqshrnb|sqshrnt|sqshrn|"
					  "sqshrun2|sqshrunb|sqshrunt|sqshrun|sqsubr|sqsub|sqxtn2|sqxtnb|sqxtnt|sqxtn|"
					  "sqxtun2|sqxtunb|sqxtunt|sqxtun|srhadd|sri|srshlr|srshl|srshr|srsra|ssbb|"
					  "sshll2|sshllb|sshllt|sshll|sshl|sshr|ssra|ssubl2|ssublbt|ssublb|ssubltb|"
					  "ssublt|ssubl|ssubw2|ssubwb|ssubwt|ssubw|st1b|st1d|st1h|st1q|st1w|st1|st2b|"
					  "st2d|st2g|st2h|st2q|st2w|st2|st3b|st3d|st3h|st3q|st3w|st3|st4b|st4d|st4h|"
					  "st4q|st4w|st4|st64bv0|st64bv|st64b|staddb|staddh|staddlb|staddlh|staddl|"
					  "stadd|stclrb|stclrh|stclrlb|stclrlh|stclrl|stclr|steorb|steorh|steorlb|"
					  "steorlh|steorl|steor|stgm|stgp|stg|stilp|stl1|stllrb|stllrh|stllr|stlrb|"
					  "stlrh|stlr|stlurb|stlurh|stlur|stlxp|stlxrb|stlxrh|stlxr|stnp|stnt1b|stnt1d|"
					  "stnt1h|stnt1w|stp|strb|strh|str|stsetb|stseth|stsetlb|stsetlh|stsetl|stset|"
					  "stsmaxb|stsmaxh|stsmaxlb|stsmaxlh|stsmaxl|stsmax|stsminb|stsminh|stsminlb|"
					  "stsminlh|stsminl|stsmin|sttrb|sttrh|sttr|stumaxb|stumaxh|stumaxlb|stumaxlh|"
					  "stumaxl|stumax|stuminb|stuminh|stuminlb|stuminlh|stuminl|stumin|sturb|sturh|"
					  "stur|stxp|stxrb|stxrh|stxr|stz2g|stzgm|stzg|subg|subhn2|subhnb|subhnt|subhn|"
					  "subps|subp|subr|subs|sub|sudot|sumlall|sumopa|sumops|sunpkhi|sunpklo|sunpk|"
					  "suqadd|suvdot|svc|svdot|swpab|swpah|swpalb|swpalh|swpal|swpa|swpb|swph|"
					  "swplb|swplh|swpl|swppal|swppa|swppl|swpp|swp|sxtb|sxth|sxtl2|sxtl|sxtw|sysl|"
					  "sysp|sys|tblq|tbl|tbnz|tbxq|tbx|tbz|tcancel|tcommit|tlbip|tlbi|trcit|trn1|"
					  "trn2|tsbcsync|tstart|tst|ttest|uabal2|uabalb|uabalt|uabal|uaba|uabdl2|"
					  "uabdlb|uabdlt|uabdl|uabd|uadalp|uaddl2|uaddlb|uaddlp|uaddlt|uaddlv|uaddl|"
					  "uaddv|uaddw2|uaddwb|uaddwt|uaddw|ubfiz|ubfm|ubfx|uclamp|ucvtf|udf|udivr|"
					  "udiv|udot|uhadd|uhsubr|uhsub|umaddl|umaxp|umaxqv|umaxv|umax|uminp|uminqv|"
					  "uminv|umin|umlal2|umlalb|umlall|umlalt|umlal|umlsl2|umlslb|umlsll|umlslt|"
					  "umlsl|ummla|umnegl|umopa|umops|umov|umsubl|umulh|umull2|umullb|umullt|umull|"
					  "uqadd|uqcvtn|uqcvt|uqdecb|uqdecd|uqdech|uqdecp|uqdecw|uqincb|uqincd|uqinch|"
					  "uqincp|uqincw|uqrshlr|uqrshl|uqrshrn2|uqrshrnb|uqrshrnt|uqrshrn|uqrshr|"
					  "uqshlr|uqshl|uqshrn2|uqshrnb|uqshrnt|uqshrn|uqsubr|uqsub|uqxtn2|uqxtnb|"
					  "uqxtnt|uqxtn|urecpe|urhadd|urshlr|urshl|urshr|ursqrte|ursra|usdot|ushll2|"
					  "ushllb|ushllt|ushll|ushl|ushr|usmlall|usmmla|usmopa|usmops|usqadd|usra|"
					  "usubl2|usublb|usublt|usubl|usubw2|usubwb|usubwt|usubw|usvdot|uunpkhi|"
					  "uunpklo|uunpk|uvdot|uxtb|uxth|uxtl2|uxtl|uxtw|uzp1|uzp2|uzpq1|uzpq2|uzp|"
					  "whilege|whilegt|whilehi|whilehs|whilele|whilelo|whilels|whilelt|whilerw|"
					  "whilewr|wrffr|xaflag|xar|xpacd|xpaci|xpaclri|xtn2|xtn|yield|zero|zip1|zip2|"
					  "zipq1|zipq2|zip)(?-i)\\b" },
					"function",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "\\b(?i)nop(\\w+)?(?-i)\\b" }, "comment", "", SyntaxPatternMatchType::RegEx },
				  { { "\\[[ \\t]*(\\w+),[ \\t]*([a-zA-Z0-9#-_]+)[ \\t]*\\]!?" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "\\{\\h*", "\\h*\\}\\^?" },
					{ "keyword" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#registers_list" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "include", "#registers" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#variables" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#numerics" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#strings" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#comments" }, "normal", "", SyntaxPatternMatchType::LuaPattern },

			  },
			  {

			  },
			  "",
			  {}

			} )
		.addRepositories( {

			{ "variables",
			  {
				  { { "(\\b|#)-?[a-zA-Z_][0-9a-zA-Z_]*\\b" },
					"normal",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "strings",
			  {
				  { { "\"", "\"" }, { "string" }, {}, "", SyntaxPatternMatchType::RegEx },
				  { { "\\'\\S\\'" }, "string", "", SyntaxPatternMatchType::RegEx },
				  { { "\"[^\"]+$" }, "normal", "", SyntaxPatternMatchType::RegEx },
				  { { "\\'\\S{2,}\\'" }, "normal", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "registers_list",
			  {
				  { { "(\\w+)(?:\\h*\\-\\h*(\\w+))?(?:,\\h*([a-zA-Z0-9,\\-\\h]+))?" },
					{ "normal", "normal", "normal", "normal" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "registers",
			  {
				  { { "\\b(?i)([rcp]([0-9]|1[0-5])|[xwbhsdq]([0-9]|1[0-9]|2[0-9]|3[0-1])|wzr|xzr|"
					  "wsp|fpsr|fpcr|a[1-4]|v([0-9]|1[0-9]|2[0-9]|3[0-1])\\.(16b|8[b|h]|4[s|h]|2[s|"
					  "d])|sl|sb|fp|ip|sp|lr|(c|s)psr(_c)?|pc|fpsid|fpscr|fpexc|APSR_nzcv|sy)(?-i)("
					  "!|\\b)" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "numerics",
			  {
				  { { "#?-?(0x|&)[0-9a-fA-F_]+\\b" }, "number", "", SyntaxPatternMatchType::RegEx },
				  { { "#?[0-9]+\\b" }, "number", "", SyntaxPatternMatchType::RegEx },
				  { { "#?0b[01]+\\b" }, "number", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "conditions",
			  {
				  { { "ne|eq|cs|hs|cc|lo|mi|pl|vs|vc|hi|ls|lt|le|gt|ge|al" },
					"operator",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "comments",
			  {
				  { { "([;@]|//|#).*$" }, "comment", "", SyntaxPatternMatchType::RegEx },
				  { { "\\/\\*", "\\*\\/" },
					{ "comment" },
					{ "comment" },
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
		} );
}

}}}} // namespace EE::UI::Doc::Language
