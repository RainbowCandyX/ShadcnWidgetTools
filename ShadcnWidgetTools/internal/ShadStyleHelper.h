#ifndef SHADSTYLEHELPER_H
#define SHADSTYLEHELPER_H

#include <QColor>
#include <QFont>
#include <QIcon>
#include <QPixmap>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QtCore/qnamespace.h>

#include <functional>

#include "ShadDef.h"
#include "ShadIconType.h"

class QFocusEvent;
class QObject;
class QPainter;
class QPainterPath;
class QWidget;

// 组件内部共用的绘制工具，不对外导出
namespace ShadStyleHelper
{
    // 可聚焦组件四周需预留同样的边距绘制焦点环
    constexpr int kFocusRingWidth = 3;
    constexpr qreal kDisabledOpacity = 0.5;

    // 在原颜色透明度基础上再乘以 alpha
    QColor withAlpha(const QColor& color, qreal alpha);
    // 预乘透明度插值，避免从透明色过渡时发黑
    QColor mix(const QColor& from, const QColor& to, qreal progress);

    QRectF insetRect(const QRect& widgetRect, qreal margin);

    // 按 LTR 计算的逻辑矩形转换为实际位置，RTL 时水平镜像
    QRect visualRect(Qt::LayoutDirection direction, const QRect& bounds, const QRect& logicalRect);
    QRectF visualRect(Qt::LayoutDirection direction, const QRectF& bounds, const QRectF& logicalRect);
    // 结果总是带 Qt::AlignAbsolute，可直接传给 QPainter::drawText
    Qt::Alignment visualAlignment(Qt::LayoutDirection direction, Qt::Alignment alignment);
    // 结果为逻辑方向（Left 表示起始侧），不带 Qt::AlignAbsolute
    Qt::Alignment logicalAlignment(Qt::LayoutDirection direction, Qt::Alignment alignment);
    QPainterPath roundedRectPath(const QRectF& rect, qreal radius, bool roundLeft = true, bool roundRight = true);
    void fillRoundedRect(QPainter* painter, const QRectF& rect, qreal radius, const QColor& color, bool roundLeft = true, bool roundRight = true);
    // 描边绘制在 rect 内侧
    void strokeRoundedRect(QPainter* painter, const QRectF& rect, qreal radius, const QColor& color, qreal width = 1.0, bool roundLeft = true, bool roundRight = true);
    void strokeDashedRoundedRect(QPainter* painter, const QRectF& rect, qreal radius, const QColor& color);
    // 焦点环绘制在 rect 外侧
    void drawFocusRing(QPainter* painter, const QRectF& rect, qreal radius, const QColor& color, qreal width = kFocusRingWidth, bool roundLeft = true, bool roundRight = true);
    // 阴影只绘制在 rect 外侧
    void drawShadowXs(QPainter* painter, const QRectF& rect, qreal radius);
    void drawShadowSm(QPainter* painter, const QRectF& rect, qreal radius);
    // Md / Lg 向下延伸超出 rect，调用方需预留绘制空间
    void drawShadowMd(QPainter* painter, const QRectF& rect, qreal radius);
    void drawShadowLg(QPainter* painter, const QRectF& rect, qreal radius);

    // 按钮组拼接信息，均为物理方向；由 ShadButtonGroup 写入子组件，未加入按钮组时为空
    struct GroupEdges
    {
        Qt::Edges squareEdges;
        Qt::Edges hiddenBorderEdges;
    };

    GroupEdges groupEdges(const QWidget* widget);
    // 传入空的 GroupEdges 恢复独立外观
    void setGroupEdges(QWidget* widget, const GroupEdges& edges);
    QPainterPath roundedRectPath(const QRectF& rect, qreal radius, const GroupEdges& edges);
    void fillRoundedRect(QPainter* painter, const QRectF& rect, qreal radius, const QColor& color, const GroupEdges& edges);
    void strokeRoundedRect(QPainter* painter, const QRectF& rect, qreal radius, const QColor& color, const GroupEdges& edges);
    void drawFocusRing(QPainter* painter, const QRectF& rect, qreal radius, const QColor& color, const GroupEdges& edges);
    void drawShadowXs(QPainter* painter, const QRectF& rect, qreal radius, const GroupEdges& edges);

    // 组件自行处理 QEvent::ToolTip 时设置为 true，ShadTooltip 不再直接显示其 toolTip
    inline constexpr const char* kCustomToolTipProperty = "_shadCustomToolTip";

    // 嵌入模式：不绘制自身的背景、边框、阴影与焦点环，内边距由容器负责
    // 由容器通过动态属性写入，组件在 QEvent::DynamicPropertyChange 中刷新
    inline constexpr const char* kEmbeddedProperty = "_shadEmbedded";
    bool isEmbedded(const QWidget* widget);
    void setEmbedded(QWidget* widget, bool embedded);
    QFont kbdFont(const QFont& baseFont);
    QSize kbdSize(const QFont& baseFont, const QString& text);
    void drawKbd(QPainter* painter, const QRectF& rect, const QFont& baseFont, const QString& text, bool isInverted);

    QColor focusRingColor(bool isDestructive);

    bool hasWindowEffect(const QWidget* widget);
    // 直接铺在窗口上的底色，跟随窗口底色不透明度
    constexpr qreal kSidebarEffectOpacity = 0.5;
    QColor windowSurfaceColor(const QWidget* widget, ShadThemeType::ThemeColor themeColor, qreal effectOpacity);
    // 选中态底色，不低于原色的 kSelectionEffectOpacity
    constexpr qreal kSelectionEffectOpacity = 0.6;
    QColor selectionColor(ShadThemeType::ThemeColor themeColor);
    // 不含焦点环与错误态；boxRect 应为整数坐标，避免边缘发虚
    void drawCheckbox(QPainter* painter, const QRectF& boxRect, Qt::CheckState state);

    // 只有键盘导航才显示焦点环
    bool resolveFocusVisible(bool currentVisible, const QFocusEvent* event);

    // 先把焦点交给仍可用的另一个按钮，避免 Qt 把焦点移到下一个组件导致滚动区域滚动
    void setNavigationButtonsEnabled(QWidget* previous, bool previousEnabled, QWidget* next, bool nextEnabled);

    // 隐藏或移除组件前调用，避免 Qt 以 Qt::TabFocusReason 把焦点交给下一个组件而显示焦点环
    void clearFocusWithin(QWidget* widget);

    void applyFont(QWidget* widget, int pixelSize, QFont::Weight weight);

    // 设置使用者也能修改的默认文字（如 accessibleName、toolTip、占位提示、对外暴露的按钮文字），构造时与收到 QEvent::LanguageChange 时调用
    // 第一次调用，或 current 仍等于上次通过本函数设置的文字时，调用 apply(text) 并记住 text；否则视为使用者改过（包括改为空），保持不变
    // 同一对象上的多个默认文字用不同的 key 区分，key 记录在对象的动态属性中
    void applyDefaultText(QObject* object, const char* key, const QString& current, const QString& text, const std::function<void(const QString &)>& apply);

    // 背景透明；QWidget::grab 会用 Window 色填充背景
    QPixmap grabContent(QWidget* widget);

    // 只保留本机已安装的字体，避免 Qt 加载全部字体别名并打印 missing font family 警告；末尾追加 emojiFontFamily()
    QStringList monospaceFontFamilies();

    // 内置 Twemoji 彩色 emoji 字体的字体族名，第一次调用时从资源加载；macOS 不加载，返回空字符串，加载失败时同样返回空字符串
    QString emojiFontFamily();

    // 按浮点宽度向上取整；QFontMetrics::horizontalAdvance 四舍五入会使 elidedText 误判省略，计算 sizeHint 时统一使用本函数
    int textWidth(const QFont& font, const QString& text);
    QString elideText(const QFont& font, const QString& text, qreal width);
    // gap 只在同时存在图标与文字时计入
    qreal iconTextWidth(const QFont& font, const QString& text, bool hasIcon, qreal gap, int iconSize);
    // 返回文字实际占用区域；AlignLeft 表示起始侧，设置 Qt::AlignAbsolute 时为物理方向
    QRectF drawIconText(QPainter* painter, const QRectF& rect, ShadIconType::IconName shadIcon, const QIcon& fallbackIcon, const QString& text, const QFont& font, const QColor& color, qreal gap, Qt::Alignment alignment, int iconSize, qreal iconRotation = 0.0, Qt::LayoutDirection direction = Qt::LeftToRight);

    constexpr int kShimmerDurationMs = 2000;
    constexpr int kSpinDurationMs = 1000;
    // text 应已按 rect 宽度省略，alignment 中 AlignLeft 表示起始侧
    void drawShimmerText(QPainter* painter, const QRectF& rect, const QString& text, const QFont& font, const QColor& color, qreal progress, Qt::Alignment alignment, Qt::LayoutDirection direction);
} // namespace ShadStyleHelper

#endif // SHADSTYLEHELPER_H
