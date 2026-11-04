#include "ShadWindowEffect.h"

#include <QRect>
#include <QString>
#include <QWidget>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadDesktopCaptureMac.h"
#include "ShadTheme.h"

#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>

namespace
{
constexpr CGFloat kBackdropRadiusScale = 4.0;

CALayer* findLayer(CALayer* layer, Class layerClass)
{
    if (layer == nil || layerClass == nil)
    {
        return nil;
    }
    if ([layer isKindOfClass:layerClass])
    {
        return layer;
    }
    for (CALayer* sublayer in layer.sublayers)
    {
        CALayer* found = findLayer(sublayer, layerClass);
        if (found != nil)
        {
            return found;
        }
    }
    return nil;
}

bool isGaussianBlurFilter(id filter)
{
    if (![filter respondsToSelector:NSSelectorFromString(@"type")] || ![filter conformsToProtocol:@protocol(NSCopying)])
    {
        return false;
    }
    const id type = [filter valueForKey:@"type"];
    return [type isKindOfClass:[NSString class]] && [type isEqualToString:@"gaussianBlur"];
}

void applyBackdropRadius(CALayer* backdrop, CGFloat radius)
{
    NSArray* filters = backdrop.filters;
    NSMutableArray* updatedFilters = [NSMutableArray arrayWithCapacity:filters.count];
    bool isChanged = false;
    for (id filter in filters)
    {
        id updatedFilter = filter;
        if (isGaussianBlurFilter(filter))
        {
            const id current = [filter valueForKey:@"inputRadius"];
            if (![current isKindOfClass:[NSNumber class]] || [current doubleValue] != radius)
            {
                updatedFilter = [filter copy];
#if !__has_feature(objc_arc)
                [updatedFilter autorelease];
#endif
                [updatedFilter setValue:@(radius) forKey:@"inputRadius"];
                isChanged = true;
            }
        }
        [updatedFilters addObject:updatedFilter];
    }
    if (isChanged)
    {
        backdrop.filters = updatedFilters;
    }
}

void hideTintLayers(CALayer* backdrop)
{
    NSArray<NSString*>* tintNames = @[@"fill", @"tone", @"desktop tint"];
    for (CALayer* sibling in backdrop.superlayer.sublayers)
    {
        if (sibling != backdrop && sibling.name != nil && [tintNames containsObject:sibling.name] && !sibling.hidden)
        {
            sibling.hidden = YES;
        }
    }
}
} // namespace

@interface ShadBlurEffectView : NSVisualEffectView
@property(nonatomic) CGFloat backdropRadius;
@end

@implementation ShadBlurEffectView

- (void)updateLayer
{
    [super updateLayer];
    CALayer* backdrop = findLayer(self.layer, NSClassFromString(@"CABackdropLayer"));
    if (backdrop == nil)
    {
        return;
    }
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    applyBackdropRadius(backdrop, self.backdropRadius);
    hideTintLayers(backdrop);
    [CATransaction commit];
}

@end

namespace
{
NSString* const kEffectViewIdentifier = @"ShadWindowEffectView";

NSView* findEffectView(NSView* container)
{
    for (NSView* subview in container.subviews)
    {
        if ([subview.identifier isEqualToString:kEffectViewIdentifier])
        {
            return subview;
        }
    }
    return nil;
}

NSImage* roundedMask(CGFloat radius)
{
    if (radius <= 0)
    {
        return nil;
    }
    const CGFloat edge = 2 * radius + 1;
    NSImage* image = [NSImage imageWithSize:NSMakeSize(edge, edge) flipped:NO drawingHandler:^BOOL(NSRect rect) {
        [[NSColor blackColor] setFill];
        [[NSBezierPath bezierPathWithRoundedRect:rect xRadius:radius yRadius:radius] fill];
        return YES;
    }];
    image.capInsets = NSEdgeInsetsMake(radius, radius, radius, radius);
    image.resizingMode = NSImageResizingModeStretch;
    return image;
}
} // namespace

namespace ShadWindowEffect
{
void apply(QWidget* window, ShadWindowType::Effect effect, int cornerRadius, const QRect& area)
{
    NSView* view = (__bridge NSView*)reinterpret_cast<void*>(window->winId());
    NSView* container = view.superview;
    if (container == nil)
    {
        return;
    }
    NSView* currentView = findEffectView(container);
    if (effect != ShadWindowType::Blur)
    {
        [currentView removeFromSuperview];
        return;
    }
    const bool isWholeWindow = area.isNull();
    const NSRect frame = isWholeWindow ? view.frame : [view convertRect:NSMakeRect(area.x(), area.y(), area.width(), area.height()) toView:container];
    const NSAutoresizingMaskOptions autoresizingMask = isWholeWindow ? (NSViewWidthSizable | NSViewHeightSizable) : NSViewNotSizable;
    const CGFloat backdropRadius = shadTheme->getWindowEffectBlurRadius() / kBackdropRadiusScale;
    if (shadTheme->isWindowEffectDesktopOnly())
    {
        ShadDesktopCapture::requestAccess();
    }
    const bool isDesktopOnly = shadTheme->isWindowEffectDesktopOnly() && ShadDesktopCapture::isAvailable();
    const Class viewClass = isDesktopOnly ? [ShadDesktopBlurView class] : [ShadBlurEffectView class];
    if (currentView != nil && ![currentView isKindOfClass:viewClass])
    {
        [currentView removeFromSuperview];
        currentView = nil;
    }
    if (isDesktopOnly)
    {
        ShadDesktopCapture::setBlurRadius(backdropRadius);
        ShadDesktopBlurView* desktopView = (ShadDesktopBlurView*)currentView;
        if (desktopView == nil)
        {
            desktopView = [[ShadDesktopBlurView alloc] initWithFrame:frame];
            desktopView.identifier = kEffectViewIdentifier;
            [container addSubview:desktopView positioned:NSWindowBelow relativeTo:view];
#if !__has_feature(objc_arc)
            [desktopView release];
#endif
        }
        desktopView.autoresizingMask = autoresizingMask;
        desktopView.frame = frame;
        desktopView.cornerRadius = cornerRadius;
        return;
    }
    ShadBlurEffectView* effectView = (ShadBlurEffectView*)currentView;
    if (effectView == nil)
    {
        effectView = [[ShadBlurEffectView alloc] initWithFrame:frame];
        effectView.backdropRadius = backdropRadius;
        effectView.identifier = kEffectViewIdentifier;
        effectView.wantsLayer = YES;
        effectView.blendingMode = NSVisualEffectBlendingModeBehindWindow;
        effectView.material = NSVisualEffectMaterialUnderWindowBackground;
        effectView.state = NSVisualEffectStateActive;
        [container addSubview:effectView positioned:NSWindowBelow relativeTo:view];
#if !__has_feature(objc_arc)
        [effectView release];
#endif
    }
    effectView.autoresizingMask = autoresizingMask;
    effectView.frame = frame;
    effectView.appearance = [NSAppearance appearanceNamed:shadTheme->isDark() ? NSAppearanceNameDarkAqua : NSAppearanceNameAqua];
    effectView.maskImage = roundedMask(cornerRadius);
    effectView.backdropRadius = backdropRadius;
    effectView.needsDisplay = YES;
}

QString systemWallpaperPath()
{
    NSScreen* screen = [NSScreen mainScreen];
    if (screen == nil)
    {
        return QString();
    }
    NSURL* url = [[NSWorkspace sharedWorkspace] desktopImageURLForScreen:screen];
    if (url == nil || !url.isFileURL)
    {
        return QString();
    }
    return QString::fromNSString(url.path);
}

void requestDesktopCaptureAccess()
{
    ShadDesktopCapture::requestAccess();
}
} // namespace ShadWindowEffect
