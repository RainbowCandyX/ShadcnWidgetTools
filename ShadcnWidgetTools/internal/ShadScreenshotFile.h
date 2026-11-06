#ifndef SHADSCREENSHOTFILE_H
#define SHADSCREENSHOTFILE_H

#include <QImage>
#include <QString>

#include <functional>

class QWidget;

namespace ShadScreenshotFile
{
    QString choosePath(QWidget* parent);

    void save(const QImage& image, const QString& path, const std::function<void(bool isSaved)>& onFinished);
}

#endif
