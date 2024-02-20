#include "spreadsheet.hpp"
#include "parser.hpp"

void Observable::notifyObservers() {
	if ( !mChanged )
		return;
	for ( auto observer : mObservers )
		observer->update();
	clearChanged();
}

void Cell::parseFormula( const std::string& formulaStr ) {
	FormulaParser parser;
	formula = parser.parseFormula( formulaStr );
}

Cell::Cell( std::string val, Spreadsheet& sheet ) : value( std::move( val ) ), sheet( sheet ) {}

Cell::~Cell() {}

bool Cell::subscribeToObservers() {
	if ( formula ) {
		for ( const auto& ref : formula->getReferences( sheet ) ) {
			if ( this == ref || hasObserver( ref ) ) {
				unsubscribeFromObservers();
				return false;
			}
			ref->addObserver( this );
		}
	}
	return true;
}

void Cell::unsubscribeFromObservers() {
	if ( formula ) {
		for ( const auto& ref : formula->getReferences( sheet ) )
			ref->deleteObserver( this );
	}
}

void Cell::setData( std::string&& data ) {
	unsubscribeFromObservers();
	value = std::move( data );
	parseFormula( value );
	calc();
	bool circular = !subscribeToObservers();
	if ( circular ) {
		displayValue = "!CIRCULAR";
		formulaContainsErrors = true;
	} else {
		setChanged();
		notifyObservers();
	}
}

std::optional<double> Cell::eval() const {
	double val;
	if ( String::fromString( val, displayValue ) )
		return val;
	return {};
}

const std::string& Cell::getValue() const {
	return value;
}

const std::string& Cell::getDisplayValue() const {
	return displayValue;
}

void Cell::calc() {
	auto oldDisplayValue = displayValue;

	if ( !hasFormula() && !value.empty() && value[0] == '=' ) {
		displayValue = "!ERR";
		formulaContainsErrors = true;
		return;
	} else if ( !hasFormula() ) {
		displayValue = value;
		formulaContainsErrors = false;
		return;
	}

	auto res = formula->eval( sheet );
	if ( res )
		displayValue = String::fromDouble( *res );
	else
		displayValue = "!ERR";

	formulaContainsErrors = res ? false : true;

	if ( oldDisplayValue != displayValue )
		setChanged();
}

void Cell::clear() {
	value = displayValue = "";
	formula = {};
	formulaContainsErrors = false;
}

void Cell::update() {
	calc();
	notifyObservers();
}

bool Cell::hasFormula() const {
	return formula && formula->type() != FormulaType::Textual;
}

Spreadsheet::Spreadsheet( int cols, int rows ) :
	Model(), mEmptyCell( std::make_unique<Cell>( "", *this ) ) {
	mCells.resize( cols );
	for ( auto& col : mCells )
		col.resize( rows );
}

Variant Spreadsheet::data( const ModelIndex& index, ModelRole role ) const {
	static const char* EMPTY = "";
	static const char* STYLE_NONE = "font_theme_normal";
	static const char* FORMULA_STYLE = "font_theme_success";
	static const char* NUMBER_STYLE = "font_theme_warning";
	static const char* ERROR_STYLE = "font_theme_error";
	switch ( role ) {
		case ModelRole::Display:
			if ( nullptr == mCells[index.column()][index.row()] )
				return Variant( EMPTY );
			return Variant( cell( index ).getDisplayValue().c_str() );
		case ModelRole::Custom: {
			if ( nullptr != mCells[index.column()][index.row()] )
				return Variant( cell( index ).getValue().c_str() );
			break;
		}
		case ModelRole::Class: {
			const auto& cell = mCells[index.column()][index.row()];
			if ( cell && cell->hasErrors() )
				return Variant( ERROR_STYLE );
			if ( cell && cell->hasFormula() ) { // Let's give it some style
				switch ( cell->getFormula()->type() ) {
					case FormulaType::CellReference:
					case FormulaType::RangeReference:
					case FormulaType::Application:
						return Variant( FORMULA_STYLE );
					case FormulaType::Number:
						return Variant( NUMBER_STYLE );
						break;
					default:
						break;
				}
			}
			return Variant( STYLE_NONE );
		}
		default:
			break;
	}
	return {};
}

void Spreadsheet::createCell( int col, int row ) {
	if ( mCells[col][row] == nullptr )
		mCells[col][row] = std::make_unique<Cell>( "", *this );
}

Cell& Spreadsheet::cell( int col, int row ) {
	createCell( col, row );
	return mCells[col][row] ? *mCells[col][row] : *mEmptyCell;
}

Cell& Spreadsheet::cell( const ModelIndex& index ) {
	return mCells[index.column()][index.row()] ? *mCells[index.column()][index.row()] : *mEmptyCell;
}

const Cell& Spreadsheet::cell( const ModelIndex& index ) const {
	return *mCells[index.column()][index.row()];
}

void Spreadsheet::setData( const ModelIndex& index, const Variant& data ) {
	createCell( index.column(), index.row() );
	cell( index ).setData( data.toString() );
}
