#include "ExampleBasePage.h"

#include <QFont>
#include <QHBoxLayout>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadFlowLayout.h"
#include "ShadLabel.h"
#include "ShadTheme.h"

ExampleBasePage::ExampleBasePage(ExampleI18n::TextFunction title, ExampleI18n::TextFunction description, QWidget* parent)
    : ShadScrollArea(parent)
{
    QWidget* contentWidget = new QWidget(this);
    _pageLayout = new QVBoxLayout(contentWidget);
    _pageLayout->setContentsMargins(32, 28, 32, 32);
    _pageLayout->setSpacing(16);

    ShadLabel* titleLabel = new ShadLabel(contentWidget);
    QFont titleFont = titleLabel->font();
    titleFont.setPixelSize(30);
    titleFont.setWeight(QFont::Bold);
    titleLabel->setFont(titleFont);
    _descriptionLabel = new ShadLabel(contentWidget);
    ExampleI18n::bind(this, [this, titleLabel, title, description]()
    {
        const QString newTitle = title();
        titleLabel->setText(newTitle);
        _descriptionLabel->setText(description());
        if (newTitle != _title)
        {
            _title = newTitle;
            Q_EMIT titleChanged(newTitle);
        }
    });
    _descriptionLabel->setColorRole(ShadThemeType::MutedForeground);
    QFont descriptionFont = _descriptionLabel->font();
    descriptionFont.setPixelSize(16);
    descriptionFont.setWeight(QFont::Normal);
    _descriptionLabel->setFont(descriptionFont);
    _descriptionLabel->setWordWrap(true);

    _pageLayout->addWidget(titleLabel);
    _pageLayout->addWidget(_descriptionLabel);
    _pageLayout->addSpacing(8);
    _pageLayout->addStretch(1);
    setWidget(contentWidget);
}

ExampleBasePage::~ExampleBasePage()
{
}

QString ExampleBasePage::getTitle() const
{
    return _title;
}

ShadCard* ExampleBasePage::addSection(ExampleI18n::TextFunction title, ExampleI18n::TextFunction description)
{
    ShadCard* card = new ShadCard(widget());
    ExampleI18n::bind(card, [card, title, description]()
    {
        card->setTitle(title());
        card->setDescription(description());
    });
    card->contentLayout()->setSpacing(12);
    // 末尾的 stretch 保持在最后
    _pageLayout->insertWidget(_pageLayout->count() - 1, card);
    return card;
}

void ExampleBasePage::addRow(ShadCard* card, const QList<QWidget*>& widgets)
{
    // 流式布局：窗口变窄时自动换行，同一行内垂直居中
    ShadFlowLayout* rowLayout = new ShadFlowLayout();
    for (QWidget* item : widgets)
    {
        rowLayout->addWidget(item);
    }
    card->contentLayout()->addLayout(rowLayout);
}

void ExampleBasePage::addItem(ShadCard* card, QWidget* widget)
{
    // 未指定对齐时 QBoxLayout 会把固定宽度的组件水平居中
    card->contentLayout()->addWidget(widget, 0, Qt::AlignLeft);
}

void ExampleBasePage::addStretchItem(ShadCard* card, QWidget* widget)
{
    // 不设置对齐，组件按卡片内容区的宽度拉伸；设置对齐后布局只按 sizeHint 分配宽度
    card->contentLayout()->addWidget(widget);
}

void ExampleBasePage::addDescriptionWidget(QWidget* widget)
{
    if (_descriptionLayout == nullptr)
    {
        const int index = _pageLayout->indexOf(_descriptionLabel);
        _pageLayout->removeWidget(_descriptionLabel);
        // 换行的 QLabel 在横排布局中会被估窄，改为单行
        _descriptionLabel->setWordWrap(false);
        _descriptionLayout = new QHBoxLayout();
        _descriptionLayout->setSpacing(8);
        _descriptionLayout->addWidget(_descriptionLabel);
        _descriptionLayout->addStretch(1);
        _pageLayout->insertLayout(index, _descriptionLayout);
    }
    _descriptionLayout->insertWidget(_descriptionLayout->count() - 1, widget, 0, Qt::AlignVCenter);
}
