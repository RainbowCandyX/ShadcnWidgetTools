#ifndef SHADTOOLTIP_H
#define SHADTOOLTIP_H

#include <QKeySequence>
#include <QObject>
#include <QRect>
#include <QString>
#include <QStringList>

#include "ShadDef.h"
#include "ShadProperty.h"

class QEvent;
class QWidget;

#define shadTooltip ShadTooltip::getInstance()

// 提示，对应 shadcn Tooltip：bg-foreground text-background text-xs rounded-md px-3 py-1.5，带箭头，淡入并缩放
// install() 后接管程序中所有组件的 toolTip：鼠标悬停（默认不等待，对应 TooltipProvider delayDuration=0）或通过键盘聚焦时显示，
// 离开、按下鼠标、滚轮、按 Esc 或组件隐藏时关闭；已显示时移到相邻组件会直接切换，不重新播放动画
class ShadTooltipPrivate;

class SHADCN_EXPORT ShadTooltip : public QObject
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadTooltip)

    public:
        static ShadTooltip* getInstance();

        // 安装到 QApplication；未安装时组件的 toolTip 仍由 Qt 原生提示显示
        void install();
        void uninstall();
        bool isInstalled() const;

        // 鼠标悬停后显示前的等待时间（毫秒），默认 0
        void setShowDelay(int milliseconds);
        int getShowDelay() const;
        // 单个组件的悬停等待时间（毫秒），覆盖全局设置；传负数恢复使用全局设置
        static void setShowDelay(QWidget* widget, int milliseconds);
        // 单个组件的悬停等待时间，未单独设置时返回 -1
        static int getShowDelay(const QWidget* widget);

        // 单个组件的提示方向，默认 Top；空间不足时翻到对侧，RTL 下 Left / Right 互换
        static void setSide(QWidget* widget, ShadPlacementType::Side side);
        static ShadPlacementType::Side getSide(const QWidget* widget);
        // 提示中跟在文字后面的快捷键（以 Kbd 样式显示），例如 {"Ctrl", "P"}；传空列表移除
        // 只有一个按键时请写成 QStringList{"S"}，单元素的花括号也能构造 QKeySequence，会与下面的版本产生二义性
        static void setShortcut(QWidget* widget, const QStringList& keys);
        // 按 QKeySequence 显示快捷键，规则与 ShadKbdGroup::setKeySequence 相同（macOS 显示 ⌘ 等符号）；只负责显示，不注册快捷键
        static void setShortcut(QWidget* widget, const QKeySequence& sequence);
        static QStringList getShortcut(const QWidget* widget);

        // 在组件的 rect（组件坐标）旁立即显示 text，用于一个组件内有多个提示区域的情况；未安装时使用 QToolTip
        void showText(QWidget* widget, const QRect& rect, const QString& text);
        void hideText();

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

    private:
        explicit ShadTooltip(QObject* parent = nullptr);
        ~ShadTooltip() override;
};

#endif // SHADTOOLTIP_H
