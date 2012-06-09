#ifndef EE_SYSTEMCRC4_H
#define EE_SYSTEMCRC4_H

#include <eepp/system/base.hpp>

namespace EE { namespace System {

/** @brief RC4 Encryption Class. For more information check Wikipedia: http://en.wikipedia.org/wiki/RC4. \n All the Decrypting functions call the Encrypting functions, there are there only for clarity, not really usefull. Use them as you wish.  */
class EE_API cRC4 {
	public:
		cRC4();
		~cRC4();
		
		/** Set the encryption Key. */
		void SetKey( const std::vector<Uint8>& Key );
		
		/** Set the encryption Key. */
		void SetKey( const std::string& Key );
		
		void EncryptByte( std::vector<Uint8>& buffer, const std::vector<Uint8>& Key );
		void EncryptByte( std::vector<Uint8>& buffer, const std::string& Key );
		
		/** Ecrypt a vector of bytes if the key is already set it. */
		void EncryptByte( std::vector<Uint8>& buffer );
		
		void DecryptByte( std::vector<Uint8>& buffer, const std::vector<Uint8>& Key );
		void DecryptByte( std::vector<Uint8>& buffer, const std::string& Key );
		void DecryptByte( std::vector<Uint8>& buffer );
		
		void EncryptString( std::string& buffer, const std::vector<Uint8>& Key );
		void EncryptString( std::string& buffer, const std::string& Key );
		
		/** Ecrypt a string if the key is already set it. */
		void EncryptString( std::string& buffer );
		
		void DecryptString( std::string& buffer, const std::vector<Uint8>& Key );
		void DecryptString( std::string& buffer, const std::string& Key );
		void DecryptString( std::string& buffer );
		
		bool EncryptFile( const std::string& SourceFile, const std::string& DestFile, const std::vector<Uint8>& Key );
		bool EncryptFile( const std::string& SourceFile, const std::string& DestFile, const std::string& Key );
		
		/** Ecrypt a file if the key is already set it. */
		bool EncryptFile( const std::string& SourceFile, const std::string& DestFile );
		
		bool DecryptFile( const std::string& SourceFile, const std::string& DestFile, const std::vector<Uint8>& Key );
		bool DecryptFile( const std::string& SourceFile, const std::string& DestFile, const std::string& Key );
		bool DecryptFile( const std::string& SourceFile, const std::string& DestFile );
	private:
		typedef struct _RC4Key {
			Uint8 state[256];
		} RC4Key;
		RC4Key mKey;
		
		void Swap( Uint8& a, Uint8& b );
};

}}

#endif
