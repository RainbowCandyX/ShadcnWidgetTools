#include "ShadFileDropZonePrivate.h"

#include <QFileInfo>
#include <QMimeData>
#include <QPointer>
#include <QString>
#include <QUrl>

#include "ShadAttachmentPrivate.h"
#include "ShadFileDropZone.h"
#include "ShadStyleHelper.h"

ShadFileDropZonePrivate::ShadFileDropZonePrivate()
{
}

ShadFileDropZonePrivate::~ShadFileDropZonePrivate()
{
}

QRect ShadFileDropZonePrivate::cardRect() const
{
    Q_Q(const ShadFileDropZone);
    const int margin = ShadStyleHelper::kFocusRingWidth;
    return q->rect().adjusted(margin, margin, -margin, -margin);
}

void ShadFileDropZonePrivate::rebuildPatterns()
{
    _patterns.clear();
    for (const QString& filter : _nameFilters)
    {
        // "图片 (*.png *.jpg)" 取括号内的部分，没有括号时整个字符串都是通配符
        QString patternText = filter;
        const qsizetype open = filter.lastIndexOf(QLatin1Char('('));
        const qsizetype close = filter.lastIndexOf(QLatin1Char(')'));
        if (open >= 0 && close > open)
        {
            patternText = filter.mid(open + 1, close - open - 1);
        }
        const QStringList patterns = patternText.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        for (const QString& pattern : patterns)
        {
            _patterns.append(QRegularExpression(QRegularExpression::wildcardToRegularExpression(pattern), QRegularExpression::CaseInsensitiveOption));
        }
    }
}

bool ShadFileDropZonePrivate::matchesFilters(const QString& fileName) const
{
    if (_patterns.isEmpty())
    {
        return true;
    }
    for (const QRegularExpression& pattern : _patterns)
    {
        if (pattern.match(fileName).hasMatch())
        {
            return true;
        }
    }
    return false;
}

QStringList ShadFileDropZonePrivate::localPaths(const QMimeData* mimeData)
{
    QStringList paths;
    if (mimeData == nullptr || !mimeData->hasUrls())
    {
        return paths;
    }
    const QList<QUrl> urls = mimeData->urls();
    for (const QUrl& url : urls)
    {
        if (url.isLocalFile())
        {
            paths.append(url.toLocalFile());
        }
    }
    return paths;
}

void ShadFileDropZonePrivate::processFiles(const QStringList& paths)
{
    Q_Q(ShadFileDropZone);
    QStringList accepted;
    QStringList missing;
    QStringList folders;
    QStringList wrongType;
    QStringList tooLarge;
    for (const QString& path : paths)
    {
        const QFileInfo info(path);
        if (!info.exists())
        {
            missing.append(path);
        }
        else if (info.isDir())
        {
            folders.append(info.absoluteFilePath());
        }
        else if (!info.isFile() || !matchesFilters(info.fileName()))
        {
            wrongType.append(info.absoluteFilePath());
        }
        else if (_maximumFileSize > 0 && info.size() > _maximumFileSize)
        {
            tooLarge.append(info.absoluteFilePath());
        }
        else
        {
            accepted.append(info.absoluteFilePath());
        }
    }
    // 单选时只保留第一个通过检查的文件，其余忽略
    if (!_isMultipleSelection && accepted.size() > 1)
    {
        accepted = QStringList{accepted.first()};
    }
    if (!accepted.isEmpty())
    {
        _lastDirectory = QFileInfo(accepted.first()).absolutePath();
    }

    // 信号处理中可能删除组件（连同本对象），先算好所有用到成员的值，每次发出前确认对象仍然存在
    const QString tooLargeReason = ShadFileDropZone::tr("文件超过 %1").arg(ShadAttachmentPrivate::formatFileSize(_maximumFileSize));
    const QString missingReason = ShadFileDropZone::tr("文件不存在");
    const QString folderReason = ShadFileDropZone::tr("不支持文件夹");
    const QString wrongTypeReason = ShadFileDropZone::tr("不支持的文件类型");
    const QPointer<ShadFileDropZone> guard(q);
    const auto reject = [&guard, q](const QStringList& rejected, const QString& reason)
    {
        if (!rejected.isEmpty() && !guard.isNull())
        {
            Q_EMIT q->filesRejected(rejected, reason);
        }
    };
    reject(missing, missingReason);
    reject(folders, folderReason);
    reject(wrongType, wrongTypeReason);
    reject(tooLarge, tooLargeReason);
    if (!accepted.isEmpty() && !guard.isNull())
    {
        Q_EMIT q->filesSelected(accepted);
    }
}
