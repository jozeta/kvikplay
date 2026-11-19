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
#import <IOKit/pwr_mgt/IOPMLib.h>
#import <MediaPlayer/MediaPlayer.h>

namespace kvikplay {

MacOSIntegration::MacOSIntegration() = default;

MacOSIntegration::~MacOSIntegration() {
    if (m_sleepAssertionId != 0) {
        IOPMAssertionRelease(m_sleepAssertionId);
        m_sleepAssertionId = 0;
    }
    [MPNowPlayingInfoCenter defaultCenter].nowPlayingInfo = nil;
    MPRemoteCommandCenter* commandCenter = [MPRemoteCommandCenter sharedCommandCenter];
    [commandCenter.playCommand removeTarget:nil];
    commandCenter.playCommand.enabled = NO;
    [commandCenter.pauseCommand removeTarget:nil];
    commandCenter.pauseCommand.enabled = NO;
    [commandCenter.togglePlayPauseCommand removeTarget:nil];
    commandCenter.togglePlayPauseCommand.enabled = NO;
    [commandCenter.nextTrackCommand removeTarget:nil];
    commandCenter.nextTrackCommand.enabled = NO;
    [commandCenter.previousTrackCommand removeTarget:nil];
    commandCenter.previousTrackCommand.enabled = NO;
    [commandCenter.changePlaybackPositionCommand removeTarget:nil];
    commandCenter.changePlaybackPositionCommand.enabled = NO;
}

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

void MacOSIntegration::installActivationObserver(QWidget* widget, std::function<void(bool activatedByMouse)> callback) {
    if (!widget) return;
    widget->winId();
    NSView* view = (__bridge NSView*)(void*)widget->winId();
    if (!view) return;
    NSWindow* window = [view window];
    if (!window) return;

    [[NSNotificationCenter defaultCenter] addObserverForName:NSWindowDidBecomeKeyNotification
                                                      object:window
                                                       queue:[NSOperationQueue mainQueue]
                                                  usingBlock:^(NSNotification* /*note*/) {
        bool byMouse = ([NSEvent pressedMouseButtons] != 0);
        if (callback) {
            callback(byMouse);
        }
    }];
}

void MacOSIntegration::setDisplaySleepInhibited(bool inhibit) {
    if (inhibit) {
        if (m_sleepAssertionId == 0) {
            CFStringRef reason = CFSTR("Kvikplay Media Playback");
            IOPMAssertionID assertion = 0;
            IOReturn ret = IOPMAssertionCreateWithName(
                kIOPMAssertionTypePreventUserIdleDisplaySleep,
                kIOPMAssertionLevelOn,
                reason,
                &assertion
            );
            if (ret == kIOReturnSuccess) {
                m_sleepAssertionId = assertion;
            }
        }
    } else {
        if (m_sleepAssertionId != 0) {
            IOPMAssertionRelease(m_sleepAssertionId);
            m_sleepAssertionId = 0;
        }
    }
}

void MacOSIntegration::setNowPlayingInfo(const NowPlayingInfo& info) {
    MPNowPlayingInfoCenter* center = [MPNowPlayingInfoCenter defaultCenter];
    if (info.title.isEmpty() && info.duration <= 0.0) {
        center.nowPlayingInfo = nil;
        return;
    }

    NSMutableDictionary* dict = [NSMutableDictionary dictionary];
    dict[MPMediaItemPropertyTitle] = info.title.toNSString();
    if (!info.artist.isEmpty()) {
        dict[MPMediaItemPropertyArtist] = info.artist.toNSString();
    }
    if (info.duration > 0.0) {
        dict[MPMediaItemPropertyPlaybackDuration] = @(info.duration);
    }
    dict[MPNowPlayingInfoPropertyElapsedPlaybackTime] = @(std::max(0.0, info.position));
    dict[MPNowPlayingInfoPropertyPlaybackRate] = @(info.playbackRate);

    center.nowPlayingInfo = dict;
}

void MacOSIntegration::clearNowPlayingInfo() {
    [MPNowPlayingInfoCenter defaultCenter].nowPlayingInfo = nil;
}

void MacOSIntegration::setupMediaKeyCommands(const MediaKeyCallbacks& callbacks) {
    m_mediaCallbacks = callbacks;
    MPRemoteCommandCenter* commandCenter = [MPRemoteCommandCenter sharedCommandCenter];

    auto onPlay = callbacks.onPlay;
    [commandCenter.playCommand removeTarget:nil];
    [commandCenter.playCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent* /*event*/) {
        if (onPlay) onPlay();
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    commandCenter.playCommand.enabled = YES;

    auto onPause = callbacks.onPause;
    [commandCenter.pauseCommand removeTarget:nil];
    [commandCenter.pauseCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent* /*event*/) {
        if (onPause) onPause();
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    commandCenter.pauseCommand.enabled = YES;

    auto onTogglePlayPause = callbacks.onTogglePlayPause;
    [commandCenter.togglePlayPauseCommand removeTarget:nil];
    [commandCenter.togglePlayPauseCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent* /*event*/) {
        if (onTogglePlayPause) onTogglePlayPause();
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    commandCenter.togglePlayPauseCommand.enabled = YES;

    auto onNextTrack = callbacks.onNextTrack;
    [commandCenter.nextTrackCommand removeTarget:nil];
    [commandCenter.nextTrackCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent* /*event*/) {
        if (onNextTrack) onNextTrack();
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    commandCenter.nextTrackCommand.enabled = YES;

    auto onPreviousTrack = callbacks.onPreviousTrack;
    [commandCenter.previousTrackCommand removeTarget:nil];
    [commandCenter.previousTrackCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent* /*event*/) {
        if (onPreviousTrack) onPreviousTrack();
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    commandCenter.previousTrackCommand.enabled = YES;

    auto onSeek = callbacks.onSeek;
    [commandCenter.changePlaybackPositionCommand removeTarget:nil];
    [commandCenter.changePlaybackPositionCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent* event) {
        if (onSeek && [event isKindOfClass:[MPChangePlaybackPositionCommandEvent class]]) {
            MPChangePlaybackPositionCommandEvent* posEvent = (MPChangePlaybackPositionCommandEvent*)event;
            onSeek(posEvent.positionTime);
            return MPRemoteCommandHandlerStatusSuccess;
        }
        return MPRemoteCommandHandlerStatusCommandFailed;
    }];
    commandCenter.changePlaybackPositionCommand.enabled = YES;
}

} // namespace kvikplay
