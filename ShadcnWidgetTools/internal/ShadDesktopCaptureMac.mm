#import "ShadDesktopCaptureMac.h"

#import <CoreGraphics/CoreGraphics.h>
#import <CoreImage/CoreImage.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <QuartzCore/QuartzCore.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>

#include <QtGlobal>

#include <atomic>
#include <cmath>

namespace
{
constexpr CGFloat kCapturePointScale = 0.5;
constexpr int32_t kFramesPerSecond = 30;
constexpr NSTimeInterval kWindowRefreshInterval = 3.0;

std::atomic<double> gBlurRadius{30.0};

CGDirectDisplayID displayIDForScreen(NSScreen* screen)
{
    NSNumber* number = screen.deviceDescription[@"NSScreenNumber"];
    return number != nil ? number.unsignedIntValue : kCGNullDirectDisplay;
}
}

API_AVAILABLE(macos(12.3))
@interface ShadDesktopCaptureSession : NSObject <SCStreamOutput, SCStreamDelegate>
- (instancetype)initWithDisplayID:(CGDirectDisplayID)displayID;
- (void)addView:(ShadDesktopBlurView*)view;
- (void)removeView:(ShadDesktopBlurView*)view;
- (void)pruneViews;
- (void)rerender;
@end

namespace
{
API_AVAILABLE(macos(12.3))
NSMutableDictionary<NSNumber*, ShadDesktopCaptureSession*>* sessions()
{
    static NSMutableDictionary<NSNumber*, ShadDesktopCaptureSession*>* instance = [NSMutableDictionary dictionary];
    return instance;
}
}

@implementation ShadDesktopCaptureSession
{
    CGDirectDisplayID _displayID;
    NSHashTable<ShadDesktopBlurView*>* _views;
    SCStream* _stream;
    NSSet<NSNumber*>* _windowIDs;
    NSTimer* _refreshTimer;
    dispatch_queue_t _queue;
    CIContext* _context;
    CIImage* _lastSource;
    id _latestImage;
    BOOL _isLoading;
    BOOL _hasReportedError;
}

- (instancetype)initWithDisplayID:(CGDirectDisplayID)displayID
{
    self = [super init];
    if (self != nil)
    {
        _displayID = displayID;
        _views = [NSHashTable weakObjectsHashTable];
        _queue = dispatch_queue_create("ShadDesktopCapture", DISPATCH_QUEUE_SERIAL);
        _context = [CIContext contextWithOptions:@{kCIContextCacheIntermediates: @NO}];
    }
    return self;
}

- (void)dealloc
{
    [_refreshTimer invalidate];
}

- (NSUInteger)liveViewCount
{
    return _views.allObjects.count;
}

- (void)addView:(ShadDesktopBlurView*)view
{
    [_views addObject:view];
    if (_latestImage != nil)
    {
        [view showImage:_latestImage];
    }
    if (_refreshTimer == nil)
    {
        __weak ShadDesktopCaptureSession* weakSelf = self;
        _refreshTimer = [NSTimer scheduledTimerWithTimeInterval:kWindowRefreshInterval repeats:YES block:^(NSTimer* timer) {
            Q_UNUSED(timer);
            [weakSelf refreshContent];
        }];
    }
    if (_stream == nil)
    {
        [self refreshContent];
    }
}

- (void)removeView:(ShadDesktopBlurView*)view
{
    [_views removeObject:view];
    [self pruneViews];
}

- (void)pruneViews
{
    if ([self liveViewCount] > 0)
    {
        return;
    }
    [_refreshTimer invalidate];
    _refreshTimer = nil;
    SCStream* stream = _stream;
    _stream = nil;
    _windowIDs = nil;
    _latestImage = nil;
    [stream stopCaptureWithCompletionHandler:^(NSError* error) {
        Q_UNUSED(error);
    }];
    dispatch_async(_queue, ^{
        self->_lastSource = nil;
    });
}

- (void)refreshContent
{
    if (_isLoading)
    {
        return;
    }
    _isLoading = YES;
    __weak ShadDesktopCaptureSession* weakSelf = self;
    [SCShareableContent getShareableContentExcludingDesktopWindows:NO onScreenWindowsOnly:YES completionHandler:^(SCShareableContent* content, NSError* error) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [weakSelf applyContent:content error:error];
        });
    }];
}

- (void)reportError:(NSString*)message error:(NSError*)error
{
    if (_hasReportedError)
    {
        return;
    }
    _hasReportedError = YES;
    NSString* detail = error != nil ? error.localizedDescription : @"UNKNOWN";
    qWarning("%s: %s", message.UTF8String, detail.UTF8String);
}

- (void)applyContent:(SCShareableContent*)content error:(NSError*)error
{
    _isLoading = NO;
    if ([self liveViewCount] == 0)
    {
        return;
    }
    if (content == nil)
    {
        [self reportError:@"获取桌面窗口失败" error:error];
        return;
    }
    SCDisplay* display = nil;
    for (SCDisplay* candidate in content.displays)
    {
        if (candidate.displayID == _displayID)
        {
            display = candidate;
            break;
        }
    }
    if (display == nil)
    {
        return;
    }
    const NSInteger iconLevel = CGWindowLevelForKey(kCGDesktopIconWindowLevelKey);
    NSMutableArray<SCWindow*>* windows = [NSMutableArray array];
    NSMutableSet<NSNumber*>* windowIDs = [NSMutableSet set];
    for (SCWindow* window in content.windows)
    {
        if (window.windowLayer < iconLevel && CGRectIntersectsRect(window.frame, display.frame))
        {
            [windows addObject:window];
            [windowIDs addObject:@(window.windowID)];
        }
    }
    if (_stream != nil && [windowIDs isEqualToSet:_windowIDs])
    {
        return;
    }
    _windowIDs = windowIDs;
    SCContentFilter* filter = [[SCContentFilter alloc] initWithDisplay:display includingWindows:windows];
    if (_stream != nil)
    {
        __weak ShadDesktopCaptureSession* weakSelf = self;
        [_stream updateContentFilter:filter completionHandler:^(NSError* updateError) {
            if (updateError != nil)
            {
                dispatch_async(dispatch_get_main_queue(), ^{
                    [weakSelf reportError:@"更新桌面捕获范围失败" error:updateError];
                });
            }
        }];
        return;
    }
    SCStreamConfiguration* configuration = [[SCStreamConfiguration alloc] init];
    configuration.width = static_cast<size_t>(MAX(1.0, std::round(display.width * kCapturePointScale)));
    configuration.height = static_cast<size_t>(MAX(1.0, std::round(display.height * kCapturePointScale)));
    configuration.minimumFrameInterval = CMTimeMake(1, kFramesPerSecond);
    configuration.pixelFormat = kCVPixelFormatType_32BGRA;
    configuration.showsCursor = NO;
    configuration.queueDepth = 3;
    SCStream* stream = [[SCStream alloc] initWithFilter:filter configuration:configuration delegate:self];
    NSError* addError = nil;
    if (![stream addStreamOutput:self type:SCStreamOutputTypeScreen sampleHandlerQueue:_queue error:&addError])
    {
        _windowIDs = nil;
        [self reportError:@"添加桌面捕获输出失败" error:addError];
        return;
    }
    _stream = stream;
    __weak ShadDesktopCaptureSession* weakSelf = self;
    [stream startCaptureWithCompletionHandler:^(NSError* startError) {
        if (startError != nil)
        {
            dispatch_async(dispatch_get_main_queue(), ^{
                [weakSelf handleStop:stream error:startError];
            });
        }
    }];
}

- (void)handleStop:(SCStream*)stream error:(NSError*)error
{
    if (_stream != stream)
    {
        return;
    }
    _stream = nil;
    _windowIDs = nil;
    [self reportError:@"桌面捕获已停止" error:error];
}

- (void)stream:(SCStream*)stream didStopWithError:(NSError*)error
{
    __weak ShadDesktopCaptureSession* weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
        [weakSelf handleStop:stream error:error];
    });
}

- (void)stream:(SCStream*)stream didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer ofType:(SCStreamOutputType)type
{
    Q_UNUSED(stream);
    if (type != SCStreamOutputTypeScreen || !CMSampleBufferIsValid(sampleBuffer))
    {
        return;
    }
    CFArrayRef attachments = CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, false);
    if (attachments != nullptr && CFArrayGetCount(attachments) > 0)
    {
        NSDictionary* info = (__bridge NSDictionary*)CFArrayGetValueAtIndex(attachments, 0);
        NSNumber* status = info[SCStreamFrameInfoStatus];
        if (status != nil && status.integerValue != SCFrameStatusComplete)
        {
            return;
        }
    }
    CVImageBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    if (pixelBuffer == nullptr)
    {
        return;
    }
    [self renderSource:[CIImage imageWithCVPixelBuffer:pixelBuffer]];
}

- (void)renderSource:(CIImage*)source
{
    _lastSource = source;
    if (source == nil)
    {
        return;
    }
    const CGRect extent = source.extent;
    const double radius = gBlurRadius.load() * kCapturePointScale;
    CIImage* output = source;
    if (radius > 0.0)
    {
        output = [[[source imageByClampingToExtent] imageByApplyingFilter:@"CIGaussianBlur" withInputParameters:@{kCIInputRadiusKey: @(radius)}] imageByCroppingToRect:extent];
    }
    CGImageRef image = [_context createCGImage:output fromRect:extent];
    if (image == nullptr)
    {
        return;
    }
    id frame = (__bridge_transfer id)image;
    __weak ShadDesktopCaptureSession* weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
        [weakSelf deliverFrame:frame];
    });
}

- (void)deliverFrame:(id)frame
{
    if (_stream == nil)
    {
        return;
    }
    _latestImage = frame;
    for (ShadDesktopBlurView* view in _views.allObjects)
    {
        [view showImage:frame];
    }
}

- (void)rerender
{
    dispatch_async(_queue, ^{
        [self renderSource:self->_lastSource];
    });
}

@end

@implementation ShadDesktopBlurView
{
    id _image;
    CGDirectDisplayID _displayID;
}

- (instancetype)initWithFrame:(NSRect)frameRect
{
    self = [super initWithFrame:frameRect];
    if (self != nil)
    {
        _displayID = kCGNullDirectDisplay;
        self.wantsLayer = YES;
        self.layer.contentsGravity = kCAGravityResize;
        self.layer.masksToBounds = YES;
    }
    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    if (@available(macOS 12.3, *))
    {
        [sessions()[@(_displayID)] pruneViews];
    }
}

- (void)setCornerRadius:(CGFloat)cornerRadius
{
    _cornerRadius = cornerRadius;
    self.layer.cornerRadius = cornerRadius;
}

- (void)viewDidMoveToWindow
{
    [super viewDidMoveToWindow];
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    [center removeObserver:self];
    [self unregisterFromSession];
    if (self.window == nil)
    {
        return;
    }
    [center addObserver:self selector:@selector(windowGeometryChanged:) name:NSWindowDidMoveNotification object:self.window];
    [center addObserver:self selector:@selector(windowGeometryChanged:) name:NSWindowDidResizeNotification object:self.window];
    [center addObserver:self selector:@selector(windowScreenChanged:) name:NSWindowDidChangeScreenNotification object:self.window];
    [self registerWithSession];
}

- (void)setFrameSize:(NSSize)newSize
{
    [super setFrameSize:newSize];
    [self updateContents];
}

- (void)setFrameOrigin:(NSPoint)newOrigin
{
    [super setFrameOrigin:newOrigin];
    [self updateContents];
}

- (void)windowGeometryChanged:(NSNotification*)notification
{
    Q_UNUSED(notification);
    [self updateContents];
}

- (void)windowScreenChanged:(NSNotification*)notification
{
    Q_UNUSED(notification);
    [self unregisterFromSession];
    [self registerWithSession];
}

- (void)registerWithSession
{
    if (@available(macOS 12.3, *))
    {
        const CGDirectDisplayID displayID = displayIDForScreen(self.window.screen);
        if (displayID == kCGNullDirectDisplay)
        {
            return;
        }
        _displayID = displayID;
        ShadDesktopCaptureSession* session = sessions()[@(displayID)];
        if (session == nil)
        {
            session = [[ShadDesktopCaptureSession alloc] initWithDisplayID:displayID];
            sessions()[@(displayID)] = session;
        }
        [session addView:self];
    }
}

- (void)unregisterFromSession
{
    if (_displayID == kCGNullDirectDisplay)
    {
        return;
    }
    if (@available(macOS 12.3, *))
    {
        [sessions()[@(_displayID)] removeView:self];
    }
    _displayID = kCGNullDirectDisplay;
    _image = nil;
    self.layer.contents = nil;
}

- (void)showImage:(id)image
{
    _image = image;
    [self updateContents];
}

- (void)updateContents
{
    NSWindow* window = self.window;
    NSScreen* screen = window.screen;
    if (_image == nil || window == nil || screen == nil)
    {
        return;
    }
    CGImageRef fullImage = (__bridge CGImageRef)_image;
    const NSRect screenFrame = screen.frame;
    if (NSWidth(screenFrame) <= 0.0 || NSHeight(screenFrame) <= 0.0)
    {
        return;
    }
    const NSRect onScreen = [window convertRectToScreen:[self convertRect:self.bounds toView:nil]];
    const CGFloat scaleX = CGImageGetWidth(fullImage) / NSWidth(screenFrame);
    const CGFloat scaleY = CGImageGetHeight(fullImage) / NSHeight(screenFrame);
    const CGRect cropRect = CGRectIntegral(CGRectMake((NSMinX(onScreen) - NSMinX(screenFrame)) * scaleX, (NSMaxY(screenFrame) - NSMaxY(onScreen)) * scaleY, NSWidth(onScreen) * scaleX, NSHeight(onScreen) * scaleY));
    const CGRect bounds = CGRectMake(0.0, 0.0, CGImageGetWidth(fullImage), CGImageGetHeight(fullImage));
    const CGRect visibleRect = CGRectIntersection(cropRect, bounds);
    if (CGRectIsEmpty(visibleRect))
    {
        return;
    }
    CGImageRef cropped = CGImageCreateWithImageInRect(fullImage, visibleRect);
    if (cropped == nullptr)
    {
        return;
    }
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    self.layer.contents = (__bridge_transfer id)cropped;
    [CATransaction commit];
}

@end

namespace ShadDesktopCapture
{
    bool isAvailable()
    {
        if (@available(macOS 12.3, *))
        {
            return CGPreflightScreenCaptureAccess();
        }
        return false;
    }

    void requestAccess()
    {
        static bool hasRequested = false;
        if (hasRequested || CGPreflightScreenCaptureAccess())
        {
            return;
        }
        hasRequested = true;
        CGRequestScreenCaptureAccess();
    }

    void setBlurRadius(CGFloat radius)
    {
        const double value = MAX(0.0, static_cast<double>(radius));
        if (gBlurRadius.exchange(value) == value)
        {
            return;
        }
        if (@available(macOS 12.3, *))
        {
            for (ShadDesktopCaptureSession* session in sessions().allValues)
            {
                [session rerender];
            }
        }
    }
}
