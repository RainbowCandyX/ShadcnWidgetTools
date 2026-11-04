#include "ShadFieldSeparator.h"

#include <QColor>
#include <QEvent>
#include <QFont>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QRectF>
#include <QSizePolicy>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadFieldSeparatorPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // h-5
    constexpr int kHeight = 20;
    // 文字两侧 px-2
    constexpr int kTextPadding = 8;
} // namespace

ShadFieldSeparator::ShadFieldSeparator(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadFieldSeparatorPrivate())
{
    Q_D(ShadFieldSeparator);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadFieldSeparator::ShadFieldSeparator(const QString& text, QWidget* parent)
    : ShadFieldSeparator(parent)
{
    setText(text);
}

ShadFieldSeparator::~ShadFieldSeparator()
{
}

void ShadFieldSeparator::setText(const QString& text)
{
    Q_D(ShadFieldSeparator);
    d->_text = text;
    updateGeometry();
    update();
}

QString ShadFieldSeparator::getText() const
{
    Q_D(const ShadFieldSeparator);
    return d->_text;
}

QSize ShadFieldSeparator::sizeHint() const
{
    Q_D(const ShadFieldSeparator);
    const int textWidth = d->_text.isEmpty() ? 0 : ShadStyleHelper::textWidth(font(), d->_text) + 2 * kTextPadding;
    return QSize(textWidth, kHeight);
}

QSize ShadFieldSeparator::minimumSizeHint() const
{
    return sizeHint();
}

void ShadFieldSeparator::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::FontChange)
    {
        updateGeometry();
    }
    QWidget::changeEvent(event);
}

void ShadFieldSeparator::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadFieldSeparator);
    QPainter painter(this);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    const QColor lineColor = shadTheme->getColor(ShadThemeType::Border);
    // Separator 位于 top-1/2，1px 高
    const int lineY = height() / 2;
    if (d->_text.isEmpty())
    {
        painter.fillRect(QRect(0, lineY, width(), 1), lineColor);
        return;
    }
    // 文字居中，背景为 bg-background，这里直接在文字两侧断开横线
    const int textWidth = qMin(width(), ShadStyleHelper::textWidth(font(), d->_text) + 2 * kTextPadding);
    const int textLeft = (width() - textWidth) / 2;
    painter.fillRect(QRect(0, lineY, textLeft, 1), lineColor);
    painter.fillRect(QRect(textLeft + textWidth, lineY, width() - textLeft - textWidth, 1), lineColor);
    painter.setPen(shadTheme->getColor(ShadThemeType::MutedForeground));
    painter.drawText(QRectF(textLeft, 0, textWidth, height()), Qt::AlignCenter, ShadStyleHelper::elideText(font(), d->_text, textWidth - 2 * kTextPadding));
}
