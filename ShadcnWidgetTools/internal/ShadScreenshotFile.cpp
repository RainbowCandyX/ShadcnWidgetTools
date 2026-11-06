#include "ShadScreenshotFile.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QMetaObject>
#include <QStandardPaths>
#include <QString>
#include <QThreadPool>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <functional>

namespace ShadScreenshotFile
{
    QString choosePath(QWidget* parent)
    {
        const QString directory = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
        const QString fileName = QCoreApplication::translate("ShadScreenshot", "截图") + QDateTime::currentDateTime().toString(QStringLiteral("_yyyyMMdd_HHmmss")) + QStringLiteral(".png");
        const QString filter = QCoreApplication::translate("ShadScreenshot", "PNG 图片 (*.png)") + QStringLiteral(";;") + QCoreApplication::translate("ShadScreenshot", "JPEG 图片 (*.jpg *.jpeg)");
        QString path = QFileDialog::getSaveFileName(parent, QCoreApplication::translate("ShadScreenshot", "保存截图"), QDir(directory).filePath(fileName), filter);
        if (!path.isEmpty() && QFileInfo(path).suffix().isEmpty())
        {
            path += QStringLiteral(".png");
        }
        return path;
    }

    void save(const QImage& image, const QString& path, const std::function<void(bool isSaved)>& onFinished)
    {
        QThreadPool::globalInstance()->start([image, path, onFinished]()
        {
            const bool isSaved = image.save(path);
            if (!isSaved)
            {
                qWarning("保存截图失败: %s", qUtf8Printable(QDir::toNativeSeparators(path)));
            }
            QCoreApplication* application = QCoreApplication::instance();
            if (application == nullptr)
            {
                return;
            }
            QMetaObject::invokeMethod(application, [onFinished, isSaved]() { onFinished(isSaved); }, Qt::QueuedConnection);
        });
    }
}
