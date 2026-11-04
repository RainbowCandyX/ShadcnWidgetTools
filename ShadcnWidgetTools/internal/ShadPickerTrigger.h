#ifndef SHADPICKERTRIGGER_H
#define SHADPICKERTRIGGER_H

#include <QFont>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include "ShadIconType.h"

class QPainter;
class QWidget;

// 选择器触发框的绘制与尺寸，组件库内部使用，ShadDatePicker 与 ShadTimePicker 共用
// 外观与 outline 按钮一致：border bg-background shadow-xs hover:bg-accent，键盘聚焦时显示焦点环；
// 内容按 LTR 逻辑排列为 起始侧图标、文字、结束侧下拉箭头，RTL 时整体镜像；没有文字时显示 muted 色的占位文字
namespace ShadPickerTrigger
{
    struct Options
    {
        QString text;
        QString placeholderText;
        ShadIconType::IconName leadingIcon{ShadIconType::None};
        bool hasChevron{false};
        qreal hoverProgress{0.0};
        bool showFocusRing{false};
    };

    void paint(QPainter* painter, const QWidget* widget, const Options& options);
    // 理想尺寸：文字宽度取 texts 中最宽的一个，包含焦点环边距
    QSize sizeHint(const QFont& font, const QStringList& texts, bool hasLeadingIcon, bool hasChevron);
    QSize minimumSizeHint();
} // namespace ShadPickerTrigger

#endif // SHADPICKERTRIGGER_H
