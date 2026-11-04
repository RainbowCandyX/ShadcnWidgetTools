#include "ShadTimePanel.h"

#include <QColor>
#include <QEvent>
#include <QHBoxLayout>
#include <QList>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QVBoxLayout>
#include <QtCore/qnamespace.h>
#include <QtGlobal>
#include <initializer_list>

#include "ShadButton.h"
#include "ShadDef.h"
#include "ShadTheme.h"
#include "ShadTimeColumn.h"

namespace
{
    constexpr int kHourCount = 24;
    constexpr int kMinuteCount = 60;
    constexpr int kSecondCount = 60;
    // 列区上下 p-1，页脚 p-2
    constexpr int kColumnsPaddingY = 4;
    constexpr int kFooterPadding = 8;
    // 列之间的分隔线宽度
    constexpr int kSeparatorWidth = 1;
} // namespace

ShadTimePanel::ShadTimePanel(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 列区固定从左到右：时 分 秒
    _columnsArea = new QWidget(this);
    _columnsArea->setLayoutDirection(Qt::LeftToRight);
    QHBoxLayout* columnsLayout = new QHBoxLayout(_columnsArea);
    columnsLayout->setContentsMargins(0, kColumnsPaddingY, 0, kColumnsPaddingY);
    columnsLayout->setSpacing(kSeparatorWidth);
    _hourColumn = new ShadTimeColumn(kHourCount, _columnsArea);
    _minuteColumn = new ShadTimeColumn(kMinuteCount, _columnsArea);
    _secondColumn = new ShadTimeColumn(kSecondCount, _columnsArea);
    for (ShadTimeColumn* column : {_hourColumn, _minuteColumn, _secondColumn})
    {
        columnsLayout->addWidget(column);
        connect(column, &ShadTimeColumn::valueSelected, this, [this]() { onValueSelected(); });
        connect(column, &ShadTimeColumn::moveFocusRequested, this, [this, column](int direction) { moveFocus(column, direction); });
        connect(column, &ShadTimeColumn::confirmRequested, this, [this]() { confirm(); });
    }
    mainLayout->addWidget(_columnsArea);

    // 页脚：顶部分隔线，结束侧为确定按钮
    _footer = new QWidget(this);
    QHBoxLayout* footerLayout = new QHBoxLayout(_footer);
    footerLayout->setContentsMargins(kFooterPadding, kFooterPadding, kFooterPadding, kFooterPadding);
    footerLayout->addStretch(1);
    _confirmButton = new ShadButton(QString(), _footer);
    retranslate();
    _confirmButton->setButtonSize(ShadButtonType::Small);
    _confirmButton->setAutoDefault(false);
    footerLayout->addWidget(_confirmButton);
    connect(_confirmButton, &ShadButton::clicked, this, [this]() { confirm(); });
    mainLayout->addWidget(_footer);

    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
    updateConfirmButton();
}

ShadTimePanel::~ShadTimePanel()
{
}

void ShadTimePanel::setSecondsVisible(bool visible)
{
    _secondColumn->setVisible(visible);
    updateConfirmButton();
    update();
}

bool ShadTimePanel::isSecondsVisible() const
{
    return !_secondColumn->isHidden();
}

void ShadTimePanel::setTime(const QTime& time)
{
    const bool isValid = time.isValid();
    _hourColumn->setSelectedValue(isValid ? time.hour() : -1);
    _minuteColumn->setSelectedValue(isValid ? time.minute() : -1);
    _secondColumn->setSelectedValue(isValid ? time.second() : -1);
    updateConfirmButton();
}

QTime ShadTimePanel::time() const
{
    const int hour = _hourColumn->selectedValue();
    const int minute = _minuteColumn->selectedValue();
    const int second = isSecondsVisible() ? _secondColumn->selectedValue() : 0;
    if (hour < 0 || minute < 0 || second < 0)
    {
        return QTime();
    }
    return QTime(hour, minute, second);
}

void ShadTimePanel::scrollToSelection()
{
    for (ShadTimeColumn* column : {_hourColumn, _minuteColumn, _secondColumn})
    {
        column->scrollToValue(qMax(0, column->selectedValue()), false);
    }
}

void ShadTimePanel::focusFirstColumn()
{
    _hourColumn->setFocus(Qt::PopupFocusReason);
}

void ShadTimePanel::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        retranslate();
    }
    QWidget::changeEvent(event);
}

void ShadTimePanel::retranslate()
{
    _hourColumn->setAccessibleName(tr("时"));
    _minuteColumn->setAccessibleName(tr("分"));
    _secondColumn->setAccessibleName(tr("秒"));
    _confirmButton->setText(tr("确定"));
}

void ShadTimePanel::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    const QColor borderColor = shadTheme->getColor(ShadThemeType::Border);
    // 列之间的分隔线，位于布局留出的 1px 间隙中
    const QList<ShadTimeColumn*> columns = visibleColumns();
    const QRect areaRect = _columnsArea->geometry();
    for (int index = 0; index + 1 < columns.size(); ++index)
    {
        const QPoint right = _columnsArea->mapTo(this, QPoint(columns.at(index)->geometry().right() + 1, 0));
        painter.fillRect(QRect(right.x(), areaRect.top(), kSeparatorWidth, areaRect.height()), borderColor);
    }
    // 页脚顶部的分隔线
    painter.fillRect(QRect(0, _footer->y(), width(), kSeparatorWidth), borderColor);
}

QList<ShadTimeColumn*> ShadTimePanel::visibleColumns() const
{
    QList<ShadTimeColumn*> columns{_hourColumn, _minuteColumn};
    if (isSecondsVisible())
    {
        columns.append(_secondColumn);
    }
    return columns;
}

void ShadTimePanel::onValueSelected()
{
    // 选择任一列后，尚未选择的列自动选择 00，使确定按钮可用
    for (ShadTimeColumn* column : visibleColumns())
    {
        if (column->selectedValue() < 0)
        {
            column->setSelectedValue(0);
        }
    }
    updateConfirmButton();
}

void ShadTimePanel::moveFocus(ShadTimeColumn* column, int direction)
{
    const QList<ShadTimeColumn*> columns = visibleColumns();
    const int index = columns.indexOf(column);
    const int target = qBound(0, index + direction, static_cast<int>(columns.size()) - 1);
    if (target != index)
    {
        columns.at(target)->setFocus(Qt::TabFocusReason);
    }
}

void ShadTimePanel::confirm()
{
    const QTime value = time();
    if (value.isValid())
    {
        Q_EMIT confirmed(value);
    }
}

void ShadTimePanel::updateConfirmButton()
{
    _confirmButton->setEnabled(time().isValid());
}
