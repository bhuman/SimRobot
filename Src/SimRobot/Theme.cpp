/**
 * @file Theme.cpp
 *
 * This file implements some helpers to handle dark mode.
 *
 * @author Thomas Röfer
 */

#include "Theme.h"

#include <QAction>
#include <QIcon>
#include <QImage>
#include <QWidget>

namespace Theme
{
  bool isDarkMode(QWidget* widget)
  {
    return widget->palette().window().color().lightness() < 128;
  }

  QAction* updateIcon(QWidget* widget, QAction* action)
  {
    const bool darkMode = isDarkMode(widget);
    if(action->icon().isMask() == darkMode)
    {
      QIcon invertedIcon;
      for(auto& size : action->icon().availableSizes())
      {
        QImage image = action->icon().pixmap(size).toImage();
        image.invertPixels();
        invertedIcon.addPixmap(QPixmap::fromImage(std::move(image)));
      }
      invertedIcon.setIsMask(!action->icon().isMask());
      action->setIcon(invertedIcon);
    }
    return action;
  }
}
