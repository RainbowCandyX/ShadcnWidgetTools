#include "ShadScreenshotFile.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QStandardPaths>
#include <QString>
#include <QWidget>

namespace ShadScreenshotFile
{
    Result save(QWidget* parent, const QImage& image, QString* filePath)
    {
        const QString directory = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
        const QString fileName = QCoreApplication::translate("ShadScreenshot", "截图") + QDateTime::currentDateTime().toString(QStringLiteral("_yyyyMMdd_HHmmss")) + QStringLiteral(".png");
        const QString filter = QCoreApplication::translate("ShadScreenshot", "PNG 图片 (*.png)") + QStringLiteral(";;") + QCoreApplication::translate("ShadScreenshot", "JPEG 图片 (*.jpg *.jpeg)");
        QString path = QFileDialog::getSaveFileName(parent, QCoreApplication::translate("ShadScreenshot", "保存截图"), QDir(directory).filePath(fileName), filter);
        if (path.isEmpty())
        {
            return Result::Cancelled;
        }
        if (QFileInfo(path).suffix().isEmpty())
        {
            path += QStringLiteral(".png");
        }
        if (filePath != nullptr)
        {
            *filePath = path;
        }
        return image.save(path) ? Result::Saved : Result::Failed;
    }
}
