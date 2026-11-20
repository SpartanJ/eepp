#include "../eepp/system/base64scalar.hpp"
#include "../tests/unit_tests/utest.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <eepp/system/base64.hpp>
#include <eepp/system/cpu.hpp>
#include <random>
#include <vector>

using namespace EE;
using namespace EE::System;

namespace {

using EncodeFunction = size_t ( * )( size_t, const unsigned char*, size_t, char* );

size_t encodeScalar( size_t inLen, const unsigned char* in, size_t outLen, char* out ) {
	if ( outLen < Base64::encodeSafeOutLen( inLen ) )
		return static_cast<size_t>( -1 );
	return Private::encodeBase64Scalar( inLen, in, out );
}

double medianNanoseconds( EncodeFunction encode, const std::vector<Uint8>& input,
						  std::vector<char>& output ) {
	// Keep every call observable even when the compiler can see the scalar implementation.
	EncodeFunction volatile call = encode;
	const size_t iterations = std::max<size_t>( 32, ( 8 * 1024 * 1024 ) / input.size() );
	std::array<double, 5> samples;
	for ( double& sample : samples ) {
		const auto begin = std::chrono::steady_clock::now();
		for ( size_t i = 0; i < iterations; ++i )
			call( input.size(), input.data(), output.size(), output.data() );
		const auto elapsed = std::chrono::steady_clock::now() - begin;
		sample = std::chrono::duration<double, std::nano>( elapsed ).count() / iterations;
	}
	std::sort( samples.begin(), samples.end() );
	return samples[samples.size() / 2];
}

} // namespace

UTEST( Benchmark, Base64Encode ) {
	const char* backend = CPU::hasAVX2() ? "AVX2" : CPU::hasNEON() ? "NEON" : "scalar";
	printf( "  Automatic encoder backend for full blocks: %s\n", backend );
	std::mt19937 rng( 42 );
	// The Kitty presenter encodes up to 3072 input bytes into a 4096-byte payload.
	for ( size_t size : { 16, 64, 3071, 3072, 1024 * 1024 } ) {
		std::vector<Uint8> input( size );
		for ( Uint8& byte : input )
			byte = static_cast<Uint8>( rng() );
		std::vector<char> expected( Base64::encodeSafeOutLen( size ) );
		std::vector<char> output( expected.size() );
		ASSERT_EQ( expected.size() - 1,
				   encodeScalar( size, input.data(), expected.size(), expected.data() ) );
		ASSERT_EQ( output.size() - 1,
				   Base64::encode( size, input.data(), output.size(), output.data() ) );
		ASSERT_TRUE( expected == output );

		const double scalar = medianNanoseconds( encodeScalar, input, output );
		const double automatic = medianNanoseconds( Base64::encode, input, output );
		printf( "  %7zu B  scalar %9.2f ns  auto %9.2f ns  %5.2fx\n", size, scalar, automatic,
				scalar / automatic );
	}
}
