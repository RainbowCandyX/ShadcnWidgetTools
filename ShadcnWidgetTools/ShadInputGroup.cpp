#include "ShadInputGroup.h"

#include <QAbstractAnimation>
#include <QEasingCurve>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QHideEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSizePolicy>
#include <QVariant>
#include <QVariantAnimation>
#include <QtGlobal>

#include "ShadButton.h"
#include "ShadIcon.h"
#include "ShadInput.h"
#include "ShadInputGroupPrivate.h"
#include "ShadLabel.h"
#include "ShadStyleHelper.h"
#include "ShadTextarea.h"
#include "ShadTheme.h"

namespace
{
    // 附加区图标 size-4，text-muted-foreground
    constexpr int kIconSize = 16;
    // 宽度由布局决定，没有附加区时的理想宽度
    constexpr int kDefaultInputWidth = 240;

    // 附加区中的图标，LoaderCircle 自动旋转（对应 Spinner）；不处理鼠标按下，事件交给输入组聚焦输入框，仍可显示自身的 toolTip
    class AddonIcon : public QWidget
    {
        public:
            AddonIcon(ShadIconType::IconName icon, QWidget* parent)
                : QWidget(parent), _icon(icon)
            {
                setFixedSize(kIconSize, kIconSize);
                if (icon == ShadIconType::LoaderCircle)
                {
                    _spinAnimation = new QVariantAnimation(this);
                    _spinAnimation->setStartValue(0.0);
                    _spinAnimation->setEndValue(360.0);
                    _spinAnimation->setDuration(ShadStyleHelper::kSpinDurationMs);
                    _spinAnimation->setLoopCount(-1);
                    _spinAnimation->setEasingCurve(QEasingCurve::Linear);
                    QObject::connect(_spinAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
                    {
                        _angle = value.toReal();
                        update();
                    });
                }
                QObject::connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
            }

        protected:
            void showEvent(QShowEvent* event) override
            {
                QWidget::showEvent(event);
                if (_spinAnimation != nullptr)
                {
                    _spinAnimation->start();
                }
            }

            void hideEvent(QHideEvent* event) override
            {
                if (_spinAnimation != nullptr)
                {
                    _spinAnimation->stop();
                }
                QWidget::hideEvent(event);
            }

            void paintEvent(QPaintEvent* event) override
            {
                Q_UNUSED(event);
                QPainter painter(this);
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.setOpacity(isEnabled() ? 1.0 : ShadStyleHelper::kDisabledOpacity);
                const QRectF iconRect(rect());
                const QPointF center = iconRect.center();
                painter.translate(center);
                painter.rotate(_angle);
                painter.translate(-center);
                ShadIcon::paint(&painter, iconRect, _icon, shadTheme->getColor(ShadThemeType::MutedForeground), kIconSize);
            }

        private:
            ShadIconType::IconName _icon{ShadIconType::None};
            QVariantAnimation* _spinAnimation{nullptr};
            qreal _angle{0.0};
    };
} // namespace

ShadInputGroup::ShadInputGroup(QWidget* parent)
    : ShadInputGroup(ShadInputGroupType::LineEdit, parent)
{
}

ShadInputGroup::ShadInputGroup(ShadInputGroupType::Control control, QWidget* parent)
    : QWidget(parent), d_ptr(new ShadInputGroupPrivate())
{
    Q_D(ShadInputGroup);
    d->q_ptr = this;
    d->_controlType = control;
    // 对应 w-full：横向占满，高度由内容决定
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setCursor(Qt::IBeamCursor);
    QWidget* widget = nullptr;
    if (control == ShadInputGroupType::PlainText)
    {
        widget = new ShadTextarea(this);
    }
    else
    {
        widget = new ShadInput(this);
    }
    // 嵌入模式：外框、焦点环与内边距由输入组负责
    ShadStyleHelper::setEmbedded(widget, true);
    widget->installEventFilter(this);
    setFocusProxy(widget);
    d->_control = widget;
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
    d->relayout();
}

ShadInputGroup::~ShadInputGroup()
{
}

ShadInput* ShadInputGroup::getInput() const
{
    Q_D(const ShadInputGroup);
    return qobject_cast<ShadInput*>(d->control());
}

ShadTextarea* ShadInputGroup::getTextarea() const
{
    Q_D(const ShadInputGroup);
    return qobject_cast<ShadTextarea*>(d->control());
}

void ShadInputGroup::setInvalid(bool invalid)
{
    Q_D(ShadInputGroup);
    if (d->_isInvalid == invalid)
    {
        return;
    }
    d->_isInvalid = invalid;
    update();
}

bool ShadInputGroup::isInvalid() const
{
    Q_D(const ShadInputGroup);
    return d->_isInvalid;
}

ShadLabel* ShadInputGroup::addText(ShadInputGroupType::AddonPosition position, const QString& text)
{
    Q_D(ShadInputGroup);
    // QLabel 不处理鼠标按下，事件会交给输入组聚焦输入框
    ShadLabel* label = new ShadLabel(text, this);
    label->setColorRole(ShadThemeType::MutedForeground);
    ShadStyleHelper::applyFont(label, 14, QFont::Medium);
    d->addAddon(position, label);
    return label;
}

QWidget* ShadInputGroup::addIcon(ShadInputGroupType::AddonPosition position, ShadIconType::IconName icon)
{
    Q_D(ShadInputGroup);
    AddonIcon* iconWidget = new AddonIcon(icon, this);
    d->addAddon(position, iconWidget);
    return iconWidget;
}

ShadButton* ShadInputGroup::addButton(ShadInputGroupType::AddonPosition position, const QString& text, ShadIconType::IconName icon, ShadButtonType::Size size)
{
    Q_D(ShadInputGroup);
    ShadButton* button = new ShadButton(text, ShadButtonType::Ghost, this);
    button->setButtonSize(size);
    button->setShadIcon(icon);
    d->addAddon(position, button);
    return button;
}

void ShadInputGroup::addWidget(ShadInputGroupType::AddonPosition position, QWidget* widget)
{
    Q_D(ShadInputGroup);
    if (widget == nullptr || widget == this)
    {
        return;
    }
    d->addAddon(position, widget);
}

void ShadInputGroup::addStretch(ShadInputGroupType::AddonPosition position)
{
    Q_D(ShadInputGroup);
    d->addAddon(position, nullptr);
}

QSize ShadInputGroup::sizeHint() const
{
    Q_D(const ShadInputGroup);
    using Position = ShadInputGroupType::AddonPosition;
    const int margin = 2 * (ShadStyleHelper::kFocusRingWidth + 1);
    // 行内：附加区 + 输入框；上下附加区单独一行
    int inlineWidth = kDefaultInputWidth;
    if (d->hasAddons(Position::InlineStart))
    {
        inlineWidth += ShadInputGroupPrivate::kAddonPadding + d->rowWidth(Position::InlineStart) + ShadInputGroupPrivate::kInputPaddingNearAddon;
    }
    if (d->hasAddons(Position::InlineEnd))
    {
        inlineWidth += ShadInputGroupPrivate::kAddonPadding + d->rowWidth(Position::InlineEnd) + ShadInputGroupPrivate::kInputPaddingNearAddon;
    }
    const int blockWidth = 2 * ShadInputGroupPrivate::kAddonPadding + qMax(d->rowWidth(Position::BlockStart), d->rowWidth(Position::BlockEnd));
    return QSize(qMax(inlineWidth, blockWidth) + margin, d->innerHeight() + margin);
}

QSize ShadInputGroup::minimumSizeHint() const
{
    Q_D(const ShadInputGroup);
    using Position = ShadInputGroupType::AddonPosition;
    const int margin = 2 * (ShadStyleHelper::kFocusRingWidth + 1);
    // min-w-0：输入框可以收窄，附加区保持完整
    const int inlineWidth = 2 * ShadInputGroupPrivate::kAddonPadding + d->rowWidth(Position::InlineStart) + d->rowWidth(Position::InlineEnd);
    return QSize(inlineWidth + margin, d->innerHeight() + margin);
}

bool ShadInputGroup::event(QEvent* event)
{
    Q_D(ShadInputGroup);
    switch (event->type())
    {
        case QEvent::LayoutRequest:
            {
                // 没有布局时，子组件 updateGeometry（例如多行输入框自动增高）会向父组件发送 LayoutRequest
                updateGeometry();
                d->relayout();
                break;
            }
        case QEvent::ChildRemoved:
            {
                updateGeometry();
                d->relayout();
                break;
            }
        default:
            {
                break;
            }
    }
    return QWidget::event(event);
}

bool ShadInputGroup::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadInputGroup);
    if (watched == d->control() && (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut))
    {
        // 输入框焦点变化时外框切换 border-ring 与焦点环
        update();
    }
    else if (event->type() == QEvent::ShowToParent || event->type() == QEvent::HideToParent)
    {
        updateGeometry();
        d->relayout();
    }
    return QWidget::eventFilter(watched, event);
}

void ShadInputGroup::changeEvent(QEvent* event)
{
    Q_D(ShadInputGroup);
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        d->relayout();
    }
    QWidget::changeEvent(event);
}

void ShadInputGroup::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadInputGroup);
    QWidget::resizeEvent(event);
    d->relayout();
}

void ShadInputGroup::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadInputGroup);
    // 点击附加区或内边距时聚焦输入框（对应 InputGroupAddon 的 onClick）
    if (event->button() == Qt::LeftButton && d->control() != nullptr && isEnabled())
    {
        d->control()->setFocus(Qt::MouseFocusReason);
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ShadInputGroup::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadInputGroup);
    using namespace ShadStyleHelper;
    const QRectF bodyRect = insetRect(rect(), kFocusRingWidth);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const bool isFocused = d->control() != nullptr && d->control()->hasFocus();
    // 放入 ShadButtonGroup 时与相邻组件拼接
    const GroupEdges edges = groupEdges(this);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    drawShadowXs(&painter, bodyRect, radius, edges);
    if (shadTheme->isDark())
    {
        fillRoundedRect(&painter, bodyRect, radius, withAlpha(shadTheme->getColor(ShadThemeType::Input), 0.3), edges);
    }
    // 输入框聚焦：border-ring ring-[3px] ring-ring/50；错误态：border-destructive 与 destructive 焦点环
    QColor borderColor = shadTheme->getColor(isFocused ? ShadThemeType::Ring : ShadThemeType::Input);
    if (d->_isInvalid)
    {
        borderColor = shadTheme->getColor(ShadThemeType::Destructive);
    }
    strokeRoundedRect(&painter, bodyRect, radius, borderColor, edges);
    if (isFocused || d->_isInvalid)
    {
        drawFocusRing(&painter, bodyRect, radius, focusRingColor(d->_isInvalid), edges);
    }
}
