/*
 Copyright (c) 2010, The Barbarian Group
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#include "cinder/app/AppCocoaTouch.h"
#include "cinder/app/CinderViewCocoaTouch.h"
#include "cinder/cocoa/CinderCocoaTouch.h"

#import <QuartzCore/QuartzCore.h>

#include <list>

@class AppImplCocoaTouch;

@interface WindowImplCocoaTouch : UIViewController<WindowImplCocoa, CinderViewCocoaTouchDelegate> {
  @public
	AppImplCocoaTouch							*mAppImpl;
	UIWindow									*mUiWindow;
	CinderViewCocoaTouch						*mCinderView;
	cinder::app::WindowRef						mWindowRef;
	cinder::DisplayRef							mDisplay;
	cinder::Vec2i								mSize, mPos;
	float										mContentScale;
	BOOL										mResizeHasFired;
	BOOL										mHidden;
}

- (void)loadView;
- (void)viewDidLoad;
- (WindowImplCocoaTouch*)initWithFormat:(cinder::app::Window::Format)format withAppImpl:(AppImplCocoaTouch*)appImpl sharedRenderer:(cinder::app::RendererRef)sharedRenderer;

// WindowImplCocoa Methods
- (BOOL)isFullScreen;
- (void)setFullScreen:(BOOL)fullScreen options:(ci::app::FullScreenOptions*)options;
- (cinder::Vec2i)getSize;
- (void)setSize:(cinder::Vec2i)size;
- (cinder::Vec2i)getPos;
- (void)setPos:(cinder::Vec2i)pos;
- (float)getContentScale;
- (void)close;
- (NSString*)getTitle;
- (BOOL)isBorderless;
- (void)setBorderless:(BOOL)borderless;
- (BOOL)isAlwaysOnTop;
- (void)setAlwaysOnTop:(BOOL)alwaysOnTop;
- (cinder::DisplayRef)getDisplay;
- (cinder::app::RendererRef)getRenderer;
- (void*)getNative;
- (const std::vector<cinder::app::TouchEvent::Touch>&)getActiveTouches;

// CinderViewCocoaTouchDelegate methods
- (void)draw;
- (void)mouseDown:(cinder::app::MouseEvent*)event;
- (void)mouseDrag:(cinder::app::MouseEvent*)event;
- (void)mouseUp:(cinder::app::MouseEvent*)event;
- (void)keyDown:(cinder::app::KeyEvent*)event;
- (void)setKeyboardString:(const std::string *)keyboardString;

@end

namespace cinder { namespace app {

AppCocoaTouch*				AppCocoaTouch::sInstance = 0;

static InterfaceOrientation convertInterfaceOrientation( UIInterfaceOrientation orientation )
{
	switch(orientation) {
		case		UIInterfaceOrientationPortrait:				return InterfaceOrientation::Portrait;
		case		UIInterfaceOrientationPortraitUpsideDown:	return InterfaceOrientation::PortraitUpsideDown;
		case		UIInterfaceOrientationLandscapeLeft:		return InterfaceOrientation::LandscapeLeft;
		case		UIInterfaceOrientationLandscapeRight:		return InterfaceOrientation::LandscapeRight;
		default:												return InterfaceOrientation::Unknown;
	}
}

} } // namespace cinder::app

@interface AppImplCocoaTouch : NSObject <UIApplicationDelegate> {
  @public
	cinder::app::AppCocoaTouch			*mApp;
	CFAbsoluteTime						mStartTime;
	std::list<WindowImplCocoaTouch*>	mWindows;
	WindowImplCocoaTouch*				mActiveWindow;

	CADisplayLink						*mDisplayLink;
	BOOL								mSetupHasFired;
	BOOL								mUpdateHasFired;
	BOOL								mAnimating;
	NSInteger 							mAnimationFrameInterval;

	bool								mProximityStateIsClose;
	bool								mIsUnplugged;
	float								mBatteryLevel;
	
	std::string							mKeyboardString;
}

- (AppImplCocoaTouch*)init;
- (cinder::app::RendererRef)findSharedRenderer:(cinder::app::RendererRef)match;
- (WindowImplCocoaTouch*)getDeviceWindow;
- (cinder::app::WindowRef)createWindow:(cinder::app::Window::Format)format;
- (void)setActiveWindow:(WindowImplCocoaTouch*)win;
- (void)updatePowerManagement;
- (void)setFrameRate:(float)frameRate;
- (void)showKeyboard;
- (void)hideKeyboard;
- (std::string&)getKeyboardString;
- (void)showStatusBar:(UIStatusBarAnimation)anim;
- (void)hideStatusBar:(UIStatusBarAnimation)anim;
- (void)displayLinkDraw:(id)sender;
- (void)proximityStateChange:(NSNotificationCenter *)notification;
- (void)batteryStateChange:(NSNotificationCenter *)notification;
- (void)batteryLevelChange:(NSNotificationCenter *)notification;

@end

@implementation AppImplCocoaTouch

- (AppImplCocoaTouch*)init
{
	self = [super init];
	
	mApp = cinder::app::AppCocoaTouch::get();
	mApp->privateSetImpl__( self );
	mStartTime = ::CFAbsoluteTimeGetCurrent();
	mAnimationFrameInterval = 1;
	mAnimating = NO;
	mUpdateHasFired = NO;
	mSetupHasFired = NO;
	mProximityStateIsClose = NO;
	mIsUnplugged = NO;
	mBatteryLevel = -1.0f;
	
	return self;
}

- (void)displayLinkDraw:(id)sender
{
	// issue initial resizes if that's necessary (only once)
	for( auto &win : mWindows ) {
		if( ! win->mResizeHasFired )
			[win resize];
	}

	mApp->privateUpdate__();
	mUpdateHasFired = YES;
	
	for( auto &win : mWindows ) {
		[win->mCinderView drawView];
	}
}

- (void)proximityStateChange:(NSNotificationCenter *)notification
{
	mProximityStateIsClose = ([[UIDevice currentDevice] proximityState] == YES);
	mApp->emitSignalProximitySensor( mProximityStateIsClose );
}

- (void)batteryStateChange:(NSNotificationCenter *)notification
{
	bool unplugged = [UIDevice currentDevice].batteryState == UIDeviceBatteryStateUnplugged;
	if( unplugged != mIsUnplugged ) {
		mIsUnplugged = unplugged;
		mApp->emitSignalBatteryState( mIsUnplugged );
	}
}

- (void)batteryLevelChange:(NSNotificationCenter *)notification
{
	mBatteryLevel = [UIDevice currentDevice].batteryLevel;
}

- (void)startAnimation
{
	if( ! mAnimating ) {
		mDisplayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(displayLinkDraw:)];
		[mDisplayLink setFrameInterval:mAnimationFrameInterval];
		[mDisplayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
		
		mAnimating = TRUE;
	}
}

- (void)stopAnimation
{
	if( mAnimating ) {
		[mDisplayLink invalidate];
		mDisplayLink = nil;
		
		mAnimating = FALSE;
	}
}

- (void)screenDidConnect:(NSNotification *)aNotification
{
	NSLog(@"A new screen got connected: %@", [aNotification object]);
	//cinder::Display::markDisplaysDirty();
}

- (void)screenDidDisconnect:(NSNotification *)aNotification
{
    NSLog(@"A screen got disconnected: %@", [aNotification object]);
//	cinder::Display::markDisplaysDirty();
}

- (void)updatePowerManagement
{
	if( ! mApp->isPowerManagementEnabled() ) {
		[UIApplication sharedApplication].idleTimerDisabled = NO; // setting to NO -> YES seems to be necessary rather than just direct to YES
		[UIApplication sharedApplication].idleTimerDisabled = YES;
	}
	else
		[UIApplication sharedApplication].idleTimerDisabled = NO;
}

- (void)applicationDidFinishLaunching:(UIApplication *)application
{
	mApp->privatePrepareSettings__();
	
	[self updatePowerManagement];
	
	if( ! mApp->getSettings().isStatusBarEnabled() ) {
		[UIApplication sharedApplication].statusBarHidden = YES;
	}

	NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
	[center addObserver:self selector:@selector(screenDidConnect:) name:UIScreenDidConnectNotification object:nil];
	[center addObserver:self selector:@selector(screenDidDisconnect:) name:UIScreenDidDisconnectNotification object:nil];
	
	mAnimationFrameInterval = std::max<float>( 1.0f, floor( 60.0f / mApp->getSettings().getFrameRate() + 0.5f ) );
	
	// build our list of requested formats; an empty list implies we should make the default window format
	std::vector<cinder::app::Window::Format> formats( mApp->getSettings().getWindowFormats() );
	if( formats.empty() )
		formats.push_back( mApp->getSettings().getDefaultWindowFormat() );
	
	for( auto &format : formats )
		[self createWindow:format];

	[self setActiveWindow:mWindows.front()];
	
	mApp->privateSetup__();
	mSetupHasFired = YES;
	
	[self startAnimation];	
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
	[self stopAnimation];
	mApp->emitDidEnterBackground();
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
	[self startAnimation];
	mApp->emitWillEnterForeground();
}

- (void)applicationWillResignActive:(UIApplication *)application
{
	[self stopAnimation];
    mApp->emitWillResignActive();
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
	[self startAnimation];
	mApp->emitDidBecomeActive();
}

- (void)applicationWillTerminate:(UIApplication *)application
{
	mApp->emitShutdown();
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
	mApp->emitMemoryWarning();
}

- (void)dealloc
{
	for( auto &win : mWindows )
		[win release];

	[super dealloc];
}

// Returns a pointer to a Renderer of the same type if any existing Windows have one of the same type
- (cinder::app::RendererRef)findSharedRenderer:(cinder::app::RendererRef)match
{
	for( auto &win : mWindows ) {
		cinder::app::RendererRef renderer = [win->mCinderView getRenderer];
		if( typeid(renderer) == typeid(match) )
			return renderer;
	}
	
	return cinder::app::RendererRef();
}

- (WindowImplCocoaTouch*)getDeviceWindow
{
	for( auto &win : mWindows ) {
		if ( [win getDisplay] == cinder::Display::getMainDisplay() )
			return win;
	}
	return nil;
}

- (cinder::app::WindowRef)createWindow:(cinder::app::Window::Format)format
{
	if( ! format.getRenderer() )
		format.setRenderer( mApp->getDefaultRenderer()->clone() );
	cinder::app::RendererRef sharedRenderer = [self findSharedRenderer:format.getRenderer()];
	mWindows.push_back( [[WindowImplCocoaTouch alloc] initWithFormat:format withAppImpl:self sharedRenderer:sharedRenderer] );
	return mWindows.back()->mWindowRef;
}

- (void)setActiveWindow:(WindowImplCocoaTouch*)win
{
	mActiveWindow = win;
}

- (void)setFrameRate:(float)frameRate
{
	mAnimationFrameInterval = std::max<float>( 1.0f, floor( 60.0f / frameRate + 0.5f ) );
	if( mDisplayLink )
		[mDisplayLink setFrameInterval:mAnimationFrameInterval];
}

- (void)showKeyboard
{
	if( ! mWindows.empty() ) {
		[mWindows.front()->mCinderView showKeyboard];
		mKeyboardString.clear();
	}
}

- (void)hideKeyboard
{
	if( ! mWindows.empty() )
		[mWindows.front()->mCinderView hideKeyboard];
}

- (std::string&)getKeyboardString
{
	return mKeyboardString;
}

- (void)showStatusBar:(UIStatusBarAnimation)anim
{
	if( [UIApplication sharedApplication].statusBarHidden != NO ) {
		[[UIApplication sharedApplication] setStatusBarHidden:NO withAnimation:anim];
		UIViewController *viewController = mWindows.front();
		[viewController.view setFrame:[viewController.view bounds]];
	}
}

- (void)hideStatusBar:(UIStatusBarAnimation)anim
{
	if( [UIApplication sharedApplication].statusBarHidden != YES ) {
		[[UIApplication sharedApplication] setStatusBarHidden:YES withAnimation:anim];
		UIViewController *viewController = mWindows.front();
		[viewController.view setFrame:[viewController.view bounds]];
	}
}

@end


namespace cinder { namespace app {

AppCocoaTouch::AppCocoaTouch()
	: App()
{
	AppCocoaTouch::sInstance = this;

	mIsKeyboardVisible = false;
}

void AppCocoaTouch::launch( const char *title, int argc, char * const argv[] )
{
	try {
		::UIApplicationMain( argc, const_cast<char**>( argv ), nil, NSStringFromClass([AppImplCocoaTouch class]) );
	}
	catch( std::exception &e ) {
		std::cout << "Uncaught Exception: " << e.what() << std::endl;
		throw e;
	}
}

WindowRef AppCocoaTouch::createWindow( const Window::Format &format )
{
	return [mImpl createWindow:format];
}

WindowRef AppCocoaTouch::getWindow() const
{
	if( ! mImpl->mActiveWindow )
		throw cinder::app::ExcInvalidWindow();
	else
		return mImpl->mActiveWindow->mWindowRef;
}

size_t AppCocoaTouch::getNumWindows() const
{
	return mImpl->mWindows.size();
}

WindowRef AppCocoaTouch::getWindowIndex( size_t index ) const
{
	if( index >= mImpl->mWindows.size() )
		throw cinder::app::ExcInvalidWindow();

	std::list<WindowImplCocoaTouch*>::iterator iter = mImpl->mWindows.begin();
	std::advance( iter, index );
	return (*iter)->mWindowRef;
}

InterfaceOrientation AppCocoaTouch::getOrientation() const
{
	WindowImplCocoaTouch *deviceWindow = [mImpl getDeviceWindow];
	return convertInterfaceOrientation( [deviceWindow interfaceOrientation] );
}

InterfaceOrientation AppCocoaTouch::getWindowOrientation() const
{
	WindowImplCocoaTouch *window = mImpl->mActiveWindow;
	return convertInterfaceOrientation( [window interfaceOrientation] );
}

void AppCocoaTouch::enableProximitySensor()
{
	[UIDevice currentDevice].proximityMonitoringEnabled = YES;
	[[NSNotificationCenter defaultCenter] addObserver:mImpl selector:@selector(proximityStateChange:)
		name:UIDeviceProximityStateDidChangeNotification object:nil];
}

void AppCocoaTouch::disableProximitySensor()
{
	[UIDevice currentDevice].proximityMonitoringEnabled = NO;
}

bool AppCocoaTouch::proximityIsClose() const
{
	return mImpl->mProximityStateIsClose;
}

void AppCocoaTouch::enableBatteryMonitoring()
{
	[UIDevice currentDevice].batteryMonitoringEnabled = YES;
	mImpl->mBatteryLevel = [UIDevice currentDevice].batteryLevel;
	mImpl->mIsUnplugged = [UIDevice currentDevice].batteryState == UIDeviceBatteryStateUnplugged;
	[[NSNotificationCenter defaultCenter] addObserver:mImpl selector:@selector(batteryLevelChange:) 
		name:UIDeviceBatteryLevelDidChangeNotification object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:mImpl selector:@selector(batteryStateChange:) 
		name:UIDeviceBatteryStateDidChangeNotification object:nil];
}

void AppCocoaTouch::disableBatteryMonitoring()
{
	[UIDevice currentDevice].batteryMonitoringEnabled = NO;
}

float AppCocoaTouch::getBatteryLevel() const
{
	return mImpl->mBatteryLevel;
}

bool AppCocoaTouch::isUnplugged() const
{
	return mImpl->mIsUnplugged;
}

//! Shows the default iOS keyboard
void AppCocoaTouch::showKeyboard()
{
	if( ! mIsKeyboardVisible )
		[mImpl showKeyboard];
	
	mIsKeyboardVisible = true;
}

//! Returns whether the iOS keyboard is visible
bool AppCocoaTouch::isKeyboardVisible() const
{
	return mIsKeyboardVisible;
}

//! Hides the default iOS keyboard
void AppCocoaTouch::hideKeyboard()
{
	if( mIsKeyboardVisible )
		[mImpl hideKeyboard];
	
	mIsKeyboardVisible = false;
}

std::string	AppCocoaTouch::getKeyboardString() const
{
	return [mImpl getKeyboardString];
}

void AppCocoaTouch::showStatusBar( AppCocoaTouch::StatusBarAnimation animation )
{
	if( animation == StatusBarAnimation::FADE ) [mImpl showStatusBar:UIStatusBarAnimationFade];
	else if( animation == StatusBarAnimation::SLIDE ) [mImpl showStatusBar:UIStatusBarAnimationSlide];
	else [mImpl showStatusBar:UIStatusBarAnimationNone];
}

bool AppCocoaTouch::isStatusBarVisible() const
{
	return [UIApplication sharedApplication].statusBarHidden == NO;
}

void AppCocoaTouch::hideStatusBar( AppCocoaTouch::StatusBarAnimation animation )
{
	if( animation == StatusBarAnimation::FADE ) [mImpl hideStatusBar:UIStatusBarAnimationFade];
	else if( animation == StatusBarAnimation::SLIDE ) [mImpl hideStatusBar:UIStatusBarAnimationSlide];
	else [mImpl hideStatusBar:UIStatusBarAnimationNone];
}

//! Returns the maximum frame-rate the App will attempt to maintain.
float AppCocoaTouch::getFrameRate() const
{
	return 60.0f / mImpl->mAnimationFrameInterval;
}

//! Sets the maximum frame-rate the App will attempt to maintain.
void AppCocoaTouch::setFrameRate( float frameRate )
{
	[mImpl setFrameRate:frameRate];
}

bool AppCocoaTouch::isFullScreen() const
{
	return true;
}

void AppCocoaTouch::setFullScreen( bool aFullScreen )
{
	// NO-OP
}

double AppCocoaTouch::getElapsedSeconds() const
{
	CFAbsoluteTime currentTime = ::CFAbsoluteTimeGetCurrent();
	return ( currentTime - mImpl->mStartTime );
}

fs::path AppCocoaTouch::getAppPath() const
{ 
	return fs::path([[[NSBundle mainBundle] bundlePath] UTF8String]);
}

void AppCocoaTouch::quit()
{
	return; // no effect on iOS
}

void AppCocoaTouch::privatePrepareSettings__()
{
	prepareSettings( &mSettings );
}

void AppCocoaTouch::enablePowerManagement( bool powerManagement )
{
	mPowerManagement = powerManagement;
	[mImpl updatePowerManagement];
}

void AppCocoaTouch::emitDidEnterBackground()
{
	mSignalDidEnterBackground();
}

void AppCocoaTouch::emitWillEnterForeground()
{
	mSignalWillEnterForeground();
}

void AppCocoaTouch::emitWillResignActive()
{
	mSignalWillResignActive();
}

void AppCocoaTouch::emitDidBecomeActive()
{
	mSignalDidBecomeActive();
}

void AppCocoaTouch::emitMemoryWarning()
{
	mSignalMemoryWarning();
}

uint32_t AppCocoaTouch::emitSupportedOrientations()
{
	return mSignalSupportedOrientations();
}

void AppCocoaTouch::emitWillRotate()
{
	mSignalWillRotate();
}

void AppCocoaTouch::emitDidRotate()
{
	mSignalDidRotate();
}

std::ostream& operator<<( std::ostream &lhs, const InterfaceOrientation &rhs )
{
	switch( rhs ) {
		case InterfaceOrientation::Portrait:			lhs << "Portrait";				break;
		case InterfaceOrientation::PortraitUpsideDown:	lhs << "PortraitUpsideDown";	break;
		case InterfaceOrientation::LandscapeLeft:		lhs << "LandscapeLeft";			break;
		case InterfaceOrientation::LandscapeRight:		lhs << "LandscapeRight";		break;
		default: lhs << "Error";
	}
	return lhs;
}

float getOrientationDegrees( InterfaceOrientation orientation )
{
	switch( orientation ) {
		case InterfaceOrientation::Portrait:			return 0.0f;
		case InterfaceOrientation::PortraitUpsideDown:	return 180.0f;
		case InterfaceOrientation::LandscapeLeft:		return 90.0f;
		case InterfaceOrientation::LandscapeRight:		return 270.0f;
		default: return 0.0f;
	}
}

} } // namespace cinder::app

@implementation WindowImplCocoaTouch;

- (WindowImplCocoaTouch*)initWithFormat:(cinder::app::Window::Format)format withAppImpl:(AppImplCocoaTouch*)appImpl sharedRenderer:(cinder::app::RendererRef)sharedRenderer
{
	self = [super initWithNibName:nil bundle:nil];

	self.wantsFullScreenLayout = YES;

	mAppImpl = appImpl;
	mResizeHasFired = NO;

	mDisplay = format.getDisplay();
	if( ! mDisplay ) // a NULL display implies the main display
		mDisplay = cinder::Display::getMainDisplay();

	mUiWindow = [[UIWindow alloc] initWithFrame:[mDisplay->getUiScreen() bounds]];
	
	mUiWindow.screen = mDisplay->getUiScreen();
	
	cinder::Area screenBounds = mDisplay->getBounds();
	CGRect screenBoundsCgRect;
	screenBoundsCgRect.origin.x = 0;
	screenBoundsCgRect.origin.y = 0;
	screenBoundsCgRect.size.width = screenBounds.getWidth();
	screenBoundsCgRect.size.height = screenBounds.getHeight();
	
	mContentScale = 1.0f;
	
	if( mAppImpl->mApp->getSettings().isHighDensityDisplayEnabled() )
		mContentScale = mUiWindow.screen.scale;
	
	mCinderView = [[CinderViewCocoaTouch alloc] initWithFrame:screenBoundsCgRect app:mAppImpl->mApp renderer:format.getRenderer() sharedRenderer:sharedRenderer contentScale:mContentScale];
	[mCinderView setDelegate:self];
	mSize = cinder::Vec2i( screenBoundsCgRect.size.width, screenBoundsCgRect.size.height );
	mPos = cinder::Vec2i::zero();
	mUiWindow.rootViewController = format.getRootViewController() ? format.getRootViewController() : self;
	mWindowRef = cinder::app::Window::privateCreate__( self, mAppImpl->mApp );

	// this needs to be last
	if( mDisplay != cinder::Display::getMainDisplay() )
		mUiWindow.hidden = NO;
	else
		[mUiWindow makeKeyAndVisible];
	
	return self;
}

- (void)loadView
{
	[super loadView];
	self.view = mCinderView;
}

- (void)viewDidLoad
{
	[super viewDidLoad];
}

// pre iOS 6
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
{
	// Only rotate after setup. On secondary monitors we don't support any rotation
	if( ! mAppImpl->mSetupHasFired || mDisplay != cinder::Display::getMainDisplay() ) {
		return ( toInterfaceOrientation == UIInterfaceOrientationPortrait );
	}

	ci::app::InterfaceOrientation orientation = ci::app::convertInterfaceOrientation( toInterfaceOrientation );
	uint32_t supportedOrientations = mAppImpl->mApp->emitSupportedOrientations();

	return ( ( supportedOrientations & orientation ) != 0 );
}

// iOS 6+
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 60000
- (NSUInteger)supportedInterfaceOrientations
{
	// Only rotate after setup. On secondary monitors we don't support any rotation
	if( ! mAppImpl->mSetupHasFired || mDisplay != cinder::Display::getMainDisplay() ) {
		return UIInterfaceOrientationMaskAll;
	}

	uint32_t supportedOrientations = mAppImpl->mApp->emitSupportedOrientations();
	NSUInteger result = 0;
	if( supportedOrientations & ci::app::InterfaceOrientation::Portrait )
		result |= UIInterfaceOrientationMaskPortrait;
	if( supportedOrientations & ci::app::InterfaceOrientation::PortraitUpsideDown )
		result |= UIInterfaceOrientationMaskPortraitUpsideDown;
	if( supportedOrientations & ci::app::InterfaceOrientation::LandscapeLeft )
		result |= UIInterfaceOrientationMaskLandscapeLeft;
	if( supportedOrientations & ci::app::InterfaceOrientation::LandscapeRight )
		result |= UIInterfaceOrientationMaskLandscapeRight;

	return result;
}
#endif

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation duration:(NSTimeInterval)duration
{
	mAppImpl->mApp->emitWillRotate();
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation
{
	mAppImpl->mApp->emitDidRotate();
}

- (BOOL)isFullScreen
{
	return YES;
}

- (void)setFullScreen:(BOOL)fullScreen options:(ci::app::FullScreenOptions*)options
{ // NO-OP
}

- (cinder::Vec2i)getSize;
{
	return mSize;
}

- (void)setSize:(cinder::Vec2i)size
{ // NO-OP
}

- (cinder::Vec2i)getPos;
{
	return mPos;
}

- (void)setPos:(cinder::Vec2i)pos;
{ // NO-OP
}

- (float)getContentScale
{
	return mContentScale;
}

- (void)close;
{ // NO-OP
}

- (NSString *)getTitle
{
	return self.title;
}

- (BOOL)isBorderless
{
	return YES;
}

- (void)setBorderless:(BOOL)borderless
{ // NO-OP
}

- (BOOL)isAlwaysOnTop
{
	return YES;
}

- (void)setAlwaysOnTop:(BOOL)alwaysOnTop
{ // NO-OP
}

- (void)hide
{ // NO-OP
}

- (void)show
{ // NO-OP
}

- (BOOL)isHidden
{
	return NO;
}

- (cinder::DisplayRef)getDisplay;
{
	return mDisplay;
}

- (cinder::app::RendererRef)getRenderer;
{
	return [mCinderView getRenderer];
}

- (void*)getNative
{
	return mCinderView;
}

- (UIViewController *)getNativeViewController
{
	return self;
}

- (const std::vector<cinder::app::TouchEvent::Touch>&)getActiveTouches
{
	return [mCinderView getActiveTouches];
}


// CinderViewCocoaTouchDelegate methods
- (void)draw
{
	[mAppImpl setActiveWindow:self];
	if( mAppImpl->mUpdateHasFired )
		mWindowRef->emitDraw();
}

- (void)resize
{	
	[mAppImpl setActiveWindow:self];

	mSize.x = [mCinderView.layer bounds].size.width; // * mCinderView.contentScaleFactor;
	mSize.y = [mCinderView.layer bounds].size.height; // * mCinderView.contentScaleFactor;

	if( mAppImpl->mSetupHasFired ) {
		mWindowRef->emitResize();
		mResizeHasFired = YES;
	}
}

- (void)mouseDown:(cinder::app::MouseEvent*)event
{
	[mAppImpl setActiveWindow:self];
	event->setWindow( mWindowRef );
	mWindowRef->emitMouseDown( event );
}

- (void)mouseDrag:(cinder::app::MouseEvent*)event
{
	[mAppImpl setActiveWindow:self];
	event->setWindow( mWindowRef );
	mWindowRef->emitMouseDrag( event );
}

- (void)mouseUp:(cinder::app::MouseEvent*)event
{
	[mAppImpl setActiveWindow:self];
	event->setWindow( mWindowRef );
	mWindowRef->emitMouseUp( event );
}

- (void)touchesBegan:(cinder::app::TouchEvent*)event
{
	[mAppImpl setActiveWindow:self];
	event->setWindow( mWindowRef );
	mWindowRef->emitTouchesBegan( event );
}

- (void)touchesMoved:(cinder::app::TouchEvent*)event
{
	[mAppImpl setActiveWindow:self];
	event->setWindow( mWindowRef );
	mWindowRef->emitTouchesMoved( event );
}

- (void)touchesEnded:(cinder::app::TouchEvent*)event
{
	[mAppImpl setActiveWindow:self];
	event->setWindow( mWindowRef );
	mWindowRef->emitTouchesEnded( event );
}

- (cinder::app::WindowRef)getWindowRef
{
	return mWindowRef;
}

- (void)keyDown:(cinder::app::KeyEvent*)event
{
	[mAppImpl setActiveWindow:self];
	event->setWindow( mWindowRef );
	mWindowRef->emitKeyDown( event );
	event->setHandled( false );
	mWindowRef->emitKeyUp( event );
}

- (void)setKeyboardString:(const std::string *)keyboardString
{
	mAppImpl->mKeyboardString = *keyboardString;
}

@end