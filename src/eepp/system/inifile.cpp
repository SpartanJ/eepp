#include <cctype>
#include <cstdarg>
#include <eepp/system/clock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/inifile.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/packregistry.hpp>

#define MAX_KEYNAME 128
#define MAX_VALUENAME 128
#define MAX_VALUEDATA 2048

namespace EE { namespace System {

IniFile::IniFile( std::string_view iniPath, bool autoLoad ) {
	if ( autoLoad )
		loadFromFile( iniPath );
	else
		path( iniPath );
}

IniFile::IniFile( const Uint8* RAWData, const Uint32& size, bool autoLoad ) {
	if ( autoLoad )
		loadFromMemory( RAWData, size );
}

IniFile::IniFile( Pack* Pack, std::string_view iniPackPath, bool autoLoad ) {
	if ( autoLoad )
		loadFromPack( Pack, iniPackPath );
}

IniFile::IniFile( IOStream& stream, bool autoLoad ) {
	if ( autoLoad )
		loadFromStream( stream );
}

bool IniFile::loadFromPack( Pack* Pack, std::string_view iniPackPath ) {
	std::string path( iniPackPath );
	if ( NULL != Pack && Pack->isOpen() && -1 != Pack->exists( path ) ) {
		ScopedBuffer buffer;

		Pack->extractFileToMemory( path, buffer );

		return loadFromMemory( buffer.get(), buffer.length() );
	}

	return false;
}

bool IniFile::loadFromStream( IOStream& stream ) {
	if ( !stream.isOpen() )
		return false;
	Clock clock;
	mBuffer.resize( stream.getSize() );
	stream.read( mBuffer.data(), mBuffer.size() );
	clear();
	readFile();
	if ( !mPath.empty() )
		Log::info( "%s loaded in %.2fms", mPath.c_str(), clock.getElapsedTime().asMilliseconds() );
	return true;
}

bool IniFile::loadFromMemory( const Uint8* RAWData, const Uint32& size ) {
	IOStreamMemory f( reinterpret_cast<const char*>( RAWData ), size );
	return loadFromStream( f );
}

bool IniFile::loadFromFile( std::string_view iniPath ) {
	path( iniPath );

	if ( FileSystem::fileExists( mPath ) ) {
		IOStreamFile f( mPath );
		return loadFromStream( f );
	} else if ( PackRegistry::instance()->isFallbackToPacksActive() ) {
		std::string tPath( mPath );

		Pack* tPack = PackRegistry::instance()->exists( tPath );

		if ( NULL != tPack ) {
			return loadFromPack( tPack, tPath );
		}
	}
	return false;
}

bool IniFile::readFile() {

	if ( mIniRead )
		return true;

	bool isBOM = false;
	if ( mBuffer.size() >= 3 && (char)0xef == mBuffer[0] && (char)0xbb == mBuffer[1] &&
		 (char)0xbf == mBuffer[2] ) {
		isBOM = true;
	}

	std::string_view buffer( mBuffer );
	if ( isBOM )
		buffer = std::string_view( mBuffer.data() + 3, mBuffer.size() - 3 );
	std::string_view line;
	std::string_view keyname, valuename, value;
	std::string::size_type pLeft, pRight;

	size_t pos = 0;
	size_t curPos = 0;
	size_t size = mBuffer.size();

	while ( pos < size ) {
		curPos = buffer.find_first_of( '\n', pos );
		if ( curPos != std::string_view::npos ) {
			line = buffer.substr( pos, curPos - pos );
			pos = curPos + 1;
		} else {
			line = buffer.substr( pos );
			pos = size;
		}

		// To be compatible with Win32, check for existence of '\r'.
		// Win32 files have the '\r' and Unix files don't at the end of a line.
		// Note that the '\r' will be written to INI files from
		// Unix so that the created INI file can be read under Win32
		// without change.
		if ( line.length() && line[line.length() - 1] == '\r' )
			line = line.substr( 0, line.size() - 1 );

		// Check that the user hasn't opened a binary file by checking the first
		// character of each line!
		if ( !line.empty() && line[0] != '\0' && !isprint( line[0] ) && !isspace( line[0] ) ) {
			Log::error( "IniFile::readFile(): Failing on char %d.", line[0] );
			return false;
		}

		if ( ( pLeft = line.find_first_of( ";#[=" ) ) != std::string_view::npos ) {
			switch ( line[pLeft] ) {
				case '[':
					if ( ( pRight = line.find_last_of( "]" ) ) != std::string_view::npos &&
						 pRight > pLeft ) {
						keyname = line.substr( pLeft + 1, pRight - pLeft - 1 );
						addKeyName( keyname );
					}
					break;
				case '=':
					valuename = String::trim( line.substr( 0, pLeft ),
											  "\t " ); // Remove the extra space between valuename
													   // and = . No spaced valuename permitted.
					value = String::lTrim( line.substr( pLeft + 1 ), "\t " );
					setValue( keyname, valuename, value );
					break;
				case ';':
				case '#':
					if ( mKeys.empty() )
						addHeaderComment( std::string{ line.substr( pLeft + 1 ) } );
					else
						addKeyComment( keyname, std::string{ line.substr( pLeft + 1 ) } );
					break;
			}
		}
	}

	if ( !mKeys.empty() ) {
		mIniRead = true;

		return true;
	}

	return false;
}

bool IniFile::writeFile() {
	IOStreamFile f( mPath, "w" );
	return writeStream( f );
}

bool IniFile::writeStream( IOStream& stream ) {
	if ( !stream.isOpen() )
		return false;

	static constexpr char commentPrefix = ';';
	static constexpr char keyPrefix = '[';
	static constexpr char keySuffix[] = "]\n";
	static constexpr char valueSeparator = '=';
	static constexpr char newline = '\n';

	// Write header mComments.
	for ( const auto& comment : mComments ) {
		stream.write( &commentPrefix, 1 );
		stream.write( comment.data(), comment.size() );
		stream.write( &newline, 1 );
	}

	if ( !mComments.empty() )
		stream.write( &newline, 1 );

	// Write Keys and values.
	for ( const auto& key : mKeys ) {
		stream.write( &keyPrefix, 1 );
		stream.write( key.name.data(), key.name.size() );
		stream.write( keySuffix, sizeof( keySuffix ) - 1 );

		// Comments.
		for ( const auto& comment : key.comments ) {
			stream.write( &commentPrefix, 1 );
			stream.write( comment.data(), comment.size() );
			stream.write( &newline, 1 );
		}

		// Values.
		for ( const auto& value : key.values ) {
			stream.write( value.name.data(), value.name.size() );
			stream.write( &valueSeparator, 1 );
			stream.write( value.data.data(), value.data.size() );
			stream.write( &newline, 1 );
		}

		stream.write( &newline, 1 );
	}

	return true;
}

long IniFile::findKey( std::string_view keyname ) const {
	for ( unsigned keyID = 0; keyID < mKeys.size(); ++keyID )
		if ( mKeys[keyID].name == keyname )
			return long( keyID );
	return noID;
}

long IniFile::findValue( const unsigned int keyID, std::string_view valuename ) const {
	if ( !mKeys.size() || keyID >= mKeys.size() )
		return noID;

	for ( unsigned valueID = 0; valueID < mKeys[keyID].values.size(); ++valueID )
		if ( mKeys[keyID].values[valueID].name == valuename )
			return long( valueID );
	return noID;
}

unsigned int IniFile::addKeyName( std::string_view keyname ) {
	mKeys.push_back( { std::string{ keyname }, {}, {} } );
	return (unsigned int)( mKeys.size() - 1 );
}

std::string IniFile::getKeyName( unsigned const keyID ) const {
	if ( keyID < mKeys.size() )
		return mKeys[keyID].name;
	else
		return "";
}

unsigned IniFile::getNumValues( unsigned const keyID ) {
	if ( keyID < mKeys.size() )
		return (unsigned int)mKeys[keyID].values.size();
	return 0;
}

unsigned IniFile::getNumValues( std::string_view keyname ) {
	long keyID = findKey( keyname );
	if ( keyID == noID )
		return 0;
	return (unsigned int)mKeys[keyID].values.size();
}

std::string IniFile::getValueName( unsigned const keyID, unsigned const valueID ) const {
	if ( keyID < mKeys.size() && valueID < mKeys[keyID].values.size() )
		return mKeys[keyID].values[valueID].name;
	return "";
}

std::string IniFile::getValueName( std::string_view keyname, unsigned const valueID ) const {
	long keyID = findKey( keyname );
	if ( keyID == noID )
		return "";
	return getValueName( keyID, valueID );
}

bool IniFile::setValue( unsigned const keyID, unsigned const valueID, std::string_view value ) {
	if ( keyID < mKeys.size() && valueID < mKeys[keyID].values.size() )
		mKeys[keyID].values[valueID].data.assign( value );

	return false;
}

bool IniFile::setValue( std::string_view keyname, std::string_view valuename,
						std::string_view value, bool create ) {
	long keyID = findKey( keyname );
	if ( keyID == noID ) {
		if ( create )
			keyID = long( addKeyName( keyname ) );
		else
			return false;
	}

	long valueID = findValue( unsigned( keyID ), valuename );
	if ( valueID == noID ) {
		if ( !create )
			return false;
		mKeys[keyID].values.push_back( { std::string{ valuename }, std::string{ value } } );
	} else
		mKeys[keyID].values[valueID].data.assign( value );

	return true;
}

bool IniFile::setValueI( std::string_view keyname, std::string_view valuename, int const value,
						 bool create ) {
	char svalue[MAX_VALUEDATA];

	String::formatBuffer( svalue, MAX_VALUEDATA, "%d", value );
	return setValue( keyname, valuename, svalue, create );
}

bool IniFile::setValueU( std::string_view keyname, std::string_view valuename,
						 const unsigned long value, bool create ) {
	char svalue[MAX_VALUEDATA];

	String::formatBuffer( svalue, MAX_VALUEDATA, "%u", value );
	return setValue( keyname, valuename, svalue, create );
}

bool IniFile::setValueF( std::string_view keyname, std::string_view valuename, double const value,
						 bool create ) {
	char svalue[MAX_VALUEDATA];

	String::formatBuffer( svalue, MAX_VALUEDATA, "%f", value );
	return setValue( keyname, valuename, svalue, create );
}

bool IniFile::setValueV( std::string_view keyname, std::string_view valuename, char* format, ... ) {
	va_list args;
	char value[MAX_VALUEDATA];

	va_start( args, format );
#ifdef EE_COMPILER_MSVC
	vsprintf_s( value, MAX_VALUEDATA, format, args );
#else
	vsnprintf( value, MAX_VALUEDATA, format, args );
#endif
	va_end( args );
	return setValue( keyname, valuename, value );
}

std::string IniFile::getValue( unsigned const keyID, unsigned const valueID,
							   std::string_view defValue ) const {
	if ( keyID < mKeys.size() && valueID < mKeys[keyID].values.size() )
		return mKeys[keyID].values[valueID].data;
	return std::string{ defValue };
}

std::string IniFile::getValue( std::string_view keyname, std::string_view valuename,
							   std::string_view defValue ) const {
	long keyID = findKey( keyname );
	if ( keyID == noID )
		return std::string{ defValue };

	long valueID = findValue( unsigned( keyID ), valuename );
	if ( valueID == noID )
		return std::string{ defValue };

	return mKeys[keyID].values[valueID].data;
}

int IniFile::getValueI( std::string_view keyname, std::string_view valuename,
						int const defValue ) const {
	char svalue[MAX_VALUEDATA];

	String::formatBuffer( svalue, MAX_VALUEDATA, "%d", defValue );
	return atoi( getValue( keyname, valuename, svalue ).c_str() );
}

unsigned long IniFile::getValueU( std::string_view keyname, std::string_view valuename,
								  const unsigned long defValue ) const {
	char svalue[MAX_VALUEDATA];

	String::formatBuffer( svalue, MAX_VALUEDATA, "%u", defValue );
	return atoi( getValue( keyname, valuename, svalue ).c_str() );
}

bool IniFile::getValueB( std::string_view keyname, std::string_view valuename,
						 const bool defValue ) const {
	std::string val = getValue( keyname, valuename, defValue ? "1" : "0" );
	char fist = !val.empty() ? val[0] : '0';
	return fist == '1' || fist == 't' || fist == 'y' || fist == 'T' || fist == 'Y';
}

double IniFile::getValueF( std::string_view keyname, std::string_view valuename,
						   double const defValue ) const {
	char svalue[MAX_VALUEDATA];

	String::formatBuffer( svalue, MAX_VALUEDATA, "%f", defValue );
	return atof( getValue( keyname, valuename, svalue ).c_str() );
}

bool IniFile::deleteValue( std::string_view keyname, std::string_view valuename ) {
	long keyID = findKey( keyname );
	if ( keyID == noID )
		return false;

	long valueID = findValue( unsigned( keyID ), valuename );
	if ( valueID == noID )
		return false;

	auto pos = mKeys[keyID].values.begin() + valueID;
	mKeys[keyID].values.erase( pos );

	return true;
}

bool IniFile::deleteKey( std::string_view keyname ) {
	long keyID = findKey( keyname );
	if ( keyID == noID )
		return false;

	mKeys.erase( mKeys.begin() + keyID );

	return true;
}

void IniFile::clear() {
	mIniRead = false;
	mKeys.clear();
	mComments.clear();
}

void IniFile::addHeaderComment( std::string_view comment ) {
	mComments.emplace_back( comment );
}

std::string IniFile::getHeaderComment( unsigned const commentID ) const {
	if ( commentID < mComments.size() )
		return mComments[commentID];
	return "";
}

bool IniFile::deleteHeaderComment( unsigned commentID ) {
	if ( commentID < mComments.size() ) {
		auto cpos = mComments.begin() + commentID;
		mComments.erase( cpos, cpos + 1 );
		return true;
	}
	return false;
}

std::map<std::string, std::string> IniFile::getKeyMap( const unsigned& keyID ) const {
	std::map<std::string, std::string> map;
	if ( keyID < mKeys.size() ) {
		for ( const auto& value : mKeys[keyID].values ) {
			map[value.name] = value.data;
		}
		return map;
	}
	return {};
}

std::map<std::string, std::string> IniFile::getKeyMap( std::string_view keyname ) const {
	long keyID = findKey( keyname );
	if ( keyID != noID )
		return getKeyMap( keyID );
	return {};
}

std::unordered_map<std::string, std::string>
IniFile::getKeyUnorderedMap( const unsigned& keyID ) const {
	std::unordered_map<std::string, std::string> map;
	if ( keyID < mKeys.size() ) {
		map.reserve( mKeys[keyID].values.size() );
		for ( const auto& value : mKeys[keyID].values ) {
			map.emplace( value.name, value.data );
		}
		return map;
	}
	return {};
}

std::unordered_map<std::string, std::string>
IniFile::getKeyUnorderedMap( std::string_view keyname ) const {
	long keyID = findKey( keyname );
	if ( keyID != noID )
		return getKeyUnorderedMap( keyID );
	return {};
}

unsigned IniFile::getNumKeyComments( unsigned const keyID ) const {
	if ( keyID < mKeys.size() )
		return (unsigned int)mKeys[keyID].comments.size();
	return 0;
}

unsigned IniFile::getNumKeyComments( std::string_view keyname ) const {
	long keyID = findKey( keyname );
	if ( keyID == noID )
		return 0;
	return (unsigned int)mKeys[keyID].comments.size();
}

bool IniFile::addKeyComment( unsigned const keyID, std::string_view comment ) {
	if ( keyID < mKeys.size() ) {
		mKeys[keyID].comments.emplace_back( comment );
		return true;
	}
	return false;
}

bool IniFile::addKeyComment( std::string_view keyname, std::string_view comment ) {
	long keyID = findKey( keyname );
	if ( keyID == noID )
		return false;
	return addKeyComment( unsigned( keyID ), comment );
}

std::string IniFile::getKeyComment( unsigned const keyID, unsigned const commentID ) const {
	if ( keyID < mKeys.size() && commentID < mKeys[keyID].comments.size() )
		return mKeys[keyID].comments[commentID];
	return "";
}

std::string IniFile::getKeyComment( std::string_view keyname, unsigned const commentID ) const {
	long keyID = findKey( keyname );
	if ( keyID == noID )
		return "";
	return getKeyComment( unsigned( keyID ), commentID );
}

bool IniFile::deleteKeyComment( unsigned const keyID, unsigned const commentID ) {
	if ( keyID < mKeys.size() && commentID < mKeys[keyID].comments.size() ) {
		auto cpos = mKeys[keyID].comments.begin() + commentID;
		mKeys[keyID].comments.erase( cpos, cpos + 1 );
		return true;
	}
	return false;
}

bool IniFile::deleteKeyComment( std::string_view keyname, unsigned const commentID ) {
	long keyID = findKey( keyname );
	if ( keyID == noID )
		return false;
	return deleteKeyComment( unsigned( keyID ), commentID );
}

bool IniFile::deleteKeyComments( unsigned const keyID ) {
	if ( keyID < mKeys.size() ) {
		mKeys[keyID].comments.clear();
		return true;
	}
	return false;
}

bool IniFile::deleteKeyComments( std::string_view keyname ) {
	long keyID = findKey( keyname );
	if ( keyID == noID )
		return false;
	return deleteKeyComments( unsigned( keyID ) );
}

bool IniFile::keyExists( std::string_view keyname ) const {
	return findKey( keyname ) != noID;
}

bool IniFile::keyValueExists( std::string_view keyname, std::string_view valuename ) const {
	long keyID = findKey( keyname );
	if ( keyID == noID )
		return false;

	long valueID = findValue( unsigned( keyID ), valuename );
	if ( valueID == noID )
		return false;

	return true;
}

}} // namespace EE::System
