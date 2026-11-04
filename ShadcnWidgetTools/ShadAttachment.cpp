#include "ShadAttachment.h"

#include <QAction>
#include <QByteArray>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDir>
#include <QEasingCurve>
#include <QEnterEvent>
#include <QEvent>
#include <QFileInfo>
#include <QFocusEvent>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QHideEvent>
#include <QImageReader>
#include <QKeyEvent>
#include <QMetaObject>
#include <QMouseEvent>
#include <QPointer>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QResizeEvent>
#include <QShowEvent>
#include <QVariant>
#include <QVariantAnimation>

#include "ShadAttachmentPrivate.h"
#include "ShadButton.h"
#include "ShadDropdownMenu.h"
#include "ShadTooltip.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTransition.h"

namespace
{
    // 对应 focus-within:ring-1 ring-ring/50
    constexpr qreal kRingWidth = 1.0;
} // namespace

ShadAttachment::ShadAttachment(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadAttachmentPrivate())
{
    Q_D(ShadAttachment);
    d->q_ptr = this;
    d->_hoverTransition = new ShadTransition(this);
    // 没有 toolTip 时显示链接地址，提示事件由 event 自行处理
    setProperty(ShadStyleHelper::kCustomToolTipProperty, true);

    d->_spinAnimation = new QVariantAnimation(this);
    d->_spinAnimation->setStartValue(0.0);
    d->_spinAnimation->setEndValue(360.0);
    d->_spinAnimation->setDuration(ShadStyleHelper::kSpinDurationMs);
    d->_spinAnimation->setLoopCount(-1);
    d->_spinAnimation->setEasingCurve(QEasingCurve::Linear);
    connect(d->_spinAnimation, &QVariantAnimation::valueChanged, this, [this, d](const QVariant& value)
    {
        d->_spinAngle = value.toReal();
        update();
    });

    d->_shimmerAnimation = new QVariantAnimation(this);
    d->_shimmerAnimation->setStartValue(0.0);
    d->_shimmerAnimation->setEndValue(1.0);
    d->_shimmerAnimation->setDuration(ShadStyleHelper::kShimmerDurationMs);
    d->_shimmerAnimation->setLoopCount(-1);
    d->_shimmerAnimation->setEasingCurve(QEasingCurve::Linear);
    connect(d->_shimmerAnimation, &QVariantAnimation::valueChanged, this, [this, d](const QVariant& value)
    {
        d->_shimmerProgress = value.toReal();
        update();
    });

    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
    // setFont 会同步发送 FontChange 并调用 updateContent，必须在动画创建之后设置
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    d->updateContent();
}

ShadAttachment::ShadAttachment(ShadIconType::IconName icon, const QString& title, const QString& description, QWidget* parent)
    : ShadAttachment(parent)
{
    Q_D(ShadAttachment);
    d->_icon = icon;
    d->_title = title;
    d->_description = description;
    d->updateContent();
}

ShadAttachment::~ShadAttachment()
{
}

void ShadAttachment::setState(ShadAttachmentType::State state)
{
    Q_D(ShadAttachment);
    if (d->_state == state)
    {
        return;
    }
    d->_state = state;
    d->updateAnimations(isVisible());
    update();
    Q_EMIT stateChanged(state);
}

ShadAttachmentType::State ShadAttachment::getState() const
{
    Q_D(const ShadAttachment);
    return d->_state;
}

void ShadAttachment::setAttachmentSize(ShadAttachmentType::Size size)
{
    Q_D(ShadAttachment);
    if (d->_size == size)
    {
        return;
    }
    d->_size = size;
    d->updateContent();
}

ShadAttachmentType::Size ShadAttachment::getAttachmentSize() const
{
    Q_D(const ShadAttachment);
    return d->_size;
}

void ShadAttachment::setOrientation(Qt::Orientation orientation)
{
    Q_D(ShadAttachment);
    if (d->_orientation == orientation)
    {
        return;
    }
    d->_orientation = orientation;
    d->updateContent();
}

Qt::Orientation ShadAttachment::getOrientation() const
{
    Q_D(const ShadAttachment);
    return d->_orientation;
}

void ShadAttachment::setShadIcon(ShadIconType::IconName icon)
{
    Q_D(ShadAttachment);
    if (d->_icon == icon)
    {
        return;
    }
    d->_icon = icon;
    d->updateContent();
}

ShadIconType::IconName ShadAttachment::getShadIcon() const
{
    Q_D(const ShadAttachment);
    return d->_icon;
}

void ShadAttachment::setImage(const QPixmap& image)
{
    Q_D(ShadAttachment);
    d->_image = image;
    d->updateContent();
}

QPixmap ShadAttachment::getImage() const
{
    Q_D(const ShadAttachment);
    return d->_image;
}

void ShadAttachment::setTitle(const QString& title)
{
    Q_D(ShadAttachment);
    if (d->_title == title)
    {
        return;
    }
    d->_title = title;
    d->updateContent();
}

QString ShadAttachment::getTitle() const
{
    Q_D(const ShadAttachment);
    return d->_title;
}

void ShadAttachment::setDescription(const QString& description)
{
    Q_D(ShadAttachment);
    if (d->_description == description)
    {
        return;
    }
    d->_description = description;
    d->updateContent();
}

QString ShadAttachment::getDescription() const
{
    Q_D(const ShadAttachment);
    return d->_description;
}

ShadButton* ShadAttachment::addAction(ShadIconType::IconName icon, const QString& toolTip)
{
    Q_D(ShadAttachment);
    ShadButton* action = new ShadButton(QString(), ShadButtonType::Ghost, this);
    action->setButtonSize(ShadButtonType::IconExtraSmall);
    action->setShadIcon(icon);
    action->setToolTip(toolTip);
    action->setAccessibleName(toolTip);
    // 监听焦点实现 focus-within，监听显示与隐藏重新排列
    action->installEventFilter(this);
    action->show();
    d->_actions.append(action);
    d->updateContent();
    return action;
}

QList<ShadButton*> ShadAttachment::getActions() const
{
    Q_D(const ShadAttachment);
    QList<ShadButton*> actions;
    for (const QPointer<ShadButton>& action : d->_actions)
    {
        if (!action.isNull())
        {
            actions.append(action.data());
        }
    }
    return actions;
}

void ShadAttachment::clearActions()
{
    Q_D(ShadAttachment);
    const QList<QPointer<ShadButton>> actions = d->_actions;
    d->_actions.clear();
    d->_isActionFocusVisible = false;
    for (const QPointer<ShadButton>& action : actions)
    {
        delete action.data();
    }
    d->updateContent();
}

void ShadAttachment::setClickable(bool clickable)
{
    Q_D(ShadAttachment);
    if (d->_isClickable == clickable)
    {
        return;
    }
    d->_isClickable = clickable;
    d->_isPressed = false;
    d->_hoverTransition->jumpTo(d->isInteractive() && isEnabled() && underMouse() ? 1.0 : 0.0);
    d->updateContent();
}

bool ShadAttachment::isClickable() const
{
    Q_D(const ShadAttachment);
    return d->_isClickable;
}

void ShadAttachment::setUrl(const QUrl& url)
{
    Q_D(ShadAttachment);
    if (d->_url == url)
    {
        return;
    }
    d->_url = url;
    d->_isPressed = false;
    d->_hoverTransition->jumpTo(d->isInteractive() && isEnabled() && underMouse() ? 1.0 : 0.0);
    d->updateContent();
}

QUrl ShadAttachment::getUrl() const
{
    Q_D(const ShadAttachment);
    return d->_url;
}

void ShadAttachment::setOpenExternalLinks(bool open)
{
    Q_D(ShadAttachment);
    d->_isOpenExternalLinks = open;
}

bool ShadAttachment::getOpenExternalLinks() const
{
    Q_D(const ShadAttachment);
    return d->_isOpenExternalLinks;
}

bool ShadAttachment::setFile(const QString& path)
{
    Q_D(ShadAttachment);
    const QFileInfo info(path);
    if (!info.exists() || !info.isFile())
    {
        qWarning("附件文件不存在或不是普通文件：%s", qUtf8Printable(QDir::toNativeSeparators(path)));
        return false;
    }
    const QString suffix = info.suffix();
    const QString typeText = suffix.isEmpty() ? tr("文件") : suffix.toUpper();
    d->_title = info.fileName();
    d->_description = QStringLiteral("%1 · %2").arg(typeText, ShadAttachmentPrivate::formatFileSize(info.size()));
    d->_icon = ShadAttachmentPrivate::iconForSuffix(suffix);
    // 只有 Qt 能解码的图片格式才读取缩略图，读取失败时保留类型图标
    const bool isImage = QImageReader::supportedImageFormats().contains(suffix.toLower().toLatin1());
    d->_image = isImage ? ShadAttachmentPrivate::loadThumbnail(info.absoluteFilePath()) : QPixmap();
    d->updateContent();
    return true;
}

QSize ShadAttachment::sizeHint() const
{
    Q_D(const ShadAttachment);
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    return d->cardSize() + QSize(margin, margin);
}

QSize ShadAttachment::minimumSizeHint() const
{
    Q_D(const ShadAttachment);
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    const QSize card = d->cardSize();
    if (d->isVertical())
    {
        return card + QSize(margin, margin);
    }
    // 横向最小宽度 min-w-40，文字可以省略
    return QSize(qMin(card.width(), ShadAttachmentPrivate::kHorizontalMinWidth) + margin, card.height() + margin);
}

bool ShadAttachment::event(QEvent* event)
{
    Q_D(ShadAttachment);
    // 没有设置 toolTip 时显示链接地址，相当于浏览器状态栏显示 href
    if (event->type() == QEvent::ToolTip)
    {
        const QString text = toolTip().isEmpty() && !d->_url.isEmpty() ? d->urlDisplayText() : toolTip();
        if (text.isEmpty())
        {
            event->ignore();
            return false;
        }
        shadTooltip->showText(this, d->cardRect(), text);
        return true;
    }
    return QWidget::event(event);
}

void ShadAttachment::contextMenuEvent(QContextMenuEvent* event)
{
    Q_D(ShadAttachment);
    if (d->_url.isEmpty() || !d->cardRect().contains(event->pos()))
    {
        QWidget::contextMenuEvent(event);
        return;
    }
    if (d->_contextMenu == nullptr)
    {
        d->_contextMenu = new ShadDropdownMenu(this);
        QAction* openAction = d->_contextMenu->addItem(tr("打开链接"), ShadIconType::ExternalLink);
        QAction* copyAction = d->_contextMenu->addItem(tr("复制链接地址"), ShadIconType::Copy);
        // 右键菜单打开链接时不发出 clicked，与浏览器中链接的右键菜单一致
        connect(openAction, &QAction::triggered, this, [this, d]()
        {
            const QPointer<ShadAttachment> guard(this);
            Q_EMIT linkActivated(d->_url);
            if (!guard.isNull() && d->_isOpenExternalLinks)
            {
                d->openUrl();
            }
        });
        connect(copyAction, &QAction::triggered, this, [d]()
        {
            if (QClipboard* clipboard = QGuiApplication::clipboard())
            {
                clipboard->setText(d->urlDisplayText());
            }
        });
    }
    d->_contextMenu->popupAt(event->globalPos());
    event->accept();
}

bool ShadAttachment::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadAttachment);
    bool isAction = false;
    for (const QPointer<ShadButton>& action : d->_actions)
    {
        if (action.data() == watched)
        {
            isAction = true;
            break;
        }
    }
    if (isAction)
    {
        switch (event->type())
        {
            case QEvent::FocusIn:
                {
                    d->_isActionFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isActionFocusVisible, static_cast<QFocusEvent*>(event));
                    update();
                    break;
                }
            case QEvent::FocusOut:
                {
                    d->_isActionFocusVisible = false;
                    update();
                    break;
                }
            case QEvent::ShowToParent:
            case QEvent::HideToParent:
                {
                    // 横向时按钮数量影响宽度，延后到显示状态更新之后重新排列
                    QMetaObject::invokeMethod(this, [d]() { d->updateContent(); }, Qt::QueuedConnection);
                    break;
                }
            default:
                {
                    break;
                }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ShadAttachment::changeEvent(QEvent* event)
{
    Q_D(ShadAttachment);
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        d->layoutActions();
    }
    else if (event->type() == QEvent::FontChange)
    {
        d->updateContent();
    }
    else if (event->type() == QEvent::EnabledChange)
    {
        d->_hoverTransition->jumpTo(d->isInteractive() && isEnabled() && underMouse() ? 1.0 : 0.0);
    }
    else if (event->type() == QEvent::LanguageChange && d->_contextMenu != nullptr)
    {
        // 右键菜单只在第一次打开时创建，语言切换后丢弃，下次打开时按新语言重建
        d->_contextMenu->deleteLater();
        d->_contextMenu = nullptr;
    }
    QWidget::changeEvent(event);
}

void ShadAttachment::showEvent(QShowEvent* event)
{
    Q_D(ShadAttachment);
    QWidget::showEvent(event);
    d->updateAnimations(true);
}

void ShadAttachment::hideEvent(QHideEvent* event)
{
    Q_D(ShadAttachment);
    d->updateAnimations(false);
    QWidget::hideEvent(event);
}

void ShadAttachment::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadAttachment);
    QWidget::resizeEvent(event);
    d->layoutActions();
}

void ShadAttachment::enterEvent(QEnterEvent* event)
{
    Q_D(ShadAttachment);
    // 鼠标移到操作按钮上时卡片仍处于悬停状态，与 CSS :hover 一致
    if (d->isInteractive() && isEnabled())
    {
        d->_hoverTransition->animateTo(1.0);
    }
    QWidget::enterEvent(event);
}

void ShadAttachment::leaveEvent(QEvent* event)
{
    Q_D(ShadAttachment);
    d->_hoverTransition->animateTo(0.0);
    QWidget::leaveEvent(event);
}

void ShadAttachment::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadAttachment);
    if (d->isInteractive() && event->button() == Qt::LeftButton && d->cardRect().contains(event->position().toPoint()))
    {
        d->_isPressed = true;
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ShadAttachment::mouseReleaseEvent(QMouseEvent* event)
{
    Q_D(ShadAttachment);
    if (d->_isPressed && event->button() == Qt::LeftButton)
    {
        d->_isPressed = false;
        event->accept();
        if (d->cardRect().contains(event->position().toPoint()))
        {
            d->activate();
        }
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void ShadAttachment::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadAttachment);
    if (d->isInteractive() && !event->isAutoRepeat())
    {
        // 与 button 一致：回车按下时触发，空格松开时触发
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
        {
            event->accept();
            d->activate();
            return;
        }
        if (event->key() == Qt::Key_Space)
        {
            event->accept();
            return;
        }
    }
    QWidget::keyPressEvent(event);
}

void ShadAttachment::keyReleaseEvent(QKeyEvent* event)
{
    Q_D(ShadAttachment);
    if (d->isInteractive() && !event->isAutoRepeat() && event->key() == Qt::Key_Space)
    {
        event->accept();
        d->activate();
        return;
    }
    QWidget::keyReleaseEvent(event);
}

void ShadAttachment::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadAttachment);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QWidget::focusInEvent(event);
    update();
}

void ShadAttachment::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadAttachment);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QWidget::focusOutEvent(event);
    update();
}

void ShadAttachment::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadAttachment);
    using namespace ShadStyleHelper;
    const ShadAttachmentPrivate::SizeMetrics metrics = ShadAttachmentPrivate::sizeMetrics(d->_size);
    const QRectF card(d->cardRect());
    const qreal radius = shadTheme->getRadius(metrics.radius);
    const Qt::LayoutDirection direction = layoutDirection();
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);

    // bg-card，可点击时 hover:bg-muted/50
    const QColor hoverBackground = withAlpha(shadTheme->getColor(ShadThemeType::Muted), 0.5);
    fillRoundedRect(&painter, card, radius, mix(shadTheme->getColor(ShadThemeType::Card), hoverBackground, d->_hoverTransition->value()));
    // border；错误态 border-destructive/30，待上传 border-dashed
    const QColor borderColor = d->_state == ShadAttachmentType::Error ? withAlpha(shadTheme->getColor(ShadThemeType::Destructive), 0.3) : shadTheme->getColor(ShadThemeType::Border);
    if (d->_state == ShadAttachmentType::Idle)
    {
        strokeDashedRoundedRect(&painter, card, radius, borderColor);
    }
    else
    {
        strokeRoundedRect(&painter, card, radius, borderColor);
    }
    if ((hasFocus() && d->_isFocusVisible) || d->_isActionFocusVisible)
    {
        drawFocusRing(&painter, card, radius, withAlpha(shadTheme->getColor(ShadThemeType::Ring), 0.5), kRingWidth);
    }

    const ShadAttachmentPrivate::ContentLayout layout = d->computeLayout();
    if (d->hasMedia())
    {
        d->paintMedia(&painter, visualRect(direction, card, layout.mediaRect));
    }
    if (!d->_title.isEmpty())
    {
        d->paintTitle(&painter, visualRect(direction, card, layout.titleRect));
    }
    if (!d->_description.isEmpty())
    {
        // text-xs text-muted-foreground；错误态 text-destructive/80
        const QRectF descriptionRect = visualRect(direction, card, layout.descriptionRect);
        const QFont font = d->descriptionFont();
        const QColor color = d->_state == ShadAttachmentType::Error ? withAlpha(shadTheme->getColor(ShadThemeType::Destructive), 0.8) : shadTheme->getColor(ShadThemeType::MutedForeground);
        painter.setFont(font);
        painter.setPen(color);
        painter.drawText(descriptionRect, visualAlignment(direction, Qt::AlignLeft | Qt::AlignVCenter), QFontMetricsF(font).elidedText(d->_description, Qt::ElideRight, descriptionRect.width()));
    }
}
