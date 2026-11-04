#include "ShadIcon.h"

#include <QFontDatabase>
#include <QPainter>
#include <QStringList>

QString ShadIcon::fontFamily()
{
    // 局部静态变量保证字体只加载一次且线程安全
    static const QString family = []() -> QString
    {
        const int fontId = QFontDatabase::addApplicationFont(QStringLiteral(":/ShadcnWidgetTools/Font/lucide.ttf"));
        if (fontId < 0)
        {
            qWarning("加载 Lucide 图标字体失败: :/ShadcnWidgetTools/Font/lucide.ttf");
            return QString();
        }
        const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        if (families.isEmpty())
        {
            qWarning("Lucide 图标字体中没有可用的字体族");
            return QString();
        }
        return families.first();
    }();
    return family;
}

QFont ShadIcon::font(int pixelSize)
{
    QFont iconFont(fontFamily());
    iconFont.setPixelSize(qMax(1, pixelSize));
    iconFont.setStyleStrategy(QFont::NoFontMerging);
    iconFont.setHintingPreference(QFont::PreferNoHinting);
    return iconFont;
}

QString ShadIcon::glyph(ShadIconType::IconName icon)
{
    if (icon == ShadIconType::None)
    {
        return QString();
    }
    return QString(QChar(static_cast<char16_t>(icon)));
}

void ShadIcon::paint(QPainter* painter, const QRectF& rect, ShadIconType::IconName icon, const QColor& color, int pixelSize)
{
    if (painter == nullptr || icon == ShadIconType::None)
    {
        return;
    }
    painter->save();
    painter->setRenderHint(QPainter::TextAntialiasing, true);
    painter->setFont(font(pixelSize));
    painter->setPen(color);
    painter->drawText(rect, Qt::AlignCenter, glyph(icon));
    painter->restore();
}

QPixmap ShadIcon::pixmap(ShadIconType::IconName icon, const QColor& color, int pixelSize, qreal devicePixelRatio)
{
    const qreal ratio = qMax<qreal>(1.0, devicePixelRatio);
    QPixmap result(qRound(pixelSize * ratio), qRound(pixelSize * ratio));
    result.setDevicePixelRatio(ratio);
    result.fill(Qt::transparent);
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing, true);
    paint(&painter, QRectF(0, 0, pixelSize, pixelSize), icon, color, pixelSize);
    painter.end();
    return result;
}

QIcon ShadIcon::icon(ShadIconType::IconName icon, const QColor& color, int pixelSize)
{
    QIcon result;
    // 同时提供 1x 与 2x 位图，适配高分屏
    result.addPixmap(pixmap(icon, color, pixelSize, 1.0));
    result.addPixmap(pixmap(icon, color, pixelSize, 2.0));
    return result;
}
