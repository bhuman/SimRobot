/**
 * @file Theme.h
 *
 * This file declares some helpers to handle dark mode.
 *
 * @author Thomas Röfer
 */

#pragma once

class QAction;
class QIcon;
class QWidget;

namespace Theme
{
  /**
   * Checks whether dark mode is active. This is currently done by testing
   * whether a "light" color is actually dark.
   * @param widget The palette of this widget is used for the color test.
   * @return Is a dark color scheme active?
   */
  bool isDarkMode(QWidget* widget);

  /**
   * Updates an icon to match the current color theme.
   * This assumes that an icon designed for light mode has its isMask
   * flag set, while an icon designed for dark mode has not. The method
   * will invert the icon and flip the flag if necessary.
   * @param widget The palette of this widget is used for detecting dark mode.
   * @param icon The icon which is updated.
   * @return The resulting icon.
   */
  QIcon updateIcon(QWidget* widget, const QIcon& icon);

  /**
   * Updates the icon of an action to match the current color theme.
   * This assumes that an icon designed for light mode has its isMask
   * flag set, while an icon designed for dark mode has not. The method
   * will invert the icon and flip the flag if necessary.
   * @param widget The palette of this widget is used for detecting dark mode.
   * @param action The action the icon of which is updated.
   * @return The action that was passed to this function.
   */
  QAction* updateIcon(QWidget* widget, QAction* action);
};
