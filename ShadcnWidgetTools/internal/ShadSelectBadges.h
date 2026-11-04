#ifndef SHADSELECTBADGES_H
#define SHADSELECTBADGES_H

#include <QFont>
#include <QRectF>
#include <QStringList>
#include <QtCore/qnamespace.h>

class QPainter;

// 多选触发框中的已选项徽章，ShadComboBox 与 ShadTreeSelect 共用，组件库内部使用
namespace ShadSelectBadges
{
    // 在 rect 内单行绘制徽章，放不下的部分合并为 +N
    void paint(QPainter* painter, const QRectF& rect, const QStringList& texts, const QFont& baseFont, Qt::LayoutDirection direction);
} // namespace ShadSelectBadges

#endif // SHADSELECTBADGES_H
