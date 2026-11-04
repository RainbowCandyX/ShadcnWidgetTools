#include "ShadAttachmentGroup.h"

#include <QColor>
#include <QEasingCurve>
#include <QEvent>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QMetaObject>
#include <QPalette>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSizePolicy>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariant>
#include <QVariantAnimation>
#include <QWheelEvent>

#include "ShadAttachment.h"
#include "ShadAttachmentGroupPrivate.h"
#include "ShadStyleHelper.h"

namespace
{
    // gap-3 py-1，扣除附件四周的焦点环边距
    constexpr int kItemSpacing = 12 - 2 * ShadStyleHelper::kFocusRingWidth;
    constexpr int kPaddingY = 4 - ShadStyleHelper::kFocusRingWidth;
} // namespace

ShadAttachmentGroup::ShadAttachmentGroup(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadAttachmentGroupPrivate())
{
    Q_D(ShadAttachmentGroup);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    d->_scrollArea = new QScrollArea(this);
    d->_scrollArea->setFrameShape(QFrame::NoFrame);
    d->_scrollArea->setFocusPolicy(Qt::NoFocus);
    // 不采用 QAbstractScrollArea 按滚动条计算的最小尺寸，分组高度只由附件决定
    d->_scrollArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    // 内容宽度由 updateContentGeometry 控制，附件保持理想宽度
    d->_scrollArea->setWidgetResizable(false);
    d->_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->_scrollArea->viewport()->setAutoFillBackground(false);
    // 与 ShadScrollArea 相同：QScrollArea::setWidget 会开启内容组件的背景填充，视口的 Window 色设为透明
    QPalette viewportPalette = d->_scrollArea->viewport()->palette();
    viewportPalette.setColor(QPalette::Window, QColor(0, 0, 0, 0));
    d->_scrollArea->viewport()->setPalette(viewportPalette);
    mainLayout->addWidget(d->_scrollArea);

    d->_content = new QWidget();
    d->_layout = new QHBoxLayout(d->_content);
    d->_layout->setContentsMargins(0, kPaddingY, 0, kPaddingY);
    d->_layout->setSpacing(kItemSpacing);
    d->_scrollArea->setWidget(d->_content);

    // 两端渐隐遮罩，没有可滚动内容时关闭以避免离屏绘制
    d->_fadeEffect = new QGraphicsOpacityEffect(d->_scrollArea->viewport());
    d->_fadeEffect->setOpacity(1.0);
    d->_fadeEffect->setEnabled(false);
    d->_scrollArea->viewport()->setGraphicsEffect(d->_fadeEffect);

    d->_snapTimer = new QTimer(this);
    d->_snapTimer->setSingleShot(true);
    d->_snapTimer->setInterval(ShadAttachmentGroupPrivate::kSnapDelayMs);
    connect(d->_snapTimer, &QTimer::timeout, this, [d]() { d->snapToNearest(); });
    d->_snapAnimation = new QVariantAnimation(this);
    d->_snapAnimation->setDuration(ShadAttachmentGroupPrivate::kSnapDurationMs);
    d->_snapAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(d->_snapAnimation, &QVariantAnimation::valueChanged, this, [d](const QVariant& value)
    {
        d->_isSnapping = true;
        d->_scrollArea->horizontalScrollBar()->setValue(value.toInt());
        d->_isSnapping = false;
    });

    QScrollBar* bar = d->_scrollArea->horizontalScrollBar();
    connect(bar, &QScrollBar::valueChanged, this, [d]()
    {
        if (!d->_isSnapping)
        {
            d->_snapTimer->start();
        }
        d->updateFade();
    });
    connect(bar, &QScrollBar::rangeChanged, this, [d]() { d->updateFade(); });

    // 滚轮从附件冒泡到视口时转换方向；内容布局变化时重新计算内容尺寸
    d->_scrollArea->viewport()->installEventFilter(this);
    d->_content->installEventFilter(this);
    d->updateContentGeometry();
}

ShadAttachmentGroup::~ShadAttachmentGroup()
{
}

void ShadAttachmentGroup::addAttachment(ShadAttachment* attachment)
{
    insertAttachment(-1, attachment);
}

void ShadAttachmentGroup::insertAttachment(int index, ShadAttachment* attachment)
{
    Q_D(ShadAttachmentGroup);
    if (attachment == nullptr)
    {
        return;
    }
    d->pruneAttachments();
    if (d->_attachments.contains(attachment))
    {
        return;
    }
    const int count = int(d->_attachments.size());
    const int position = index < 0 || index > count ? count : index;
    d->_attachments.insert(position, attachment);
    d->_layout->insertWidget(position, attachment);
    d->updateContentGeometry();
}

void ShadAttachmentGroup::removeAttachment(ShadAttachment* attachment)
{
    Q_D(ShadAttachmentGroup);
    if (attachment == nullptr || !d->_attachments.removeOne(attachment))
    {
        return;
    }
    d->_layout->removeWidget(attachment);
    // 通常由附件上的移除按钮触发，按钮有焦点，先清除焦点再隐藏
    ShadStyleHelper::clearFocusWithin(attachment);
    attachment->hide();
    attachment->setParent(nullptr);
    d->updateContentGeometry();
}

void ShadAttachmentGroup::clear()
{
    Q_D(ShadAttachmentGroup);
    const QList<QPointer<ShadAttachment>> attachments = d->_attachments;
    d->_attachments.clear();
    for (const QPointer<ShadAttachment>& attachment : attachments)
    {
        delete attachment.data();
    }
    d->updateContentGeometry();
}

int ShadAttachmentGroup::count() const
{
    Q_D(const ShadAttachmentGroup);
    int result = 0;
    for (const QPointer<ShadAttachment>& attachment : d->_attachments)
    {
        if (!attachment.isNull())
        {
            ++result;
        }
    }
    return result;
}

ShadAttachment* ShadAttachmentGroup::attachmentAt(int index) const
{
    Q_D(const ShadAttachmentGroup);
    int current = 0;
    for (const QPointer<ShadAttachment>& attachment : d->_attachments)
    {
        if (attachment.isNull())
        {
            continue;
        }
        if (current == index)
        {
            return attachment.data();
        }
        ++current;
    }
    return nullptr;
}

void ShadAttachmentGroup::scrollToAttachment(ShadAttachment* attachment)
{
    Q_D(ShadAttachmentGroup);
    if (attachment == nullptr || !d->_attachments.contains(attachment))
    {
        return;
    }
    // 刚添加的附件还没有布局，先同步完成布局再读取位置
    d->_layout->activate();
    d->updateContentGeometry();
    QScrollBar* bar = d->_scrollArea->horizontalScrollBar();
    const int start = d->logicalLeft(attachment);
    const int end = start + attachment->width();
    const int viewportWidth = d->_scrollArea->viewport()->width();
    // 已完整可见时不动；在起始侧之外时对齐起始边，在结束侧之外时让结束边进入视口
    int target = bar->value();
    if (start < bar->value())
    {
        target = start + ShadStyleHelper::kFocusRingWidth - ShadAttachmentGroupPrivate::kScrollPadding;
    }
    else if (end > bar->value() + viewportWidth)
    {
        target = end - viewportWidth;
    }
    d->animateTo(qBound(0, target, bar->maximum()));
}

QSize ShadAttachmentGroup::sizeHint() const
{
    Q_D(const ShadAttachmentGroup);
    return d->_layout->sizeHint();
}

QSize ShadAttachmentGroup::minimumSizeHint() const
{
    Q_D(const ShadAttachmentGroup);
    // min-w-0：宽度可以任意收窄，内容横向滚动
    return QSize(0, d->_layout->sizeHint().height());
}

bool ShadAttachmentGroup::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadAttachmentGroup);
    if (watched == d->_scrollArea->viewport() && event->type() == QEvent::Wheel)
    {
        if (d->handleWheel(static_cast<QWheelEvent*>(event)))
        {
            return true;
        }
    }
    else if (watched == d->_scrollArea->viewport() && event->type() == QEvent::Resize)
    {
        // 视口宽度决定渐隐宽度，滚动区域显示后才得到最终尺寸
        d->updateFade();
    }
    else if (watched == d->_content && event->type() == QEvent::LayoutRequest)
    {
        // 附件尺寸或显示状态变化，等布局处理完这次请求后再更新内容尺寸
        QMetaObject::invokeMethod(this, [d]() { d->updateContentGeometry(); }, Qt::QueuedConnection);
    }
    return QWidget::eventFilter(watched, event);
}

void ShadAttachmentGroup::changeEvent(QEvent* event)
{
    Q_D(ShadAttachmentGroup);
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        // 渐隐的起始侧随方向互换
        d->updateFade();
    }
    QWidget::changeEvent(event);
}

void ShadAttachmentGroup::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadAttachmentGroup);
    QWidget::resizeEvent(event);
    d->updateContentGeometry();
}
