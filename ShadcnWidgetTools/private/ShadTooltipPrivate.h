#ifndef SHADTOOLTIPPRIVATE_H
#define SHADTOOLTIPPRIVATE_H

#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QString>
#include <QStringList>

#include "ShadProperty.h"

class QTimer;
class QWidget;
class ShadTooltip;
class ShadTooltipWindow;

class ShadTooltipPrivate
{
    SHAD_D_CREATE(ShadTooltip)

    public:
        ShadTooltipPrivate();
        ~ShadTooltipPrivate();

        // 第一次使用时创建提示窗口（必须在 QApplication 之后创建）
        ShadTooltipWindow* window();
        // 在组件的 rect（组件坐标）旁显示
        void showFor(QWidget* widget, const QRect& rect, const QString& text, const QStringList& keys);
        void hide();
        // 向光标下的组件发送 ToolTip 事件，组件忽略时依次交给父组件，与 Qt 的处理一致
        void requestAt(const QPoint& globalPos);
        // 组件是否有需要显示的提示（设置了 toolTip，或自行处理提示事件）
        static bool hasToolTip(const QWidget* widget);

        bool _isInstalled{false};
        int _showDelay{0};
        QPointer<ShadTooltipWindow> _window;
        QPointer<QWidget> _target;
        // 当前提示对应的区域（目标组件坐标），光标离开该区域时关闭
        QRect _targetRect;
        QTimer* _showTimer{nullptr};
        // 正在等待悬停延迟的组件，离开该组件时取消等待
        QPointer<QWidget> _pendingWidget;
};

#endif // SHADTOOLTIPPRIVATE_H
