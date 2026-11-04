#include "NotificationPage.h"

#include <QHBoxLayout>
#include <QList>
#include <QObject>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadLabel.h"
#include "ShadNotification.h"
#include "ShadSlider.h"
#include "ShadToggle.h"
#include "ShadToggleGroup.h"

NotificationPage::NotificationPage(QWidget* parent)
    : ExampleBasePage([] { return tr("消息通知"); }, [] { return tr("ShadNotification：从窗口边缘弹出的消息条，带倒计时，到时自动关闭。"); }, parent)
{
    ShadCard* typeCard = addSection([] { return tr("类型"); }, [] { return tr("不同类型显示不同的状态图标与倒计时颜色"); });
    struct TypeDemo
    {
        ExampleI18n::TextFunction buttonText;
        ExampleI18n::TextFunction title;
        ExampleI18n::TextFunction text;
        ShadNotificationType::Type type;
    };
    const QList<TypeDemo> typeDemos{
        {[] { return tr("信息"); }, [] { return tr("新版本"); }, [] { return tr("版本 0.3 已发布，可以随时更新。"); }, ShadNotificationType::Info},
        {[] { return tr("成功"); }, [] { return tr("保存成功"); }, [] { return tr("所有更改已同步到云端。"); }, ShadNotificationType::Success},
        {[] { return tr("警告"); }, [] { return tr("空间不足"); }, [] { return tr("剩余空间低于 10%，请及时清理。"); }, ShadNotificationType::Warning},
        {[] { return tr("错误"); }, [] { return tr("上传失败"); }, [] { return tr("网络连接已断开，请检查后重试。"); }, ShadNotificationType::Error},
    };
    QList<QWidget*> typeButtons;
    for (const TypeDemo& demo : typeDemos)
    {
        ShadButton* button = new ShadButton(QString(), ShadButtonType::Outline, this);
        const ExampleI18n::TextFunction buttonText = demo.buttonText;
        ExampleI18n::bind(button, [button, buttonText]() { button->setText(buttonText()); });
        connect(button, &ShadButton::clicked, this, [this, demo]() { notify(demo.type, demo.title(), demo.text(), _duration); });
        typeButtons.append(button);
    }
    addRow(typeCard, typeButtons);

    ShadCard* positionCard = addSection([] { return tr("位置"); }, [] { return tr("同一位置的消息条按顺序堆叠，关闭后其余的自动补位"); });
    ShadToggleGroup* positionGroup = new ShadToggleGroup(ShadToggleGroupType::Single, this);
    positionGroup->setVariant(ShadToggleType::Outline);
    const QList<ExampleI18n::TextFunction> positionNames{[] { return tr("上中"); }, [] { return tr("下中"); }, [] { return tr("左侧"); }, [] { return tr("右侧"); }, [] { return tr("左上"); }, [] { return tr("右上"); }, [] { return tr("左下"); }, [] { return tr("右下"); }};
    for (int index = 0; index < positionNames.size(); ++index)
    {
        positionGroup->addItem(QString());
    }
    ExampleI18n::bind(positionGroup, [positionGroup, positionNames]()
    {
        for (int index = 0; index < positionNames.size(); ++index)
        {
            positionGroup->itemAt(index)->setText(positionNames.at(index)());
        }
    });
    positionGroup->setCurrentIndex(static_cast<int>(_position));
    connect(positionGroup, &ShadToggleGroup::currentIndexChanged, this, [this, positionGroup, positionNames](int index)
    {
        if (index < 0)
        {
            positionGroup->setCurrentIndex(static_cast<int>(_position));
            return;
        }
        _position = static_cast<ShadNotificationType::Position>(index);
        notify(ShadNotificationType::Info, tr("位置已切换"), tr("消息条现在显示在%1").arg(positionNames.at(index)()), _duration);
    });
    addItem(positionCard, positionGroup);

    ShadCard* durationCard = addSection([] { return tr("显示时长"); }, [] { return tr("倒计时结束后自动关闭"); });
    ShadSlider* durationSlider = new ShadSlider(Qt::Horizontal, this);
    durationSlider->setRange(1, 10);
    durationSlider->setValue(_duration / 1000);
    durationSlider->setFixedWidth(240);
    ShadLabel* durationLabel = new ShadLabel(this);
    durationLabel->setColorRole(ShadThemeType::MutedForeground);
    const auto updateDurationLabel = [this, durationLabel]() { durationLabel->setText(tr("%1 秒").arg(_duration / 1000)); };
    connect(durationSlider, &ShadSlider::valueChanged, this, [this, updateDurationLabel](int value)
    {
        _duration = value * 1000;
        updateDurationLabel();
    });
    ExampleI18n::bind(durationLabel, updateDurationLabel);
    addRow(durationCard, {durationSlider, durationLabel});

    ShadCard* stackCard = addSection([] { return tr("堆叠与关闭"); }, [] { return tr("超出窗口高度时先关闭最早的一条"); });
    ShadButton* longTextButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(longTextButton, &ShadButton::clicked, this, [this]() { notify(ShadNotificationType::Info, tr("更新说明"), tr("本次更新新增了消息通知组件，支持八个弹出位置、四种类型和倒计时进度条，文字较长时会自动换行，消息条的高度随之增加。"), _duration); });
    ShadButton* burstButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(burstButton, &ShadButton::clicked, this, [this]()
    {
        for (int i = 1; i <= 5; ++i)
        {
            notify(ShadNotificationType::Success, tr("第 %1 条").arg(i), tr("连续弹出的消息条依次排列。"), _duration + i * 500);
        }
    });
    ShadButton* persistentButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(persistentButton, &ShadButton::clicked, this, [this]() { notify(ShadNotificationType::Warning, tr("需要手动关闭"), tr("这条消息不会自动消失，点击右侧的关闭按钮。"), 0); });
    ShadButton* closeAllButton = new ShadButton(QString(), ShadButtonType::Secondary, this);
    connect(closeAllButton, &ShadButton::clicked, this, [this]() { ShadNotification::closeAll(window()); });
    addRow(stackCard, {longTextButton, burstButton, persistentButton, closeAllButton});

    ShadCard* windowCard = addSection([] { return tr("独立窗口"); }, [] { return tr("通过 parent 参数指定在哪个窗口弹出"); });
    ShadButton* windowButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(windowButton, &ShadButton::clicked, this, [this]() { ExampleLayout::presentTransientWindow(_childWindow, [this]() { return createChildWindow(); }); });
    addItem(windowCard, windowButton);

    ExampleI18n::bind(this, [longTextButton, burstButton, persistentButton, closeAllButton, windowButton]()
    {
        longTextButton->setText(tr("长文本"));
        burstButton->setText(tr("连续弹出 5 条"));
        persistentButton->setText(tr("不自动关闭"));
        closeAllButton->setText(tr("全部关闭"));
        windowButton->setText(tr("打开独立窗口"));
    });
}

NotificationPage::~NotificationPage()
{
    delete _childWindow.data();
}

void NotificationPage::notify(ShadNotificationType::Type type, const QString& title, const QString& text, int duration, QWidget* host)
{
    QWidget* target = host != nullptr ? host : window();
    switch (type)
    {
        case ShadNotificationType::Info:
            ShadNotification::info(title, text, duration, _position, target);
            break;
        case ShadNotificationType::Success:
            ShadNotification::success(title, text, duration, _position, target);
            break;
        case ShadNotificationType::Warning:
            ShadNotification::warning(title, text, duration, _position, target);
            break;
        case ShadNotificationType::Error:
            ShadNotification::error(title, text, duration, _position, target);
            break;
    }
}

ShadWidget* NotificationPage::createChildWindow()
{
    ShadWidget* childWindow = new ShadWidget();
    childWindow->resize(480, 320);
    ExampleLayout::addStayOnTopButton(childWindow);
    childWindow->setAttribute(Qt::WA_QuitOnClose, false);
    QVBoxLayout* layout = new QVBoxLayout(childWindow);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);
    ShadLabel* descriptionLabel = new ShadLabel(childWindow);
    descriptionLabel->setColorRole(ShadThemeType::MutedForeground);
    descriptionLabel->setWordWrap(true);
    layout->addWidget(descriptionLabel);
    layout->addStretch(1);
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(8 - 2 * ExampleLayout::kFocusRingWidth);
    ShadButton* localButton = new ShadButton(childWindow);
    ShadButton* mainButton = new ShadButton(QString(), ShadButtonType::Outline, childWindow);
    ShadButton* closeAllButton = new ShadButton(QString(), ShadButtonType::Secondary, childWindow);
    QObject::connect(localButton, &ShadButton::clicked, childWindow, [this, childWindow]() { notify(ShadNotificationType::Success, tr("独立窗口中的消息"), tr("显示在当前窗口。"), _duration, childWindow); });
    QObject::connect(mainButton, &ShadButton::clicked, childWindow, [this]() { notify(ShadNotificationType::Info, tr("来自独立窗口的消息"), tr("显示在主窗口。"), _duration); });
    QObject::connect(closeAllButton, &ShadButton::clicked, childWindow, [childWindow]() { ShadNotification::closeAll(childWindow); });
    buttonLayout->addWidget(localButton);
    buttonLayout->addWidget(mainButton);
    buttonLayout->addWidget(closeAllButton);
    buttonLayout->addStretch(1);
    layout->addLayout(buttonLayout);
    ExampleI18n::bind(childWindow, [childWindow, descriptionLabel, localButton, mainButton, closeAllButton]()
    {
        childWindow->setWindowTitle(tr("独立窗口消息"));
        descriptionLabel->setText(tr("把窗口作为最后一个参数传入，消息条就会在该窗口内弹出，位置和时长沿用页面上的设置。"));
        localButton->setText(tr("在此窗口显示"));
        mainButton->setText(tr("在主窗口显示"));
        closeAllButton->setText(tr("全部关闭"));
    });
    return childWindow;
}
