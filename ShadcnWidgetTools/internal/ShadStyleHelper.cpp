#include "ShadStyleHelper.h"

#include <QApplication>
#include <QBrush>
#include <QByteArray>
#include <QObject>
#include <QFocusEvent>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QChar>
#include <QLinearGradient>
#include <QList>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QRegion>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QWidget>
#include <QtGlobal>
#include <QtMath>

#include "ShadIcon.h"
#include "ShadTheme.h"
#include "ShadWindowFrame.h"

namespace
{
    void drawShadowLayer(QPainter* painter, const QRectF& rect, qreal radius, qreal offsetY, qreal spread, qreal alpha)
    {
        QPainterPath shadowPath;
        shadowPath.addRoundedRect(rect.translated(0, offsetY).adjusted(-spread, -spread, spread, spread), radius + spread, radius + spread);
        QPainterPath innerPath;
        innerPath.addRoundedRect(rect, radius, radius);
        painter->fillPath(shadowPath.subtracted(innerPath), QColor(0, 0, 0, qRound(alpha * 255)));
    }

    constexpr const char* kGroupSquareEdgesProperty = "_shadGroupSquareEdges";
    constexpr const char* kGroupHiddenBorderEdgesProperty = "_shadGroupHiddenBorderEdges";

    // 半径顺序：左上、右上、右下、左下
    QPainterPath cornerRectPath(const QRectF& rect, qreal topLeft, qreal topRight, qreal bottomRight, qreal bottomLeft)
    {
        const qreal limit = qMin(rect.width(), rect.height()) / 2.0;
        topLeft = qBound<qreal>(0.0, topLeft, limit);
        topRight = qBound<qreal>(0.0, topRight, limit);
        bottomRight = qBound<qreal>(0.0, bottomRight, limit);
        bottomLeft = qBound<qreal>(0.0, bottomLeft, limit);
        // Qt 的 arcTo 角度以 3 点钟方向为 0 度、逆时针为正
        QPainterPath path;
        path.moveTo(rect.left() + topLeft, rect.top());
        path.lineTo(rect.right() - topRight, rect.top());
        if (topRight > 0.0)
        {
            path.arcTo(QRectF(rect.right() - 2 * topRight, rect.top(), 2 * topRight, 2 * topRight), 90.0, -90.0);
        }
        path.lineTo(rect.right(), rect.bottom() - bottomRight);
        if (bottomRight > 0.0)
        {
            path.arcTo(QRectF(rect.right() - 2 * bottomRight, rect.bottom() - 2 * bottomRight, 2 * bottomRight, 2 * bottomRight), 0.0, -90.0);
        }
        path.lineTo(rect.left() + bottomLeft, rect.bottom());
        if (bottomLeft > 0.0)
        {
            path.arcTo(QRectF(rect.left(), rect.bottom() - 2 * bottomLeft, 2 * bottomLeft, 2 * bottomLeft), 270.0, -90.0);
        }
        path.lineTo(rect.left(), rect.top() + topLeft);
        if (topLeft > 0.0)
        {
            path.arcTo(QRectF(rect.left(), rect.top(), 2 * topLeft, 2 * topLeft), 180.0, -90.0);
        }
        path.closeSubpath();
        return path;
    }

    bool isRoundCorner(Qt::Edges squareEdges, Qt::Edge horizontalEdge, Qt::Edge verticalEdge)
    {
        return !squareEdges.testFlag(horizontalEdge) && !squareEdges.testFlag(verticalEdge);
    }

    Qt::Alignment swapHorizontalAlignment(Qt::Alignment alignment)
    {
        const bool isLeft = alignment.testFlag(Qt::AlignLeft);
        const bool isRight = alignment.testFlag(Qt::AlignRight);
        Qt::Alignment result = alignment & ~Qt::Alignment(Qt::AlignLeft | Qt::AlignRight);
        if (isLeft)
        {
            result |= Qt::AlignRight;
        }
        if (isRight)
        {
            result |= Qt::AlignLeft;
        }
        return result;
    }
} // namespace

namespace ShadStyleHelper
{
    QColor withAlpha(const QColor& color, qreal alpha)
    {
        QColor result = color;
        result.setAlphaF(static_cast<float>(qBound<qreal>(0.0, color.alphaF() * alpha, 1.0)));
        return result;
    }

    QColor mix(const QColor& from, const QColor& to, qreal progress)
    {
        const float t = static_cast<float>(qBound<qreal>(0.0, progress, 1.0));
        const float fromAlpha = from.alphaF();
        const float toAlpha = to.alphaF();
        const float alpha = fromAlpha + (toAlpha - fromAlpha) * t;
        if (alpha <= 0.0f)
        {
            return QColor(0, 0, 0, 0);
        }
        const auto channel = [&](float fromValue, float toValue) -> float
        {
            const float premultiplied = fromValue * fromAlpha + (toValue * toAlpha - fromValue * fromAlpha) * t;
            return qBound(0.0f, premultiplied / alpha, 1.0f);
        };
        return QColor::fromRgbF(channel(from.redF(), to.redF()), channel(from.greenF(), to.greenF()), channel(from.blueF(), to.blueF()), alpha);
    }

    QRectF insetRect(const QRect& widgetRect, qreal margin)
    {
        return QRectF(widgetRect).adjusted(margin, margin, -margin, -margin);
    }

    QRect visualRect(Qt::LayoutDirection direction, const QRect& bounds, const QRect& logicalRect)
    {
        if (direction != Qt::RightToLeft)
        {
            return logicalRect;
        }
        return QRect(bounds.left() + bounds.right() - logicalRect.right(), logicalRect.top(), logicalRect.width(), logicalRect.height());
    }

    QRectF visualRect(Qt::LayoutDirection direction, const QRectF& bounds, const QRectF& logicalRect)
    {
        if (direction != Qt::RightToLeft)
        {
            return logicalRect;
        }
        return QRectF(bounds.left() + bounds.right() - logicalRect.right(), logicalRect.top(), logicalRect.width(), logicalRect.height());
    }

    Qt::Alignment visualAlignment(Qt::LayoutDirection direction, Qt::Alignment alignment)
    {
        // 没有水平对齐时视为 AlignLeft
        if (!alignment.testAnyFlags(Qt::AlignLeft | Qt::AlignRight | Qt::AlignHCenter | Qt::AlignJustify))
        {
            alignment |= Qt::AlignLeft;
        }
        if (alignment.testFlag(Qt::AlignAbsolute))
        {
            return alignment;
        }
        // 附加 AlignAbsolute，避免 QPainter 再次按布局方向对调
        const Qt::Alignment result = direction == Qt::RightToLeft ? swapHorizontalAlignment(alignment) : alignment;
        return result | Qt::AlignAbsolute;
    }

    Qt::Alignment logicalAlignment(Qt::LayoutDirection direction, Qt::Alignment alignment)
    {
        if (direction != Qt::RightToLeft || !alignment.testFlag(Qt::AlignAbsolute))
        {
            return alignment & ~Qt::Alignment(Qt::AlignAbsolute);
        }
        return swapHorizontalAlignment(alignment) & ~Qt::Alignment(Qt::AlignAbsolute);
    }

    QPainterPath roundedRectPath(const QRectF& rect, qreal radius, bool roundLeft, bool roundRight)
    {
        QPainterPath path;
        const qreal clampedRadius = qBound<qreal>(0.0, radius, qMin(rect.width(), rect.height()) / 2.0);
        if (roundLeft && roundRight)
        {
            path.addRoundedRect(rect, clampedRadius, clampedRadius);
            return path;
        }
        const qreal leftRadius = roundLeft ? clampedRadius : 0.0;
        const qreal rightRadius = roundRight ? clampedRadius : 0.0;
        path.moveTo(rect.left() + leftRadius, rect.top());
        path.lineTo(rect.right() - rightRadius, rect.top());
        if (rightRadius > 0.0)
        {
            path.arcTo(QRectF(rect.right() - 2 * rightRadius, rect.top(), 2 * rightRadius, 2 * rightRadius), 90.0, -90.0);
        }
        path.lineTo(rect.right(), rect.bottom() - rightRadius);
        if (rightRadius > 0.0)
        {
            path.arcTo(QRectF(rect.right() - 2 * rightRadius, rect.bottom() - 2 * rightRadius, 2 * rightRadius, 2 * rightRadius), 0.0, -90.0);
        }
        path.lineTo(rect.left() + leftRadius, rect.bottom());
        if (leftRadius > 0.0)
        {
            path.arcTo(QRectF(rect.left(), rect.bottom() - 2 * leftRadius, 2 * leftRadius, 2 * leftRadius), 270.0, -90.0);
        }
        path.lineTo(rect.left(), rect.top() + leftRadius);
        if (leftRadius > 0.0)
        {
            path.arcTo(QRectF(rect.left(), rect.top(), 2 * leftRadius, 2 * leftRadius), 180.0, -90.0);
        }
        path.closeSubpath();
        return path;
    }

    void fillRoundedRect(QPainter* painter, const QRectF& rect, qreal radius, const QColor& color, bool roundLeft, bool roundRight)
    {
        if (color.alpha() == 0)
        {
            return;
        }
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->fillPath(roundedRectPath(rect, radius, roundLeft, roundRight), color);
        painter->restore();
    }

    void strokeDashedRoundedRect(QPainter* painter, const QRectF& rect, qreal radius, const QColor& color)
    {
        if (color.alpha() == 0)
        {
            return;
        }
        constexpr qreal kDashLength = 3.0;
        QPen pen(color, 1.0);
        pen.setDashPattern(QList<qreal>{kDashLength, kDashLength});
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(roundedRectPath(rect.adjusted(0.5, 0.5, -0.5, -0.5), qMax<qreal>(0.0, radius - 0.5)));
        painter->restore();
    }

    void strokeRoundedRect(QPainter* painter, const QRectF& rect, qreal radius, const QColor& color, qreal width, bool roundLeft, bool roundRight)
    {
        if (color.alpha() == 0 || width <= 0)
        {
            return;
        }
        const qreal half = width / 2.0;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(QPen(color, width));
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(roundedRectPath(rect.adjusted(half, half, -half, -half), qMax<qreal>(0.0, radius - half), roundLeft, roundRight));
        painter->restore();
    }

    void drawFocusRing(QPainter* painter, const QRectF& rect, qreal radius, const QColor& color, qreal width, bool roundLeft, bool roundRight)
    {
        const QPainterPath outerPath = roundedRectPath(rect.adjusted(-width, -width, width, width), radius + width, roundLeft, roundRight);
        const QPainterPath innerPath = roundedRectPath(rect, radius, roundLeft, roundRight);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->fillPath(outerPath.subtracted(innerPath), color);
        painter->restore();
    }

    void drawShadowXs(QPainter* painter, const QRectF& rect, qreal radius)
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        drawShadowLayer(painter, rect, radius, 1.0, 0.5, 0.05);
        painter->restore();
    }

    void drawShadowSm(QPainter* painter, const QRectF& rect, qreal radius)
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        drawShadowLayer(painter, rect, radius, 1.0, 1.5, 0.04);
        drawShadowLayer(painter, rect, radius, 1.0, 0.75, 0.06);
        painter->restore();
    }

    void drawShadowMd(QPainter* painter, const QRectF& rect, qreal radius)
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        drawShadowLayer(painter, rect, radius, 1.0, 0.5, 0.05);
        drawShadowLayer(painter, rect, radius, 2.0, 1.5, 0.035);
        drawShadowLayer(painter, rect, radius, 3.0, 3.0, 0.025);
        drawShadowLayer(painter, rect, radius, 4.0, 4.5, 0.015);
        painter->restore();
    }

    void drawShadowLg(QPainter* painter, const QRectF& rect, qreal radius)
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        drawShadowLayer(painter, rect, radius, 2.0, 0.5, 0.05);
        drawShadowLayer(painter, rect, radius, 4.0, 2.0, 0.035);
        drawShadowLayer(painter, rect, radius, 6.0, 4.0, 0.025);
        drawShadowLayer(painter, rect, radius, 8.0, 6.0, 0.018);
        drawShadowLayer(painter, rect, radius, 10.0, 8.0, 0.012);
        painter->restore();
    }

    GroupEdges groupEdges(const QWidget* widget)
    {
        GroupEdges edges;
        if (widget == nullptr)
        {
            return edges;
        }
        edges.squareEdges = Qt::Edges::fromInt(widget->property(kGroupSquareEdgesProperty).toInt());
        edges.hiddenBorderEdges = Qt::Edges::fromInt(widget->property(kGroupHiddenBorderEdgesProperty).toInt());
        return edges;
    }

    void setGroupEdges(QWidget* widget, const GroupEdges& edges)
    {
        if (widget == nullptr)
        {
            return;
        }
        const GroupEdges current = groupEdges(widget);
        if (current.squareEdges == edges.squareEdges && current.hiddenBorderEdges == edges.hiddenBorderEdges)
        {
            return;
        }
        widget->setProperty(kGroupSquareEdgesProperty, edges.squareEdges.toInt() != 0 ? QVariant(edges.squareEdges.toInt()) : QVariant());
        widget->setProperty(kGroupHiddenBorderEdgesProperty, edges.hiddenBorderEdges.toInt() != 0 ? QVariant(edges.hiddenBorderEdges.toInt()) : QVariant());
        widget->update();
    }

    QPainterPath roundedRectPath(const QRectF& rect, qreal radius, const GroupEdges& edges)
    {
        const Qt::Edges square = edges.squareEdges;
        const qreal topLeft = isRoundCorner(square, Qt::LeftEdge, Qt::TopEdge) ? radius : 0.0;
        const qreal topRight = isRoundCorner(square, Qt::RightEdge, Qt::TopEdge) ? radius : 0.0;
        const qreal bottomRight = isRoundCorner(square, Qt::RightEdge, Qt::BottomEdge) ? radius : 0.0;
        const qreal bottomLeft = isRoundCorner(square, Qt::LeftEdge, Qt::BottomEdge) ? radius : 0.0;
        return cornerRectPath(rect, topLeft, topRight, bottomRight, bottomLeft);
    }

    void fillRoundedRect(QPainter* painter, const QRectF& rect, qreal radius, const QColor& color, const GroupEdges& edges)
    {
        if (color.alpha() == 0)
        {
            return;
        }
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->fillPath(roundedRectPath(rect, radius, edges), color);
        painter->restore();
    }

    void strokeRoundedRect(QPainter* painter, const QRectF& rect, qreal radius, const QColor& color, const GroupEdges& edges)
    {
        if (color.alpha() == 0)
        {
            return;
        }
        // 隐藏边两端都是直角，裁剪不会影响圆角
        QRectF clip = rect;
        const Qt::Edges hidden = edges.hiddenBorderEdges;
        clip.adjust(hidden.testFlag(Qt::LeftEdge) ? 1.0 : 0.0, hidden.testFlag(Qt::TopEdge) ? 1.0 : 0.0, hidden.testFlag(Qt::RightEdge) ? -1.0 : 0.0, hidden.testFlag(Qt::BottomEdge) ? -1.0 : 0.0);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setClipRect(clip, Qt::IntersectClip);
        painter->setPen(QPen(color, 1.0));
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(roundedRectPath(rect.adjusted(0.5, 0.5, -0.5, -0.5), qMax<qreal>(0.0, radius - 0.5), edges));
        painter->restore();
    }

    void drawFocusRing(QPainter* painter, const QRectF& rect, qreal radius, const QColor& color, const GroupEdges& edges)
    {
        const qreal width = kFocusRingWidth;
        const QPainterPath outerPath = roundedRectPath(rect.adjusted(-width, -width, width, width), radius + width, edges);
        const QPainterPath innerPath = roundedRectPath(rect, radius, edges);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->fillPath(outerPath.subtracted(innerPath), color);
        painter->restore();
    }

    void drawShadowXs(QPainter* painter, const QRectF& rect, qreal radius, const GroupEdges& edges)
    {
        constexpr qreal kOffsetY = 1.0;
        constexpr qreal kSpread = 0.5;
        constexpr qreal kAlpha = 0.05;
        const QPainterPath shadowPath = roundedRectPath(rect.translated(0, kOffsetY).adjusted(-kSpread, -kSpread, kSpread, kSpread), radius + kSpread, edges);
        const QPainterPath innerPath = roundedRectPath(rect, radius, edges);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->fillPath(shadowPath.subtracted(innerPath), QColor(0, 0, 0, qRound(kAlpha * 255)));
        painter->restore();
    }

    bool isEmbedded(const QWidget* widget)
    {
        return widget != nullptr && widget->property(kEmbeddedProperty).toBool();
    }

    void setEmbedded(QWidget* widget, bool embedded)
    {
        if (widget == nullptr || isEmbedded(widget) == embedded)
        {
            return;
        }
        widget->setProperty(kEmbeddedProperty, embedded ? QVariant(true) : QVariant());
        widget->updateGeometry();
        widget->update();
    }

    QFont kbdFont(const QFont& baseFont)
    {
        QFont font = baseFont;
        font.setPixelSize(12);
        font.setWeight(QFont::Medium);
        return font;
    }

    QSize kbdSize(const QFont& baseFont, const QString& text)
    {
        constexpr int kHeight = 20;
        constexpr int kPaddingX = 4;
        return QSize(qMax(kHeight, textWidth(kbdFont(baseFont), text) + 2 * kPaddingX), kHeight);
    }

    void drawKbd(QPainter* painter, const QRectF& rect, const QFont& baseFont, const QString& text, bool isInverted)
    {
        const QColor background = isInverted ? withAlpha(shadTheme->getColor(ShadThemeType::Background), shadTheme->isDark() ? 0.1 : 0.2) : shadTheme->getColor(ShadThemeType::Muted);
        const QColor foreground = shadTheme->getColor(isInverted ? ShadThemeType::Background : ShadThemeType::MutedForeground);
        fillRoundedRect(painter, rect, shadTheme->getRadius(ShadThemeType::Small), background);
        painter->save();
        painter->setFont(kbdFont(baseFont));
        painter->setPen(foreground);
        painter->drawText(rect, Qt::AlignCenter, text);
        painter->restore();
    }

    QColor focusRingColor(bool isDestructive)
    {
        if (isDestructive)
        {
            return withAlpha(shadTheme->getColor(ShadThemeType::Destructive), shadTheme->isDark() ? 0.4 : 0.2);
        }
        return withAlpha(shadTheme->getColor(ShadThemeType::Ring), 0.5);
    }

    bool hasWindowEffect(const QWidget* widget)
    {
        const ShadWindowFrame* frame = widget->window()->findChild<ShadWindowFrame*>(QString(), Qt::FindDirectChildrenOnly);
        return frame != nullptr && frame->hasWindowEffect();
    }

    QColor windowSurfaceColor(const QWidget* widget, ShadThemeType::ThemeColor themeColor, qreal effectOpacity)
    {
        if (!hasWindowEffect(widget))
        {
            return shadTheme->getColor(themeColor);
        }
        const QWidget* window = widget->window();
        // 中间只有不绘制背景的容器时才算直接铺在窗口上
        for (const QWidget* ancestor = widget->parentWidget(); ancestor != nullptr && ancestor != window; ancestor = ancestor->parentWidget())
        {
            if (ancestor->metaObject() != &QWidget::staticMetaObject && !ancestor->inherits("ShadSidebarLayout"))
            {
                return shadTheme->getColor(themeColor);
            }
        }
        const QColor color = shadTheme->getColor(shadTheme->getThemeMode(), themeColor);
        return withAlpha(color, effectOpacity * shadTheme->getWindowEffectOpacity(shadTheme->getWindowEffect()));
    }

    QColor selectionColor(ShadThemeType::ThemeColor themeColor)
    {
        const QColor color = shadTheme->getColor(themeColor);
        const QColor original = shadTheme->getColor(shadTheme->getThemeMode(), themeColor);
        if (color.alphaF() >= original.alphaF() * kSelectionEffectOpacity)
        {
            return color;
        }
        return withAlpha(original, kSelectionEffectOpacity);
    }

    void drawCheckbox(QPainter* painter, const QRectF& boxRect, Qt::CheckState state)
    {
        constexpr qreal kBoxRadius = 4.0;
        constexpr int kMarkSize = 14;
        const bool isMarked = state != Qt::Unchecked;
        QColor background(0, 0, 0, 0);
        QColor borderColor = shadTheme->getColor(ShadThemeType::Input);
        if (isMarked)
        {
            background = shadTheme->getColor(ShadThemeType::Primary);
            borderColor = background;
        }
        else if (shadTheme->isDark())
        {
            background = withAlpha(shadTheme->getColor(ShadThemeType::Input), 0.3);
        }
        fillRoundedRect(painter, boxRect, kBoxRadius, background);
        strokeRoundedRect(painter, boxRect, kBoxRadius, borderColor);
        if (isMarked)
        {
            const ShadIconType::IconName markIcon = state == Qt::PartiallyChecked ? ShadIconType::Minus : ShadIconType::Check;
            ShadIcon::paint(painter, boxRect, markIcon, shadTheme->getColor(ShadThemeType::PrimaryForeground), kMarkSize);
        }
    }

    bool resolveFocusVisible(bool currentVisible, const QFocusEvent* event)
    {
        if (event == nullptr)
        {
            return currentVisible;
        }
        switch (event->reason())
        {
            case Qt::TabFocusReason:
            case Qt::BacktabFocusReason:
            case Qt::ShortcutFocusReason:
                return true;
            case Qt::ActiveWindowFocusReason:
            case Qt::PopupFocusReason:
                // 窗口切换或弹窗关闭后恢复焦点时保持原状态
                return currentVisible;
            default:
                return false;
        }
    }

    void setNavigationButtonsEnabled(QWidget* previous, bool previousEnabled, QWidget* next, bool nextEnabled)
    {
        // 先启用将变为可用的按钮，焦点才能交给它
        if (previousEnabled)
        {
            previous->setEnabled(true);
        }
        if (nextEnabled)
        {
            next->setEnabled(true);
        }
        const auto releaseFocus = [](QWidget* button, bool enabled, QWidget* other, bool otherEnabled)
        {
            if (enabled || !button->hasFocus())
            {
                return;
            }
            if (otherEnabled && other->isVisible() && other->focusPolicy() != Qt::NoFocus)
            {
                other->setFocus(Qt::OtherFocusReason);
            }
            else
            {
                button->clearFocus();
            }
        };
        releaseFocus(previous, previousEnabled, next, nextEnabled);
        releaseFocus(next, nextEnabled, previous, previousEnabled);
        previous->setEnabled(previousEnabled);
        next->setEnabled(nextEnabled);
    }

    void clearFocusWithin(QWidget* widget)
    {
        QWidget* focusWidget = QApplication::focusWidget();
        if (widget != nullptr && focusWidget != nullptr && (focusWidget == widget || widget->isAncestorOf(focusWidget)))
        {
            focusWidget->clearFocus();
        }
    }

    void applyFont(QWidget* widget, int pixelSize, QFont::Weight weight)
    {
        QFont widgetFont = widget->font();
        widgetFont.setPixelSize(pixelSize);
        widgetFont.setWeight(weight);
        widget->setFont(widgetFont);
    }

    void applyDefaultText(QObject* object, const char* key, const QString& current, const QString& text, const std::function<void(const QString &)>& apply)
    {
        const QByteArray propertyName = QByteArrayLiteral("_shadDefaultText_") + key;
        const QVariant previous = object->property(propertyName.constData());
        if (previous.isValid() && current != previous.toString())
        {
            return;
        }
        object->setProperty(propertyName.constData(), text);
        apply(text);
    }

    QPixmap grabContent(QWidget* widget)
    {
        const qreal ratio = widget->devicePixelRatioF();
        QPixmap pixmap(QSize(qCeil(widget->width() * ratio), qCeil(widget->height() * ratio)));
        pixmap.setDevicePixelRatio(ratio);
        pixmap.fill(Qt::transparent);
        // 不带 DrawWindowBackground，跳过根组件的背景填充
        widget->render(&pixmap, QPoint(), QRegion(), QWidget::DrawChildren);
        return pixmap;
    }

    QStringList monospaceFontFamilies()
    {
        // 字体在运行期间不变，只筛选一次；QFontDatabase::families 只列出已安装的字体，不触发别名加载
        static const QStringList families = []()
        {
            const QStringList installed = QFontDatabase::families();
            const QStringList candidates{QStringLiteral("Menlo"), QStringLiteral("Monaco"), QStringLiteral("Consolas"), QStringLiteral("Liberation Mono"), QStringLiteral("DejaVu Sans Mono"), QStringLiteral("Courier New")};
            QStringList result;
            for (const QString& family : candidates)
            {
                if (installed.contains(family, Qt::CaseInsensitive))
                {
                    result.append(family);
                }
            }
            if (result.isEmpty())
            {
                result.append(QFontDatabase::systemFont(QFontDatabase::FixedFont).family());
            }
            // 等宽字体缺少的 emoji 回退到内置彩色 emoji 字体
            const QString emojiFamily = emojiFontFamily();
            if (!emojiFamily.isEmpty())
            {
                result.append(emojiFamily);
            }
            return result;
        }();
        return families;
    }

    QString emojiFontFamily()
    {
#ifdef Q_OS_MACOS
        // 系统自带的 Apple Color Emoji 已包含国旗，资源中没有打包该字体
        return QString();
#else
        static const QString family = []() -> QString
        {
            const QString fontPath = QStringLiteral(":/ShadcnWidgetTools/Font/TwemojiMozilla.ttf");
            const int fontId = QFontDatabase::addApplicationFont(fontPath);
            if (fontId < 0)
            {
                qWarning("加载 emoji 字体失败: %s", qUtf8Printable(fontPath));
                return QString();
            }
            const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
            if (families.isEmpty())
            {
                qWarning("emoji 字体中没有可用的字体族: %s", qUtf8Printable(fontPath));
                return QString();
            }
            return families.constFirst();
        }();
        return family;
#endif
    }

    int textWidth(const QFont& font, const QString& text)
    {
        if (text.isEmpty())
        {
            return 0;
        }
        return qCeil(QFontMetricsF(font).horizontalAdvance(text));
    }

    QString elideText(const QFont& font, const QString& text, qreal width)
    {
        if (text.isEmpty() || width >= textWidth(font, text))
        {
            return text;
        }
        return QFontMetricsF(font).elidedText(text, Qt::ElideRight, qMax<qreal>(0.0, width));
    }

    qreal iconTextWidth(const QFont& font, const QString& text, bool hasIcon, qreal gap, int iconSize)
    {
        const qreal textWidth = text.isEmpty() ? 0.0 : QFontMetricsF(font).horizontalAdvance(text);
        const qreal iconWidth = hasIcon ? iconSize : 0.0;
        const qreal spacing = (hasIcon && !text.isEmpty()) ? gap : 0.0;
        return iconWidth + spacing + textWidth;
    }

    QRectF drawIconText(QPainter* painter, const QRectF& rect, ShadIconType::IconName shadIcon, const QIcon& fallbackIcon, const QString& text, const QFont& font, const QColor& color, qreal gap, Qt::Alignment alignment, int iconSize, qreal iconRotation, Qt::LayoutDirection direction)
    {
        const bool hasIcon = shadIcon != ShadIconType::None || !fallbackIcon.isNull();
        const QFontMetricsF fontMetrics(font);
        const qreal spacing = (hasIcon && !text.isEmpty()) ? gap : 0.0;
        const qreal maxTextWidth = qMax<qreal>(0.0, rect.width() - (hasIcon ? iconSize + spacing : 0.0));
        const QString elidedText = text.isEmpty() ? QString() : fontMetrics.elidedText(text, Qt::ElideRight, maxTextWidth);
        const qreal textWidth = elidedText.isEmpty() ? 0.0 : fontMetrics.horizontalAdvance(elidedText);
        const qreal totalWidth = (hasIcon ? iconSize : 0.0) + spacing + textWidth;

        // 先按 LTR 的逻辑坐标排列（Left 为起始侧），绘制前再按方向镜像
        const Qt::Alignment horizontalAlignment = logicalAlignment(direction, alignment);
        qreal x = rect.left() + (rect.width() - totalWidth) / 2.0;
        if (horizontalAlignment & Qt::AlignLeft)
        {
            x = rect.left();
        }
        else if (horizontalAlignment & Qt::AlignRight)
        {
            x = rect.right() - totalWidth;
        }

        if (hasIcon)
        {
            const QRectF iconRect = visualRect(direction, rect, QRectF(x, rect.center().y() - iconSize / 2.0, iconSize, iconSize));
            if (shadIcon != ShadIconType::None && !qFuzzyIsNull(iconRotation))
            {
                const QPointF center = iconRect.center();
                painter->save();
                painter->translate(center);
                painter->rotate(iconRotation);
                painter->translate(-center);
                ShadIcon::paint(painter, iconRect, shadIcon, color, iconSize);
                painter->restore();
            }
            else if (shadIcon != ShadIconType::None)
            {
                ShadIcon::paint(painter, iconRect, shadIcon, color, iconSize);
            }
            else
            {
                fallbackIcon.paint(painter, iconRect.toAlignedRect(), Qt::AlignCenter);
            }
            x += iconSize + spacing;
        }

        if (elidedText.isEmpty())
        {
            return QRectF();
        }
        const QRectF textRect = visualRect(direction, rect, QRectF(x, rect.top(), textWidth + 1.0, rect.height()));
        painter->save();
        painter->setFont(font);
        painter->setPen(color);
        painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignVCenter, elidedText);
        painter->restore();
        return visualRect(direction, rect, QRectF(x, rect.top(), textWidth, rect.height()));
    }

    void drawShimmerText(QPainter* painter, const QRectF& rect, const QString& text, const QFont& font, const QColor& color, qreal progress, Qt::Alignment alignment, Qt::LayoutDirection direction)
    {
        constexpr qreal kShimmerAngleDeg = 110.0;
        const QFontMetricsF metrics(font);
        const qreal spread = 3.0 * metrics.horizontalAdvance(QLatin1Char('0')) + 40.0;
        const qreal width = rect.width();
        const qreal imageWidth = 2.0 * width + 2.0 * spread;
        const qreal position = direction == Qt::RightToLeft ? progress : 1.0 - progress;
        const qreal imageLeft = rect.left() + position * (width - imageWidth);
        const QPointF center(imageLeft + imageWidth / 2.0, rect.center().y());
        const qreal angle = qDegreesToRadians(kShimmerAngleDeg);
        const QPointF axis(qSin(angle), -qCos(angle));
        const qreal length = imageWidth * qAbs(axis.x()) + rect.height() * qAbs(axis.y());
        QLinearGradient gradient(center - axis * (length / 2.0), center + axis * (length / 2.0));
        QColor highlight = withAlpha(color, 0.2);
        if (shadTheme->isDark())
        {
            float hue = 0.0f;
            float saturation = 0.0f;
            float lightness = 0.0f;
            float alpha = 0.0f;
            color.getHslF(&hue, &saturation, &lightness, &alpha);
            highlight = QColor::fromHslF(hue, saturation, qMax(0.8f, qMin(1.0f, lightness + 0.4f)), qMin(1.0f, alpha + 0.4f));
        }
        const QColor middle = mix(highlight, color, 0.5);
        const qreal outer = qBound<qreal>(0.0, spread / length, 0.5);
        gradient.setColorAt(0.0, color);
        gradient.setColorAt(0.5 - outer, color);
        gradient.setColorAt(0.5 - outer / 2.0, middle);
        gradient.setColorAt(0.5, highlight);
        gradient.setColorAt(0.5 + outer / 2.0, middle);
        gradient.setColorAt(0.5 + outer, color);
        gradient.setColorAt(1.0, color);
        painter->save();
        painter->setFont(font);
        painter->setPen(QPen(QBrush(gradient), 1.0));
        painter->drawText(rect, visualAlignment(direction, alignment), text);
        painter->restore();
    }
} // namespace ShadStyleHelper
