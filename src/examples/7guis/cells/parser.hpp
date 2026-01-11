#ifndef PARSER_HPP
#define PARSER_HPP

#include "formula.hpp"

#include <eepp/system/luapattern.hpp>

using namespace EE::System;

enum class TokenType {
	EPSILON = 0,
	EQUALS = 1,
	IDENT = 2,
	DECIMAL = 3,
	OPEN_BRACKET = 4,
	CLOSE_BRACKET = 5,
	COMMA = 6,
	COLON = 7,
	CELL = 8,
	PLUS = 9,
    MINUS = 10,
    STAR = 11,
    SLASH = 12,
    PERCENT = 13,
};

struct Token {
	TokenType token;
	std::string sequence;
};

struct TokenInfo {
	std::string pattern;
	TokenType token;
	TokenInfo( std::string regex, TokenType token ) :
		pattern( std::move( regex ) ), token( token ) {}
};

struct Tokenizer {
	std::vector<TokenInfo> tokenInfos;
	std::deque<Token> tokens;

	void add( std::string regex, TokenType token );

	bool tokenize( const std::string& s );
};

class FormulaParser {
  public:
	std::shared_ptr<Formula> parseFormula( std::string _formulaString );

  protected:
	static Tokenizer tokenizer;

	static void initTokenizer();

	std::string formulaString;
	std::deque<Token> tokens;
	Token lookahead;

	void nextToken();

	std::shared_ptr<Formula> application();

	std::shared_ptr<Formula> expression();

	std::shared_ptr<Formula> formula();

	std::shared_ptr<Formula> factor();

    std::shared_ptr<Formula> term();
};

#endif // PARSER_HPP
