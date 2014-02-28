#pragma once

#include "cinder/Cinder.h"
#include "cinder/Filesystem.h"
#include "cinder/CurrentFunction.h"
#include "cinder/CinderAssert.h"

#include <sstream>
#include <fstream>
#include <vector>
#include <memory>
#include <mutex>

#if defined( CINDER_MSW ) && ( _MSC_VER < 1800 )
	#define CINDER_NO_VARIADIC_TEMPLATES
	#include <boost/preprocessor/repetition.hpp>
	#include <boost/preprocessor/control/if.hpp>
#endif

#if ( defined( CINDER_COCOA ) && DEBUG ) || ( defined( CINDER_MSW ) && defined( _DEBUG ) )
	// by default, CI_LOG_V is enabled in debug builds, but apps can also define this elsewhere to force it on in release.
	#define CI_ENABLE_LOG_V
#endif

#define CINDER_LOG_STREAM( level, stream ) ::cinder::log::Entry( level, ::cinder::log::Location( CINDER_CURRENT_FUNCTION, __FILE__, __LINE__ ) ) << stream

#if defined( CI_ENABLE_LOG_V )
	#define CI_LOG_V( stream )	CINDER_LOG_STREAM( ::cinder::log::Level::VERBOSE, stream )
#else
	#define CI_LOG_V( stream )
#endif

#define CI_LOG_I( stream )	CINDER_LOG_STREAM( ::cinder::log::Level::INFO, stream )
#define CI_LOG_W( stream )	CINDER_LOG_STREAM( ::cinder::log::Level::WARNING, stream )
#define CI_LOG_E( stream )	CINDER_LOG_STREAM( ::cinder::log::Level::ERROR, stream )
#define CI_LOG_F( stream )	CINDER_LOG_STREAM( ::cinder::log::Level::FATAL, stream )

namespace cinder { namespace log {

enum Level {
	VERBOSE,
	INFO,
	WARNING,
	ERROR,
	FATAL
};

extern std::ostream& operator<<( std::ostream &lhs, const Level &rhs );

struct Location {
	Location() {}

	Location( const std::string &functionName, const std::string &fileName, const size_t &lineNumber )
		: mFunctionName( functionName ), mFileName( fileName ), mLineNumber( lineNumber )
	{}

	const std::string&	getFileName() const				{ return mFileName; }
	const std::string&	getFunctionName() const			{ return mFunctionName; }
	size_t				getLineNumber() const			{ return mLineNumber; }

private:
	std::string mFunctionName, mFileName;
	size_t		mLineNumber;
};

struct Metadata {

	std::string toString() const;

	Level		mLevel;
	Location	mLocation;

	friend std::ostream& operator<<( std::ostream &os, const Metadata &rhs );
};

class Logger {
  public:
	virtual ~Logger()	{}

	virtual void write( const Metadata &meta, const std::string &text ) = 0;
};

class LoggerConsole : public Logger {
  public:
	virtual ~LoggerConsole()	{}

	virtual void write( const Metadata &meta, const std::string &text ) override;
};

class LoggerFile : public Logger {
  public:
	//! If \a filePath is empty, uses the default ('cinder.log' next to app binary)
	LoggerFile( const fs::path &filePath = fs::path() );
	virtual ~LoggerFile();

	virtual void write( const Metadata &meta, const std::string &text ) override;

	const fs::path&		getFilePath() const		{ return mFilePath; }

  protected:
	fs::path		mFilePath;
	std::ofstream	mStream;
};

#if defined( CINDER_COCOA )

//! sends output to NSLog so it can be viewed from the Mac Console app
// TODO: this could probably be much faster with syslog: https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man3/syslog.3.html
class LoggerNSLog : public Logger {
  public:
	virtual void write( const Metadata &meta, const std::string& text ) override;
};

#endif

class LoggerImplMulti;

class LogManager {
public:
	// Returns a pointer to the shared instance. To enable logging during shutdown, this instance is leaked at shutdown.
	static LogManager* instance()	{ return sInstance; }
	//! Destroys the shared instance. Useful to remove false positives with leak detectors like valgrind.
	static void destroyInstance()	{ delete sInstance; }
	//! Restores LogManager to its default state.
	void restoreToDefault();

	//! Resets the current Logger stack so only \a logger exists.
	void resetLogger( Logger *logger );
	//! Adds \a logger to the current stack of loggers.
	void addLogger( Logger *logger );
	//! Remove \a logger to the current stack of loggers.
	void removeLogger( Logger *logger );
	//! Returns a pointer to the current base Logger instance.
	Logger* getLogger()	{ return mLogger.get(); }
	//! Returns a vector of all current loggers
	std::vector<Logger *> getAllLoggers();
	//! Returns the mutex used for thread safe loggers. Also used when adding or resetting new loggers.
	std::mutex& getMutex() const			{ return mMutex; }

	void enableConsoleLogging();
	void disableConsoleLogging();
	void setConsoleLoggingEnabled( bool b = true )		{ b ? enableConsoleLogging() : disableConsoleLogging(); }
	bool isConsoleLoggingEnabled() const				{ return mConsoleLoggingEnabled; }

	void enableFileLogging( const fs::path &filePath = fs::path() );
	void disableFileLogging();
	void setFileLoggingEnabled( bool b = true, const fs::path &filePath = fs::path() )			{ b ? enableFileLogging( filePath ) : disableFileLogging(); }
	bool isFileLoggingEnabled() const					{ return mFileLoggingEnabled; }

	void enableSystemLogging();
	void disableSystemLogging();
	void setSystemLoggingEnabled( bool b = true )		{ b ? enableSystemLogging() : disableSystemLogging(); }
	bool isSystemLoggingEnabled() const					{ return mSystemLoggingEnabled; }

protected:
	LogManager();

	std::unique_ptr<Logger>	mLogger;
	LoggerImplMulti			*mLoggerMulti;
	mutable std::mutex		mMutex;
	bool					mConsoleLoggingEnabled, mFileLoggingEnabled, mSystemLoggingEnabled;

	static LogManager *sInstance;
};

LogManager* manager();

struct Entry {
	// TODO: move &&location
	Entry( Level level, const Location &location )
		: mHasContent( false )
	{
		mMetaData.mLevel = level;
		mMetaData.mLocation = location;
	}

	~Entry()
	{
		if( mHasContent )
			writeToLog();
	}

	template <typename T>
	Entry& operator<<( T rhs )
	{
		mHasContent = true;
		mStream << rhs;
		return *this;
	}

	void writeToLog()
	{
		manager()->getLogger()->write( mMetaData, mStream.str() );
	}

	const Metadata&	getMetaData() const	{ return mMetaData; }

private:

	Metadata			mMetaData;
	bool				mHasContent;
	std::stringstream	mStream;
};


template<class LoggerT>
class ThreadSafeT : public LoggerT {
  public:

#if ! defined( CINDER_NO_VARIADIC_TEMPLATES )
	template <typename... Args>
	ThreadSafeT( Args &&... args )
	: LoggerT( std::forward<Args>( args )... )
	{}
#else
#define CTOR(z, n, unused)														\
	BOOST_PP_IF( n, template <, ) BOOST_PP_ENUM_PARAMS( n, typename Arg )		\
		BOOST_PP_IF(n, >, )														\
			ThreadSafeT( BOOST_PP_ENUM_BINARY_PARAMS( n, Arg, arg ) )			\
				: LoggerT( BOOST_PP_ENUM_PARAMS( n, arg ) )						\
			{}

	BOOST_PP_REPEAT(5, CTOR, ~)
#undef CTOR
#endif

	virtual void write( const Metadata &meta, const std::string &text ) override
	{
		std::lock_guard<std::mutex> lock( manager()->getMutex() );
		LoggerT::write( meta, text );
	}
};

typedef ThreadSafeT<LoggerConsole>		LoggerConsoleThreadSafe;
typedef ThreadSafeT<LoggerFile>			LoggerFileThreadSafe;

} } // namespace cinder::log