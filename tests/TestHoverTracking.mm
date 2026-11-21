#include "TestCheck.h"
#include <QApplication>
#include <QWidget>
#include <iostream>
#include "platform/macos/MacOSIntegration.h"
#import <Cocoa/Cocoa.h>

// Regression test for the 1.1.0/1.2.0 crash: the hover tracking area's owner must answer
// every event its options ask AppKit to send. A missing mouseExited: raised
// "unrecognized selector" when the pointer left the window (e.g. when the update dialog
// opened) and aborted the app.
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QWidget w;
    w.resize(320, 180);
    w.show();
    QApplication::processEvents();

    kvikplay::MacOSIntegration platform;
    platform.installHoverObserver(&w, [] {});

    NSView* view = (__bridge NSView*)(void*)w.winId();
    int checked = 0;
    for (NSTrackingArea* area in view.trackingAreas) {
        id owner = area.owner;
        if (![NSStringFromClass([owner class]) isEqualToString:@"KvikplayHoverTarget"]) continue;
        const NSTrackingAreaOptions o = area.options;
        if (o & NSTrackingMouseMoved) CHECK([owner respondsToSelector:@selector(mouseMoved:)]);
        if (o & NSTrackingMouseEnteredAndExited) {
            CHECK([owner respondsToSelector:@selector(mouseEntered:)]);
            CHECK([owner respondsToSelector:@selector(mouseExited:)]);
        }
        if (o & NSTrackingCursorUpdate) CHECK([owner respondsToSelector:@selector(cursorUpdate:)]);
        ++checked;
    }
    CHECK(checked == 1);

    // And actually deliver the exit event the way AppKit does: must not throw.
    NSEvent* exitEvent = [NSEvent enterExitEventWithType:NSEventTypeMouseExited location:NSZeroPoint
                                           modifierFlags:0 timestamp:0 windowNumber:view.window.windowNumber
                                                 context:nil eventNumber:0 trackingNumber:0 userData:nil];
    for (NSTrackingArea* area in view.trackingAreas) {
        id owner = area.owner;
        if ([NSStringFromClass([owner class]) isEqualToString:@"KvikplayHoverTarget"]) {
            @try {
                [owner mouseExited:exitEvent];
            } @catch (NSException* e) {
                std::cerr << "mouseExited: threw " << e.name.UTF8String << std::endl;
                CHECK(false);
            }
        }
    }
    std::cout << "[TestHoverTracking] All tests passed!" << std::endl;
    return 0;
}
