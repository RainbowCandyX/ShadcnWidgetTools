#include "ShadExpanderItem.h"

#include <QHBoxLayout>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QSizePolicy>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadTheme.h"

namespace
{
    // px-4 py-3
    constexpr int kPaddingX = 16;
    constexpr int kPaddingY = 12;
    constexpr int kSeparatorWidth = 1;
} // namespace

ShadExpanderItem::ShadExpanderItem(QWidget* content, QWidget* parent)
    : QWidget(parent), _content(content)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(kPaddingX, kSeparatorWidth + kPaddingY, kPaddingX, kPaddingY);
    // 不能横向拉伸的组件靠起始侧
    const bool canGrow = (content->sizePolicy().horizontalPolicy() & QSizePolicy::GrowFlag) != 0;
    layout->addWidget(content, 0, canGrow ? Qt::Alignment() : (Qt::AlignLeading | Qt::AlignVCenter));
}

ShadExpanderItem::~ShadExpanderItem()
{
}

QWidget* ShadExpanderItem::content() const
{
    return _content;
}

QWidget* ShadExpanderItem::takeContent()
{
    QWidget* content = _content;
    if (content != nullptr)
    {
        layout()->removeWidget(content);
        content->hide();
        content->setParent(nullptr);
        _content = nullptr;
    }
    return content;
}

void ShadExpanderItem::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(QRect(0, 0, width(), kSeparatorWidth), shadTheme->getColor(ShadThemeType::Border));
}
