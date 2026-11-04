#include "ExampleMutedItem.h"

#include <QColor>
#include <QHBoxLayout>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>

#include "ShadDef.h"
#include "ShadTheme.h"

namespace
{
    // p-4 gap-4
    constexpr int kPadding = 16;
    constexpr int kGap = 16;
    // bg-muted/50
    constexpr qreal kBackgroundAlpha = 0.5;
} // namespace

ExampleMutedItem::ExampleMutedItem(QWidget* parent)
    : QWidget(parent)
{
    _layout = new QHBoxLayout(this);
    _layout->setContentsMargins(kPadding, kPadding, kPadding, kPadding);
    _layout->setSpacing(kGap);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ExampleMutedItem::~ExampleMutedItem()
{
}

QHBoxLayout* ExampleMutedItem::itemLayout() const
{
    return _layout;
}

void ExampleMutedItem::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QColor background = shadTheme->getColor(ShadThemeType::Muted);
    background.setAlphaF(background.alphaF() * kBackgroundAlpha);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Large);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(background);
    painter.drawRoundedRect(QRectF(rect()), radius, radius);
}
