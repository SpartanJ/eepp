#include <cctype>
#include <cstdarg>
#include <eepp/system/clock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/inifile.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/packmanager.hpp>

#define MAX_KEYNAME 128
#define MAX_VALUENAME 128
#define MAX_VALUEDATA 2048

namespace EE { namespace System {

IniFile::IniFile( const std::string& iniPath, bool autoLoad ) {
	if ( autoLoad )
		loadFromFile( iniPath );
	else
		path( iniPath );
}

IniFile::IniFile( const Uint8* RAWData, const Uint32& size, bool autoLoad ) {
	if ( autoLoad )
		loadFromMemory( RAWData, size );
}

IniFile::IniFile( Pack* Pack, const std::string& iniPackPath, bool autoLoad ) {
	if ( autoLoad )
		loadFromPack( Pack, iniPackPath );
}

IniFile::IniFile( IOStream& stream, bool autoLoad ) {
	if ( autoLoad )
		loadFromStream( stream );
}

bool IniFile::loadFromPack( Pack* Pack, const std::string& iniPackPath ) {
	if ( NULL != Pack && Pack->isOpen() && -1 != Pack->exists( iniPackPath ) ) {
		ScopedBuffer buffer;

		Pack->extractFileToMemory( iniPackPath, buffer );

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

bool IniFile::loadFromFile( const std::string& iniPath ) {
	path( iniPath );

	if ( FileSystem::fileExists( iniPath ) ) {
		IOStreamFile f( mPath );
		return loadFromStream( f );
	} else if ( PackManager::instance()->isFallbackToPacksActive() ) {
		std::string tPath( iniPath );

		Pack* tPack = PackManager::instance()->exists( tPath );

		if ( NULL != tPack ) {
			return loadFromPack( tPack, tPath );
		}
	}
	return false;
}

bool IniFile::readFile() {

	if ( mIniReaded )
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

		// Check that the user hasn't openned a binary file by checking the first
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
													   // and = . No spaced valuename permited.
					value = String::lTrim( line.substr( pLeft + 1 ), "\t " );
					setValue( keyname, valuename, value );
					break;
				case ';':
				case '#':
					if ( !mNames.size() )
						addHeaderComment( std::string{ line.substr( pLeft + 1 ) } );
					else
						addKeyComment( keyname, std::string{ line.substr( pLeft + 1 ) } );
					break;
			}
		}
	}

	if ( mNames.size() ) {
		mIniReaded = true;

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

	unsigned commentID, keyID, valueID;
	std::string str;

	// Write header mComments.
	for ( commentID = 0; commentID < mComments.size(); ++commentID ) {
		str = ';' + mComments[commentID] + '\n';
		stream.write( str.c_str(), str.size() );
	}

	if ( mComments.size() ) {
		str = "\n";
		stream.write( str.c_str(), str.size() );
	}

	// Write Keys and values.
	for ( keyID = 0; keyID < mKeys.size(); ++keyID ) {
		str = '[' + mNames[keyID] + ']' + '\n';
		stream.write( str.c_str(), str.size() );

		// Comments.
		for ( commentID = 0; commentID < mKeys[keyID].comments.size(); ++commentID ) {
			str = ';' + mKeys[keyID].comments[commentID] + '\n';
			stream.write( str.c_str(), str.size() );
		}

		// Values.
		for ( valueID = 0; valueID < mKeys[keyID].names.size(); ++valueID ) {
			str = mKeys[keyID].names[valueID] + '=' + mKeys[keyID].values[valueID] + '\n';
			stream.write( str.c_str(), str.size() );
		}

		str = "\n";
		stream.write( str.c_str(), str.size() );
	}

	return true;
}

long IniFile::findKey( const std::string& keyname ) const {
	for ( unsigned keyID = 0; keyID < mNames.size(); ++keyID )
		if ( mNames[keyID] == keyname )
			return long( keyID );
	return noID;
}

long IniFile::findKey( const std::string_view& keyname ) const {
	for ( unsigned keyID = 0; keyID < mNames.size(); ++keyID )
		if ( mNames[keyID] == keyname )
			return long( keyID );
	return noID;
}

long IniFile::findValue( unsigned const keyID, const std::string& valuename ) const {
	if ( !mKeys.size() || keyID >= mKeys.size() )
		return noID;

	for ( unsigned valueID = 0; valueID < mKeys[keyID].names.size(); ++valueID )
		if ( mKeys[keyID].names[valueID] == valuename )
			return long( valueID );
	return noID;
}

long IniFile::findValue( const unsigned int keyID, const std::string_view& valuename ) const {
	if ( !mKeys.size() || keyID >= mKeys.size() )
		return noID;

	for ( unsigned valueID = 0; valueID < mKeys[keyID].names.size(); ++valueID )
		if ( mKeys[keyID].names[valueID] == valuename )
			return long( valueID );
	return noID;
}

unsigned IniFile::addKeyName( const std::string& keyname ) {
	mNames.resize( mNames.size() + 1, keyname );
	mKeys.resize( mKeys.size() + 1 );
	return (unsigned int)( mNames.size() - 1 );
}

unsigned int IniFile::addKeyName( const std::string_view& keyname ) {
	mNames.resize( mNames.size() + 1, std::string{ keyname } );
	mKeys.resize( mKeys.size() + 1 );
	return (unsigned int)( mNames.size() - 1 );
}

std::string IniFile::getKeyName( unsigned const keyID ) const {
	if ( keyID < mNames.size() )
		return mNames[keyID];
	else
		return "";
}

unsigned IniFile::getNumValues( unsigned const keyID ) {
	if ( keyID < mKeys.size() )
		return (unsigned int)mKeys[keyID].names.size();
	return 0;
}

unsigned IniFile::getNumValues( const std::string& keyname ) {
	long keyID = findKey( keyname );
	if ( keyID == noID )
		return 0;
	return (unsigned int)mKeys[keyID].names.size();
}

std::string IniFile::getValueName( unsigned const keyID, unsigned const valueID ) const {
	if ( keyID < mKeys.size() && valueID < mKeys[keyID].names.size() )
		return mKeys[keyID].names[valueID];
	return "";
}

std::string IniFile::getValueName( const std::string& keyname, unsigned const valueID ) const {
	long keyID = findKey( keyname );
	if ( keyID == noID )
		return "";
	return getValueName( keyID, valueID );
}

bool IniFile::setValue( unsigned const keyID, unsigned const valueID, const std::string& value ) {
	if ( keyID < mKeys.size() && valueID < mKeys[keyID].names.size() )
		mKeys[keyID].values[valueID] = value;

	return false;
}

bool IniFile::setValue( const std::string& keyname, const std::string& valuename,
						const std::string& value, bool create ) {
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
		mKeys[keyID].names.resize( mKeys[keyID].names.size() + 1, valuename );
		mKeys[keyID].values.resize( mKeys[keyID].values.size() + 1, value );
	} else
		mKeys[keyID].values[valueID] = value;

	return true;
}

bool IniFile::setValue( const std::string_view& keyname, const std::string_view& valuename,
						const std::string_view& value, bool create ) {
	long keyID = findKey( keyname );
	if ( keyID == noID ) {
		if ( create )
			keyID = long( addKeyName( std::string{ keyname } ) );
		else
			return false;
	}

	long valueID = findValue( unsigned( keyID ), valuename );
	if ( valueID == noID ) {
		if ( !create )
			return false;
		mKeys[keyID].names.resize( mKeys[keyID].names.size() + 1, std::string{ valuename } );
		mKeys[keyID].values.resize( mKeys[keyID].values.size() + 1, std::string{ value } );
	} else
		mKeys[keyID].values[valueID] = value;

	return true;
}

bool IniFile::setValueI( const std::string& keyname, const std::string& valuename, int const value,
						 bool create ) {
	char svalue[MAX_VALUEDATA];

	String::formatBuffer( svalue, MAX_VALUEDATA, "%d", value );
	return setValue( keyname, valuename, svalue, create );
}

bool IniFile::setValueU( const std::string& keyname, const std::string& valuename,
						 const unsigned long value, bool create ) {
	char svalue[MAX_VALUEDATA];

	String::formatBuffer( svalue, MAX_VALUEDATA, "%u", value );
	return setValue( keyname, valuename, svalue, create );
}

bool IniFile::setValueF( const std::string& keyname, const std::string& valuename,
						 double const value, bool create ) {
	char svalue[MAX_VALUEDATA];

	String::formatBuffer( svalue, MAX_VALUEDATA, "%f", value );
	return setValue( keyname, valuename, svalue, create );
}

bool IniFile::setValueV( const std::string& keyname, const std::string& valuename, char* format,
						 ... ) {
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
							   const std::string& defValue ) const {
	if ( keyID < mKeys.size() && valueID < mKeys[keyID].names.size() )
		return mKeys[keyID].values[valueID];
	return defValue;
}

std::string IniFile::getValue( const std::string& keyname, const std::string& valuename,
							   const std::string& defValue ) const {
	long keyID = findKey( keyname );
	if ( keyID == noID )
		return defValue;

	long valueID = findValue( unsigned( keyID ), valuename );
	if ( valueID == noID )
		return defValue;

	return mKeys[keyID].values[valueID];
}

int IniFile::getValueI( const std::string& keyname, const std::string& valuename,
						int const defValue ) const {
	char svalue[MAX_VALUEDATA];

	String::formatBuffer( svalue, MAX_VALUEDATA, "%d", defValue );
	return atoi( getValue( keyname, valuename, svalue ).c_str() );
}

unsigned long IniFile::getValueU( const std::string& keyname, const std::string& valuename,
								  const unsigned long defValue ) const {
	char svalue[MAX_VALUEDATA];

	String::formatBuffer( svalue, MAX_VALUEDATA, "%u", defValue );
	return atoi( getValue( keyname, valuename, svalue ).c_str() );
}

bool IniFile::getValueB( const std::string& keyname, const std::string& valuename,
						 const bool defValue ) const {
	std::string val = getValue( keyname, valuename, defValue ? "1" : "0" );
	char fist = !val.empty() ? val[0] : '0';
	return fist == '1' || fist == 't' || fist == 'y' || fist == 'T' || fist == 'Y';
}

double IniFile::getValueF( const std::string& keyname, const std::string& valuename,
						   double const defValue ) const {
	char svalue[MAX_VALUEDATA];

	String::formatBuffer( svalue, MAX_VALUEDATA, "%f", defValue );
	return atof( getValue( keyname, valuename, svalue ).c_str() );
}

bool IniFile::deleteValue( const std::string& keyname, const std::string& valuename ) {
	long keyID = findKey( keyname );
	if ( keyID == noID )
		return false;

	long valueID = findValue( unsigned( keyID ), valuename );
	if ( valueID == noID )
		return false;

	// This looks strange, but is neccessary.
	std::vector<std::string>::iterator npos = mKeys[keyID].names.begin() + valueID;
	std::vector<std::string>::iterator vpos = mKeys[keyID].values.begin() + valueID;
	mKeys[keyID].names.erase( npos, npos + 1 );
	mKeys[keyID].values.erase( vpos, vpos + 1 );

	return true;
}

bool IniFile::deleteKey( const std::string& keyname ) {
	long keyID = findKey( keyname );
	if ( keyID == noID )
		return false;

	std::vector<std::string>::iterator npos = mNames.begin() + keyID;
	std::vector<key>::iterator kpos = mKeys.begin() + keyID;
	mNames.erase( npos, npos + 1 );
	mKeys.erase( kpos, kpos + 1 );

	return true;
}

void IniFile::clear() {
	mIniReaded = false;
	mNames.clear();
	mKeys.clear();
	mComments.clear();
}

void IniFile::addHeaderComment( const std::string& comment ) {
	mComments.resize( mComments.size() + 1, comment );
}

std::string IniFile::getHeaderComment( unsigned const commentID ) const {
	if ( commentID < mComments.size() )
		return mComments[commentID];
	return "";
}

bool IniFile::deleteHeaderComment( unsigned commentID ) {
	if ( commentID < mComments.size() ) {
		std::vector<std::string>::iterator cpos = mComments.begin() + commentID;
		mComments.erase( cpos, cpos + 1 );
		return true;
	}
	return false;
}

std::map<std::string, std::string> IniFile::getKeyMap( const unsigned& keyID ) const {
	std::map<std::string, std::string> map;
	if ( keyID < mKeys.size() ) {
		for ( size_t i = 0; i < mKeys[keyID].names.size(); i++ ) {
			map[mKeys[keyID].names[i]] = mKeys[keyID].values[i];
		}
		return map;
	}
	return {};
}

std::map<std::string, std::string> IniFile::getKeyMap( const std::string& keyname ) const {
	long keyID = findKey( keyname );
	if ( keyID != noID )
		return getKeyMap( keyID );
	return {};
}

std::unordered_map<std::string, std::string>
IniFile::getKeyUnorderedMap( const unsigned& keyID ) const {
	std::unordered_map<std::string, std::string> map;
	if ( keyID < mKeys.size() ) {
		for ( size_t i = 0; i < mKeys[keyID].names.size(); i++ ) {
			map[mKeys[keyID].names[i]] = mKeys[keyID].values[i];
		}
		return map;
	}
	return {};
}

std::unordered_map<std::string, std::string>
IniFile::getKeyUnorderedMap( const std::string& keyname ) const {
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

unsigned IniFile::getNumKeyComments( const std::string& keyname ) const {
	long keyID = findKey( keyname );
	if ( keyID == noID )
		return 0;
	return (unsigned int)mKeys[keyID].comments.size();
}

bool IniFile::addKeyComment( unsigned const keyID, const std::string& comment ) {
	if ( keyID < mKeys.size() ) {
		mKeys[keyID].comments.resize( mKeys[keyID].comments.size() + 1, comment );
		return true;
	}
	return false;
}

bool IniFile::addKeyComment( unsigned const keyID, const std::string_view& comment ) {
	if ( keyID < mKeys.size() ) {
		mKeys[keyID].comments.resize( mKeys[keyID].comments.size() + 1, std::string{ comment } );
		return true;
	}
	return false;
}

bool IniFile::addKeyComment( const std::string& keyname, const std::string& comment ) {
	long keyID = findKey( keyname );
	if ( keyID == noID )
		return false;
	return addKeyComment( unsigned( keyID ), comment );
}

bool IniFile::addKeyComment( const std::string_view& keyname, const std::string_view& comment ) {
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

std::string IniFile::getKeyComment( const std::string& keyname, unsigned const commentID ) const {
	long keyID = findKey( keyname );
	if ( keyID == noID )
		return "";
	return getKeyComment( unsigned( keyID ), commentID );
}

bool IniFile::deleteKeyComment( unsigned const keyID, unsigned const commentID ) {
	if ( keyID < mKeys.size() && commentID < mKeys[keyID].comments.size() ) {
		std::vector<std::string>::iterator cpos = mKeys[keyID].comments.begin() + commentID;
		mKeys[keyID].comments.erase( cpos, cpos + 1 );
		return true;
	}
	return false;
}

bool IniFile::deleteKeyComment( const std::string& keyname, unsigned const commentID ) {
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

bool IniFile::deleteKeyComments( const std::string& keyname ) {
	long keyID = findKey( keyname );
	if ( keyID == noID )
		return false;
	return deleteKeyComments( unsigned( keyID ) );
}

bool IniFile::keyExists( const std::string& keyname ) const {
	return findKey( keyname ) != noID;
}

bool IniFile::keyValueExists( const std::string& keyname, const std::string& valuename ) const {
	long keyID = findKey( keyname );
	if ( keyID == noID )
		return false;

	long valueID = findValue( unsigned( keyID ), valuename );
	if ( valueID == noID )
		return false;

	return true;
}

}} // namespace EE::System
