#include <eepp/ui/models/stringmapmodel.hpp>
#include <eepp/ui/models/modelindex.hpp>
#include "utest.hpp"

using namespace EE::UI::Models;

UTEST( StringMapModel, InitialState ) {
	std::map<std::string, std::vector<std::string>> data = {
		{ "A", { "1", "2" } },
		{ "B", { "3" } }
	};
	auto model = StringMapModel<std::string>::create( data );
	
	ASSERT_EQ( (int)model->rowCount(), 2 );
	ASSERT_EQ( (int)model->columnCount(), 1 );
	
	ModelIndex idxA = model->index( 0, 0 );
	ModelIndex idxB = model->index( 1, 0 );
	
	ASSERT_TRUE( idxA.isValid() );
	ASSERT_TRUE( idxB.isValid() );
	
	// Ordering is map ordering, so A then B.
	ASSERT_STDSTREQ( model->data( idxA ).asStdString(), "A" );
	ASSERT_STDSTREQ( model->data( idxB ).asStdString(), "B" );
	
	ASSERT_EQ( (int)model->rowCount( idxA ), 2 );
	ASSERT_EQ( (int)model->rowCount( idxB ), 1 );
	
	ModelIndex idxA1 = model->index( 0, 0, idxA );
	ASSERT_TRUE( idxA1.isValid() );
	ASSERT_STDSTREQ( model->data( idxA1 ).asStdString(), "1" );
}

UTEST( StringMapModel, Filter ) {
	std::map<std::string, std::vector<std::string>> data = {
		{ "Cat", { "Item 1" } },
		{ "Dog", { "Item 2" } }
	};
	auto model = StringMapModel<std::string>::create( data );
	
	model->filter( "Cat" );
	ASSERT_EQ( (int)model->rowCount(), 1 );
	ModelIndex idx = model->index( 0, 0 );
	ASSERT_STDSTREQ( model->data( idx ).asStdString(), "Cat" );
	
	model->filter( "Item 2" );
	ASSERT_EQ( (int)model->rowCount(), 1 );
	idx = model->index( 0, 0 );
	ASSERT_STDSTREQ( model->data( idx ).asStdString(), "Dog" );
	
	model->filter( "" );
	ASSERT_EQ( (int)model->rowCount(), 2 );
}
