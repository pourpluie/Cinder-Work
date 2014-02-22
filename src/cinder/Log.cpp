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

namespace {

class LoggerImplMulti : public Logger {
public:

//	void pushBack( Logger *logger )	{ mLoggers.push_back( unique_ptr<Logger>( logger ) ); }
	void pushBack( Logger *logger )	{ mLoggers.push_back( logger ); }

	virtual void write( const Metadata &meta, const std::string &text ) override;

protected:
//	std::vector<std::unique_ptr<Logger> >	mLoggers;
	std::vector<Logger *>	mLoggers;
};

Logger*				sLoggerInstance = new LoggerThreadSafe;	// Leaky singleton to enable logging during shutdown
LoggerImplMulti*	sLoggerInstanceMulti = nullptr;

} // anonymous namespace

mutex			sMutex;

Logger* logger()
{
	return sLoggerInstance;
}

void reset( Logger *logger )
{
	lock_guard<mutex> lock( sMutex );

	delete sLoggerInstance;
	sLoggerInstance = logger;

	LoggerImplMulti *multi = dynamic_cast<LoggerImplMulti *>( logger );
	sLoggerInstanceMulti = multi ? multi : nullptr;
}

// TODO: memory management here could be better, sort out with LogManager
// - probably can avoid the LoggerImplMulti there
void add( Logger *logger )
{
	if( ! sLoggerInstanceMulti ) {
		Logger *currentLogger = sLoggerInstance;
		sLoggerInstanceMulti = new LoggerImplMulti;
		sLoggerInstanceMulti->pushBack( currentLogger );
		sLoggerInstance = sLoggerInstanceMulti;
	}

	sLoggerInstanceMulti->pushBack( logger );
}

void Logger::write( const Metadata &meta, const string &text )
{
	app::console() << meta << text << endl;
}

void LoggerImplMulti::write( const Metadata &meta, const string &text )
{
	for( auto &logger : mLoggers )
		logger->write( meta, text );
}

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

void LoggerNSLog::write( const Metadata &meta, const string& text )
{
	NSString *textNs = [NSString stringWithCString:text.c_str() encoding:NSUTF8StringEncoding];
	NSString *metaDataNs = [NSString stringWithCString:meta.toString().c_str() encoding:NSUTF8StringEncoding];
	NSLog( @"%@%@", metaDataNs, textNs );
}

#endif

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
