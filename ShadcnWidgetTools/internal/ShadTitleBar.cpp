#include "ShadTitleBar.h"

#include <QAbstractButton>
#include <QApplication>
#include <QEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QList>
#include <QMouseEvent>
#include <QPixmap>
#include <QRect>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QSize>
#include <QSizePolicy>
#include <QString>
#include <QWindow>
#include <QtGlobal>

#include "ShadLabel.h"
#include "ShadStyleHelper.h"
#include "ShadTabViewBar.h"
#include "ShadWindowButton.h"

namespace
{
    constexpr int kPaddingLeft = 12;
    constexpr int kIconSize = 16;
    constexpr int kIconGap = 8;
    constexpr int kAreaSpacing = 12;
    constexpr int kTitlePixelSize = 13;
} // namespace

ShadTitleBar::ShadTitleBar(QWidget* window)
    : QWidget(window), _window(window)
{
    _mainLayout = new QHBoxLayout(this);
    _mainLayout->setSpacing(0);

    // 图标与标题不接收鼠标事件，事件交给标题栏处理拖动
    _iconLabel = new QLabel(this);
    _iconLabel->setFixedSize(kIconSize, kIconSize);
    _iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    _titleLabel = new ShadLabel(this);
    _titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    ShadStyleHelper::applyFont(_titleLabel, kTitlePixelSize, QFont::Medium);
    // 窗口变窄时允许标题被截断，不撑大窗口的最小宽度
    _titleLabel->setMinimumWidth(1);

    _leftAreaLayout = new QHBoxLayout();
    _leftAreaLayout->setContentsMargins(0, 0, 0, 0);
    _leftAreaLayout->setSpacing(0);
    _rightAreaLayout = new QHBoxLayout();
    _rightAreaLayout->setContentsMargins(0, 0, 0, 0);
    _rightAreaLayout->setSpacing(0);

    _stayOnTopButton = new ShadWindowButton(ShadWindowButton::Kind::StayOnTop, this);
    _stayOnTopButton->setCheckable(true);
    _stayOnTopButton->setVisible(false);
    _minimizeButton = new ShadWindowButton(ShadWindowButton::Kind::Minimize, this);
    _maximizeButton = new ShadWindowButton(ShadWindowButton::Kind::Maximize, this);
    _closeButton = new ShadWindowButton(ShadWindowButton::Kind::Close, this);

    _buttonLayout = new QHBoxLayout();
    _buttonLayout->setSpacing(0);

    _mainLayout->addWidget(_iconLabel, 0, Qt::AlignVCenter);
    _mainLayout->addWidget(_titleLabel, 0, Qt::AlignVCenter);
    _mainLayout->addSpacing(kAreaSpacing);
    _mainLayout->addLayout(_leftAreaLayout);
    _mainLayout->addStretch(1);
    _mainLayout->addLayout(_rightAreaLayout);
    updateButtonLayout();

    // 点击后按钮已切换勾选状态，按新状态设置窗口
    connect(_stayOnTopButton, &QAbstractButton::clicked, this, [this](bool checked) { setStayOnTop(checked); });
    connect(_minimizeButton, &QAbstractButton::clicked, this, [this]() { _window->showMinimized(); });
    connect(_maximizeButton, &QAbstractButton::clicked, this, [this]() { toggleMaximize(); });
    connect(_closeButton, &QAbstractButton::clicked, this, [this]() { requestClose(); });
    connect(_window, &QWidget::windowTitleChanged, this, [this]() { updateWindowTitle(); });
    connect(_window, &QWidget::windowIconChanged, this, [this]() { updateWindowIcon(); });

    updateWindowIcon();
    updateWindowTitle();
    updateStayOnTopButton();
}

ShadTitleBar::~ShadTitleBar()
{
}

void ShadTitleBar::setBarHeight(int height)
{
    setFixedHeight(height);
    _stayOnTopButton->setFixedSize(ShadWindowButton::kButtonWidth, height);
    _minimizeButton->setFixedSize(ShadWindowButton::kButtonWidth, height);
    _maximizeButton->setFixedSize(ShadWindowButton::kButtonWidth, height);
    _closeButton->setFixedSize(ShadWindowButton::kButtonWidth, height);
}

void ShadTitleBar::setButtonFlags(ShadWindowType::ButtonFlags buttonFlags)
{
    _buttonFlags = buttonFlags;
    _stayOnTopButton->setVisible(buttonFlags.testFlag(ShadWindowType::StayOnTopButton));
    _minimizeButton->setVisible(buttonFlags.testFlag(ShadWindowType::MinimizeButton));
    _maximizeButton->setVisible(buttonFlags.testFlag(ShadWindowType::MaximizeButton));
    _closeButton->setVisible(buttonFlags.testFlag(ShadWindowType::CloseButton));
}

ShadWindowType::ButtonFlags ShadTitleBar::getButtonFlags() const
{
    return _buttonFlags;
}

void ShadTitleBar::setButtonPosition(ShadWindowType::ButtonPosition position)
{
#ifdef Q_OS_MACOS
    if (_buttonPosition == position)
    {
        return;
    }
    _buttonPosition = position;
    updateButtonLayout();
#else
    Q_UNUSED(position);
#endif
}

ShadWindowType::ButtonPosition ShadTitleBar::getButtonPosition() const
{
    return _buttonPosition;
}

void ShadTitleBar::setAreaWidget(ShadWindowType::TitleBarArea area, QWidget* widget)
{
    const bool isLeft = area == ShadWindowType::LeftArea;
    QPointer<QWidget>& currentWidget = isLeft ? _leftAreaWidget : _rightAreaWidget;
    QHBoxLayout* areaLayout = isLeft ? _leftAreaLayout : _rightAreaLayout;
    if (currentWidget == widget)
    {
        return;
    }
    if (!currentWidget.isNull())
    {
        areaLayout->removeWidget(currentWidget);
        ShadStyleHelper::clearFocusWithin(currentWidget);
        currentWidget->hide();
        currentWidget->deleteLater();
    }
    currentWidget = widget;
    if (widget != nullptr)
    {
        areaLayout->addWidget(widget, 0, Qt::AlignVCenter);
    }
}

QWidget* ShadTitleBar::getAreaWidget(ShadWindowType::TitleBarArea area) const
{
    return area == ShadWindowType::LeftArea ? _leftAreaWidget.data() : _rightAreaWidget.data();
}

void ShadTitleBar::setDefaultCloseEnabled(bool enabled)
{
    _isDefaultCloseEnabled = enabled;
}

bool ShadTitleBar::isDefaultCloseEnabled() const
{
    return _isDefaultCloseEnabled;
}

void ShadTitleBar::requestClose()
{
    Q_EMIT closeButtonClicked();
    if (_isDefaultCloseEnabled)
    {
        _window->close();
    }
}

void ShadTitleBar::setStayOnTop(bool stayOnTop)
{
    if (isStayOnTop() == stayOnTop)
    {
        updateStayOnTopButton();
        return;
    }
    Qt::WindowFlags flags = _window->windowFlags();
    flags.setFlag(Qt::WindowStaysOnTopHint, stayOnTop);
    QWindow* handle = _window->windowHandle();
    if (handle == nullptr)
    {
        _window->setWindowFlags(flags);
    }
    else
    {
        // setWindowFlags 会隐藏并重建原生窗口，这里只更新原生窗口的标志
        _window->overrideWindowFlags(flags);
        handle->setFlags(flags);
    }
    updateStayOnTopButton();
    Q_EMIT stayOnTopChanged(stayOnTop);
}

bool ShadTitleBar::isStayOnTop() const
{
    return _window->windowFlags().testFlag(Qt::WindowStaysOnTopHint);
}

bool ShadTitleBar::isCaptionAt(const QPoint& localPos) const
{
    if (!rect().contains(localPos))
    {
        return false;
    }
    // 图标与标题设置了 WA_TransparentForMouseEvents，不会被 childAt 找到
    QWidget* child = childAt(localPos);
    if (child == nullptr)
    {
        return true;
    }
    if (const ShadTabViewBar* tabBar = qobject_cast<const ShadTabViewBar*>(child))
    {
        return tabBar->isBlankAt(tabBar->mapFrom(this, localPos));
    }
    return false;
}

bool ShadTitleBar::canMaximize() const
{
    return _buttonFlags.testFlag(ShadWindowType::MaximizeButton) && _window->minimumSize() != _window->maximumSize();
}

void ShadTitleBar::toggleMaximize()
{
    if (!canMaximize())
    {
        return;
    }
    if (_window->isMaximized())
    {
        _window->showNormal();
    }
    else
    {
        _window->showMaximized();
    }
}

ShadWindowButton* ShadTitleBar::getMaximizeButton() const
{
    return _maximizeButton;
}

void ShadTitleBar::updateMaximizeState()
{
    _maximizeButton->setKind(_window->isMaximized() ? ShadWindowButton::Kind::Restore : ShadWindowButton::Kind::Maximize);
}

void ShadTitleBar::setWindowActive(bool isActive)
{
    _titleLabel->setColorRole(isActive ? ShadThemeType::Foreground : ShadThemeType::MutedForeground);
    _stayOnTopButton->setWindowActive(isActive);
    _minimizeButton->setWindowActive(isActive);
    _maximizeButton->setWindowActive(isActive);
    _closeButton->setWindowActive(isActive);
}

void ShadTitleBar::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        // 刷新置顶按钮的提示
        updateStayOnTopButton();
    }
    QWidget::changeEvent(event);
}

void ShadTitleBar::showEvent(QShowEvent* event)
{
    // 置顶状态会被 setWindowFlags 直接修改，显示时重新同步
    updateStayOnTopButton();
    QWidget::showEvent(event);
}

void ShadTitleBar::mousePressEvent(QMouseEvent* event)
{
#ifndef Q_OS_WIN
    // Windows 上可拖动区域由 WM_NCHITTEST 返回 HTCAPTION 交给系统处理，不会走到这里
    if (event->button() == Qt::LeftButton && !_window->isFullScreen() && isCaptionAt(event->position().toPoint()))
    {
        _isCaptionPressed = true;
        _pressGlobalPos = event->globalPosition().toPoint();
        event->accept();
        return;
    }
#endif
    QWidget::mousePressEvent(event);
}

void ShadTitleBar::mouseMoveEvent(QMouseEvent* event)
{
#ifndef Q_OS_WIN
    if (_isManualMoving && event->buttons().testFlag(Qt::LeftButton))
    {
        _window->move(_moveStartWindowPos + event->globalPosition().toPoint() - _pressGlobalPos);
        event->accept();
        return;
    }
    if (_isCaptionPressed && event->buttons().testFlag(Qt::LeftButton))
    {
        if ((event->globalPosition().toPoint() - _pressGlobalPos).manhattanLength() >= QApplication::startDragDistance())
        {
            _isCaptionPressed = false;
            // Wayland 下只能由系统移动窗口
            QWindow* handle = _window->windowHandle();
            if (handle == nullptr || !handle->startSystemMove())
            {
                _isManualMoving = true;
                _moveStartWindowPos = _window->pos();
            }
        }
        event->accept();
        return;
    }
#endif
    QWidget::mouseMoveEvent(event);
}

void ShadTitleBar::mouseReleaseEvent(QMouseEvent* event)
{
    _isCaptionPressed = false;
    _isManualMoving = false;
    QWidget::mouseReleaseEvent(event);
}

void ShadTitleBar::mouseDoubleClickEvent(QMouseEvent* event)
{
#ifndef Q_OS_WIN
    if (event->button() == Qt::LeftButton && isCaptionAt(event->position().toPoint()))
    {
        _isCaptionPressed = false;
        toggleMaximize();
        event->accept();
        return;
    }
#endif
    QWidget::mouseDoubleClickEvent(event);
}

void ShadTitleBar::updateWindowIcon()
{
    const QIcon icon = _window->windowIcon();
    if (icon.isNull())
    {
        _iconLabel->clear();
        _iconLabel->setVisible(false);
    }
    else
    {
        _iconLabel->setPixmap(icon.pixmap(QSize(kIconSize, kIconSize), devicePixelRatioF()));
        _iconLabel->setVisible(true);
    }
    // contentsMargins 不随布局方向镜像，左右对称设置，RTL 时图标在右侧同样留出间距
    const int iconGap = icon.isNull() ? 0 : kIconGap;
    _titleLabel->setContentsMargins(iconGap, 0, iconGap, 0);
}

void ShadTitleBar::updateStayOnTopButton()
{
    const bool stayOnTop = isStayOnTop();
    const QSignalBlocker blocker(_stayOnTopButton);
    _stayOnTopButton->setChecked(stayOnTop);
    _stayOnTopButton->setToolTip(stayOnTop ? tr("取消置顶") : tr("置顶"));
    _stayOnTopButton->update();
}

void ShadTitleBar::updateButtonLayout()
{
    const bool isLeft = _buttonPosition == ShadWindowType::ButtonsOnLeft;
    _mainLayout->removeItem(_buttonLayout);
    _buttonLayout->setParent(nullptr);
    if (isLeft)
    {
        _mainLayout->insertLayout(0, _buttonLayout);
    }
    else
    {
        _mainLayout->addLayout(_buttonLayout);
    }
    _mainLayout->setContentsMargins(isLeft ? 0 : kPaddingLeft, 0, isLeft ? kPaddingLeft : 0, 0);
    _buttonLayout->setContentsMargins(isLeft ? 0 : kAreaSpacing, 0, isLeft ? kAreaSpacing : 0, 0);
    const QList<ShadWindowButton*> buttons = isLeft ? QList<ShadWindowButton*>{_closeButton, _minimizeButton, _maximizeButton, _stayOnTopButton} : QList<ShadWindowButton*>{_stayOnTopButton, _minimizeButton, _maximizeButton, _closeButton};
    for (ShadWindowButton* button : buttons)
    {
        _buttonLayout->removeWidget(button);
    }
    for (ShadWindowButton* button : buttons)
    {
        _buttonLayout->addWidget(button);
    }
}

void ShadTitleBar::updateWindowTitle()
{
    const QString title = _window->windowTitle();
    _titleLabel->setText(title);
    _titleLabel->setVisible(!title.isEmpty());
}
