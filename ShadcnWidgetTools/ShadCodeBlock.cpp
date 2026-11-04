#include "ShadCodeBlock.h"

#include <QApplication>
#include <QColor>
#include <QEasingCurve>
#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QIcon>
#include <QPaintEvent>
#include <QPainter>
#include <QPointer>
#include <QRect>
#include <QRectF>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSizePolicy>
#include <QTimer>
#include <QVariant>
#include <QVariantAnimation>
#include <QWidget>

#include <initializer_list>

#include "ShadButton.h"
#include "ShadCodeBlockPrivate.h"
#include "ShadCodeBlockView.h"
#include "ShadScrollArea.h"
#include "ShadScrollBar.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadCodeBlock::ShadCodeBlock(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadCodeBlockPrivate())
{
    Q_D(ShadCodeBlock);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // 代码区始终从左到右：横向滚动从左侧开始，纵向滚动条位于右侧
    d->_scrollArea = new ShadScrollArea(this);
    d->_scrollArea->setWidgetResizable(false);
    d->_scrollArea->setLayoutDirection(Qt::LeftToRight);
    d->_scrollArea->setFocusPolicy(Qt::NoFocus);
    // 滚动条两端避开 rounded-lg 圆角
    const int trackInset = shadTheme->getRadius(ShadThemeType::Large) - ShadCodeBlockPrivate::kBorderWidth;
    d->_scrollArea->getOverlayScrollBar(Qt::Vertical)->setTrackInset(trackInset);
    d->_scrollArea->getOverlayScrollBar(Qt::Horizontal)->setTrackInset(trackInset);
    d->_view = new ShadCodeBlockView();
    d->_scrollArea->setWidget(d->_view);
    setFocusProxy(d->_view);

    // 四周渐隐遮罩，没有可滚动内容时关闭以避免离屏绘制
    d->_fadeEffect = new QGraphicsOpacityEffect(d->_scrollArea->viewport());
    d->_fadeEffect->setOpacity(1.0);
    d->_fadeEffect->setEnabled(false);
    d->_scrollArea->viewport()->setGraphicsEffect(d->_fadeEffect);
    d->_view->installEventFilter(this);
    d->_scrollArea->viewport()->installEventFilter(this);
    for (QScrollBar* bar : {d->_scrollArea->horizontalScrollBar(), d->_scrollArea->verticalScrollBar()})
    {
        connect(bar, &QScrollBar::valueChanged, this, [d]() { d->updateFade(); });
        connect(bar, &QScrollBar::rangeChanged, this, [d]() { d->updateFade(); });
    }

    d->_actions = new ShadCodeBlockActions(this);
    d->_actions->installEventFilter(this);
    d->_actionsEffect = new QGraphicsOpacityEffect(d->_actions);
    d->_actionsEffect->setOpacity(1.0);
    d->_actionsEffect->setEnabled(false);
    d->_actions->setGraphicsEffect(d->_actionsEffect);
    d->_actionsAnimation = new QVariantAnimation(this);
    d->_actionsAnimation->setDuration(ShadCodeBlockPrivate::kFadeDurationMs);
    d->_actionsAnimation->setEasingCurve(QEasingCurve::InOutCubic);
    connect(d->_actionsAnimation, &QVariantAnimation::valueChanged, this, [d](const QVariant& value) { d->_actionsEffect->setOpacity(value.toReal()); });

    d->_copyButton = new ShadButton(QString(), ShadButtonType::Ghost, d->_actions);
    d->_copyButton->setButtonSize(ShadButtonType::IconExtraSmall);
    d->_copyButton->setShadIcon(ShadIconType::Copy);
    d->_copyButton->setToolTip(tr("复制"));
    d->_copyButton->hide();
    connect(d->_copyButton, &ShadButton::clicked, this, [d]() { d->copyAll(); });
    d->_copyResetTimer = new QTimer(this);
    d->_copyResetTimer->setSingleShot(true);
    d->_copyResetTimer->setInterval(ShadCodeBlockPrivate::kCopiedResetMs);
    connect(d->_copyResetTimer, &QTimer::timeout, this, [d]() { d->resetCopyButton(); });

    connect(d->_view, &ShadCodeBlockView::contentSizeChanged, this, [d]() { d->updateLayout(); });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this, d]()
    {
        update();
        d->_actions->update();
    });
    // focus-within：焦点位于代码区或操作按钮时显示悬浮操作区
    connect(qApp, &QApplication::focusChanged, this, [this, d](QWidget* previous, QWidget* current)
    {
        Q_UNUSED(previous);
        const bool isWithin = current != nullptr && (current == this || isAncestorOf(current));
        if (d->_isFocusWithin == isWithin)
        {
            return;
        }
        d->_isFocusWithin = isWithin;
        d->updateActionsOpacity(true);
    });
    d->updateActionsOpacity(false);
    d->updateLayout();
}

ShadCodeBlock::ShadCodeBlock(const QString& code, QWidget* parent)
    : ShadCodeBlock(parent)
{
    setCode(code);
}

ShadCodeBlock::~ShadCodeBlock()
{
    Q_D(ShadCodeBlock);
    // 私有数据先于子组件释放；子组件在 QWidget 析构时删除，期间的焦点变化与滚动条信号不能再进入访问私有数据的回调
    QObject::disconnect(qApp, nullptr, this, nullptr);
    QObject::disconnect(d->_scrollArea->horizontalScrollBar(), nullptr, this, nullptr);
    QObject::disconnect(d->_scrollArea->verticalScrollBar(), nullptr, this, nullptr);
    QObject::disconnect(d->_view, nullptr, this, nullptr);
}

void ShadCodeBlock::setVariant(ShadCodeBlockType::Variant variant)
{
    Q_D(ShadCodeBlock);
    if (d->_variant == variant)
    {
        return;
    }
    d->_variant = variant;
    d->_view->setDestructive(variant == ShadCodeBlockType::Destructive);
    update();
}

ShadCodeBlockType::Variant ShadCodeBlock::getVariant() const
{
    Q_D(const ShadCodeBlock);
    return d->_variant;
}

void ShadCodeBlock::setCodeBlockSize(ShadCodeBlockType::Size size)
{
    Q_D(ShadCodeBlock);
    if (d->_size == size)
    {
        return;
    }
    d->_size = size;
    d->_view->setFontPixelSize(d->metrics().codePixelSize);
    d->updateLayout();
    updateGeometry();
}

ShadCodeBlockType::Size ShadCodeBlock::getCodeBlockSize() const
{
    Q_D(const ShadCodeBlock);
    return d->_size;
}

void ShadCodeBlock::setCode(const QString& code)
{
    Q_D(ShadCodeBlock);
    d->_view->setCode(code);
}

QString ShadCodeBlock::getCode() const
{
    Q_D(const ShadCodeBlock);
    return d->_view->getCode();
}

void ShadCodeBlock::appendCode(const QString& text)
{
    Q_D(ShadCodeBlock);
    if (text.isEmpty())
    {
        return;
    }
    d->_view->setCode(d->_view->getCode() + text);
}

QString ShadCodeBlock::toPlainText() const
{
    Q_D(const ShadCodeBlock);
    return d->_view->plainText();
}

int ShadCodeBlock::lineCount() const
{
    Q_D(const ShadCodeBlock);
    return d->_view->lineCount();
}

void ShadCodeBlock::setLanguage(const QString& language)
{
    Q_D(ShadCodeBlock);
    d->_view->setLanguage(language);
}

QString ShadCodeBlock::getLanguage() const
{
    Q_D(const ShadCodeBlock);
    return d->_view->getLanguage();
}

void ShadCodeBlock::setLineVariant(int lineIndex, ShadCodeBlockType::LineVariant variant)
{
    Q_D(ShadCodeBlock);
    d->_view->setLineVariant(lineIndex, variant);
}

ShadCodeBlockType::LineVariant ShadCodeBlock::getLineVariant(int lineIndex) const
{
    Q_D(const ShadCodeBlock);
    return d->_view->getLineVariant(lineIndex);
}

void ShadCodeBlock::clearLineVariants()
{
    Q_D(ShadCodeBlock);
    d->_view->clearLineVariants();
}

void ShadCodeBlock::setLineNumbersVisible(bool visible)
{
    Q_D(ShadCodeBlock);
    d->_view->setLineNumbersVisible(visible);
}

bool ShadCodeBlock::isLineNumbersVisible() const
{
    Q_D(const ShadCodeBlock);
    return d->_view->isLineNumbersVisible();
}

void ShadCodeBlock::setStartLine(int line)
{
    Q_D(ShadCodeBlock);
    d->_view->setStartLine(line);
}

int ShadCodeBlock::getStartLine() const
{
    Q_D(const ShadCodeBlock);
    return d->_view->getStartLine();
}

void ShadCodeBlock::setWordWrap(bool wrap)
{
    Q_D(ShadCodeBlock);
    if (d->_view->isWordWrap() == wrap)
    {
        return;
    }
    d->_view->setWordWrap(wrap);
    Q_EMIT wordWrapChanged(wrap);
}

bool ShadCodeBlock::isWordWrap() const
{
    Q_D(const ShadCodeBlock);
    return d->_view->isWordWrap();
}

void ShadCodeBlock::setMaximumContentHeight(int height)
{
    Q_D(ShadCodeBlock);
    const int value = qMax(0, height);
    if (d->_maximumContentHeight == value)
    {
        return;
    }
    d->_maximumContentHeight = value;
    d->updateLayout();
}

int ShadCodeBlock::getMaximumContentHeight() const
{
    Q_D(const ShadCodeBlock);
    return d->_maximumContentHeight;
}

void ShadCodeBlock::setTitle(const QString& title)
{
    Q_D(ShadCodeBlock);
    if (d->_title == title)
    {
        return;
    }
    d->_title = title;
    // 标题栏出现或消失时，操作区在标题栏与悬浮之间切换
    d->updateActionsOpacity(false);
    d->updateLayout();
}

QString ShadCodeBlock::getTitle() const
{
    Q_D(const ShadCodeBlock);
    return d->_title;
}

void ShadCodeBlock::setTitleIcon(ShadIconType::IconName icon)
{
    Q_D(ShadCodeBlock);
    if (d->_titleIcon == icon)
    {
        return;
    }
    d->_titleIcon = icon;
    d->updateActionsOpacity(false);
    d->updateLayout();
}

ShadIconType::IconName ShadCodeBlock::getTitleIcon() const
{
    Q_D(const ShadCodeBlock);
    return d->_titleIcon;
}

void ShadCodeBlock::setCopyButtonVisible(bool visible)
{
    Q_D(ShadCodeBlock);
    if (d->_isCopyButtonVisible == visible)
    {
        return;
    }
    d->_isCopyButtonVisible = visible;
    d->_copyButton->setVisible(visible);
    d->updateLayout();
}

bool ShadCodeBlock::isCopyButtonVisible() const
{
    Q_D(const ShadCodeBlock);
    return d->_isCopyButtonVisible;
}

void ShadCodeBlock::addActionWidget(QWidget* widget)
{
    Q_D(ShadCodeBlock);
    if (widget == nullptr || d->_actionWidgets.contains(widget))
    {
        return;
    }
    // setParent 会隐藏组件；之前没有被显式隐藏的组件在加入后显示
    const bool wasHidden = widget->testAttribute(Qt::WA_WState_ExplicitShowHide) && widget->testAttribute(Qt::WA_WState_Hidden);
    widget->setParent(d->_actions);
    widget->setVisible(!wasHidden);
    d->_actionWidgets.append(widget);
    // 复制按钮保持在最后，键盘焦点顺序与显示顺序一致
    QWidget::setTabOrder(widget, d->_copyButton);
    d->updateLayout();
}

QHBoxLayout* ShadCodeBlock::footerLayout()
{
    Q_D(ShadCodeBlock);
    if (d->_footerLayout == nullptr)
    {
        d->_footer = new QWidget(this);
        d->_footer->hide();
        // 底栏添加或移除组件时会收到 LayoutRequest
        d->_footer->installEventFilter(this);
        d->_footerLayout = new QHBoxLayout(d->_footer);
        d->_footerLayout->setContentsMargins(0, 0, 0, 0);
        // gap-2，扣除相邻组件两侧的焦点环边距
        d->_footerLayout->setSpacing(ShadCodeBlockPrivate::kHeaderGap - 2 * ShadStyleHelper::kFocusRingWidth);
    }
    return d->_footerLayout;
}

QString ShadCodeBlock::selectedText() const
{
    Q_D(const ShadCodeBlock);
    return d->_view->selectedText();
}

QSize ShadCodeBlock::sizeHint() const
{
    Q_D(const ShadCodeBlock);
    const int frame = 2 * (ShadStyleHelper::kFocusRingWidth + ShadCodeBlockPrivate::kBorderWidth);
    return QSize(d->_view->naturalWidth() + frame, d->_preferredHeight);
}

QSize ShadCodeBlock::minimumSizeHint() const
{
    Q_D(const ShadCodeBlock);
    // min-w-0：宽度可以任意收窄，代码横向滚动
    const int frame = 2 * (ShadStyleHelper::kFocusRingWidth + ShadCodeBlockPrivate::kBorderWidth);
    return QSize(frame + 2 * d->metrics().paddingX, d->_preferredHeight);
}

bool ShadCodeBlock::event(QEvent* event)
{
    Q_D(ShadCodeBlock);
    if (event->type() == QEvent::LayoutRequest)
    {
        // 底栏的尺寸变化
        d->updateLayout();
    }
    return QWidget::event(event);
}

bool ShadCodeBlock::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadCodeBlock);
    if (watched == d->_view && (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut))
    {
        // 只有键盘导航聚焦时显示焦点环（focus-visible）
        const bool isVisible = event->type() == QEvent::FocusIn && ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, static_cast<QFocusEvent*>(event));
        if (d->_isFocusVisible != isVisible)
        {
            d->_isFocusVisible = isVisible;
            update();
        }
    }
    else if ((watched == d->_actions || watched == d->_footer) && event->type() == QEvent::LayoutRequest)
    {
        // 操作组件显示、隐藏或尺寸变化，底栏添加或移除组件
        d->updateLayout();
    }
    else if (watched == d->_scrollArea->viewport() && event->type() == QEvent::Resize)
    {
        d->updateFade();
    }
    return QWidget::eventFilter(watched, event);
}

void ShadCodeBlock::changeEvent(QEvent* event)
{
    Q_D(ShadCodeBlock);
    if (event->type() == QEvent::LayoutDirectionChange || event->type() == QEvent::FontChange)
    {
        // 标题栏、操作区与底栏随方向镜像；代码区固定从左到右，不受影响
        d->updateLayout();
    }
    else if (event->type() == QEvent::EnabledChange)
    {
        update();
    }
    else if (event->type() == QEvent::LanguageChange)
    {
        // 复制后的短暂提示期间保持"已复制"
        d->updateCopyToolTip(d->_copyResetTimer->isActive() ? tr("已复制") : tr("复制"));
    }
    QWidget::changeEvent(event);
}

void ShadCodeBlock::enterEvent(QEnterEvent* event)
{
    Q_D(ShadCodeBlock);
    d->_isHovered = true;
    d->updateActionsOpacity(true);
    QWidget::enterEvent(event);
}

void ShadCodeBlock::leaveEvent(QEvent* event)
{
    Q_D(ShadCodeBlock);
    d->_isHovered = false;
    d->updateActionsOpacity(true);
    QWidget::leaveEvent(event);
}

void ShadCodeBlock::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadCodeBlock);
    QWidget::resizeEvent(event);
    d->updateLayout();
}

void ShadCodeBlock::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadCodeBlock);
    using namespace ShadStyleHelper;
    const QRectF bodyRect = insetRect(rect(), kFocusRingWidth);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Large);
    const bool isDark = shadTheme->isDark();
    const bool isDestructive = d->_variant == ShadCodeBlockType::Destructive;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    // Default：border-input，暗色 bg-input/30，键盘聚焦代码区时边框为 ring；Destructive：border-destructive/30（暗色 /40）
    if (!isDestructive && isDark)
    {
        fillRoundedRect(&painter, bodyRect, radius, withAlpha(shadTheme->getColor(ShadThemeType::Input), 0.3));
    }
    QColor borderColor = shadTheme->getColor(d->_isFocusVisible ? ShadThemeType::Ring : ShadThemeType::Input);
    if (isDestructive)
    {
        borderColor = withAlpha(shadTheme->getColor(ShadThemeType::Destructive), isDark ? 0.4 : 0.3);
    }
    strokeRoundedRect(&painter, bodyRect, radius, borderColor);
    if (d->_isFocusVisible)
    {
        drawFocusRing(&painter, bodyRect, radius, focusRingColor(false));
    }
    if (d->hasHeader() && !d->_titleRect.isEmpty())
    {
        // CodeBlockTitle：font-medium，图标与文字之间 me-1.5，超长时省略；RTL 时从右侧开始排列
        const ShadCodeBlockPrivate::Metrics values = d->metrics();
        QFont titleFont = font();
        titleFont.setPixelSize(values.titlePixelSize);
        titleFont.setWeight(QFont::Medium);
        const QColor titleColor = shadTheme->getColor(isDestructive ? ShadThemeType::Destructive : ShadThemeType::Foreground);
        const QRect titleRect = visualRect(layoutDirection(), rect(), d->_titleRect);
        drawIconText(&painter, QRectF(titleRect), d->_titleIcon, QIcon(), d->_title, titleFont, titleColor, ShadCodeBlockPrivate::kTitleIconGap, Qt::AlignLeft | Qt::AlignVCenter, values.titleIconSize, 0.0, layoutDirection());
    }
}
