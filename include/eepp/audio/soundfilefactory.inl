
namespace EE { namespace Audio {

namespace priv
{
	template <typename T> SoundFileReader* createReader() {return new T;}
	template <typename T> SoundFileWriter* createWriter() {return new T;}
}

template <typename T>
void SoundFileFactory::registerReader()
{
	// Make sure the same class won't be registered twice
	unregisterReader<T>();

	// Create a new factory with the functions provided by the class
	ReaderFactory factory;
	factory.check = &T::check;
	factory.usesFileExtension = &T::usesFileExtension;
	factory.create = &priv::createReader<T>;

	// Add it
	s_readers.push_back(factory);
}

template <typename T>
void SoundFileFactory::unregisterReader()
{
	// Remove the instance(s) of the reader from the array of factories
	for (ReaderFactoryArray::iterator it = s_readers.begin(); it != s_readers.end(); ) {
		if (it->create == &priv::createReader<T>)
			it = s_readers.erase(it);
		else
			++it;
	}
}

template <typename T>
void SoundFileFactory::registerWriter() {
	// Make sure the same class won't be registered twice
	unregisterWriter<T>();

	// Create a new factory with the functions provided by the class
	WriterFactory factory;
	factory.check = &T::check;
	factory.create = &priv::createWriter<T>;

	// Add it
	s_writers.push_back(factory);
}

template <typename T>
void SoundFileFactory::unregisterWriter() {
	// Remove the instance(s) of the writer from the array of factories
	for (WriterFactoryArray::iterator it = s_writers.begin(); it != s_writers.end(); ) {
		if (it->create == &priv::createWriter<T>)
			it = s_writers.erase(it);
		else
			++it;
	}
}

}}
