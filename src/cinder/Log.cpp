#include "cinder/Log.h"
#include "cinder/CinderAssert.h"
#include "cinder/Utilities.h"
#include "cinder/app/App.h"

#if defined( CINDER_COCOA )
	#import <Foundation/Foundation.h>
#endif

#include <mutex>

using namespace std;

namespace cinder { namespace log {

class LoggerImplMulti : public Logger {
public:

	void add( Logger *logger )					{ mLoggers.push_back( unique_ptr<Logger>( logger ) ); }
	void add( unique_ptr<Logger> &&logger )		{ mLoggers.emplace_back( move( logger ) ); }

	virtual void write( const Metadata &meta, const std::string &text ) override;

protected:
	std::vector<std::unique_ptr<Logger> >	mLoggers;
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
	: mLogger( new LoggerThreadSafe ), mLoggerMulti( nullptr )
{
}

void LogManager::resetLogger( Logger *logger )
{
	lock_guard<mutex> lock( mMutex );

	mLogger.reset( logger );

	LoggerImplMulti *multi = dynamic_cast<LoggerImplMulti *>( logger );
	mLoggerMulti = multi ? multi : nullptr;
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

// ----------------------------------------------------------------------------------------------------
// MARK: - Logger
// ----------------------------------------------------------------------------------------------------

void Logger::write( const Metadata &meta, const string &text )
{
	app::console() << meta << text << endl;
}

// ----------------------------------------------------------------------------------------------------
// MARK: - LoggerImplMulti
// ----------------------------------------------------------------------------------------------------

void LoggerImplMulti::write( const Metadata &meta, const string &text )
{
	for( auto &logger : mLoggers )
		logger->write( meta, text );
}

// ----------------------------------------------------------------------------------------------------
// MARK: - LoggerFile
// ----------------------------------------------------------------------------------------------------

LoggerFile::LoggerFile()
{
	mStream.open( "cinder.log" );
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
		case Level::VERBOSE:		lhs << "verbose";	break;
		case Level::INFO:			lhs << "info";		break;
		case Level::WARNING:		lhs << "WARNING";	break;
		case Level::ERROR:			lhs << "ERROR";		break;
		case Level::FATAL:			lhs << "FATAL";		break;
		default: CI_ASSERT_NOT_REACHABLE();
	}
	return lhs;
}

} } // namespace cinder::log
