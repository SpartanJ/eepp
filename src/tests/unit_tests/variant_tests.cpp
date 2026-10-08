#include "utest.hpp"

#include <eepp/graphics/rectangledrawable.hpp>
#include <eepp/ui/models/variant.hpp>
#include <limits>
#include <type_traits>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::Math;
using namespace EE::UI;
using namespace EE::UI::Models;

static_assert( static_cast<int>( Variant::Type::Invalid ) == 0 );
static_assert( static_cast<int>( Variant::Type::StringPtr ) == 15 );
static_assert( static_cast<int>( Variant::Type::StdStringPtr ) == 16 );
static_assert( std::is_nothrow_move_constructible_v<std::string> );
static_assert( std::is_nothrow_move_constructible_v<String> );
static_assert( std::is_nothrow_move_constructible_v<DrawablePtr> );

UTEST( Variant, ownedStdStringLifecycle ) {
	const std::string shortString = "ecode";
	const std::string longString( 256, 'x' );

	Variant shortValue( shortString );
	EXPECT_TRUE( shortValue.is( Variant::Type::StdString ) );
	EXPECT_TRUE( shortValue.isString() );
	EXPECT_TRUE( shortValue.isStdStringLike() );
	EXPECT_TRUE( shortValue.asStdString() == shortString );
	EXPECT_TRUE( shortValue.asStdStringView() == shortString );
	EXPECT_EQ( shortValue.size(), shortString.size() );

	Variant copied( shortValue );
	EXPECT_TRUE( copied == shortValue );

	Variant longValue( longString );
	const char* longData = longValue.asStdString().data();
	Variant moved( std::move( longValue ) );
	EXPECT_TRUE( !longValue.isValid() );
	EXPECT_TRUE( moved.asStdString() == longString );
	EXPECT_EQ( moved.asStdString().data(), longData );

	Variant copyAssigned( std::string( 512, 'z' ) );
	copyAssigned = moved;
	EXPECT_TRUE( copyAssigned.asStdString() == longString );

	Variant moveAssigned( std::string( 128, 'q' ) );
	const char* movedData = moved.asStdString().data();
	moveAssigned = std::move( moved );
	EXPECT_TRUE( !moved.isValid() );
	EXPECT_EQ( moveAssigned.asStdString().data(), movedData );
	EXPECT_TRUE( moveAssigned.asStdString() == longString );

	copyAssigned = copyAssigned;
	EXPECT_TRUE( copyAssigned.asStdString() == longString );
	Variant* self = &moveAssigned;
	moveAssigned = std::move( *self );
	EXPECT_TRUE( moveAssigned.asStdString() == longString );

	Variant empty( std::string{} );
	EXPECT_TRUE( empty.asStdString().empty() );
	empty.reset();
	EXPECT_TRUE( !empty.isValid() );
}

UTEST( Variant, ownedStringLifecycle ) {
	String source( "héllo" );
	Variant value( source );
	EXPECT_TRUE( value.is( Variant::Type::String ) );
	EXPECT_TRUE( value.asString() == source );

	Variant copied( value );
	EXPECT_TRUE( copied.asString() == source );

	String movedSource( "moved" );
	Variant movedValue( std::move( movedSource ) );
	EXPECT_TRUE( movedValue.asString() == String( "moved" ) );

	Variant moved( std::move( value ) );
	EXPECT_TRUE( !value.isValid() );
	EXPECT_TRUE( moved.asString() == source );

	Variant copyAssigned( String( "capacity" ) );
	copyAssigned = copied;
	EXPECT_TRUE( copyAssigned.asString() == source );

	Variant moveAssigned( String( "old" ) );
	moveAssigned = std::move( copied );
	EXPECT_TRUE( !copied.isValid() );
	EXPECT_TRUE( moveAssigned.asString() == source );

	moveAssigned.reset();
	EXPECT_TRUE( !moveAssigned.isValid() );
}

UTEST( Variant, borrowedStringsRemainBorrowed ) {
	std::string stdString = "borrowed std::string";
	String string = "borrowed String";

	Variant stdPtr = Variant::fromRef( stdString );
	EXPECT_TRUE( stdPtr.is( Variant::Type::StdStringPtr ) );
	EXPECT_EQ( &stdPtr.asStdStringPtr(), &stdString );
	EXPECT_TRUE( stdPtr.asStdStringView() == stdString );
	EXPECT_TRUE( stdPtr.toString() == stdString );
	EXPECT_EQ( stdPtr.size(), stdString.size() );

	Variant stdPtrCopy( stdPtr );
	Variant stdPtrMove( std::move( stdPtrCopy ) );
	EXPECT_TRUE( !stdPtrCopy.isValid() );
	EXPECT_EQ( &stdPtrMove.asStdStringPtr(), &stdString );
	stdPtrMove.reset();
	EXPECT_TRUE( stdString == "borrowed std::string" );

	Variant stringPtr( &string );
	EXPECT_TRUE( stringPtr.is( Variant::Type::StringPtr ) );
	EXPECT_EQ( &stringPtr.asStringPtr(), &string );
	EXPECT_TRUE( stringPtr.toString() == string.toUtf8() );
	stringPtr.reset();
	EXPECT_TRUE( string == String( "borrowed String" ) );

	Variant cstr( "borrowed cstr" );
	EXPECT_TRUE( cstr.is( Variant::Type::cstr ) );
	EXPECT_TRUE( cstr.asStdStringView() == "borrowed cstr" );
	EXPECT_EQ( cstr.size(), 13u );
}

UTEST( Variant, stringComparisonAndOrdering ) {
	std::string borrowed = "beta";
	Variant owned( std::string( "beta" ) );
	Variant pointer = Variant::fromRef( borrowed );
	Variant cstr( "beta" );
	Variant alpha( std::string( "alpha" ) );

	EXPECT_TRUE( owned == pointer );
	EXPECT_TRUE( pointer == cstr );
	EXPECT_TRUE( alpha < pointer );
	EXPECT_TRUE( owned.toString() == "beta" );
}

UTEST( Variant, valueAndPointerAlternatives ) {
	int pointedValue = 42;
	void* data = &pointedValue;
	auto* icon = reinterpret_cast<UIIcon*>( &pointedValue );
	const Vector2f vector( 10.f, 20.f );
	const Rectf rect( 1.f, 2.f, 3.f, 4.f );

	Variant boolValue( true );
	Variant floatValue( 1.5f );
	Variant intValue( -42 );
	Variant uintValue( std::numeric_limits<unsigned int>::max() );
	Variant int64Value( Int64( -9000000000 ) );
	Variant uint64Value( Uint64( 18000000000ULL ) );
	Variant dataValue( data );
	Variant iconValue( icon );
	Variant vectorValue( vector );
	Variant rectValue( rect );

	EXPECT_TRUE( boolValue.asBool() );
	EXPECT_EQ( floatValue.asFloat(), 1.5f );
	EXPECT_EQ( intValue.asInt(), -42 );
	EXPECT_EQ( uintValue.asUint(), std::numeric_limits<unsigned int>::max() );
	EXPECT_EQ( int64Value.asInt64(), Int64( -9000000000 ) );
	EXPECT_EQ( uint64Value.asUint64(), Uint64( 18000000000ULL ) );
	EXPECT_EQ( dataValue.asDataPtr(), data );
	EXPECT_EQ( iconValue.asIcon(), icon );
	EXPECT_TRUE( vectorValue.asVector2f() == vector );
	EXPECT_TRUE( rectValue.asRectf() == rect );
	EXPECT_EQ( vectorValue.size(), sizeof( Vector2f ) );
	EXPECT_EQ( rectValue.size(), sizeof( Rectf ) );

	Variant copied( vectorValue );
	EXPECT_TRUE( copied.asVector2f() == vector );
	Variant moved( std::move( copied ) );
	EXPECT_TRUE( !copied.isValid() );
	EXPECT_TRUE( moved.asVector2f() == vector );
	moved = rectValue;
	EXPECT_TRUE( moved.is( Variant::Type::Rectf ) );
	EXPECT_TRUE( moved.asRectf() == rect );
}

UTEST( Variant, typeTransitions ) {
	Variant value( std::string( "text" ) );
	value = Variant( 42 );
	EXPECT_TRUE( value.is( Variant::Type::Int ) );
	EXPECT_EQ( value.asInt(), 42 );

	value = Variant( String( "unicode" ) );
	EXPECT_TRUE( value.is( Variant::Type::String ) );
	value = Variant( DrawablePtr{} );
	EXPECT_TRUE( value.is( Variant::Type::Drawable ) );
	value.reset();
	EXPECT_TRUE( !value.isValid() );

	value = Variant( Vector2f( 2.f, 3.f ) );
	value = Variant( std::string( "again" ) );
	EXPECT_TRUE( value.is( Variant::Type::StdString ) );
	EXPECT_TRUE( value.asStdString() == "again" );
}

UTEST( Variant, drawableOwnershipLifecycle ) {
	auto drawable = makeResource<RectangleDrawable>( Vector2f( 1.f, 2.f ), Sizef( 3.f, 4.f ) );
	ASSERT_TRUE( drawable != nullptr );
	const auto initialRefs = drawable.use_count();

	Variant original( drawable );
	EXPECT_EQ( drawable.use_count(), initialRefs + 1 );
	{
		Variant copied( original );
		EXPECT_EQ( drawable.use_count(), initialRefs + 2 );
		Variant moved( std::move( copied ) );
		EXPECT_TRUE( !copied.isValid() );
		EXPECT_EQ( drawable.use_count(), initialRefs + 2 );
		Variant moveAssigned( DrawablePtr{} );
		moveAssigned = std::move( moved );
		EXPECT_TRUE( !moved.isValid() );
		EXPECT_EQ( drawable.use_count(), initialRefs + 2 );

		Variant assigned( DrawablePtr{} );
		assigned = original;
		EXPECT_EQ( drawable.use_count(), initialRefs + 3 );
		assigned = Variant( 7 );
		EXPECT_EQ( drawable.use_count(), initialRefs + 2 );
		moveAssigned.reset();
		EXPECT_EQ( drawable.use_count(), initialRefs + 1 );
	}

	original.reset();
	EXPECT_EQ( drawable.use_count(), initialRefs );
}
