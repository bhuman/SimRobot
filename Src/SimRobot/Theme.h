/**
 * @file Theme.h
 *
 * This file declares some helpers to handle dark mode.
 *
 * @author Thomas Röfer
 */

#pragma once

#include <QAction>
#include <QIcon>
#include <QImage>
#include <QPushButton>
#include <QWidget>

namespace Theme
{
  /**
   * Checks whether dark mode is active. This is currently done by testing
   * whether a "light" color is actually dark.
   * @param widget The palette of this widget is used for the color test.
   * @return Is a dark color scheme active?
   */
  inline bool isDarkMode(QWidget* widget)
  {
    return widget->palette().text().color().lightness() > 128;
  }

  /**
   * Updates an icon to match the current color theme.
   * This assumes that an icon designed for light mode has its isMask
   * flag set, while an icon designed for dark mode has not. The method
   * will invert the icon and flip the flag if necessary.
   * @param widget The palette of this widget is used for detecting dark mode.
   * @param icon The icon which is updated.
   * @return The resulting icon.
   */
  inline QIcon updateIcon(QWidget* widget, const QIcon& icon)
  {
    if(icon.isMask() == isDarkMode(widget))
    {
      QIcon invertedIcon;
      for(auto& size : icon.availableSizes())
      {
        QImage image = icon.pixmap(size).toImage();
        image.invertPixels();
        invertedIcon.addPixmap(QPixmap::fromImage(std::move(image)));
      }
      invertedIcon.setIsMask(!icon.isMask());
      return invertedIcon;
    }
    else
      return icon;
  }

  /**
   * Updates the icon of an action to match the current color theme.
   * This assumes that an icon designed for light mode has its isMask
   * flag set, while an icon designed for dark mode has not. The method
   * will invert the icon and flip the flag if necessary.
   * @param widget The palette of this widget is used for detecting dark mode.
   * @param action The action the icon of which is updated.
   * @return The action that was passed to this function.
   */
  inline QAction* updateIcon(QWidget* widget, QAction* action)
  {
    if(!action->icon().isNull() && action->icon().isMask() == isDarkMode(widget))
      action->setIcon(updateIcon(widget, action->icon()));
    return action;
  }

  /**
   * Updates the icon of a push button to match the current color theme.
   * This assumes that an icon designed for light mode has its isMask
   * flag set, while an icon designed for dark mode has not. The method
   * will invert the icon and flip the flag if necessary.
   * @param widget The palette of this widget is used for detecting dark mode.
   * @param button The button the icon of which is updated.
   * @return The button that was passed to this function.
   */
  inline QPushButton* updateIcon(QWidget* widget, QPushButton* button)
  {
    if(!button->icon().isNull() && button->icon().isMask() == isDarkMode(widget))
      button->setIcon(updateIcon(widget, button->icon()));
    return button;
  }
}
