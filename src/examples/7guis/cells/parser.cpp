#include "parser.hpp"

Tokenizer FormulaParser::tokenizer = Tokenizer();

void Tokenizer::add( std::string regex, TokenType token ) {
	tokenInfos.push_back( TokenInfo( "^" + regex, token ) );
}

bool Tokenizer::tokenize( const std::string& s ) {
	tokens.clear();
	LuaPattern::Range matches[4];
	size_t i = 0;
	while ( i < s.size() ) {
		bool match = false;
		for ( auto& info : tokenInfos ) {
			LuaPattern regex( info.pattern );
			if ( regex.matches( s, matches, i ) ) {
				std::string tok = s.substr( matches[0].start, matches[0].end - matches[0].start );
				tokens.emplace_back( Token{ info.token, std::move( tok ) } );
				i = matches[0].end;
				match = true;
				break;
			}
		}
		if ( !match )
			return false;
	}
	return true;
}

void FormulaParser::initTokenizer() {
	if ( !tokenizer.tokenInfos.empty() )
		return;
	tokenizer.add( "%a%d+", TokenType::CELL );
	tokenizer.add( "%a%w*", TokenType::IDENT );
	tokenizer.add( "-?%d+%.?%d*", TokenType::DECIMAL );
	tokenizer.add( "=", TokenType::EQUALS );
	tokenizer.add( ",", TokenType::COMMA );
	tokenizer.add( ":", TokenType::COLON );
	tokenizer.add( "%(", TokenType::OPEN_BRACKET );
	tokenizer.add( "%)", TokenType::CLOSE_BRACKET );
}

void FormulaParser::nextToken() {
	tokens.pop_front();
	if ( tokens.empty() )
		lookahead = Token( { TokenType::EPSILON, "" } );
	else
		lookahead = tokens.front();
}

std::shared_ptr<Formula> FormulaParser::application() {
	auto opName = lookahead.sequence;
	nextToken();
	if ( lookahead.token != TokenType::OPEN_BRACKET )
		return nullptr;
	nextToken();
	std::vector<std::shared_ptr<Formula>> args;
	while ( true ) {
		if ( lookahead.token == TokenType::EPSILON )
			return nullptr;
		args.emplace_back( expression() );
		if ( lookahead.token == TokenType::COMMA )
			nextToken();
		if ( lookahead.token == TokenType::CLOSE_BRACKET )
			return std::make_shared<SheetFunction>( opName, args );
	}
}

std::shared_ptr<Formula> FormulaParser::expression() {
	switch ( lookahead.token ) {
		case TokenType::CELL: {
			if ( lookahead.sequence.size() < 2 )
				return nullptr;
			String::toUpperInPlace( lookahead.sequence );
			int c = lookahead.sequence[0] - 'A';
			int r = 0;
			if ( !String::fromString( r, lookahead.sequence.substr( 1 ) ) )
				return nullptr;
			r = std::max( 0, r - 1 );
			nextToken();
			if ( lookahead.token == TokenType::COLON ) { // Range
				nextToken();
				if ( lookahead.token == TokenType::CELL ) {
					String::toUpperInPlace( lookahead.sequence );
					int c2 = lookahead.sequence[0] - 'A';
					int r2 = 0;
					if ( !String::fromString( r2, lookahead.sequence.substr( 1 ) ) )
						return nullptr;
					r2 = std::max( 0, r2 - 1 );
					nextToken();
					return std::make_shared<RangeReference>(
						std::make_shared<CellReference>( c, r ),
						std::make_shared<CellReference>( c2, r2 ) );
				} else {
					return nullptr;
				}
			} else {
				return std::make_shared<CellReference>( c, r );
			}
		}
		case TokenType::DECIMAL: {
			double val = 0;
			String::fromString( val, lookahead.sequence );
			return std::make_shared<Number>( val );
		}
		case TokenType::IDENT:
			return application();
		default:
			return nullptr;
	}
}

std::shared_ptr<Formula> FormulaParser::formula() {
	switch ( lookahead.token ) {
		case TokenType::DECIMAL: {
			auto n = lookahead.sequence;
			nextToken();
			double val = 0;
			String::fromString( val, n );
			return std::make_shared<Number>( val );
		}
		case TokenType::EQUALS:
			nextToken();
			return expression();
		case TokenType::EPSILON:
			return std::make_shared<Textual>();
		default:
			return std::make_shared<Textual>( formulaString );
	}
}

std::shared_ptr<Formula> FormulaParser::parseFormula( std::string _formulaString ) {
	initTokenizer();
	formulaString = std::move( _formulaString );
	String::replaceAll( formulaString, " ", "" );
	if ( !tokenizer.tokenize( formulaString ) )
		return {};
	tokens = tokenizer.tokens;
	if ( tokens.empty() )
		return std::make_shared<Textual>();
	lookahead = tokens.front();
	return formula();
}
