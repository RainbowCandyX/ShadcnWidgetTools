#include "ShadQrCode.h"

#include <QColor>
#include <QEvent>
#include <QFont>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QRectF>
#include <QSizePolicy>
#include <QString>
#include <QtGlobal>

#include "ShadQrCodePrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadQrCode::ShadQrCode(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadQrCodePrivate())
{
    Q_D(ShadQrCode);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFixedSize(d->_size, d->_size);
    ShadStyleHelper::applyFont(this, 12, QFont::Normal);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadQrCode::ShadQrCode(const QString& text, QWidget* parent)
    : ShadQrCode(parent)
{
    setText(text);
}

ShadQrCode::~ShadQrCode()
{
}

void ShadQrCode::setText(const QString& text)
{
    Q_D(ShadQrCode);
    if (d->_text == text)
    {
        return;
    }
    d->_text = text;
    d->encode();
    update();
    Q_EMIT textChanged();
}

QString ShadQrCode::getText() const
{
    Q_D(const ShadQrCode);
    return d->_text;
}

void ShadQrCode::setErrorCorrectionLevel(ShadQrCodeType::ErrorCorrection level)
{
    Q_D(ShadQrCode);
    if (d->_level == level)
    {
        return;
    }
    d->_level = level;
    d->encode();
    update();
}

ShadQrCodeType::ErrorCorrection ShadQrCode::getErrorCorrectionLevel() const
{
    Q_D(const ShadQrCode);
    return d->_level;
}

bool ShadQrCode::isValid() const
{
    Q_D(const ShadQrCode);
    return d->_symbol.size > 0;
}

int ShadQrCode::getVersion() const
{
    Q_D(const ShadQrCode);
    return d->_symbol.version;
}

int ShadQrCode::getModuleCount() const
{
    Q_D(const ShadQrCode);
    return d->_symbol.size;
}

void ShadQrCode::setQrSize(int size)
{
    Q_D(ShadQrCode);
    const int qrSize = qMax(1, size);
    if (d->_size == qrSize)
    {
        return;
    }
    d->_size = qrSize;
    setFixedSize(qrSize, qrSize);
    update();
}

int ShadQrCode::getQrSize() const
{
    Q_D(const ShadQrCode);
    return d->_size;
}

void ShadQrCode::setQuietZone(int modules)
{
    Q_D(ShadQrCode);
    const int quietZone = qMax(0, modules);
    if (d->_quietZone == quietZone)
    {
        return;
    }
    d->_quietZone = quietZone;
    update();
}

int ShadQrCode::getQuietZone() const
{
    Q_D(const ShadQrCode);
    return d->_quietZone;
}

void ShadQrCode::setForegroundColor(const QColor& color)
{
    Q_D(ShadQrCode);
    d->_foregroundColor = color;
    update();
}

QColor ShadQrCode::getForegroundColor() const
{
    Q_D(const ShadQrCode);
    return d->_foregroundColor;
}

void ShadQrCode::setBackgroundColor(const QColor& color)
{
    Q_D(ShadQrCode);
    d->_backgroundColor = color;
    update();
}

QColor ShadQrCode::getBackgroundColor() const
{
    Q_D(const ShadQrCode);
    return d->_backgroundColor;
}

QSize ShadQrCode::sizeHint() const
{
    Q_D(const ShadQrCode);
    return QSize(d->_size, d->_size);
}

void ShadQrCode::changeEvent(QEvent* event)
{
    // 错误状态的提示文字在绘制时翻译
    if (event->type() == QEvent::EnabledChange || event->type() == QEvent::LanguageChange)
    {
        update();
    }
    QWidget::changeEvent(event);
}

void ShadQrCode::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadQrCode);
    using namespace ShadStyleHelper;
    const int side = qMin(width(), height());
    const QRect outerRect((width() - side) / 2, (height() - side) / 2, side, side);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    QPainter painter(this);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    if (d->_isTooLong)
    {
        // 错误状态：bg-muted 虚线边框，中间显示原因
        fillRoundedRect(&painter, QRectF(outerRect), radius, shadTheme->getColor(ShadThemeType::Muted));
        strokeDashedRoundedRect(&painter, QRectF(outerRect), radius, shadTheme->getColor(ShadThemeType::Border));
        const int padding = ShadQrCodePrivate::kErrorPadding;
        painter.setPen(shadTheme->getColor(ShadThemeType::MutedForeground));
        painter.setFont(font());
        painter.drawText(outerRect.adjusted(padding, padding, -padding, -padding), Qt::AlignCenter | Qt::TextWordWrap, tr("内容过长，无法生成二维码"));
        return;
    }

    const QColor background = hasWindowEffect(this) ? withAlpha(d->_backgroundColor, shadTheme->getWindowEffectSurfaceOpacity()) : d->_backgroundColor;
    fillRoundedRect(&painter, QRectF(outerRect), radius, background);
    strokeRoundedRect(&painter, QRectF(outerRect), radius, shadTheme->getColor(ShadThemeType::Border));
    if (d->_symbol.size == 0)
    {
        return;
    }

    // 格子边长取整数像素，剩余空间均分在四周
    const int border = ShadQrCodePrivate::kBorderWidth;
    const int totalModules = d->_symbol.size + 2 * d->_quietZone;
    const int innerSide = side - 2 * border;
    const int moduleSize = qMax(1, innerSide / totalModules);
    const int codeSide = moduleSize * totalModules;
    const int originX = outerRect.x() + (side - codeSide) / 2 + d->_quietZone * moduleSize;
    const int originY = outerRect.y() + (side - codeSide) / 2 + d->_quietZone * moduleSize;
    painter.setRenderHint(QPainter::Antialiasing, false);
    // 同一行相邻的深色格子合并为一个矩形绘制，减少绘制次数
    for (int y = 0; y < d->_symbol.size; ++y)
    {
        int x = 0;
        while (x < d->_symbol.size)
        {
            if (!d->isDark(x, y))
            {
                ++x;
                continue;
            }
            const int runStart = x;
            while (x < d->_symbol.size && d->isDark(x, y))
            {
                ++x;
            }
            painter.fillRect(QRect(originX + runStart * moduleSize, originY + y * moduleSize, (x - runStart) * moduleSize, moduleSize), d->_foregroundColor);
        }
    }
}
