#include "ShadImageViewerToolbar.h"

#include <QColor>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QRectF>
#include <QSizePolicy>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadImageViewerButton.h"
#include "ShadStyleHelper.h"

namespace
{
    constexpr int kHeight = 40;
    constexpr int kPadding = 4;
    constexpr int kSpacing = 2;
    constexpr int kCounterPadding = 8;
    constexpr int kSeparatorHeight = 16;
    const QColor kCounterColor(255, 255, 255);
    const QColor kSeparatorColor(255, 255, 255, 51);
} // namespace

ShadImageViewerToolbar::ShadImageViewerToolbar(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::NoFocus);
    setFixedHeight(kHeight);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    _layout = new QHBoxLayout(this);
    _layout->setContentsMargins(kPadding, kPadding, kPadding, kPadding);
    _layout->setSpacing(kSpacing);

    _counterLabel = new QLabel(this);
    ShadStyleHelper::applyFont(_counterLabel, 14, QFont::Medium);
    QPalette counterPalette = _counterLabel->palette();
    counterPalette.setColor(QPalette::WindowText, kCounterColor);
    _counterLabel->setPalette(counterPalette);
    _counterLabel->setContentsMargins(kCounterPadding, 0, kCounterPadding, 0);
    _counterLabel->setAlignment(Qt::AlignCenter);
    _layout->addWidget(_counterLabel);

    _separator = new QWidget(this);
    _separator->setFixedSize(1, kSeparatorHeight);
    _separator->setAutoFillBackground(true);
    QPalette separatorPalette = _separator->palette();
    separatorPalette.setColor(QPalette::Window, kSeparatorColor);
    _separator->setPalette(separatorPalette);
    _layout->addWidget(_separator, 0, Qt::AlignVCenter);
}

ShadImageViewerToolbar::~ShadImageViewerToolbar()
{
}

QColor ShadImageViewerToolbar::backgroundColor()
{
    // 半透明深色底，图片铺满时仍能看清图标
    return QColor(24, 24, 27, 217);
}

QColor ShadImageViewerToolbar::borderColor()
{
    return QColor(255, 255, 255, 26);
}

ShadImageViewerButton* ShadImageViewerToolbar::addButton(ShadIconType::IconName icon)
{
    ShadImageViewerButton* button = new ShadImageViewerButton(icon, ShadImageViewerButton::Square, this);
    _layout->addWidget(button);
    return button;
}

QLabel* ShadImageViewerToolbar::addLabel(const QString& widestText)
{
    QLabel* label = new QLabel(this);
    ShadStyleHelper::applyFont(label, 14, QFont::Medium);
    label->setPalette(_counterLabel->palette());
    label->setAlignment(Qt::AlignCenter);
    label->setFixedWidth(ShadStyleHelper::textWidth(label->font(), widestText) + 2 * kCounterPadding);
    _layout->addWidget(label);
    return label;
}

void ShadImageViewerToolbar::setCounterText(const QString& text)
{
    _counterLabel->setText(text);
}

void ShadImageViewerToolbar::setCounterVisible(bool visible)
{
    _counterLabel->setVisible(visible);
    _separator->setVisible(visible);
}

void ShadImageViewerToolbar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF bounds = QRectF(rect());
    const qreal radius = bounds.height() / 2.0;
    ShadStyleHelper::fillRoundedRect(&painter, bounds, radius, backgroundColor());
    ShadStyleHelper::strokeRoundedRect(&painter, bounds, radius, borderColor());
}
