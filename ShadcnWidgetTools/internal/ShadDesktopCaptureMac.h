#ifndef SHADDESKTOPCAPTUREMAC_H
#define SHADDESKTOPCAPTUREMAC_H

#import <AppKit/AppKit.h>

@interface ShadDesktopBlurView : NSView
@property(nonatomic) CGFloat cornerRadius;
- (void)showImage:(id)image;
@end

namespace ShadDesktopCapture
{
    bool isAvailable();
    void requestAccess();
    void setBlurRadius(CGFloat radius);
}

#endif
