#include <eepp/audio/soundfilereaderwav.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/core/debug.hpp>
#include <algorithm>
#include <cctype>
#include <cassert>
#include <cstring>
using namespace EE::System;

namespace
{
	using namespace EE;
	using namespace EE::System;

	// The following functions read integers as little endian and
	// return them in the host byte order

	bool decode(IOStream& stream, Uint8& value) {
		 return stream.read((char*)&value, sizeof(value)) == sizeof(value);
	}

	bool decode(IOStream& stream, Int16& value) {
		unsigned char bytes[sizeof(value)];
		if (stream.read((char*)bytes, sizeof(bytes)) != sizeof(bytes))
			return false;

		value = bytes[0] | (bytes[1] << 8);

		return true;
	}

	bool decode(IOStream& stream, Uint16& value) {
		unsigned char bytes[sizeof(value)];
		if (stream.read((char*)bytes, sizeof(bytes)) != sizeof(bytes))
			return false;

		value = bytes[0] | (bytes[1] << 8);

		return true;
	}

	bool decode24bit(IOStream& stream, Uint32& value) {
		unsigned char bytes[3];
		if (stream.read((char*)bytes, sizeof(bytes)) != sizeof(bytes))
			return false;

		value = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16);

		return true;
	}

	bool decode(IOStream& stream, Uint32& value) {
		unsigned char bytes[sizeof(value)];
		if (stream.read((char*)bytes, sizeof(bytes)) != sizeof(bytes))
			return false;

		value = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

		return true;
	}

	const Uint64 mainChunkSize = 12;

	const Uint16 waveFormatPcm = 1;

	const Uint16 waveFormatExtensible= 65534;

	const char* waveSubformatPcm =
		"\x01\x00\x00\x00\x00\x00\x10\x00"
		"\x80\x00\x00\xAA\x00\x38\x9B\x71";
}

namespace EE  { namespace Audio { namespace Private {

bool SoundFileReaderWav::check(IOStream& stream) {
	char header[mainChunkSize];
	if (stream.read(header, sizeof(header)) < static_cast<Int64>(sizeof(header)))
		return false;

	return (header[0] == 'R') && (header[1] == 'I') && (header[2] == 'F') && (header[3] == 'F')
		&& (header[8] == 'W') && (header[9] == 'A') && (header[10] == 'V') && (header[11] == 'E');
}

SoundFileReaderWav::SoundFileReaderWav() :
	mStream		(NULL),
	mBytesPerSample(0),
	mDataStart	 (0),
	mDataEnd	   (0)
{}

bool SoundFileReaderWav::open(IOStream& stream, Info& info) {
	mStream = &stream;

	if (!parseHeader(info)) {
		eePRINTL( "Failed to open WAV sound file (invalid or unsupported file)" );
		return false;
	}

	return true;
}

void SoundFileReaderWav::seek(Uint64 sampleOffset) {
	assert(mStream);

	mStream->seek(mDataStart + sampleOffset * mBytesPerSample);
}

Uint64 SoundFileReaderWav::read(Int16* samples, Uint64 maxCount) {
	assert(mStream);

	Uint64 count = 0;
	while ((count < maxCount) && (static_cast<Uint64>(mStream->tell()) < mDataEnd)) {
		switch (mBytesPerSample) {
			case 1:
			{
				Uint8 sample = 0;
				if (decode(*mStream, sample))
					*samples++ = (static_cast<Int16>(sample) - 128) << 8;
				else
					return count;
				break;
			}
			case 2:
			{
				Int16 sample = 0;
				if (decode(*mStream, sample))
					*samples++ = sample;
				else
					return count;
				break;
			}
			case 3:
			{
				Uint32 sample = 0;
				if (decode24bit(*mStream, sample))
					*samples++ = sample >> 8;
				else
					return count;
				break;
			}
			case 4:
			{
				Uint32 sample = 0;
				if (decode(*mStream, sample))
					*samples++ = sample >> 16;
				else
					return count;
				break;
			}
			default:
			{
				assert(false);
				return 0;
			}
		}

		++count;
	}

	return count;
}

bool SoundFileReaderWav::parseHeader(Info& info) {
	assert(mStream);

	// If we are here, it means that the first part of the header
	// (the format) has already been checked
	char mainChunk[mainChunkSize];
	if (mStream->read(mainChunk, sizeof(mainChunk)) != sizeof(mainChunk))
		return false;

	// Parse all the sub-chunks
	bool dataChunkFound = false;
	while (!dataChunkFound) {
		// Parse the sub-chunk id and size
		char subChunkId[4];
		if (mStream->read(subChunkId, sizeof(subChunkId)) != sizeof(subChunkId))
			return false;
		Uint32 subChunkSize = 0;
		if (!decode(*mStream, subChunkSize))
			return false;
		Int64 subChunkStart = mStream->tell();
		if (subChunkStart == -1)
			return false;

		// Check which chunk it is
		if ((subChunkId[0] == 'f') && (subChunkId[1] == 'm') && (subChunkId[2] == 't') && (subChunkId[3] == ' ')) {
			// "fmt" chunk

			// Audio format
			Uint16 format = 0;
			if (!decode(*mStream, format))
				return false;
			if ((format != waveFormatPcm) && (format != waveFormatExtensible))
				return false;

			// Channel count
			Uint16 channelCount = 0;
			if (!decode(*mStream, channelCount))
				return false;
			info.channelCount = channelCount;

			// Sample rate
			Uint32 sampleRate = 0;
			if (!decode(*mStream, sampleRate))
				return false;
			info.sampleRate = sampleRate;

			// Byte rate
			Uint32 byteRate = 0;
			if (!decode(*mStream, byteRate))
				return false;

			// Block align
			Uint16 blockAlign = 0;
			if (!decode(*mStream, blockAlign))
				return false;

			// Bits per sample
			Uint16 bitsPerSample = 0;
			if (!decode(*mStream, bitsPerSample))
				return false;

			if (bitsPerSample != 8 && bitsPerSample != 16 && bitsPerSample != 24 && bitsPerSample != 32) {
				eePRINTL( "Unsupported sample size: %d bit (Supported sample sizes are 8/16/24/32 bit)", bitsPerSample );
				return false;
			}
			mBytesPerSample = bitsPerSample / 8;

			if (format == waveFormatExtensible) {
				// Extension size
				Uint16 extensionSize = 0;
				if (!decode(*mStream, extensionSize))
					return false;

				// Valid bits per sample
				Uint16 validBitsPerSample = 0;
				if (!decode(*mStream, validBitsPerSample))
					return false;

				// Channel mask
				Uint32 channelMask = 0;
				if (!decode(*mStream, channelMask))
					return false;

				// Subformat
				char subformat[16];
				if (mStream->read(subformat, sizeof(subformat)) != sizeof(subformat))
					return false;

				if (std::memcmp(subformat, waveSubformatPcm, sizeof(subformat)) != 0) {
					eePRINTL( "Unsupported format: extensible format with non-PCM subformat" );
					return false;
				}

				if (validBitsPerSample != bitsPerSample) {
					eePRINTL( "Unsupported format: sample size (%d bits) and sample container size (%d bits) differ", validBitsPerSample, bitsPerSample );
					return false;
				}
			}

			// Skip potential extra information
			if (mStream->seek(subChunkStart + subChunkSize) == -1)
				return false;
		} else if ((subChunkId[0] == 'd') && (subChunkId[1] == 'a') && (subChunkId[2] == 't') && (subChunkId[3] == 'a')) {
			// "data" chunk

			// Compute the total number of samples
			info.sampleCount = subChunkSize / mBytesPerSample;

			// Store the start and end position of samples in the file
			mDataStart = subChunkStart;
			mDataEnd = mDataStart + info.sampleCount * mBytesPerSample;

			dataChunkFound = true;
		} else {
			// unknown chunk, skip it
			if (mStream->seek(mStream->tell() + subChunkSize) == -1)
				return false;
		}
	}

	return true;
}

}}}
