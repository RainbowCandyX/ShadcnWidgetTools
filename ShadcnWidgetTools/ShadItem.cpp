#include "ShadItem.h"

#include <QBoxLayout>
#include <QColor>
#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QSizePolicy>
#include <QString>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadItemContent.h"
#include "ShadItemGroup.h"
#include "ShadItemMedia.h"
#include "ShadItemPrivate.h"
#include "ShadLabel.h"
#include "ShadMaxWidthBoxLayout.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTransition.h"

namespace
{
    // 操作区布局：按钮等可聚焦组件四周各有 3px 焦点环边距
    // 尺寸扣除这部分边距、摆放时再向外扩出，按钮的可视边缘与内容对齐，行高按按钮的可视高度计算
    // 按钮是列表项的直接子组件，扩出的焦点环落在列表项的内边距中，不会被裁剪
    class ShadItemActionsLayout : public QHBoxLayout
    {
        public:
            ShadItemActionsLayout()
                : QHBoxLayout()
            {
            }

            QSize sizeHint() const override
            {
                return shrink(QHBoxLayout::sizeHint());
            }

            QSize minimumSize() const override
            {
                return shrink(QHBoxLayout::minimumSize());
            }

            void setGeometry(const QRect& rect) override
            {
                const int ring = ShadStyleHelper::kFocusRingWidth;
                QHBoxLayout::setGeometry(rect.adjusted(-ring, -ring, ring, ring));
            }

        private:
            QSize shrink(const QSize& size) const
            {
                if (isEmpty())
                {
                    return size;
                }
                const int ring = ShadStyleHelper::kFocusRingWidth;
                return QSize(qMax(0, size.width() - 2 * ring), qMax(0, size.height() - 2 * ring));
            }
    };

    // 页眉与页脚的便捷文字
    ShadLabel* createEdgeLabel(const QString& text, bool isHeader, QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(text, parent);
        label->setWordWrap(true);
        if (isHeader)
        {
            // text-sm font-medium
            ShadStyleHelper::applyFont(label, 14, QFont::Medium);
        }
        else
        {
            // text-sm text-muted-foreground
            ShadStyleHelper::applyFont(label, 14, QFont::Normal);
            label->setColorRole(ShadThemeType::MutedForeground);
        }
        return label;
    }
} // namespace

ShadItem::ShadItem(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadItemPrivate())
{
    Q_D(ShadItem);
    d->q_ptr = this;
    QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);

    // 设置最大宽度后，描述按实际宽度计算行数
    d->_layout = new ShadMaxWidthBoxLayout(QBoxLayout::TopToBottom, this);
    d->_rowLayout = new QHBoxLayout();
    d->_rowLayout->setContentsMargins(0, 0, 0, 0);
    d->_media = new ShadItemMedia(this);
    d->_content = new ShadItemContent(this);
    d->_actionsLayout = new ShadItemActionsLayout();
    d->_actionsLayout->setContentsMargins(0, 0, 0, 0);
    // gap-2，扣除相邻组件两侧的焦点环边距
    d->_actionsLayout->setSpacing(ShadItemPrivate::kActionGap - 2 * ShadStyleHelper::kFocusRingWidth);
    d->_rowLayout->addWidget(d->_media, 0, Qt::AlignVCenter);
    // flex-1：内容占据剩余宽度
    d->_rowLayout->addWidget(d->_content, 1);
    d->_rowLayout->addLayout(d->_actionsLayout);
    d->_layout->addLayout(d->_rowLayout);

    d->_hoverTransition = new ShadTransition(this, ShadItemPrivate::kHoverDurationMs);
    d->applySize();
    d->updateMediaAlignment();
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadItem::ShadItem(const QString& title, const QString& description, QWidget* parent)
    : ShadItem(parent)
{
    setTitle(title);
    setDescription(description);
}

ShadItem::~ShadItem()
{
}

void ShadItem::setVariant(ShadItemType::Variant variant)
{
    Q_D(ShadItem);
    if (d->_variant == variant)
    {
        return;
    }
    d->_variant = variant;
    update();
}

ShadItemType::Variant ShadItem::getVariant() const
{
    Q_D(const ShadItem);
    return d->_variant;
}

void ShadItem::setSize(ShadItemType::Size size)
{
    Q_D(ShadItem);
    if (d->_size == size)
    {
        return;
    }
    d->_size = size;
    d->applySize();
    // 分组的间距按组内最小尺寸决定
    if (ShadItemGroup* group = qobject_cast<ShadItemGroup*>(parentWidget()))
    {
        group->updateLayout();
    }
}

ShadItemType::Size ShadItem::getSize() const
{
    Q_D(const ShadItem);
    return d->_size;
}

void ShadItem::setTitle(const QString& title)
{
    Q_D(ShadItem);
    d->_content->setTitle(title);
}

QString ShadItem::getTitle() const
{
    Q_D(const ShadItem);
    return d->_content->getTitle();
}

void ShadItem::setDescription(const QString& description)
{
    Q_D(ShadItem);
    d->_content->setDescription(description);
    d->updateMediaAlignment();
}

QString ShadItem::getDescription() const
{
    Q_D(const ShadItem);
    return d->_content->getDescription();
}

void ShadItem::setMediaIcon(ShadIconType::IconName icon)
{
    Q_D(ShadItem);
    d->_media->setIcon(icon);
    d->updateMediaAlignment();
}

ShadIconType::IconName ShadItem::getMediaIcon() const
{
    Q_D(const ShadItem);
    return d->_media->getIcon();
}

void ShadItem::setMediaPixmap(const QPixmap& pixmap)
{
    Q_D(ShadItem);
    d->_media->setPixmap(pixmap);
    d->updateMediaAlignment();
}

QPixmap ShadItem::getMediaPixmap() const
{
    Q_D(const ShadItem);
    return d->_media->getPixmap();
}

void ShadItem::setMediaWidget(QWidget* widget)
{
    Q_D(ShadItem);
    d->_media->setWidget(widget);
    d->updateMediaAlignment();
}

QWidget* ShadItem::getMediaWidget() const
{
    Q_D(const ShadItem);
    return d->_media->getWidget();
}

void ShadItem::addActionWidget(QWidget* widget)
{
    Q_D(ShadItem);
    if (widget == nullptr || d->_actionsLayout->indexOf(widget) >= 0)
    {
        return;
    }
    widget->setParent(this);
    d->_actionsLayout->addWidget(widget, 0, Qt::AlignVCenter);
    widget->show();
}

void ShadItem::removeActionWidget(QWidget* widget)
{
    Q_D(ShadItem);
    if (widget == nullptr || d->_actionsLayout->indexOf(widget) < 0)
    {
        return;
    }
    d->_actionsLayout->removeWidget(widget);
    ShadStyleHelper::clearFocusWithin(widget);
    widget->hide();
    widget->setParent(nullptr);
}

void ShadItem::setHeaderWidget(QWidget* widget)
{
    Q_D(ShadItem);
    d->replaceEdgeWidget(d->_header, widget, true);
}

QWidget* ShadItem::getHeaderWidget() const
{
    Q_D(const ShadItem);
    return d->_header.data();
}

void ShadItem::setFooterWidget(QWidget* widget)
{
    Q_D(ShadItem);
    d->replaceEdgeWidget(d->_footer, widget, false);
}

QWidget* ShadItem::getFooterWidget() const
{
    Q_D(const ShadItem);
    return d->_footer.data();
}

void ShadItem::setHeaderText(const QString& text)
{
    setHeaderWidget(text.isEmpty() ? nullptr : createEdgeLabel(text, true, this));
}

void ShadItem::setFooterText(const QString& text)
{
    setFooterWidget(text.isEmpty() ? nullptr : createEdgeLabel(text, false, this));
}

void ShadItem::setClickable(bool clickable)
{
    Q_D(ShadItem);
    if (d->_isClickable == clickable)
    {
        return;
    }
    d->_isClickable = clickable;
    d->_isPressed = false;
    setFocusPolicy(clickable ? Qt::StrongFocus : Qt::NoFocus);
    if (clickable)
    {
        setCursor(Qt::PointingHandCursor);
    }
    else
    {
        unsetCursor();
        d->_isFocusVisible = false;
        if (hasFocus())
        {
            clearFocus();
        }
    }
    d->_hoverTransition->jumpTo(clickable && underMouse() ? 1.0 : 0.0);
    update();
}

bool ShadItem::isClickable() const
{
    Q_D(const ShadItem);
    return d->_isClickable;
}

void ShadItem::changeEvent(QEvent* event)
{
    Q_D(ShadItem);
    QWidget::changeEvent(event);
    if (event->type() == QEvent::EnabledChange)
    {
        d->_isPressed = false;
        d->_hoverTransition->jumpTo(isEnabled() && d->_isClickable && underMouse() ? 1.0 : 0.0);
        update();
    }
}

void ShadItem::enterEvent(QEnterEvent* event)
{
    Q_D(ShadItem);
    if (d->_isClickable)
    {
        d->_hoverTransition->animateTo(1.0);
    }
    QWidget::enterEvent(event);
}

void ShadItem::leaveEvent(QEvent* event)
{
    Q_D(ShadItem);
    d->_hoverTransition->animateTo(0.0);
    QWidget::leaveEvent(event);
}

void ShadItem::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadItem);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QWidget::focusInEvent(event);
    update();
}

void ShadItem::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadItem);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QWidget::focusOutEvent(event);
    update();
}

void ShadItem::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadItem);
    const int key = event->key();
    if (d->_isClickable && !event->isAutoRepeat() && (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space))
    {
        event->accept();
        // 槽函数中可能删除列表项，发出信号后不再访问成员
        Q_EMIT clicked();
        return;
    }
    QWidget::keyPressEvent(event);
}

void ShadItem::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadItem);
    if (d->_isClickable && event->button() == Qt::LeftButton)
    {
        d->_isPressed = true;
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ShadItem::mouseReleaseEvent(QMouseEvent* event)
{
    Q_D(ShadItem);
    if (d->_isPressed && event->button() == Qt::LeftButton)
    {
        d->_isPressed = false;
        event->accept();
        if (rect().contains(event->position().toPoint()))
        {
            Q_EMIT clicked();
        }
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void ShadItem::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadItem);
    using namespace ShadStyleHelper;
    const QRectF bodyRect = insetRect(rect(), kFocusRingWidth);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const QColor muted = shadTheme->getColor(ShadThemeType::Muted);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    // Muted 为 bg-muted/50；可点击时悬停过渡到 bg-muted
    QColor background = d->_variant == ShadItemType::Muted ? withAlpha(muted, 0.5) : QColor(Qt::transparent);
    background = mix(background, muted, d->_hoverTransition->value());
    if (background.alpha() > 0)
    {
        fillRoundedRect(&painter, bodyRect, radius, background);
    }
    // Outline 为 border-border，其余边框透明；键盘聚焦时 border-ring 并显示焦点环
    const bool isFocusVisible = d->_isFocusVisible && hasFocus();
    if (isFocusVisible)
    {
        strokeRoundedRect(&painter, bodyRect, radius, shadTheme->getColor(ShadThemeType::Ring));
        drawFocusRing(&painter, bodyRect, radius, focusRingColor(false));
    }
    else if (d->_variant == ShadItemType::Outline)
    {
        strokeRoundedRect(&painter, bodyRect, radius, shadTheme->getColor(ShadThemeType::Border));
    }
}
