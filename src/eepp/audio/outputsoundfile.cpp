#include <eepp/audio/outputsoundfile.hpp>
#include <eepp/audio/soundfilewriter.hpp>
#include <eepp/audio/soundfilefactory.hpp>

namespace EE { namespace Audio {

OutputSoundFile::OutputSoundFile() :
	mWriter(NULL)
{}

OutputSoundFile::~OutputSoundFile() {
	// Close the file in case it was open
	close();
}

bool OutputSoundFile::openFromFile(const std::string& filename, unsigned int sampleRate, unsigned int channelCount) {
	// If the file is already open, first close it
	close();

	// Find a suitable writer for the file type
	mWriter = SoundFileFactory::createWriterFromFilename(filename);
	if (!mWriter)
		return false;

	// Pass the stream to the reader
	if (!mWriter->open(filename, sampleRate, channelCount)) {
		close();
		return false;
	}

	return true;
}

void OutputSoundFile::write(const Int16* samples, Uint64 count) {
	if (mWriter && samples && count)
		mWriter->write(samples, count);
}

void OutputSoundFile::close() {
	// Destroy the reader
	delete mWriter;
	mWriter = NULL;
}

}}
