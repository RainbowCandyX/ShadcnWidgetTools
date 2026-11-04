#include "ShadAlert.h"

#include <QEvent>
#include <QFont>
#include <QLabel>
#include <QMargins>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QtGlobal>

#include "ShadAlertPrivate.h"
#include "ShadIcon.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // 1px 边框 + px-4 py-3，图标 size-4 向下偏移 translate-y-0.5
    constexpr int kBodyInsetX = 1 + 16;
    constexpr int kBodyInsetY = 1 + 12;
    constexpr int kIconSize = 16;
    constexpr int kIconOffsetY = 2;
    // 最小宽度下至少留给文字的宽度
    constexpr int kMinTextWidth = 64;
} // namespace

ShadAlert::ShadAlert(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadAlertPrivate())
{
    Q_D(ShadAlert);
    d->q_ptr = this;
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy.setHeightForWidth(true);
    setSizePolicy(sizePolicy);
    // 标题 font-medium，描述 text-sm；颜色由 updateContent 按变体设置，位置由 layoutLabels 计算
    d->_titleLabel = new QLabel(this);
    d->_titleLabel->setWordWrap(true);
    ShadStyleHelper::applyFont(d->_titleLabel, 14, QFont::Medium);
    d->_descriptionLabel = new QLabel(this);
    d->_descriptionLabel->setWordWrap(true);
    ShadStyleHelper::applyFont(d->_descriptionLabel, 14, QFont::Normal);
    d->updateContent();
    connect(shadTheme, &ShadTheme::themeChanged, this, [d]() { d->updateContent(); });
}

ShadAlert::ShadAlert(ShadIconType::IconName icon, const QString& title, const QString& description, ShadAlertType::Variant variant, QWidget* parent)
    : ShadAlert(parent)
{
    Q_D(ShadAlert);
    d->_icon = icon;
    d->_variant = variant;
    d->_titleLabel->setText(title);
    d->_descriptionLabel->setText(description);
    d->updateContent();
}

ShadAlert::~ShadAlert()
{
}

void ShadAlert::setVariant(ShadAlertType::Variant variant)
{
    Q_D(ShadAlert);
    if (d->_variant == variant)
    {
        return;
    }
    d->_variant = variant;
    d->updateContent();
}

ShadAlertType::Variant ShadAlert::getVariant() const
{
    Q_D(const ShadAlert);
    return d->_variant;
}

void ShadAlert::setShadIcon(ShadIconType::IconName icon)
{
    Q_D(ShadAlert);
    if (d->_icon == icon)
    {
        return;
    }
    d->_icon = icon;
    d->updateContent();
}

ShadIconType::IconName ShadAlert::getShadIcon() const
{
    Q_D(const ShadAlert);
    return d->_icon;
}

void ShadAlert::setTitle(const QString& title)
{
    Q_D(ShadAlert);
    d->_titleLabel->setText(title);
    d->updateContent();
}

QString ShadAlert::getTitle() const
{
    Q_D(const ShadAlert);
    return d->_titleLabel->text();
}

void ShadAlert::setDescription(const QString& description)
{
    Q_D(ShadAlert);
    d->_descriptionLabel->setText(description);
    d->updateContent();
}

QString ShadAlert::getDescription() const
{
    Q_D(const ShadAlert);
    return d->_descriptionLabel->text();
}

void ShadAlert::setAction(QWidget* action)
{
    Q_D(ShadAlert);
    if (d->_action == action)
    {
        return;
    }
    if (!d->_action.isNull())
    {
        ShadStyleHelper::clearFocusWithin(d->_action);
        d->_action->hide();
        d->_action->deleteLater();
    }
    d->_action = action;
    if (action != nullptr)
    {
        // 操作组件不参与布局，绝对定位在右上角
        action->setParent(this);
        action->show();
    }
    d->updateContent();
}

QWidget* ShadAlert::getAction() const
{
    Q_D(const ShadAlert);
    return d->_action.data();
}

void ShadAlert::setCustomColors(ShadThemeType::ThemeMode themeMode, const QColor& background, const QColor& border, const QColor& foreground)
{
    Q_D(ShadAlert);
    ShadAlertPrivate::CustomColors& colors = themeMode == ShadThemeType::Dark ? d->_darkColors : d->_lightColors;
    colors.background = background;
    colors.border = border;
    colors.foreground = foreground;
    d->updateContent();
}

void ShadAlert::clearCustomColors()
{
    Q_D(ShadAlert);
    d->_lightColors = ShadAlertPrivate::CustomColors();
    d->_darkColors = ShadAlertPrivate::CustomColors();
    d->updateContent();
}

bool ShadAlert::hasHeightForWidth() const
{
    return true;
}

int ShadAlert::heightForWidth(int width) const
{
    Q_D(const ShadAlert);
    const int alertWidth = qMin(width, maximumWidth());
    const QMargins margins = d->textMargins();
    const int textWidth = qMax(1, alertWidth - margins.left() - margins.right());
    return qMax(margins.top() + d->textHeightForWidth(textWidth) + margins.bottom(), minimumHeight());
}

QSize ShadAlert::sizeHint() const
{
    Q_D(const ShadAlert);
    const QMargins margins = d->textMargins();
    int textWidth = 0;
    if (!d->_titleLabel->text().isEmpty())
    {
        textWidth = qMax(textWidth, d->_titleLabel->sizeHint().width());
    }
    if (!d->_descriptionLabel->text().isEmpty())
    {
        textWidth = qMax(textWidth, d->_descriptionLabel->sizeHint().width());
    }
    const int width = qMin(margins.left() + textWidth + margins.right(), maximumWidth());
    return QSize(width, heightForWidth(width));
}

QSize ShadAlert::minimumSizeHint() const
{
    Q_D(const ShadAlert);
    // 高度由 heightForWidth 决定，这里只给出能显示少量文字的最小宽度
    const QMargins margins = d->textMargins();
    return QSize(margins.left() + kMinTextWidth + margins.right(), qMax(margins.top() + margins.bottom(), minimumHeight()));
}

void ShadAlert::changeEvent(QEvent* event)
{
    Q_D(ShadAlert);
    if (event->type() == QEvent::LayoutDirectionChange || event->type() == QEvent::EnabledChange)
    {
        d->updateContent();
    }
    QWidget::changeEvent(event);
}

void ShadAlert::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadAlert);
    QWidget::resizeEvent(event);
    d->layoutLabels();
    d->updateActionGeometry();
}

void ShadAlert::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadAlert);
    using namespace ShadStyleHelper;
    const QRectF bodyRect(rect());
    // rounded-lg border bg-card
    const qreal radius = shadTheme->getRadius(ShadThemeType::Large);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    fillRoundedRect(&painter, bodyRect, radius, d->backgroundColor());
    strokeRoundedRect(&painter, bodyRect, radius, d->borderColor());
    if (d->_icon != ShadIconType::None)
    {
        // 图标与标题同色（*:[svg]:text-current），与第一行文字对齐，位于起始侧
        const QRectF iconRect(kBodyInsetX, kBodyInsetY + kIconOffsetY, kIconSize, kIconSize);
        ShadIcon::paint(&painter, visualRect(layoutDirection(), bodyRect, iconRect), d->_icon, d->foregroundColor(), kIconSize);
    }
}
