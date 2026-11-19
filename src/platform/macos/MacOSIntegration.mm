#include "MacOSIntegration.h"
#include "formats/FormatRegistry.h"
#include <QWidget>
#include <QWindow>
#include <QDebug>
#include <QUrl>
#include <QDesktopServices>

#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

namespace kvikplay {

MacOSIntegration::MacOSIntegration() = default;
MacOSIntegration::~MacOSIntegration() = default;

void MacOSIntegration::setupFramelessWindow(QWidget* widget) {
    if (!widget) return;

    // Ensure the native view and window are realized
    widget->winId();
    NSView* view = (__bridge NSView*)(void*)widget->winId();
    if (!view) return;

    NSWindow* window = [view window];
    if (!window) return;

    // Enable native Cocoa resizing on the borderless window
    window.styleMask |= NSWindowStyleMaskResizable;

    // Support rounded corners and transparent window surface
    [window setOpaque:NO];
    [window setBackgroundColor:[NSColor clearColor]];

    view.wantsLayer = YES;
    view.layer.cornerRadius = 14.0;
    view.layer.masksToBounds = YES;

    // Window drop shadow
    [window setHasShadow:YES];
    [window invalidateShadow];

    // Maintain dark / modern appearance
    window.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
}

void MacOSIntegration::setWindowRounded(QWidget* widget, bool rounded) {
    if (!widget) return;

    NSView* view = (__bridge NSView*)(void*)widget->winId();
    if (!view) return;

    view.layer.cornerRadius = rounded ? 14.0 : 0.0;
    view.layer.masksToBounds = YES;

    NSWindow* window = [view window];
    if (window) {
        [window setHasShadow:rounded ? YES : NO];
        [window invalidateShadow];
    }
}

void MacOSIntegration::setWindowAspectRatio(QWidget* widget, double ratio, int displayWidth, int displayHeight) {
    if (!widget) return;

    NSView* view = (__bridge NSView*)(void*)widget->winId();
    if (!view) return;

    NSWindow* window = [view window];
    if (!window) return;

    NSSize size;
    if (displayWidth > 0 && displayHeight > 0) {
        size = NSMakeSize(displayWidth, displayHeight);
    } else if (ratio > 0.01) {
        size = NSMakeSize(ratio * 1000.0, 1000.0);
    } else {
        size = NSMakeSize(16.0, 9.0);
    }

    [window setAspectRatio:size];
}

bool MacOSIntegration::isDefaultPlayerForExtension(const QString& ext) {
    QString cleanExt = ext.startsWith('.') ? ext.mid(1) : ext;
    NSString* nsExt = cleanExt.toNSString();

    UTType* utType = [UTType typeWithFilenameExtension:nsExt];
    if (!utType) return false;

    CFStringRef uti = (__bridge CFStringRef)utType.identifier;
    CFStringRef handler = LSCopyDefaultRoleHandlerForContentType(uti, kLSRolesAll);
    if (!handler) return false;

    NSString* currentAppId = [[NSBundle mainBundle] bundleIdentifier];
    if (!currentAppId) {
        currentAppId = @"com.kvikplay.player";
    }

    bool isMatch = [(__bridge NSString*)handler caseInsensitiveCompare:currentAppId] == NSOrderedSame;
    CFRelease(handler);
    return isMatch;
}

bool MacOSIntegration::registerFileAssociations(const QStringList& extensions) {
    NSString* bundleId = [[NSBundle mainBundle] bundleIdentifier];
    if (!bundleId) {
        bundleId = @"com.kvikplay.player";
    }

    bool success = true;
    for (const auto& ext : extensions) {
        QString cleanExt = ext.startsWith('.') ? ext.mid(1) : ext;
        NSString* nsExt = cleanExt.toNSString();

        UTType* utType = [UTType typeWithFilenameExtension:nsExt];
        if (utType) {
            CFStringRef uti = (__bridge CFStringRef)utType.identifier;
            OSStatus status = LSSetDefaultRoleHandlerForContentType(uti, kLSRolesAll, (__bridge CFStringRef)bundleId);
            if (status != noErr) {
                success = false;
            }
        }
    }

    return success;
}

void MacOSIntegration::openSystemDefaultAppsSettings() {
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:"]];
}

} // namespace kvikplay
