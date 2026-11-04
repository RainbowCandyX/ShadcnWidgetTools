#include "ShadSidebarTrigger.h"

#include <QEvent>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QtCore/qnamespace.h>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadSidebar.h"
#include "ShadStyleHelper.h"
#include "ShadTooltip.h"

namespace
{
    // 悬停多久后显示提示（毫秒）
    constexpr int kToolTipDelay = 500;

    // 无障碍名称与提示使用默认文字，使用者改过的保持不变
    void applyDefaultTexts(ShadSidebarTrigger* trigger)
    {
        const QString text = ShadSidebarTrigger::tr("切换侧栏");
        ShadStyleHelper::applyDefaultText(trigger, "accessibleName", trigger->accessibleName(), text, [trigger](const QString& value) { trigger->setAccessibleName(value); });
        ShadStyleHelper::applyDefaultText(trigger, "toolTip", trigger->toolTip(), text, [trigger](const QString& value) { trigger->setToolTip(value); });
    }
} // namespace

// ShadButton 已占用 d_ptr，切换按钮自己的数据单独存放
class ShadSidebarTriggerPrivate
{
    public:
        QPointer<ShadSidebar> _sidebar;
        QMetaObject::Connection _sideConnection;
};

ShadSidebarTrigger::ShadSidebarTrigger(QWidget* parent)
    : ShadButton(parent), _private(new ShadSidebarTriggerPrivate())
{
    setVariant(ShadButtonType::Ghost);
    setButtonSize(ShadButtonType::IconSmall);
    applyDefaultTexts(this);
    // 与侧栏切换条一致，悬停停留一段时间后才显示提示
    ShadTooltip::setShowDelay(this, kToolTipDelay);
    connect(this, &ShadSidebarTrigger::clicked, this, [this]()
    {
        if (!_private->_sidebar.isNull())
        {
            _private->_sidebar->toggle();
        }
    });
    updateIcon();
}

ShadSidebarTrigger::ShadSidebarTrigger(ShadSidebar* sidebar, QWidget* parent)
    : ShadSidebarTrigger(parent)
{
    setSidebar(sidebar);
}

ShadSidebarTrigger::~ShadSidebarTrigger()
{
    QObject::disconnect(_private->_sideConnection);
}

void ShadSidebarTrigger::setSidebar(ShadSidebar* sidebar)
{
    QObject::disconnect(_private->_sideConnection);
    _private->_sidebar = sidebar;
    if (sidebar != nullptr)
    {
        _private->_sideConnection = connect(sidebar, &ShadSidebar::sideChanged, this, [this]() { updateIcon(); });
    }
    updateIcon();
}

ShadSidebar* ShadSidebarTrigger::getSidebar() const
{
    return _private->_sidebar.data();
}

bool ShadSidebarTrigger::event(QEvent* event)
{
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        updateIcon();
    }
    else if (event->type() == QEvent::LanguageChange)
    {
        applyDefaultTexts(this);
    }
    return ShadButton::event(event);
}

void ShadSidebarTrigger::updateIcon()
{
    // 侧栏实际显示在右侧时图标朝右（对应 RTL 下的水平翻转）
    const bool isSidebarAtEnd = !_private->_sidebar.isNull() && _private->_sidebar->getSide() == ShadSidebarType::End;
    const bool isRightToLeft = layoutDirection() == Qt::RightToLeft;
    setShadIcon(isSidebarAtEnd != isRightToLeft ? ShadIconType::PanelRight : ShadIconType::PanelLeft);
}
