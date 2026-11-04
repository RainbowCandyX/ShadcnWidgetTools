#include "ShadDropdownMenu.h"

#include <QAction>
#include <QActionGroup>
#include <QEasingCurve>
#include <QEvent>
#include <QGuiApplication>
#include <QHideEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QScreen>
#include <QTimer>
#include <QVariant>
#include <QVariantAnimation>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <utility>

#include "ShadComboBoxSearch.h"
#include "ShadDropdownMenuPrivate.h"
#include "ShadPlacement.h"
#include "ShadPopupEffect.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // 对应 duration-100
    constexpr int kOpenDurationMs = 100;
} // namespace

ShadDropdownMenu::ShadDropdownMenu(QWidget* parent)
    : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint), d_ptr(new ShadDropdownMenuPrivate())
{
    Q_D(ShadDropdownMenu);
    d->q_ptr = this;
    // 圆角、描边与阴影自绘，窗口背景透明
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    new ShadPopupEffect(this, [this]()
    {
        Q_D(ShadDropdownMenu);
        return d->cardRect();
    });

    d->_openAnimation = new QVariantAnimation(this);
    d->_openAnimation->setDuration(kOpenDurationMs);
    d->_openAnimation->setStartValue(0.0);
    d->_openAnimation->setEndValue(1.0);
    // 对应 CSS 的 ease 缓动
    QEasingCurve easingCurve(QEasingCurve::BezierSpline);
    easingCurve.addCubicBezierSegment(QPointF(0.25, 0.1), QPointF(0.25, 1.0), QPointF(1.0, 1.0));
    d->_openAnimation->setEasingCurve(easingCurve);
    connect(d->_openAnimation, &QVariantAnimation::valueChanged, this, [this, d](const QVariant& value)
    {
        d->_openProgress = value.toReal();
        setWindowOpacity(d->_openProgress);
        d->layoutWidgets();
        update();
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });

    d->_submenuTimer = new QTimer(this);
    d->_submenuTimer->setSingleShot(true);
    connect(d->_submenuTimer, &QTimer::timeout, this, [d]()
    {
        if (d->_pendingSubmenuIndex >= 0)
        {
            d->openSubmenu(d->_pendingSubmenuIndex, false);
        }
        else
        {
            d->closeSubmenu();
        }
    });
}

ShadDropdownMenu::~ShadDropdownMenu()
{
}

QAction* ShadDropdownMenu::addItem(const QString& text, ShadIconType::IconName icon, const QString& shortcutText)
{
    Q_D(ShadDropdownMenu);
    QAction* action = new QAction(text, this);
    ShadDropdownMenuPrivate::Entry entry;
    entry.kind = ShadDropdownMenuPrivate::EntryKind::Item;
    entry.action = action;
    entry.icon = icon;
    entry.shortcutText = shortcutText;
    d->_entries.append(entry);
    connect(action, &QAction::changed, this, [this]() { update(); });
    return action;
}

QAction* ShadDropdownMenu::addCheckableItem(const QString& text, bool checked)
{
    Q_D(ShadDropdownMenu);
    QAction* action = new QAction(text, this);
    action->setCheckable(true);
    action->setChecked(checked);
    ShadDropdownMenuPrivate::Entry entry;
    entry.kind = ShadDropdownMenuPrivate::EntryKind::CheckableItem;
    entry.action = action;
    d->_entries.append(entry);
    connect(action, &QAction::changed, this, [this]() { update(); });
    return action;
}

QAction* ShadDropdownMenu::addRadioItem(const QString& text, QActionGroup* group, bool checked)
{
    Q_D(ShadDropdownMenu);
    QAction* action = new QAction(text, this);
    action->setCheckable(true);
    if (group != nullptr)
    {
        group->addAction(action);
    }
    action->setChecked(checked);
    ShadDropdownMenuPrivate::Entry entry;
    entry.kind = ShadDropdownMenuPrivate::EntryKind::RadioItem;
    entry.action = action;
    d->_entries.append(entry);
    connect(action, &QAction::changed, this, [this]() { update(); });
    return action;
}

ShadDropdownMenu* ShadDropdownMenu::addMenu(const QString& text, ShadIconType::IconName icon)
{
    Q_D(ShadDropdownMenu);
    QAction* action = new QAction(text, this);
    ShadDropdownMenu * submenu = new ShadDropdownMenu(this);
    ShadDropdownMenuPrivate* submenuPrivate = submenu->d_func();
    submenuPrivate->_parentMenu = this;
    submenuPrivate->_menuAction = action;
    submenuPrivate->setMenubarStyle(d->_isMenubarStyle);
    ShadDropdownMenuPrivate::Entry entry;
    entry.kind = ShadDropdownMenuPrivate::EntryKind::Submenu;
    entry.action = action;
    entry.submenu = submenu;
    entry.icon = icon;
    d->_entries.append(entry);
    connect(action, &QAction::changed, this, [this]() { update(); });
    return submenu;
}

QAction* ShadDropdownMenu::menuAction() const
{
    Q_D(const ShadDropdownMenu);
    return d->_menuAction;
}

int ShadDropdownMenu::addLabel(const QString& text, bool inset)
{
    Q_D(ShadDropdownMenu);
    int labelIndex = 0;
    for (const ShadDropdownMenuPrivate::Entry& entry : std::as_const(d->_entries))
    {
        if (entry.kind == ShadDropdownMenuPrivate::EntryKind::Label)
        {
            ++labelIndex;
        }
    }
    ShadDropdownMenuPrivate::Entry entry;
    entry.kind = ShadDropdownMenuPrivate::EntryKind::Label;
    entry.labelText = text;
    entry.isInset = inset;
    d->_entries.append(entry);
    return labelIndex;
}

void ShadDropdownMenu::setLabelText(int labelIndex, const QString& text)
{
    Q_D(ShadDropdownMenu);
    int current = 0;
    for (ShadDropdownMenuPrivate::Entry& entry : d->_entries)
    {
        if (entry.kind != ShadDropdownMenuPrivate::EntryKind::Label)
        {
            continue;
        }
        if (current == labelIndex)
        {
            entry.labelText = text;
            // 与菜单项文字变化（QAction::changed）的处理一致，只重绘
            update();
            return;
        }
        ++current;
    }
}

void ShadDropdownMenu::addWidget(QWidget* widget)
{
    Q_D(ShadDropdownMenu);
    if (widget == nullptr)
    {
        return;
    }
    widget->setParent(this);
    // 鼠标事件交给菜单处理高亮与关闭
    widget->setAttribute(Qt::WA_TransparentForMouseEvents);
    // setParent 会隐藏组件，这里恢复，随菜单一起显示
    widget->show();
    ShadDropdownMenuPrivate::Entry entry;
    entry.kind = ShadDropdownMenuPrivate::EntryKind::Widget;
    entry.widget = widget;
    d->_entries.append(entry);
    d->layoutWidgets();
}

void ShadDropdownMenu::addSeparator()
{
    Q_D(ShadDropdownMenu);
    ShadDropdownMenuPrivate::Entry entry;
    entry.kind = ShadDropdownMenuPrivate::EntryKind::Separator;
    d->_entries.append(entry);
}

void ShadDropdownMenu::clear()
{
    Q_D(ShadDropdownMenu);
    d->closeSubmenu();
    for (const ShadDropdownMenuPrivate::Entry& entry : std::as_const(d->_entries))
    {
        delete entry.action;
        delete entry.widget.data();
        delete entry.submenu.data();
    }
    d->_entries.clear();
    d->_highlightIndex = -1;
    update();
}

void ShadDropdownMenu::setItemDestructive(QAction* action, bool destructive)
{
    Q_D(ShadDropdownMenu);
    for (ShadDropdownMenuPrivate::Entry& entry : d->_entries)
    {
        if (entry.action == action)
        {
            entry.isDestructive = destructive;
            update();
            return;
        }
    }
}

void ShadDropdownMenu::setItemInset(QAction* action, bool inset)
{
    Q_D(ShadDropdownMenu);
    for (ShadDropdownMenuPrivate::Entry& entry : d->_entries)
    {
        if (entry.action == action)
        {
            entry.isInset = inset;
            update();
            return;
        }
    }
}

void ShadDropdownMenu::setMatchAnchorWidth(bool match)
{
    Q_D(ShadDropdownMenu);
    d->_isMatchAnchorWidth = match;
}

bool ShadDropdownMenu::isMatchAnchorWidth() const
{
    Q_D(const ShadDropdownMenu);
    return d->_isMatchAnchorWidth;
}

void ShadDropdownMenu::popup(QWidget* anchor, Qt::Alignment alignment)
{
    Q_D(ShadDropdownMenu);
    if (anchor == nullptr)
    {
        return;
    }
    // 按可视边缘对齐
    d->popupForRect(ShadDropdownMenuPrivate::anchorRect(anchor), alignment, anchor->layoutDirection());
    // 记录锚点，用于再次点击 anchor 时关闭菜单
    d->_anchor = anchor;
}

void ShadDropdownMenu::popup(QWidget* anchor, ShadPlacementType::Side side, ShadPlacementType::Align align)
{
    Q_D(ShadDropdownMenu);
    if (anchor == nullptr)
    {
        return;
    }
    const Qt::LayoutDirection direction = anchor->layoutDirection();
    setLayoutDirection(direction);
    const QRect anchorRect = ShadDropdownMenuPrivate::anchorRect(anchor);
    d->_anchorWidth = anchorRect.width();
    const ShadPlacement::Result placement = ShadPlacement::place(anchorRect, d->cardSize(), side, align, ShadDropdownMenuPrivate::kSideOffset, direction);
    d->showAt(placement.rect.topLeft(), placement.side);
    d->_anchor = anchor;
}

void ShadDropdownMenu::popup(const QRect& anchorRect, Qt::Alignment alignment)
{
    Q_D(ShadDropdownMenu);
    d->popupForRect(anchorRect, alignment, d->fallbackDirection());
}

void ShadDropdownMenu::popupAt(const QPoint& globalPos)
{
    Q_D(ShadDropdownMenu);
    const Qt::LayoutDirection direction = d->fallbackDirection();
    // 菜单项按该方向绘制
    setLayoutDirection(direction);
    d->_anchorWidth = 0;
    const QSize card = d->cardSize();
    // RTL 时 globalPos 为菜单右上角，菜单向左展开
    QPoint topLeft(direction == Qt::RightToLeft ? globalPos.x() + 1 - card.width() : globalPos.x(), globalPos.y());
    if (const QScreen* screen = QGuiApplication::screenAt(globalPos))
    {
        const QRect available = screen->availableGeometry();
        topLeft.setX(qBound(available.left() + ShadDropdownMenuPrivate::kScreenMargin, topLeft.x(), available.right() + 1 - ShadDropdownMenuPrivate::kScreenMargin - card.width()));
        topLeft.setY(qBound(available.top() + ShadDropdownMenuPrivate::kScreenMargin, topLeft.y(), available.bottom() + 1 - ShadDropdownMenuPrivate::kScreenMargin - card.height()));
    }
    d->_anchor = nullptr;
    d->showAt(topLeft, ShadPlacementType::Bottom);
}

QSize ShadDropdownMenu::sizeHint() const
{
    Q_D(const ShadDropdownMenu);
    return d->cardSize() + QSize(2 * ShadDropdownMenuPrivate::kShadowMargin, 2 * ShadDropdownMenuPrivate::kShadowMargin);
}

void ShadDropdownMenu::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadDropdownMenu);
    switch (event->key())
    {
        case Qt::Key_Down:
        case Qt::Key_Up:
            {
                d->moveHighlight(event->key() == Qt::Key_Down ? 1 : -1);
                event->accept();
                return;
            }
        case Qt::Key_Home:
        case Qt::Key_End:
            {
                d->highlightEdge(event->key() == Qt::Key_Home);
                event->accept();
                return;
            }
        case Qt::Key_Return:
        case Qt::Key_Enter:
            {
                d->activate(d->_highlightIndex, true);
                event->accept();
                return;
            }
        case Qt::Key_Left:
        case Qt::Key_Right:
            {
                // 结束侧打开子菜单，起始侧收起本子菜单
                const bool isEndKey = (event->key() == Qt::Key_Right) == (layoutDirection() == Qt::LeftToRight);
                const bool isOnSubmenu = d->isSelectable(d->_highlightIndex) && d->_entries.at(d->_highlightIndex).kind == ShadDropdownMenuPrivate::EntryKind::Submenu;
                if (isEndKey && isOnSubmenu)
                {
                    d->openSubmenu(d->_highlightIndex, true);
                    event->accept();
                    return;
                }
                if (!isEndKey && !d->_parentMenu.isNull())
                {
                    hide();
                    event->accept();
                    return;
                }
                event->ignore();
                return;
            }
        case Qt::Key_Space:
            {
                // 按键查找过程中空格作为查找文字的一部分
                const bool isTypeaheadActive = !d->_typeaheadBuffer.isEmpty() && d->_typeaheadTimer.isValid() && d->_typeaheadTimer.elapsed() <= ShadComboBoxSearch::kTypeaheadTimeoutMs;
                if (isTypeaheadActive)
                {
                    d->handleTypeahead(event->text());
                }
                else
                {
                    d->activate(d->_highlightIndex, true);
                }
                event->accept();
                return;
            }
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            {
                d->rootMenu()->hide();
                hide();
                event->accept();
                return;
            }
        default:
            {
                break;
            }
    }
    const QString text = event->text();
    const bool hasCommandModifier = event->modifiers().testAnyFlags(Qt::ControlModifier | Qt::MetaModifier | Qt::AltModifier);
    if (!hasCommandModifier && !text.isEmpty() && text.at(0).isPrint())
    {
        d->handleTypeahead(text);
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void ShadDropdownMenu::mouseMoveEvent(QMouseEvent* event)
{
    Q_D(ShadDropdownMenu);
    if (d->forwardToParent(event))
    {
        return;
    }
    if (!d->_parentMenu.isNull())
    {
        // 鼠标进入子菜单后父菜单不再收起它
        ShadDropdownMenuPrivate* parentPrivate = d->_parentMenu->d_func();
        parentPrivate->_submenuTimer->stop();
        parentPrivate->_pendingSubmenuIndex = -1;
        parentPrivate->setHighlight(parentPrivate->_openSubmenuIndex);
    }
    d->hoverEntry(d->entryAt(event->position().toPoint()));
    QWidget::mouseMoveEvent(event);
}

void ShadDropdownMenu::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadDropdownMenu);
    if (d->forwardToParent(event))
    {
        event->accept();
        return;
    }
    if (!d->cardRect().contains(event->position().toPoint()))
    {
        // 点击 anchor 关闭菜单时不再把这次按下转发给 anchor，否则会立刻重新打开
        if (!d->_anchor.isNull())
        {
            const QPoint anchorPos = d->_anchor->mapFromGlobal(event->globalPosition().toPoint());
            if (d->_anchor->rect().contains(anchorPos))
            {
                setAttribute(Qt::WA_NoMouseReplay);
            }
        }
        hide();
        event->accept();
        return;
    }
    event->accept();
}

void ShadDropdownMenu::mouseReleaseEvent(QMouseEvent* event)
{
    Q_D(ShadDropdownMenu);
    if (d->forwardToParent(event))
    {
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton)
    {
        d->activate(d->entryAt(event->position().toPoint()));
    }
    event->accept();
}

void ShadDropdownMenu::leaveEvent(QEvent* event)
{
    Q_D(ShadDropdownMenu);
    if (d->_openSubmenu.isNull() || !d->_openSubmenu->isVisible())
    {
        d->setHighlight(-1);
    }
    QWidget::leaveEvent(event);
}

void ShadDropdownMenu::hideEvent(QHideEvent* event)
{
    Q_D(ShadDropdownMenu);
    d->_openAnimation->stop();
    d->closeSubmenu();
    d->_highlightIndex = -1;
    if (!d->_parentMenu.isNull())
    {
        ShadDropdownMenuPrivate* parentPrivate = d->_parentMenu->d_func();
        if (parentPrivate->_openSubmenu == this)
        {
            parentPrivate->_openSubmenu = nullptr;
            parentPrivate->_openSubmenuIndex = -1;
            d->_parentMenu->update();
        }
    }
    Q_EMIT aboutToHide();
    QWidget::hideEvent(event);
}

void ShadDropdownMenu::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadDropdownMenu);
    using namespace ShadStyleHelper;
    const QRectF card = d->cardRect();
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    // 子菜单为 shadow-lg
    if (d->_parentMenu.isNull())
    {
        drawShadowMd(&painter, card, radius);
    }
    else
    {
        drawShadowLg(&painter, card, radius);
    }
    ShadPopupEffect::paintBackdrop(&painter, this, card, radius);
    fillRoundedRect(&painter, card, radius, shadTheme->getColor(ShadThemeType::Popover));
    strokeRoundedRect(&painter, card, radius, withAlpha(shadTheme->getColor(ShadThemeType::Foreground), 0.1));
    for (int index = 0; index < int(d->_entries.size()); ++index)
    {
        if (d->isVisibleEntry(index))
        {
            d->paintEntry(&painter, index);
        }
    }
}
