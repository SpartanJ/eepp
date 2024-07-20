#ifndef EE_WINDOW_PLATFORM_HPP
#define EE_WINDOW_PLATFORM_HPP

#include <eepp/config.hpp>
#include <string>

namespace EE { namespace Window {

class EE_API PlatformHelper {
  public:
	virtual ~PlatformHelper() {}

	/** Open a URL in a separate, system-provided application.
	 * @return true if success
	 */
	virtual bool openURL( const std::string& url ) = 0;

	virtual char* iconv( const char* tocode, const char* fromcode, const char* inbuf,
						 size_t inbytesleft ) = 0;

	virtual void iconvFree( char* buf ) = 0;

#if EE_PLATFORM == EE_PLATFORM_ANDROID
	/** @return The Activity object for the application */
	virtual void* getActivity() = 0;

	/** @return The JNI environment for the current thread */
	virtual void* getJNIEnv() = 0;

	/** @return The path used for external storage for this application.
	 * This path is unique to your application, but is public and can be
	 * written to by other applications.
	 */
	virtual std::string getExternalStoragePath() = 0;

	/** @return The path used for internal storage for this application.
	 * This path is unique to your application and cannot be written to
	 * by other applications.
	 */
	virtual std::string getInternalStoragePath() = 0;

	/** @return The application APK file path */
	virtual std::string getApkPath() = 0;

	/** @return True if the external storage is readable. */
	virtual bool isExternalStorageReadable() = 0;

	/** @return True if the external storage is writeable. */
	virtual bool isExternalStorageWritable() = 0;
#endif
};

}} // namespace EE::Window

#endif
