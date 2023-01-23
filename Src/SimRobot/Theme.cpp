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

  QIcon updateIcon(QWidget* widget, const QIcon& icon)
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

  QAction* updateIcon(QWidget* widget, QAction* action)
  {
    if(action->icon().isMask() == isDarkMode(widget))
      action->setIcon(updateIcon(widget, action->icon()));
    return action;
  }
}
