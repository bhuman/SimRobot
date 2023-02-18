/**
 * @file SimRobot/AppleHelper.cpp
 * This file implements a function that sets the window background color
 * using Objective C++ on macOS.
 * @author Thomas Röfer
 */

#import <AppKit/AppKit.h>
#import <QWindow>

void updateBackgroundColor(QWindow* window)
{
  NSView* view = reinterpret_cast<NSView*>(window->winId());
  view.window.backgroundColor = [NSColor windowBackgroundColor];
}
