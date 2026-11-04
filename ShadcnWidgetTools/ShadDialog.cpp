#include "ShadDialog.h"

#include <QAbstractAnimation>
#include <QByteArray>
#include <QEasingCurve>
#include <QFont>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QRect>
#include <QScreen>
#include <QString>
#include <QVBoxLayout>
#include <QVariant>
#include <QVariantAnimation>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDialogPrivate.h"
#include "ShadLabel.h"
#include "ShadStyleHelper.h"
#include "ShadTitleBar.h"
#include "ShadWindowFrame.h"

ShadDialog::ShadDialog(QWidget* parent)
    : QDialog(parent), d_ptr(new ShadDialogPrivate())
{
    Q_D(ShadDialog);
    d->q_ptr = this;
    setSizeGripEnabled(false);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);

    d->_titleBar = new ShadTitleBar(this);
    d->_titleBar->setBarHeight(d->_titleBarHeight);
    d->_titleBar->setWindowActive(false);
    d->_titleBar->setButtonFlags(ShadWindowType::CloseButton);
    d->_frame = new ShadWindowFrame(this, d->_titleBar);
    d->updateWindowHints();

    d->_body = new QWidget(this);
    QVBoxLayout* bodyLayout = new QVBoxLayout(d->_body);
    bodyLayout->setContentsMargins(ShadDialogPrivate::kPadding, ShadDialogPrivate::kPadding, ShadDialogPrivate::kPadding, ShadDialogPrivate::kPadding);
    bodyLayout->setSpacing(ShadDialogPrivate::kSectionGap);

    d->_header = new QWidget(d->_body);
    QVBoxLayout* headerLayout = new QVBoxLayout(d->_header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(ShadDialogPrivate::kHeaderGap);
    d->_titleLabel = new ShadLabel(d->_header);
    d->_titleLabel->setColorRole(ShadThemeType::Foreground);
    d->_titleLabel->setWordWrap(true);
    ShadStyleHelper::applyFont(d->_titleLabel, 14, QFont::Medium);
    d->_descriptionLabel = new ShadLabel(d->_header);
    d->_descriptionLabel->setColorRole(ShadThemeType::MutedForeground);
    d->_descriptionLabel->setWordWrap(true);
    ShadStyleHelper::applyFont(d->_descriptionLabel, 14, QFont::Normal);
    headerLayout->addWidget(d->_titleLabel);
    headerLayout->addWidget(d->_descriptionLabel);
    headerLayout->setAlignment(Qt::AlignTop);
    bodyLayout->addWidget(d->_header);

    d->_contentLayout = new QVBoxLayout();
    d->_contentLayout->setContentsMargins(0, 0, 0, 0);
    d->_contentLayout->setSpacing(ShadDialogPrivate::kSectionGap);
    bodyLayout->addLayout(d->_contentLayout, 1);
    // 内容区不能再变高时由这里接住多余高度，空白留在内容与页脚之间
    bodyLayout->addStretch(0);

    d->_footer = new QWidget(d->_body);
    d->_footerLayout = new QHBoxLayout(d->_footer);
    d->_footerLayout->setContentsMargins(0, 0, 0, 0);
    d->_footerLayout->setSpacing(ShadDialogPrivate::kFooterGap - 2 * ShadStyleHelper::kFocusRingWidth);
    // AlignRight 在 RTL 下自动镜像
    d->_footerLayout->setAlignment(Qt::AlignRight | Qt::AlignBottom);
    d->_footer->setVisible(false);
    bodyLayout->addWidget(d->_footer);

    d->_rootLayout = new QVBoxLayout(this);
    d->_rootLayout->setContentsMargins(0, 0, 0, 0);
    d->_rootLayout->setSpacing(0);
    d->_rootLayout->addWidget(d->_titleBar);
    d->_rootLayout->addWidget(d->_body, 1);

    d->_animation = new QVariantAnimation(this);
    d->_animation->setDuration(ShadDialogPrivate::kAnimationDurationMs);
    d->_animation->setStartValue(0.0);
    d->_animation->setEndValue(1.0);
    d->_animation->setEasingCurve(QEasingCurve::OutCubic);
    connect(d->_animation, &QVariantAnimation::valueChanged, this, [d](const QVariant& value) { d->setProgress(value.toReal()); });
    connect(d->_animation, &QVariantAnimation::finished, this, [this, d]()
    {
        if (!d->_isClosing)
        {
            return;
        }
        d->_isClosing = false;
        QDialog::done(d->_pendingResult);
    });

    d->updateHeader();
}

ShadDialog::ShadDialog(const QString& title, const QString& description, QWidget* parent)
    : ShadDialog(parent)
{
    setTitle(title);
    setDescription(description);
}

ShadDialog::~ShadDialog()
{
    Q_D(ShadDialog);
    // 断开捕获 d 的连接，避免子对象析构时访问已释放的私有数据
    QObject::disconnect(d->_animation, nullptr, this, nullptr);
    d->releaseOverlay();
}

void ShadDialog::setTitle(const QString& title)
{
    Q_D(ShadDialog);
    d->_titleLabel->setText(title);
    d->updateHeader();
}

QString ShadDialog::getTitle() const
{
    Q_D(const ShadDialog);
    return d->_titleLabel->text();
}

void ShadDialog::setDescription(const QString& description)
{
    Q_D(ShadDialog);
    d->_descriptionLabel->setText(description);
    d->updateHeader();
}

QString ShadDialog::getDescription() const
{
    Q_D(const ShadDialog);
    return d->_descriptionLabel->text();
}

void ShadDialog::setTitleBarHeight(int height)
{
    Q_D(ShadDialog);
    const int barHeight = qMax(0, height);
    if (d->_titleBarHeight == barHeight)
    {
        return;
    }
    d->_titleBarHeight = barHeight;
    d->_titleBar->setBarHeight(barHeight);
}

int ShadDialog::getTitleBarHeight() const
{
    Q_D(const ShadDialog);
    return d->_titleBarHeight;
}

void ShadDialog::setButtonFlags(ShadWindowType::ButtonFlags buttonFlags)
{
    Q_D(ShadDialog);
    d->_titleBar->setButtonFlags(buttonFlags);
    // 修改窗口标志会重建原生窗口，显示期间不同步
    if (!isVisible())
    {
        d->updateWindowHints();
    }
}

ShadWindowType::ButtonFlags ShadDialog::getButtonFlags() const
{
    Q_D(const ShadDialog);
    return d->_titleBar->getButtonFlags();
}

void ShadDialog::setButtonPosition(ShadWindowType::ButtonPosition position)
{
    Q_D(ShadDialog);
    d->_titleBar->setButtonPosition(position);
}

ShadWindowType::ButtonPosition ShadDialog::getButtonPosition() const
{
    Q_D(const ShadDialog);
    return d->_titleBar->getButtonPosition();
}

void ShadDialog::setOverlayVisible(bool visible)
{
    Q_D(ShadDialog);
    d->_isOverlayVisible = visible;
}

bool ShadDialog::isOverlayVisible() const
{
    Q_D(const ShadDialog);
    return d->_isOverlayVisible;
}

QVBoxLayout* ShadDialog::contentLayout() const
{
    Q_D(const ShadDialog);
    return d->_contentLayout;
}

QHBoxLayout* ShadDialog::footerLayout()
{
    Q_D(ShadDialog);
    d->_footer->setVisible(true);
    return d->_footerLayout;
}

void ShadDialog::moveToCenter()
{
    Q_D(ShadDialog);
    if (isMaximized() || isFullScreen())
    {
        return;
    }
    QWidget* host = d->hostWindow();
    QRect area;
    if (host != nullptr && host->isVisible())
    {
        area = host->geometry();
    }
    else if (QScreen* screen = d->targetScreen())
    {
        area = screen->availableGeometry();
    }
    if (!area.isValid())
    {
        return;
    }
    // move 使用含系统边框的位置，需扣除与客户区的偏移
    QRect clientRect = geometry();
    clientRect.moveCenter(area.center());
    const QPoint frameOffset = geometry().topLeft() - pos();
    move(clientRect.topLeft() - frameOffset);
}

void ShadDialog::open()
{
    Q_D(ShadDialog);
    d->_flagsBeforeOpen = windowFlags();
#ifdef Q_OS_MACOS
    d->_flagsBeforeOpen |= Qt::FramelessWindowHint;
#endif
    d->_isOpening = true;
    QDialog::open();
    d->_isOpening = false;
}

void ShadDialog::done(int result)
{
    Q_D(ShadDialog);
    if (!isVisible())
    {
        QDialog::done(result);
        return;
    }
    if (d->_isClosing)
    {
        return;
    }
    d->_isClosing = true;
    d->_pendingResult = result;
    d->_animation->setDirection(QAbstractAnimation::Backward);
    if (d->_animation->state() != QAbstractAnimation::Running)
    {
        d->_animation->start();
    }
}

void ShadDialog::setVisible(bool visible)
{
    Q_D(ShadDialog);
    if (!visible || isVisible())
    {
        QDialog::setVisible(visible);
        return;
    }
    d->_animation->stop();
    d->_isClosing = false;
    // 窗口尚未显示，此时恢复 open() 之前的窗口标志不会闪烁
    if (d->_isOpening && windowFlags() != d->_flagsBeforeOpen)
    {
        setWindowFlags(d->_flagsBeforeOpen);
    }
#ifdef Q_OS_MACOS
    // macOS 上有父窗口的窗口模态显示为 sheet，改为应用模态
    if (windowModality() == Qt::WindowModal && parentWidget() != nullptr)
    {
        d->_isModalityOverridden = true;
        d->_wasModalitySet = testAttribute(Qt::WA_SetWindowModality);
        setWindowModality(Qt::ApplicationModal);
    }
#endif
    // 顶层窗口不继承父组件的布局方向；同步后清除标记，不影响用户显式设置的方向
    QWidget* parent = parentWidget();
    if (parent != nullptr && !testAttribute(Qt::WA_SetLayoutDirection))
    {
        setLayoutDirection(parent->layoutDirection());
        setAttribute(Qt::WA_SetLayoutDirection, false);
    }
    d->updateWindowHints();
    d->autoResize();
    d->setProgress(0.0);
    QDialog::setVisible(true);
    if (!isVisible())
    {
        return;
    }
    // 模态在显示前设置，此时才能判断是否需要遮罩
    d->showOverlay();
    d->focusFirstChild();
    d->_animation->setDirection(QAbstractAnimation::Forward);
    d->_animation->start();
}

bool ShadDialog::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
    Q_D(ShadDialog);
    if (d->_frame->handleNativeEvent(eventType, message, result))
    {
        return true;
    }
    return QDialog::nativeEvent(eventType, message, result);
}

void ShadDialog::hideEvent(QHideEvent* event)
{
    Q_D(ShadDialog);
    // 父窗口最小化引起的 spontaneous 隐藏不重置
    if (!event->spontaneous())
    {
        d->_animation->stop();
        d->_isClosing = false;
        d->releaseOverlay();
        if (d->_isModalityOverridden)
        {
            d->_isModalityOverridden = false;
            setWindowModality(Qt::WindowModal);
            setAttribute(Qt::WA_SetWindowModality, d->_wasModalitySet);
        }
    }
    QDialog::hideEvent(event);
}

void ShadDialog::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadDialog);
    QPainter painter(this);
    d->_frame->paint(&painter);
}
