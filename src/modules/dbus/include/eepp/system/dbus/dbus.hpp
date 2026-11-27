#ifndef EE_SYSTEM_DBUS_DBUS_HPP
#define EE_SYSTEM_DBUS_DBUS_HPP

#include <dbus/dbus.h>
#include <eepp/config.hpp>
#include <memory>
#include <string_view>

namespace EE::System::Dbus {

class DbusTransportLibrary;

/** An iterator borrowed from its DbusMessage. The DbusMessage must remain alive. */
class DbusReader {
  public:
	DbusReader child( int type ) const;

	bool next();

	int type() const;

	int elementType() const;

	const char* string( int type = DBUS_TYPE_STRING ) const;

	bool read( Uint32& value ) const;

  private:
	friend class DbusMessage;
	explicit DbusReader( const DbusTransportLibrary& library ) : mLibrary( &library ) {}
	const DbusTransportLibrary* mLibrary;
	mutable DBusMessageIter mIter = DBUS_MESSAGE_ITER_INIT_CLOSED;
	bool mValid{ false };
};

/** An append iterator borrowed from its unsent DbusMessage. */
class DbusWriter {
  public:
	bool string( const char* value, int type = DBUS_TYPE_STRING );

	bool write( Uint32 value );

	template <typename F> bool container( int type, const char* signature, F append ) {
		DbusWriter child( *mLibrary );
		if ( !open( child, type, signature ) )
			return false;
		if ( append( child ) && close( child ) )
			return true;
		abandon( child );
		return false;
	}

  private:
	friend class DbusMessage;
	explicit DbusWriter( const DbusTransportLibrary& library ) : mLibrary( &library ) {}

	bool open( DbusWriter& child, int type, const char* signature );

	bool close( DbusWriter& child );

	void abandon( DbusWriter& child );

	const DbusTransportLibrary* mLibrary;
	DBusMessageIter mIter = DBUS_MESSAGE_ITER_INIT_CLOSED;
};

class DbusMessage {
  public:
	DbusMessage() = default;

	~DbusMessage();

	DbusMessage( DbusMessage&& other ) noexcept;

	DbusMessage& operator=( DbusMessage&& other ) noexcept;

	explicit operator bool() const { return mMessage != nullptr; }

	DbusReader read() const;

	DbusWriter write();

	std::string_view sender() const;

	std::string_view path() const;

	std::string_view error() const;

	bool signature( const char* value ) const;

	bool signal( const char* interface, const char* member ) const;

  private:
	friend class DbusConnection;
	DbusMessage( const DbusTransportLibrary* library, DBusMessage* message );

	const DbusTransportLibrary* mLibrary{ nullptr };
	DBusMessage* mMessage{ nullptr };
};

/** A private session connection serviced by one owner thread. Only wake() is cross-thread. */
class DbusConnection {
  public:
	enum class Step { Idle, Ready, Failed };

	DbusConnection();

	~DbusConnection();

	bool open();

	void close();

	bool connected() const;

	std::string_view uniqueName() const;

	DbusMessage method( const char* destination, const char* path, const char* interface,
						const char* member ) const;

	/** Start one call with a finite method-reply timeout. */
	bool call( DbusMessage message, int milliseconds = 5000 );

	void cancel();

	DbusMessage reply();

	DbusMessage signal();

	/** Wait for readiness and dispatch at most one message. A caller may interrupt it via wake().
	 */
	Step wait();

	void wake() noexcept;

  private:
	struct Impl;
	std::unique_ptr<Impl> mImpl;
};

} // namespace EE::System::Dbus

#endif
