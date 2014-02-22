#pragma once

#include "cinder/Cinder.h"
#include "cinder/CurrentFunction.h"

#include <sstream>
#include <vector>
#include <memory>
#include <mutex>

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

class Logger;
struct Entry;

Logger* logger();
void reset( Logger *logger );
void add( Logger *logger );


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

//typedef std::shared_ptr<class Logger>	LoggerRef;

class Logger {
  public:
	Logger()	{}
	virtual ~Logger()	{}

	virtual void write( const Metadata &meta, const std::string &text );
};

/*
class LoggerFile : public Logger {
	static std::string logFileLocation()	{ return "."; }
	// TODO:
	virtual void writeToLog( const Metadata &metadata, const std::string &text ) override {}
};
*/

#if defined( CINDER_COCOA )

//! sends output to NSLog so it can be viewed from the Mac Console app
// TODO: this could probably be much faster with syslog: https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man3/syslog.3.html
class LoggerNSLog : public Logger {
  public:
	virtual void write( const Metadata &meta, const std::string& text ) override;
};

#endif

extern std::mutex	sMutex;

template<class LoggerT>
class ThreadSafeT : public LoggerT {
  public:

	virtual void write( const Metadata &meta, const std::string &text ) override
	{
		std::lock_guard<std::mutex> lock( sMutex );
		LoggerT::write( meta, text );
	}
};

typedef ThreadSafeT<Logger>			LoggerThreadSafe;

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
		logger()->write( mMetaData, mStream.str() );
	}

	const Metadata&	getMetaData() const	{ return mMetaData; }

private:

	Metadata			mMetaData;
	bool				mHasContent;
	std::stringstream	mStream;
};

} } // namespace cinder::log
