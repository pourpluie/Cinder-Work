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

	void pushBack( Logger *logger )	{ mLoggers.push_back( unique_ptr<Logger>( logger ) ); }

	virtual void write( const Metadata &meta, const std::string &text ) override;

protected:
	std::vector<std::unique_ptr<Logger> >	mLoggers;
};

Logger*				sInstance = new LoggerThreadSafe;	// Leaky singleton to enable logging during shutdown
LoggerImplMulti*	sInstanceMulti = nullptr;

} // anonymous namespace

mutex			sMutex;

Logger* logger()
{
	return sInstance;
}

void reset( Logger *logger )
{
	lock_guard<mutex> lock( sMutex );

	delete sInstance;
	sInstance = logger;

	LoggerImplMulti *multi = dynamic_cast<LoggerImplMulti *>( logger );
	sInstanceMulti = multi ? multi : nullptr;
}

void add( Logger *logger )
{
	if( ! sInstanceMulti ) {
		Logger *currentLogger = sInstance;
		reset( new LoggerImplMulti );
		sInstanceMulti->pushBack( currentLogger );
	}

	sInstanceMulti->pushBack( logger );
}

void Logger::write( const Metadata &meta, const std::string &text )
{
	app::console() << meta << text << endl;
}

void LoggerImplMulti::write( const Metadata &meta, const std::string &text )
{
	for( auto &logger : mLoggers )
		logger->write( meta, text );
}

#if defined( CINDER_COCOA )

void LoggerNSLog::write( const Metadata &meta, const string& text )
{
	NSString *textNs = [NSString stringWithCString:text.c_str() encoding:NSUTF8StringEncoding];
	NSString *metaDataNs = [NSString stringWithCString:meta.toString().c_str() encoding:NSUTF8StringEncoding];
	NSLog( @"%@ %@", metaDataNs, textNs );
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

std::ostream& operator<<( std::ostream &lhs, const Level &rhs )
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
