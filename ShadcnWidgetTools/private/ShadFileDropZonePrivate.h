#ifndef SHADFILEDROPZONEPRIVATE_H
#define SHADFILEDROPZONEPRIVATE_H

#include <QList>
#include <QRect>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include "ShadIconType.h"
#include "ShadProperty.h"

class QMimeData;
class ShadFileDropZone;
class ShadTransition;

class ShadFileDropZonePrivate
{
    SHAD_D_CREATE(ShadFileDropZone)

    public:
        ShadFileDropZonePrivate();
        ~ShadFileDropZonePrivate();

        // 卡片区域（扣除焦点环边距）
        QRect cardRect() const;
        // 从 "说明 (*.png *.jpg)" 格式的过滤器中取出通配符并编译，结果缓存在 _patterns
        void rebuildPatterns();
        bool matchesFilters(const QString& fileName) const;
        // 拖拽数据中的本地文件路径（不含网络地址）
        static QStringList localPaths(const QMimeData* mimeData);
        // 检查并分组发出 filesSelected / filesRejected
        void processFiles(const QStringList& paths);

        QString _title;
        QString _description;
        ShadIconType::IconName _icon{ShadIconType::Upload};
        QStringList _nameFilters;
        QList<QRegularExpression> _patterns;
        bool _isMultipleSelection{true};
        qint64 _maximumFileSize{0};
        // 文件对话框上次选择的目录，下次打开时从这里开始
        QString _lastDirectory;
        bool _isDragOver{false};
        bool _isPressed{false};
        bool _isFocusVisible{false};
        ShadTransition* _hoverTransition{nullptr};
};

#endif // SHADFILEDROPZONEPRIVATE_H
