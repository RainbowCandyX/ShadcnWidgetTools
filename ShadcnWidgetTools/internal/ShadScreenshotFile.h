#ifndef SHADSCREENSHOTFILE_H
#define SHADSCREENSHOTFILE_H

#include <QImage>
#include <QString>

class QWidget;

namespace ShadScreenshotFile
{
    enum class Result
    {
        Saved = 0,
        Failed,
        Cancelled,
    };

    Result save(QWidget* parent, const QImage& image, QString* filePath);
}

#endif
