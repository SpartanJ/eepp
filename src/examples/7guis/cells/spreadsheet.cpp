#include "spreadsheet.hpp"
#include "parser.hpp"

void Cell::parseFormula( const std::string& formulaStr ) {
	FormulaParser parser;
	formula = parser.parseFormula( formulaStr );
}

void Cell::setData( std::string&& data, Spreadsheet& sheet ) {
	value = std::move( data );
	parseFormula( value );
	calc( sheet );
}

std::optional<double> Cell::eval() const {
	double val;
	if ( String::fromString( val, displayValue ) )
		return val;
	return {};
}

void Cell::calc( Spreadsheet& sheet ) {
	if ( !formula || formula->type() == FormulaType::Textual ) {
		displayValue = value;
		return;
	}
	auto res = formula->eval( sheet );
	if ( res )
		displayValue = String::fromDouble( *res );
	else
		displayValue = "!ERR";
}

Spreadsheet::Spreadsheet() : Model(), mEmptyCell( std::make_unique<Cell>( "" ) ) {}

Variant Spreadsheet::data( const ModelIndex& index, ModelRole role ) const {
	if ( nullptr == mCells[index.column()][index.row()] )
		return {};
	switch ( role ) {
		case EE::UI::Models::ModelRole::Display:
			return Variant( cell( index ).getDisplayValue().c_str() );
		case EE::UI::Models::ModelRole::Custom:
			return Variant( cell( index ).getValue().c_str() );
		default:
			break;
	}
	return {};
}

void Spreadsheet::createCell( int col, int row ) {
	if ( mCells[col][row] == nullptr )
		mCells[col][row] = std::make_unique<Cell>( "" );
}

Cell& Spreadsheet::cell( int col, int row ) {
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
	cell( index ).setData( data.toString(), *this );
}
