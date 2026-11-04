#include "ShadDockGroup.h"

#include <QEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QStackedLayout>
#include <QString>
#include <QVBoxLayout>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDockHeader.h"
#include "ShadDockPanel.h"
#include "ShadIconType.h"
#include "ShadPageTransition.h"
#include "ShadSeparator.h"
#include "ShadStyleHelper.h"
#include "ShadTabViewBar.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kDocumentWidth = 480;
    constexpr int kDocumentHeight = 320;
    constexpr int kToolWidth = 260;
    constexpr int kToolHeight = 220;
} // namespace

ShadDockGroup::ShadDockGroup(Kind kind, QWidget* parent)
    : QWidget(parent), _kind(kind)
{
    _layout = new QVBoxLayout(this);
    _layout->setContentsMargins(0, 0, 0, 0);
    _layout->setSpacing(0);
    _bar = new ShadTabViewBar(this);
    _bar->setAddButtonVisible(false);
    _bar->setDetachEnabled(true);
    _bar->setTabsClosable(kind == Kind::Document);
    _bar->installEventFilter(this);
    _topSeparator = new ShadSeparator(Qt::Horizontal, this);
    _pageArea = new QWidget(this);
    _pageStack = new QStackedLayout(_pageArea);
    _pageStack->setContentsMargins(0, 0, 0, 0);
    if (kind == Kind::Tool)
    {
        _header = new ShadDockHeader(this);
        _header->installEventFilter(this);
        _bottomSeparator = new ShadSeparator(Qt::Horizontal, this);
        _layout->addWidget(_header);
        _layout->addWidget(_topSeparator);
        _layout->addWidget(_pageArea, 1);
        _layout->addWidget(_bottomSeparator);
        _layout->addWidget(_bar);
        connect(_header, &ShadDockHeader::closeClicked, this, [this]()
        {
            if (ShadDockPanel* current = currentPanel())
            {
                Q_EMIT closeRequested(current);
            }
        });
        connect(_header, &ShadDockHeader::dragRequested, this, &ShadDockGroup::groupDragRequested);
    }
    else
    {
        _layout->addWidget(_bar);
        _layout->addWidget(_topSeparator);
        _layout->addWidget(_pageArea, 1);
    }
    _transition = new ShadPageTransition(_pageArea);
    connect(_bar, &ShadTabViewBar::currentChanged, this, [this](int index)
    {
        ShadDockPanel* next = panel(index);
        // 移除当前面板时 QStackedLayout 会自行切换，以上次显示的面板为准
        ShadDockPanel* previous = _shownPanel.data();
        _shownPanel = next;
        const bool isAnimated = previous != nullptr && next != nullptr && previous != next && _panels.contains(previous);
        if (isAnimated)
        {
            _transition->capture(previous);
        }
        if (next != nullptr)
        {
            _pageStack->setCurrentWidget(next);
        }
        if (isAnimated)
        {
            _transition->play(next, indexOf(next) >= indexOf(previous) ? 1 : -1);
        }
        updateChrome();
        Q_EMIT currentChanged(currentPanel());
    });
    connect(_bar, &ShadTabViewBar::tabMoved, this, [this](int from, int to) { _panels.move(from, to); });
    connect(_bar, &ShadTabViewBar::tabCloseRequested, this, [this](int index)
    {
        if (index >= 0 && index < _panels.size())
        {
            Q_EMIT closeRequested(_panels.at(index));
        }
    });
    connect(_bar, &ShadTabViewBar::tabDragOutRequested, this, [this](int index, const QPoint& grabOffset)
    {
        if (index >= 0 && index < _panels.size())
        {
            Q_EMIT panelDragRequested(_panels.at(index), grabOffset);
        }
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
    updateChrome();
}

ShadDockGroup::~ShadDockGroup()
{
    const QList<ShadDockPanel*> remaining = _panels;
    _panels.clear();
    for (ShadDockPanel* panel : remaining)
    {
        delete panel;
    }
}

ShadDockGroup::Kind ShadDockGroup::kind() const
{
    return _kind;
}

void ShadDockGroup::setCentral(bool central)
{
    _isCentral = central;
}

bool ShadDockGroup::isCentral() const
{
    return _isCentral;
}

int ShadDockGroup::insertPanel(int index, ShadDockPanel* panel)
{
    if (panel == nullptr || _panels.contains(panel))
    {
        return -1;
    }
    const int position = index < 0 || index > count() ? count() : index;
    // 先记录面板，添加标签时会切换页面
    _pageStack->addWidget(panel);
    _panels.insert(position, panel);
    _bar->insertTab(position, panel->getIcon(), panel->getTitle());
    _bar->setTabToolTip(position, panel->getTitleToolTip());
    _bar->setTabFlags(position, tabFlagsOf(panel));
    updateChrome();
    return position;
}

void ShadDockGroup::removePanel(ShadDockPanel* panel)
{
    const int index = indexOf(panel);
    if (index < 0)
    {
        return;
    }
    _transition->finish();
    ShadStyleHelper::clearFocusWithin(panel);
    _panels.removeAt(index);
    _pageStack->removeWidget(panel);
    panel->hide();
    panel->setParent(nullptr);
    _bar->removeTab(index);
    updateChrome();
}

bool ShadDockGroup::forgetPanel(const QObject* panel)
{
    for (int index = 0; index < _panels.size(); ++index)
    {
        if (static_cast<const QObject*>(_panels.at(index)) == panel)
        {
            _transition->finish();
            _pageStack->removeWidget(_panels.at(index));
            _panels.removeAt(index);
            _bar->removeTab(index);
            updateChrome();
            return true;
        }
    }
    return false;
}

int ShadDockGroup::count() const
{
    return int(_panels.size());
}

ShadDockPanel* ShadDockGroup::panel(int index) const
{
    return index >= 0 && index < _panels.size() ? _panels.at(index) : nullptr;
}

int ShadDockGroup::indexOf(const ShadDockPanel* panel) const
{
    for (int index = 0; index < _panels.size(); ++index)
    {
        if (_panels.at(index) == panel)
        {
            return index;
        }
    }
    return -1;
}

QList<ShadDockPanel*> ShadDockGroup::panels() const
{
    return _panels;
}

void ShadDockGroup::setCurrentPanel(ShadDockPanel* panel)
{
    const int index = indexOf(panel);
    if (index >= 0)
    {
        _bar->setCurrentIndex(index);
    }
}

ShadDockPanel* ShadDockGroup::currentPanel() const
{
    return panel(_bar->currentIndex());
}

void ShadDockGroup::updatePanel(ShadDockPanel* panel)
{
    const int index = indexOf(panel);
    if (index < 0)
    {
        return;
    }
    _bar->setTabText(index, panel->getTitle());
    _bar->setTabIcon(index, panel->getIcon());
    _bar->setTabToolTip(index, panel->getTitleToolTip());
    _bar->setTabFlags(index, tabFlagsOf(panel));
    updateChrome();
}

void ShadDockGroup::setActive(bool active)
{
    if (_header != nullptr)
    {
        _header->setActive(active);
    }
}

QPoint ShadDockGroup::dragAnchor(int index, const QPoint& grabOffset) const
{
    if (!_bar->isHidden())
    {
        return _bar->mapTo(this, _bar->restingTabRect(index).topLeft() + grabOffset);
    }
    if (_header != nullptr)
    {
        return headerAnchor(QPoint(grabOffset.x(), _header->height() / 2));
    }
    return grabOffset;
}

QPoint ShadDockGroup::headerAnchor(const QPoint& grabPos) const
{
    if (_header == nullptr)
    {
        return grabPos;
    }
    const QPoint bounded(qBound(0, grabPos.x(), qMax(0, _header->width() - 1)), qBound(0, grabPos.y(), qMax(0, _header->height() - 1)));
    return _header->mapTo(this, bounded);
}

QRect ShadDockGroup::dropStripRect() const
{
    if (!_bar->isHidden())
    {
        return _bar->geometry();
    }
    return _header != nullptr ? _header->geometry() : QRect();
}

int ShadDockGroup::insertionIndexAt(const QPoint& pos) const
{
    return _bar->isHidden() ? count() : _bar->insertionSlotAt(_bar->mapFrom(this, pos));
}

void ShadDockGroup::setDropIndicator(int index)
{
    _bar->setDropIndicator(_bar->isHidden() ? -1 : index);
}

QSize ShadDockGroup::sizeHint() const
{
    return _kind == Kind::Document ? QSize(kDocumentWidth, kDocumentHeight) : QSize(kToolWidth, kToolHeight);
}

bool ShadDockGroup::eventFilter(QObject* watched, QEvent* event)
{
    if ((watched == _bar || watched == _header) && event->type() == QEvent::MouseButtonPress)
    {
        Q_EMIT pressed();
    }
    return QWidget::eventFilter(watched, event);
}

void ShadDockGroup::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), shadTheme->getColor(ShadThemeType::Background));
}

void ShadDockGroup::updateChrome()
{
    const bool hasTabs = _kind == Kind::Document ? count() > 0 : count() > 1;
    _bar->setVisible(hasTabs);
    if (_kind == Kind::Document)
    {
        _topSeparator->setVisible(hasTabs);
        return;
    }
    _bottomSeparator->setVisible(hasTabs);
    const ShadDockPanel* current = currentPanel();
    _header->setTitle(current != nullptr ? current->getIcon() : ShadIconType::None, current != nullptr ? current->getTitle() : QString());
    _header->setCloseVisible(current != nullptr && !current->getPanelFlags().testFlag(ShadDockType::PanelNotClosable));
    bool isMovable = count() > 0;
    for (const ShadDockPanel* panel : _panels)
    {
        isMovable = isMovable && !panel->getPanelFlags().testFlag(ShadDockType::PanelNotMovable);
    }
    _header->setDragEnabled(isMovable);
}

ShadTabViewType::TabFlags ShadDockGroup::tabFlagsOf(const ShadDockPanel* panel)
{
    ShadTabViewType::TabFlags flags = ShadTabViewType::NoTabFlag;
    flags.setFlag(ShadTabViewType::TabNotClosable, panel->getPanelFlags().testFlag(ShadDockType::PanelNotClosable));
    flags.setFlag(ShadTabViewType::TabNotMovable, panel->getPanelFlags().testFlag(ShadDockType::PanelNotMovable));
    return flags;
}
