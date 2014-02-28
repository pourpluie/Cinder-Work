#include "cinder/Log.h"
#include "cinder/CinderAssert.h"
#include "cinder/Utilities.h"
#include "cinder/app/App.h"

#if defined( CINDER_COCOA )
	#import <Foundation/Foundation.h>
#endif

#include <mutex>

#define DEFAULT_FILE_LOG_PATH "cinder.log"

// TODO: consider storing Logger's as shared_ptr instead
//	- they really aren't shared, but makes swapping them in and out and LogManager's handles easier

using namespace std;

namespace cinder { namespace log {

class LoggerImplMulti : public Logger {
public:

	void add( Logger *logger )							{ mLoggers.push_back( std::unique_ptr<Logger>( logger ) ); }
	void add( std::unique_ptr<Logger> &&logger )		{ mLoggers.emplace_back( move( logger ) ); }

	template <typename LoggerT>
	Logger* findType();

	void remove( Logger *logger );

	const vector<unique_ptr<Logger> >& getLoggers() const	{ return mLoggers; }

	virtual void write( const Metadata &meta, const std::string &text ) override;

private:
	vector<unique_ptr<Logger> >	mLoggers; // TODO: make set? don't want duplicates
};

// ----------------------------------------------------------------------------------------------------
// MARK: - LogManager
// ----------------------------------------------------------------------------------------------------

LogManager*	LogManager::sInstance = new LogManager;	// note: leaks to enable logging during shutdown

LogManager* manager()
{
	return LogManager::instance();
}

LogManager::LogManager()
{
	restoreToDefault();
}

void LogManager::resetLogger( Logger *logger )
{
	lock_guard<mutex> lock( mMutex );

	mLogger.reset( logger );

	LoggerImplMulti *multi = dynamic_cast<LoggerImplMulti *>( logger );
	mLoggerMulti = multi ? multi : nullptr;

	mConsoleLoggingEnabled = mFileLoggingEnabled = mSystemLoggingEnabled = false;
}

void LogManager::addLogger( Logger *logger )
{
	lock_guard<mutex> lock( mMutex );

	if( ! mLoggerMulti ) {
		auto loggerMulti = unique_ptr<LoggerImplMulti>( new LoggerImplMulti );
		loggerMulti->add( move( mLogger ) );
		mLoggerMulti = loggerMulti.get();
		mLogger = move( loggerMulti );
	}

	mLoggerMulti->add( logger );
}


void LogManager::removeLogger( Logger *logger )
{
	CI_ASSERT( mLoggerMulti );

	mLoggerMulti->remove( logger );
}

void LogManager::restoreToDefault()
{
	lock_guard<mutex> lock( mMutex );

	mLogger.reset( new LoggerConsoleThreadSafe );
	mLoggerMulti = nullptr;
	mConsoleLoggingEnabled = true;
	mFileLoggingEnabled = false;
	mSystemLoggingEnabled = false;
}

vector<Logger *> LogManager::getAllLoggers()
{
	vector<Logger *> result;

	if( mLoggerMulti ) {
		for( const auto &logger : mLoggerMulti->getLoggers() )
			result.push_back( logger.get() );
	}
	else
		result.push_back( mLogger.get() );

	return result;
}

void LogManager::enableConsoleLogging()
{
	if( mConsoleLoggingEnabled )
		return;

	addLogger( new LoggerConsoleThreadSafe );
	mConsoleLoggingEnabled = true;
}
	
void LogManager::enableFileLogging( const fs::path &path )
{
	if( mFileLoggingEnabled ) {
		// if path has changed, destroy previous file logger and make a new one
		auto logger = mLoggerMulti->findType<LoggerFile>();
		if( logger )
			disableFileLogging();
		else
			return;
	}

	addLogger( new LoggerFileThreadSafe( path ) );
	mFileLoggingEnabled = true;
}

void LogManager::enableSystemLogging()
{
	if( mSystemLoggingEnabled )
		return;

#if defined( CINDER_COCOA )
	addLogger( new LoggerNSLog );
	mSystemLoggingEnabled = true;
#endif
}

void LogManager::disableConsoleLogging()
{
	if( ! mConsoleLoggingEnabled || ! mLoggerMulti )
		return;

	auto logger = mLoggerMulti->findType<LoggerConsole>();
	mLoggerMulti->remove( logger );

	mConsoleLoggingEnabled = false;
}

void LogManager::disableFileLogging()
{
	if( ! mFileLoggingEnabled || ! mLoggerMulti )
		return;

	auto logger = mLoggerMulti->findType<LoggerFile>();
	mLoggerMulti->remove( logger );

	mFileLoggingEnabled = false;
}

void LogManager::disableSystemLogging()
{
	if( ! mSystemLoggingEnabled || ! mLoggerMulti )
		return;

#if defined( CINDER_COCOA )
	auto logger = mLoggerMulti->findType<LoggerNSLog>();
	mLoggerMulti->remove( logger );
	mSystemLoggingEnabled = false;
#endif
}

// ----------------------------------------------------------------------------------------------------
// MARK: - LoggerConsole
// ----------------------------------------------------------------------------------------------------

void LoggerConsole::write( const Metadata &meta, const string &text )
{
	app::console() << meta << text << endl;
}

// ----------------------------------------------------------------------------------------------------
// MARK: - LoggerImplMulti
// ----------------------------------------------------------------------------------------------------

template <typename LoggerT>
Logger* LoggerImplMulti::findType()
{
	for( const auto &logger : mLoggers ) {
		if( dynamic_cast<LoggerT *>( logger.get() ) )
			return logger.get();
	}

	return nullptr;
}

void LoggerImplMulti::remove( Logger *logger )
{
	mLoggers.erase( remove_if( mLoggers.begin(), mLoggers.end(),
							  [logger]( const std::unique_ptr<Logger> &o ) {
								  return o.get() == logger;
							  } ),
				   mLoggers.end() );
}

void LoggerImplMulti::write( const Metadata &meta, const string &text )
{
	for( auto &logger : mLoggers )
		logger->write( meta, text );
}

// ----------------------------------------------------------------------------------------------------
// MARK: - LoggerFile
// ----------------------------------------------------------------------------------------------------

LoggerFile::LoggerFile( const fs::path &filePath )
	: mFilePath( filePath )
{
	if( mFilePath.empty() )
		mFilePath = DEFAULT_FILE_LOG_PATH;
	
	mStream.open( mFilePath.string() );
}

LoggerFile::~LoggerFile()
{
	mStream.close();
}

void LoggerFile::write( const Metadata &meta, const string &text )
{
	mStream << meta << text << endl;
}

#if defined( CINDER_COCOA )

// ----------------------------------------------------------------------------------------------------
// MARK: - LoggerNSLog
// ----------------------------------------------------------------------------------------------------

void LoggerNSLog::write( const Metadata &meta, const string& text )
{
	NSString *textNs = [NSString stringWithCString:text.c_str() encoding:NSUTF8StringEncoding];
	NSString *metaDataNs = [NSString stringWithCString:meta.toString().c_str() encoding:NSUTF8StringEncoding];
	NSLog( @"%@%@", metaDataNs, textNs );
}

#endif

// ----------------------------------------------------------------------------------------------------
// MARK: - Helper Classes
// ----------------------------------------------------------------------------------------------------

string Metadata::toString() const
{
	stringstream ss;
	ss << *this;
	return ss.str();
}

ostream& operator<<( ostream &os, const Metadata &rhs )
{
	os << "|" << rhs.mLevel << "| " << rhs.mLocation.getFunctionName() << "[" << rhs.mLocation.getLineNumber() << "] | ";
	return os;
}

ostream& operator<<( ostream &lhs, const Level &rhs )
{
	switch( rhs ) {
		case LEVEL_VERBOSE:		lhs << "verbose";	break;
		case LEVEL_INFO:			lhs << "info";		break;
		case LEVEL_WARNING:		lhs << "WARNING";	break;
		case LEVEL_ERROR:			lhs << "ERROR";		break;
		case LEVEL_FATAL:			lhs << "FATAL";		break;
		default: CI_ASSERT_NOT_REACHABLE();
	}
	return lhs;
}

} } // namespace cinder::log
