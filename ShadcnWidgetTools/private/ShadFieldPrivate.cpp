#include "ShadFieldPrivate.h"

#include <QAbstractButton>
#include <QBoxLayout>
#include <QLayoutItem>
#include <QList>
#include <QMetaObject>
#include <QString>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <initializer_list>

#include "ShadField.h"
#include "ShadLabel.h"
#include "ShadStyleHelper.h"

ShadFieldPrivate::ShadFieldPrivate()
{
}

ShadFieldPrivate::~ShadFieldPrivate()
{
}

bool ShadFieldPrivate::isHorizontal() const
{
    Q_Q(const ShadField);
    switch (_orientation)
    {
        case ShadFieldType::Horizontal:
            return true;
        case ShadFieldType::Responsive:
            return q->width() >= kResponsiveBreakpoint;
        case ShadFieldType::Vertical:
            break;
    }
    return false;
}

void ShadFieldPrivate::rebuildLayout()
{
    // 删除布局项不会删除其中的组件
    while (QLayoutItem* item = _layout->takeAt(0))
    {
        delete item;
    }
    while (QLayoutItem* item = _contentLayout->takeAt(0))
    {
        delete item;
    }
    const bool isHorizontalLayout = isHorizontal();
    _isLaidOutHorizontal = isHorizontalLayout;
    _layout->setDirection(isHorizontalLayout ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
    const bool hasLabel = !_label->text().isEmpty();
    const bool hasDescription = !_description->text().isEmpty();
    const bool hasError = !_errors.isEmpty();
    QWidget* control = _control.data();
    const int ring = ShadStyleHelper::kFocusRingWidth;

    if (!isHorizontalLayout)
    {
        // 竖排：文字与控件的可视边缘对齐
        const int textInset = control != nullptr ? ring : 0;
        QList<QWidget*> items;
        for (ShadLabel* label : {_label, _description, _error})
        {
            label->setContentsMargins(textInset, 0, textInset, 0);
        }
        if (hasLabel)
        {
            items.append(_label);
        }
        if (control != nullptr)
        {
            items.append(control);
        }
        if (hasDescription)
        {
            items.append(_description);
        }
        if (hasError)
        {
            items.append(_error);
        }
        QWidget* previous = nullptr;
        for (QWidget* item : items)
        {
            if (previous != nullptr)
            {
                // gap-3 扣除控件上下的焦点环边距
                _layout->addSpacing(kGap - (previous == control ? ring : 0) - (item == control ? ring : 0));
            }
            _layout->addWidget(item);
            previous = item;
        }
        _content->setVisible(false);
    }
    else
    {
        // 横排：标签、说明与错误为一列（FieldContent，gap-1），与控件并排（gap-3）
        for (ShadLabel* label : {_label, _description, _error})
        {
            label->setContentsMargins(0, 0, 0, 0);
        }
        QList<QWidget*> contentItems;
        if (hasLabel)
        {
            contentItems.append(_label);
        }
        if (hasDescription)
        {
            contentItems.append(_description);
        }
        if (hasError)
        {
            contentItems.append(_error);
        }
        for (int index = 0; index < contentItems.size(); ++index)
        {
            if (index > 0)
            {
                _contentLayout->addSpacing(kContentGap);
            }
            _contentLayout->addWidget(contentItems.at(index));
        }
        // 有说明或错误时顶部对齐（has-[>[data-slot=field-content]]:items-start），文字顶端与控件的可视边缘对齐
        const bool isTopAligned = hasDescription || hasError;
        const Qt::Alignment alignment = isTopAligned ? Qt::AlignTop : Qt::AlignVCenter;
        _contentLayout->setContentsMargins(0, isTopAligned && control != nullptr ? ring : 0, 0, 0);
        const bool hasContent = !contentItems.isEmpty();
        const int gap = kGap - (control != nullptr ? ring : 0);
        if (control != nullptr && _controlPosition == ShadFieldType::Start)
        {
            _layout->addWidget(control, 0, alignment);
            if (hasContent)
            {
                _layout->addSpacing(gap);
            }
        }
        if (hasContent)
        {
            // 对应 *:data-[slot=field-label]:flex-auto：文字一列占据剩余宽度
            _layout->addWidget(_content, 1, alignment);
        }
        if (control != nullptr && _controlPosition == ShadFieldType::End)
        {
            if (hasContent)
            {
                _layout->addSpacing(gap);
            }
            _layout->addWidget(control, 0, alignment);
        }
        _content->setVisible(hasContent);
    }
    _label->setVisible(hasLabel);
    _description->setVisible(hasDescription);
    _error->setVisible(hasError);
    if (control != nullptr)
    {
        control->setVisible(true);
    }
}

void ShadFieldPrivate::updateErrorText()
{
    // 重复的消息只保留第一次出现的位置
    QStringList messages;
    for (const QString& error : _errors)
    {
        if (!error.isEmpty() && !messages.contains(error))
        {
            messages.append(error);
        }
    }
    _errors = messages;
    if (messages.size() == 1)
    {
        _error->setText(messages.first());
        return;
    }
    // 多条时显示为列表，对应 ul list-disc gap-1
    QStringList lines;
    for (const QString& message : messages)
    {
        lines.append(QStringLiteral("• ") + message);
    }
    _error->setText(lines.join(QLatin1Char('\n')));
}

void ShadFieldPrivate::applyInvalid()
{
    // 对应 data-[invalid=true]:text-destructive；说明文字保持 muted-foreground
    _label->setColorRole(_isInvalid ? ShadThemeType::Destructive : ShadThemeType::Foreground);
    QWidget* control = _control.data();
    if (control != nullptr && control->metaObject()->indexOfProperty("invalid") >= 0)
    {
        control->setProperty("invalid", _isInvalid);
    }
}

void ShadFieldPrivate::applyMargins()
{
    const int margin = _isChoiceCard ? ShadStyleHelper::kFocusRingWidth + kCardBorder + kCardPadding : 0;
    _layout->setContentsMargins(margin, margin, margin, margin);
}

bool ShadFieldPrivate::isControlChecked() const
{
    const QAbstractButton* button = qobject_cast<const QAbstractButton*>(_control.data());
    return button != nullptr && button->isChecked();
}
