/**
 * @file SimRobot/AppleHelper.cpp
 * This file implements a function that sets the window background color
 * and makes the title bar drawable using Objective C++ on macOS.
 * @author Thomas Röfer
 */

#import <AppKit/AppKit.h>
#import <QWindow>

void fixMainWindow(WId window)
{
  NSView* view = reinterpret_cast<NSView*>(window);
  view.window.backgroundColor = [NSColor windowBackgroundColor];
  view.window.styleMask |= NSWindowStyleMaskFullSizeContentView;
}
