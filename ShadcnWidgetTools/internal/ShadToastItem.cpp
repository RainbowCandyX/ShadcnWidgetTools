#include "ShadToastItem.h"

#include <QAbstractAnimation>
#include <QColor>
#include <QEasingCurve>
#include <QEvent>
#include <QFontMetrics>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPointF>
#include <QRectF>
#include <QResizeEvent>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <QVariantAnimation>
#include <QtGlobal>

#include "ShadButton.h"
#include "ShadDef.h"
#include "ShadIcon.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTransition.h"

namespace
{
    // ToastContent：p-4 gap-3，图标 size-4，标题与描述间距 gap-1，文字行高 20px（text-sm）
    constexpr int kPadding = 16;
    constexpr int kGap = 12;
    constexpr int kIconSize = 16;
    constexpr int kTextGap = 4;
    constexpr int kLineHeight = 20;
    // 操作按钮 size-sm 与关闭按钮 icon-sm 的可视高度
    constexpr int kButtonHeight = 32;
    // 位移动画：500ms cubic-bezier(0.22, 1, 0.36, 1)
    constexpr int kMoveDuration = 500;
    // 收起时后面 toast 的内容淡出：250ms
    constexpr int kContentFadeDuration = 250;
    // 底部倒计时条的高度
    constexpr int kCountdownHeight = 4;

    // 状态色：成功 green、信息 blue、警告 amber，浅色主题取 600 色阶、暗色取 400；错误为 destructive；其余类型返回无效颜色
    QColor statusColor(ShadToastType::Type type)
    {
        const bool isDark = shadTheme->isDark();
        switch (type)
        {
            case ShadToastType::Success:
                return isDark ? QColor(0x4a, 0xde, 0x80) : QColor(0x16, 0xa3, 0x4a);
            case ShadToastType::Info:
                return isDark ? QColor(0x60, 0xa5, 0xfa) : QColor(0x25, 0x63, 0xeb);
            case ShadToastType::Warning:
                return isDark ? QColor(0xfb, 0xbf, 0x24) : QColor(0xd9, 0x77, 0x06);
            case ShadToastType::Error:
                return shadTheme->getColor(ShadThemeType::Destructive);
            case ShadToastType::Default:
            case ShadToastType::Loading:
                break;
        }
        return QColor();
    }
} // namespace

ShadToastItem::ShadToastItem(int id, QWidget* parent)
    : QWidget(parent), _id(id)
{
    _titleFont = font();
    _titleFont.setPixelSize(14);
    _titleFont.setWeight(QFont::Medium);
    _descriptionFont = font();
    _descriptionFont.setPixelSize(14);
    _descriptionFont.setWeight(QFont::Normal);

    _actionButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    _actionButton->setButtonSize(ShadButtonType::Small);
    _actionButton->hide();
    connect(_actionButton, &ShadButton::clicked, this, [this]()
    {
        if (_options.onAction)
        {
            _options.onAction();
        }
        Q_EMIT actionTriggered(_id);
    });

    _closeButton = new ShadButton(QString(), ShadButtonType::Ghost, this);
    _closeButton->setButtonSize(ShadButtonType::IconSmall);
    _closeButton->setShadIcon(ShadIconType::X);
    _closeButton->setToolTip(tr("关闭"));
    _closeButton->hide();
    connect(_closeButton, &ShadButton::clicked, this, [this]() { Q_EMIT dismissRequested(_id); });

    _dismissTimer = new QTimer(this);
    _dismissTimer->setSingleShot(true);
    connect(_dismissTimer, &QTimer::timeout, this, [this]() { Q_EMIT dismissRequested(_id); });

    _geometryAnimation = new QVariantAnimation(this);
    _geometryAnimation->setDuration(kMoveDuration);
    QEasingCurve moveCurve(QEasingCurve::BezierSpline);
    moveCurve.addCubicBezierSegment(QPointF(0.22, 1.0), QPointF(0.36, 1.0), QPointF(1.0, 1.0));
    _geometryAnimation->setEasingCurve(moveCurve);
    connect(_geometryAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
    {
        setGeometry(value.toRect());
        Q_EMIT geometryAnimating();
    });
    connect(_geometryAnimation, &QVariantAnimation::finished, this, [this]()
    {
        if (_isExiting)
        {
            Q_EMIT exitFinished(_id);
        }
    });

    // loading 图标对应 animate-spin（1s 匀速一圈）
    _spinAnimation = new QVariantAnimation(this);
    _spinAnimation->setStartValue(0.0);
    _spinAnimation->setEndValue(360.0);
    _spinAnimation->setDuration(ShadStyleHelper::kSpinDurationMs);
    _spinAnimation->setLoopCount(-1);
    _spinAnimation->setEasingCurve(QEasingCurve::Linear);
    connect(_spinAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
    {
        _spinAngle = value.toReal();
        update();
    });

    // 倒计时条随关闭计时匀速缩短
    _countdownAnimation = new QVariantAnimation(this);
    _countdownAnimation->setEndValue(0.0);
    _countdownAnimation->setEasingCurve(QEasingCurve::Linear);
    connect(_countdownAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
    {
        _countdownProgress = value.toReal();
        update();
    });

    _contentTransition = new ShadTransition(this, kContentFadeDuration);
    _contentTransition->jumpTo(1.0);
    _opacityTransition = new ShadTransition(this, kMoveDuration);
    _opacityTransition->jumpTo(1.0);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadToastItem::~ShadToastItem()
{
}

int ShadToastItem::toastId() const
{
    return _id;
}

void ShadToastItem::setOptions(const ShadToastOptions& options, int defaultDuration)
{
    _options = options;
    if (options.duration >= 0)
    {
        _effectiveDuration = options.duration;
    }
    else
    {
        // loading 默认不自动关闭，等待 updateToast 改为其他类型
        _effectiveDuration = options.type == ShadToastType::Loading ? 0 : defaultDuration;
    }
    _actionButton->setText(options.actionText);
    if (options.type == ShadToastType::Loading)
    {
        if (_spinAnimation->state() != QAbstractAnimation::Running)
        {
            _spinAnimation->start();
        }
    }
    else
    {
        _spinAnimation->stop();
        _spinAngle = 0.0;
    }
    // 内容变化后重新计时
    startDismissTimer();
    layoutChildren();
    update();
}

int ShadToastItem::naturalHeight(int width) const
{
    const int textBlockWidth = textWidth(width);
    const int titleBlock = titleHeight();
    const int descriptionBlock = descriptionHeight(textBlockWidth);
    int contentHeight = titleBlock + descriptionBlock + ((titleBlock > 0 && descriptionBlock > 0) ? kTextGap : 0);
    if (_options.type != ShadToastType::Default)
    {
        contentHeight = qMax(contentHeight, kIconSize);
    }
    if (_options.closable || !_options.actionText.isEmpty())
    {
        contentHeight = qMax(contentHeight, kButtonHeight);
    }
    return contentHeight + 2 * kPadding;
}

void ShadToastItem::moveTo(const QRect& target, bool animate)
{
    _isPlaced = true;
    if (!animate)
    {
        _geometryAnimation->stop();
        setGeometry(target);
        Q_EMIT geometryAnimating();
        return;
    }
    const bool isRunning = _geometryAnimation->state() == QAbstractAnimation::Running;
    if (isRunning && _geometryAnimation->endValue().toRect() == target)
    {
        return;
    }
    if (!isRunning && geometry() == target)
    {
        return;
    }
    _geometryAnimation->stop();
    _geometryAnimation->setStartValue(geometry());
    _geometryAnimation->setEndValue(target);
    _geometryAnimation->start();
}

bool ShadToastItem::isPlaced() const
{
    return _isPlaced;
}

void ShadToastItem::setContentVisible(bool visible)
{
    if (_isContentVisible == visible)
    {
        return;
    }
    _isContentVisible = visible;
    _contentTransition->animateTo(visible ? 1.0 : 0.0);
    layoutChildren();
}

bool ShadToastItem::isExiting() const
{
    return _isExiting;
}

void ShadToastItem::startExit(const QRect& target)
{
    _isExiting = true;
    _dismissTimer->stop();
    // 倒计时条停在当前长度，随卡片一起淡出
    _countdownAnimation->stop();
    // 离场时立即隐藏按钮（避免残留悬停态），卡片随位移一起淡出，减少滑过其他 toast 时的遮挡感
    layoutChildren();
    _opacityTransition->animateTo(0.0);
    moveTo(target, true);
    // 已在离场位置时动画不会启动，直接结束
    if (_geometryAnimation->state() != QAbstractAnimation::Running)
    {
        Q_EMIT exitFinished(_id);
    }
}

qreal ShadToastItem::opacity() const
{
    return _opacityTransition->value();
}

int ShadToastItem::cardRadius()
{
    return shadTheme->getBaseRadius() + 6;
}

void ShadToastItem::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    const QRectF cardRect(rect());
    const qreal radius = qMin<qreal>(cardRadius(), cardRect.height() / 2.0);
    const qreal itemOpacity = opacity();
    if (itemOpacity <= 0.0)
    {
        return;
    }
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(itemOpacity);
    // 卡片背景与边框，对应 bg-popover border
    fillRoundedRect(&painter, cardRect, radius, shadTheme->getColor(ShadThemeType::Popover));
    strokeRoundedRect(&painter, cardRect, radius, shadTheme->getColor(ShadThemeType::Border));

    const qreal contentOpacity = _contentTransition->value();
    if (contentOpacity <= 0.0)
    {
        return;
    }
    painter.setOpacity(itemOpacity * contentOpacity);
    const ContentLayout layout = computeLayout(width(), height());
    const QColor foreground = shadTheme->getColor(ShadThemeType::PopoverForeground);

    // 状态图标：使用状态色，loading 旋转
    ShadIconType::IconName icon = ShadIconType::None;
    switch (_options.type)
    {
        case ShadToastType::Success:
            icon = ShadIconType::CircleCheck;
            break;
        case ShadToastType::Info:
            icon = ShadIconType::Info;
            break;
        case ShadToastType::Warning:
            icon = ShadIconType::TriangleAlert;
            break;
        case ShadToastType::Error:
            icon = ShadIconType::OctagonX;
            break;
        case ShadToastType::Loading:
            icon = ShadIconType::LoaderCircle;
            break;
        case ShadToastType::Default:
            break;
    }
    if (icon != ShadIconType::None)
    {
        const QColor status = statusColor(_options.type);
        const QColor iconColor = status.isValid() ? status : foreground;
        const QPointF center = layout.iconRect.center();
        painter.save();
        painter.translate(center);
        painter.rotate(_spinAngle);
        painter.translate(-center);
        ShadIcon::paint(&painter, layout.iconRect, icon, iconColor, kIconSize);
        painter.restore();
    }

    if (!_options.title.isEmpty())
    {
        painter.setFont(_titleFont);
        painter.setPen(foreground);
        const QString elidedTitle = elideText(_titleFont, _options.title, layout.titleRect.width());
        painter.drawText(layout.titleRect, visualAlignment(layoutDirection(), Qt::AlignLeft | Qt::AlignVCenter), elidedTitle);
    }
    if (!_options.description.isEmpty())
    {
        painter.setFont(_descriptionFont);
        painter.setPen(shadTheme->getColor(ShadThemeType::MutedForeground));
        painter.drawText(layout.descriptionRect, visualAlignment(layoutDirection(), Qt::AlignLeft | Qt::AlignTop) | Qt::TextWordWrap, _options.description);
    }

    // 底部倒计时条：只有自动关闭的 toast 显示，按剩余时间向起始侧缩短；裁剪到卡片圆角内
    if (_effectiveDuration > 0)
    {
        const QRectF innerRect = cardRect.adjusted(1.0, 1.0, -1.0, -1.0);
        const QRectF logicalBar(innerRect.left(), innerRect.bottom() - kCountdownHeight, innerRect.width() * _countdownProgress, kCountdownHeight);
        painter.save();
        painter.setClipPath(roundedRectPath(innerRect, qMax<qreal>(0.0, radius - 1.0)));
        // 与状态图标同色，默认类型为 primary 色
        const QColor status = statusColor(_options.type);
        const QColor barColor = status.isValid() ? status : shadTheme->getColor(ShadThemeType::Primary);
        painter.fillRect(visualRect(layoutDirection(), cardRect, logicalBar), barColor);
        painter.restore();
    }
}

void ShadToastItem::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    layoutChildren();
}

void ShadToastItem::changeEvent(QEvent* event)
{
    // 布局方向切换时尺寸不变，不会触发 resizeEvent，需要重新摆放按钮
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        layoutChildren();
    }
    else if (event->type() == QEvent::LanguageChange)
    {
        _closeButton->setToolTip(tr("关闭"));
    }
    QWidget::changeEvent(event);
}

ShadToastItem::ContentLayout ShadToastItem::computeLayout(int width, int height) const
{
    ContentLayout layout;
    const qreal centerY = height / 2.0;
    qreal left = kPadding;
    qreal right = width - kPadding;
    if (_options.type != ShadToastType::Default)
    {
        layout.iconRect = QRectF(left, centerY - kIconSize / 2.0, kIconSize, kIconSize);
        left += kIconSize + kGap;
    }
    if (_options.closable)
    {
        layout.closeRect = QRectF(right - kButtonHeight, centerY - kButtonHeight / 2.0, kButtonHeight, kButtonHeight);
        right -= kButtonHeight + kGap;
    }
    if (!_options.actionText.isEmpty())
    {
        const int actionWidth = _actionButton->sizeHint().width() - 2 * ShadStyleHelper::kFocusRingWidth;
        layout.actionRect = QRectF(right - actionWidth, centerY - kButtonHeight / 2.0, actionWidth, kButtonHeight);
        right -= actionWidth + kGap;
    }
    const qreal textBlockWidth = qMax<qreal>(0.0, right - left);
    const int titleBlock = titleHeight();
    const int descriptionBlock = descriptionHeight(qRound(textBlockWidth));
    const int gap = (titleBlock > 0 && descriptionBlock > 0) ? kTextGap : 0;
    const qreal top = centerY - (titleBlock + gap + descriptionBlock) / 2.0;
    layout.titleRect = QRectF(left, top, textBlockWidth, titleBlock);
    layout.descriptionRect = QRectF(left, top + titleBlock + gap, textBlockWidth, descriptionBlock);
    // 以上按 LTR 计算，RTL 时图标与文字在起始侧（右侧），操作与关闭按钮在结束侧（左侧）
    if (layoutDirection() == Qt::RightToLeft)
    {
        using ShadStyleHelper::visualRect;
        const QRectF bounds(0, 0, width, height);
        layout.iconRect = visualRect(Qt::RightToLeft, bounds, layout.iconRect);
        layout.titleRect = visualRect(Qt::RightToLeft, bounds, layout.titleRect);
        layout.descriptionRect = visualRect(Qt::RightToLeft, bounds, layout.descriptionRect);
        layout.actionRect = visualRect(Qt::RightToLeft, bounds, layout.actionRect);
        layout.closeRect = visualRect(Qt::RightToLeft, bounds, layout.closeRect);
    }
    return layout;
}

int ShadToastItem::textWidth(int width) const
{
    int available = width - 2 * kPadding;
    if (_options.type != ShadToastType::Default)
    {
        available -= kIconSize + kGap;
    }
    if (_options.closable)
    {
        available -= kButtonHeight + kGap;
    }
    if (!_options.actionText.isEmpty())
    {
        available -= _actionButton->sizeHint().width() - 2 * ShadStyleHelper::kFocusRingWidth + kGap;
    }
    return qMax(0, available);
}

int ShadToastItem::titleHeight() const
{
    if (_options.title.isEmpty())
    {
        return 0;
    }
    return qMax(kLineHeight, QFontMetrics(_titleFont).height());
}

int ShadToastItem::descriptionHeight(int width) const
{
    if (_options.description.isEmpty())
    {
        return 0;
    }
    const QRect bounds = QFontMetrics(_descriptionFont).boundingRect(QRect(0, 0, qMax(1, width), 100000), Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, _options.description);
    return qMax(kLineHeight, bounds.height());
}

void ShadToastItem::layoutChildren()
{
    // 收起在后面或正在离场时不显示按钮；离场动画中 resizeEvent 也会调用这里，不能重新显示
    if (!_isContentVisible || _isExiting)
    {
        // 点击关闭或操作按钮后按钮有焦点，直接隐藏会让焦点跳到 Tab 顺序中的下一个组件
        ShadStyleHelper::clearFocusWithin(this);
        _actionButton->hide();
        _closeButton->hide();
        return;
    }
    const ContentLayout layout = computeLayout(width(), height());
    const int margin = ShadStyleHelper::kFocusRingWidth;
    // 按钮组件四周带焦点环边距，按可视区域外扩摆放
    _actionButton->setGeometry(layout.actionRect.adjusted(-margin, -margin, margin, margin).toRect());
    _actionButton->setVisible(!_options.actionText.isEmpty());
    _closeButton->setGeometry(layout.closeRect.adjusted(-margin, -margin, margin, margin).toRect());
    _closeButton->setVisible(_options.closable);
}

void ShadToastItem::startDismissTimer()
{
    _dismissTimer->stop();
    _countdownAnimation->stop();
    _countdownProgress = 1.0;
    if (_effectiveDuration > 0 && !_isExiting)
    {
        _dismissTimer->start(_effectiveDuration);
        _countdownAnimation->setStartValue(1.0);
        _countdownAnimation->setDuration(_effectiveDuration);
        _countdownAnimation->start();
    }
}
