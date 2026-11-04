#include "DrawerPage.h"

#include <QCoreApplication>
#include <QFont>
#include <QHBoxLayout>
#include <QList>
#include <QObject>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <initializer_list>
#include <memory>
#include <utility>

#include "ExampleI18n.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadDrawer.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadScrollArea.h"

namespace
{
    constexpr int kGoalDefault = 350;
    constexpr int kGoalStep = 10;
    constexpr int kGoalMinimum = 200;
    constexpr int kGoalMaximum = 400;
    constexpr int kGoalFontSize = 72;
    constexpr int kGoalValueWidth = 200;
    constexpr int kGoalGap = 8;
    constexpr int kParagraphCount = 12;
    constexpr int kParagraphGap = 16;
    constexpr int kNestedDepth = 3;
    // 吸附点示例的初始位置（百分比）
    constexpr int kInitialSnapPercent = 40;

    // 抽屉的标题与说明随语言刷新，打开期间切换语言同样生效
    void bindDrawerTexts(ShadDrawer* drawer, ExampleI18n::TextFunction title, ExampleI18n::TextFunction description)
    {
        ExampleI18n::bind(drawer, [drawer, title = std::move(title), description = std::move(description)]()
        {
            drawer->setTitle(title());
            drawer->setDescription(description());
        });
    }

    ShadButton* addFooterButton(ShadDrawer* drawer, ExampleI18n::TextFunction text, ShadButtonType::Variant variant)
    {
        ShadButton* button = new ShadButton(QString(), variant, drawer);
        ExampleI18n::bind(button, [button, text = std::move(text)]() { button->setText(text()); });
        drawer->footerLayout()->addWidget(button);
        QObject::connect(button, &ShadButton::clicked, drawer, [drawer]() { drawer->close(); });
        return button;
    }

    ShadButton* createIconButton(ShadIconType::IconName icon, QWidget* parent)
    {
        ShadButton* button = new ShadButton(parent);
        button->setVariant(ShadButtonType::Outline);
        button->setButtonSize(ShadButtonType::Icon);
        button->setShadIcon(icon);
        button->setRoundedFull(true);
        return button;
    }

    ShadLabel* createMutedText(QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        return label;
    }

    QWidget* createParagraphs(int count, QWidget* parent)
    {
        QWidget* content = new QWidget(parent);
        QVBoxLayout* layout = new QVBoxLayout(content);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(kParagraphGap);
        for (int index = 0; index < count; ++index)
        {
            ShadLabel* paragraph = new ShadLabel(content);
            ExampleI18n::bind(paragraph, [paragraph, index]() { paragraph->setText(QCoreApplication::translate("DrawerPage", "第 %1 段示例文字。").arg(index + 1)); });
            paragraph->setWordWrap(true);
            layout->addWidget(paragraph);
        }
        return content;
    }

    ShadDrawer* createGoalDrawer(QWidget* parent)
    {
        ShadDrawer* drawer = new ShadDrawer(parent);
        bindDrawerTexts(drawer, [] { return QCoreApplication::translate("DrawerPage", "每日目标"); }, [] { return QCoreApplication::translate("DrawerPage", "设置每天的活动目标。"); });
        QWidget* goal = new QWidget(drawer);
        QHBoxLayout* layout = new QHBoxLayout(goal);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(kGoalGap);
        ShadButton* minusButton = createIconButton(ShadIconType::Minus, goal);
        ShadButton* plusButton = createIconButton(ShadIconType::Plus, goal);
        QVBoxLayout* valueLayout = new QVBoxLayout();
        valueLayout->setContentsMargins(0, 0, 0, 0);
        valueLayout->setSpacing(0);
        ShadLabel* valueLabel = new ShadLabel(QString::number(kGoalDefault), goal);
        QFont valueFont = valueLabel->font();
        valueFont.setPixelSize(kGoalFontSize);
        valueFont.setWeight(QFont::Bold);
        valueLabel->setFont(valueFont);
        valueLabel->setAlignment(Qt::AlignCenter);
        valueLabel->setFixedWidth(kGoalValueWidth);
        ShadLabel* unitLabel = createMutedText(goal);
        ExampleI18n::bind(unitLabel, [unitLabel]() { unitLabel->setText(QCoreApplication::translate("DrawerPage", "卡路里 / 天")); });
        unitLabel->setAlignment(Qt::AlignCenter);
        valueLayout->addWidget(valueLabel);
        valueLayout->addWidget(unitLabel);
        layout->addStretch(1);
        layout->addWidget(minusButton, 0, Qt::AlignVCenter);
        layout->addLayout(valueLayout);
        layout->addWidget(plusButton, 0, Qt::AlignVCenter);
        layout->addStretch(1);
        drawer->contentLayout()->addWidget(goal);

        const auto adjust = [valueLabel, minusButton, plusButton](int delta)
        {
            const int value = qBound(kGoalMinimum, valueLabel->text().toInt() + delta, kGoalMaximum);
            valueLabel->setText(QString::number(value));
            minusButton->setEnabled(value > kGoalMinimum);
            plusButton->setEnabled(value < kGoalMaximum);
        };
        QObject::connect(minusButton, &ShadButton::clicked, drawer, [adjust]() { adjust(-kGoalStep); });
        QObject::connect(plusButton, &ShadButton::clicked, drawer, [adjust]() { adjust(kGoalStep); });

        addFooterButton(drawer, [] { return QCoreApplication::translate("DrawerPage", "提交"); }, ShadButtonType::Default);
        addFooterButton(drawer, [] { return QCoreApplication::translate("DrawerPage", "取消"); }, ShadButtonType::Outline);
        return drawer;
    }

    ShadDrawer* createDirectionDrawer(ExampleI18n::TextFunction title, ExampleI18n::TextFunction description, ShadDrawerType::SwipeDirection direction, QWidget* parent)
    {
        ShadDrawer* drawer = new ShadDrawer(parent);
        bindDrawerTexts(drawer, std::move(title), std::move(description));
        drawer->setSwipeDirection(direction);
        drawer->contentLayout()->addStretch(1);
        addFooterButton(drawer, [] { return QCoreApplication::translate("DrawerPage", "关闭"); }, ShadButtonType::Outline);
        return drawer;
    }

    ShadDrawer* createScrollDrawer(QWidget* parent)
    {
        ShadDrawer* drawer = new ShadDrawer(parent);
        bindDrawerTexts(drawer, [] { return QCoreApplication::translate("DrawerPage", "可滚动内容"); }, [] { return QCoreApplication::translate("DrawerPage", "内容超出时滚动。"); });
        ShadScrollArea* area = new ShadScrollArea(drawer);
        area->setWidget(createParagraphs(kParagraphCount, area));
        drawer->contentLayout()->addWidget(area, 1);
        addFooterButton(drawer, [] { return QCoreApplication::translate("DrawerPage", "关闭"); }, ShadButtonType::Outline);
        return drawer;
    }

    QList<ShadDrawer*> createNestedDrawers(QWidget* parent)
    {
        QList<ShadDrawer*> drawers;
        for (int level = 1; level <= kNestedDepth; ++level)
        {
            ShadDrawer* drawer = new ShadDrawer(parent);
            bindDrawerTexts(drawer, [level] { return QCoreApplication::translate("DrawerPage", "第 %1 层").arg(level); }, [] { return QCoreApplication::translate("DrawerPage", "下层会缩小后退。"); });
            drawer->setSwipeHandleVisible(true);
            drawers.append(drawer);
        }
        for (int index = 0; index < drawers.size(); ++index)
        {
            ShadDrawer* drawer = drawers.at(index);
            if (index + 1 < drawers.size())
            {
                ShadDrawer* next = drawers.at(index + 1);
                ShadButton* nextButton = new ShadButton(drawer);
                const int nextLevel = index + 2;
                ExampleI18n::bind(nextButton, [nextButton, nextLevel]() { nextButton->setText(QCoreApplication::translate("DrawerPage", "打开第 %1 层").arg(nextLevel)); });
                drawer->footerLayout()->addWidget(nextButton);
                QObject::connect(nextButton, &ShadButton::clicked, next, &ShadDrawer::open);
            }
            addFooterButton(drawer, [] { return QCoreApplication::translate("DrawerPage", "关闭"); }, ShadButtonType::Outline);
        }
        return drawers;
    }

    ShadDrawer* createSnapDrawer(ExampleI18n::TextFunction title, bool isSequential, QWidget* parent)
    {
        ShadDrawer* drawer = new ShadDrawer(parent);
        bindDrawerTexts(drawer, std::move(title), [] { return QCoreApplication::translate("DrawerPage", "上下拖动切换位置。"); });
        drawer->setSwipeHandleVisible(true);
        drawer->setSnapPoints({0.4, 0.7, 1.0});
        drawer->setSequentialSnapEnabled(isSequential);
        drawer->contentLayout()->addWidget(createParagraphs(kParagraphCount, drawer));
        return drawer;
    }
} // namespace

DrawerPage::DrawerPage(QWidget* parent)
    : ExampleBasePage([] { return tr("抽屉"); }, [] { return tr("ShadDrawer：从窗口边缘滑出的面板。"); }, parent)
{
    // 方向示例与其他示例共用的抽屉说明
    const ExampleI18n::TextFunction swipeDescription = [] { return tr("拖动或点击外部关闭。"); };

    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("默认从底部滑出"); });
    ShadDrawer* goalDrawer = createGoalDrawer(this);
    ShadButton* goalButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(goalButton, &ShadButton::clicked, goalDrawer, &ShadDrawer::open);
    addItem(basicCard, goalButton);

    ShadCard* directionCard = addSection([] { return tr("方向"); }, [] { return tr("从四个边缘滑出"); });
    // 按钮文字与抽屉标题分别是完整的可翻译文本
    struct DirectionDemo
    {
        ExampleI18n::TextFunction buttonText;
        ExampleI18n::TextFunction title;
        ShadDrawerType::SwipeDirection direction;
    };
    const QList<DirectionDemo> directions{
        {[] { return tr("顶部"); }, [] { return tr("从顶部滑出"); }, ShadDrawerType::Up},
        {[] { return tr("右侧"); }, [] { return tr("从右侧滑出"); }, ShadDrawerType::Right},
        {[] { return tr("底部"); }, [] { return tr("从底部滑出"); }, ShadDrawerType::Down},
        {[] { return tr("左侧"); }, [] { return tr("从左侧滑出"); }, ShadDrawerType::Left},
    };
    QList<QWidget*> directionButtons;
    for (const DirectionDemo& direction : directions)
    {
        ShadDrawer* drawer = createDirectionDrawer(direction.title, swipeDescription, direction.direction, this);
        ShadButton* button = new ShadButton(QString(), ShadButtonType::Outline, this);
        const ExampleI18n::TextFunction buttonText = direction.buttonText;
        ExampleI18n::bind(button, [button, buttonText]() { button->setText(buttonText()); });
        connect(button, &ShadButton::clicked, drawer, &ShadDrawer::open);
        directionButtons.append(button);
    }
    addRow(directionCard, directionButtons);

    ShadCard* handleCard = addSection([] { return tr("拖动把手"); }, [] { return QStringLiteral("setSwipeHandleVisible"); });
    ShadDrawer* handleDrawer = createDirectionDrawer([] { return tr("拖动把手"); }, swipeDescription, ShadDrawerType::Down, this);
    handleDrawer->setSwipeHandleVisible(true);
    ShadButton* handleButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(handleButton, &ShadButton::clicked, handleDrawer, &ShadDrawer::open);
    addItem(handleCard, handleButton);

    ShadCard* scrollCard = addSection([] { return tr("可滚动内容"); }, [] { return tr("正文放入 ShadScrollArea"); });
    ShadDrawer* scrollDrawer = createScrollDrawer(this);
    ShadButton* scrollButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(scrollButton, &ShadButton::clicked, scrollDrawer, &ShadDrawer::open);
    addItem(scrollCard, scrollButton);

    ShadCard* nestedCard = addSection([] { return tr("嵌套"); }, [] { return tr("抽屉中再打开抽屉"); });
    const QList<ShadDrawer*> nestedDrawers = createNestedDrawers(this);
    ShadButton* nestedButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(nestedButton, &ShadButton::clicked, nestedDrawers.first(), &ShadDrawer::open);
    addItem(nestedCard, nestedButton);

    ShadCard* modelessCard = addSection([] { return tr("非模态"); }, [] { return tr("setModal(false)：无遮罩，背景可操作"); });
    ShadDrawer* modelessDrawer = createDirectionDrawer([] { return tr("非模态"); }, swipeDescription, ShadDrawerType::Right, this);
    modelessDrawer->setModal(false);
    ShadButton* modelessButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(modelessButton, &ShadButton::clicked, modelessDrawer, &ShadDrawer::open);
    ShadButton* counterButton = new ShadButton(QString(), ShadButtonType::Secondary, this);
    // 点击次数保存在共享状态中，切换语言时按当前次数重新生成
    const std::shared_ptr<int> clickCount = std::make_shared<int>(0);
    const auto updateCounterText = [counterButton, clickCount]() { counterButton->setText(tr("点击 %1 次").arg(*clickCount)); };
    connect(counterButton, &ShadButton::clicked, counterButton, [clickCount, updateCounterText]()
    {
        ++*clickCount;
        updateCounterText();
    });
    ExampleI18n::bind(counterButton, updateCounterText);
    addRow(modelessCard, {modelessButton, counterButton});

    ShadCard* snapCard = addSection([] { return tr("吸附点"); }, [] { return tr("setSnapPoints：松手后停在最近的位置"); });
    ShadLabel* snapStatus = createMutedText(this);
    ShadDrawer* snapDrawer = createSnapDrawer([] { return tr("吸附点"); }, false, this);
    ShadDrawer* sequentialDrawer = createSnapDrawer([] { return tr("逐级吸附"); }, true, this);
    // 最近一次吸附的位置，切换语言时按它重新生成状态文字
    const std::shared_ptr<int> snapPercent = std::make_shared<int>(kInitialSnapPercent);
    const auto updateSnapStatus = [snapStatus, snapPercent]() { snapStatus->setText(tr("吸附点：%1%").arg(*snapPercent)); };
    for (ShadDrawer* drawer : {snapDrawer, sequentialDrawer})
    {
        connect(drawer, &ShadDrawer::snapPointChanged, snapStatus, [snapPercent, updateSnapStatus](qreal snapPoint)
        {
            *snapPercent = qRound(snapPoint * 100);
            updateSnapStatus();
        });
    }
    ExampleI18n::bind(snapStatus, updateSnapStatus);
    ShadButton* snapButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(snapButton, &ShadButton::clicked, snapDrawer, &ShadDrawer::open);
    ShadButton* sequentialButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(sequentialButton, &ShadButton::clicked, sequentialDrawer, &ShadDrawer::open);
    addRow(snapCard, {snapButton, sequentialButton, snapStatus});

    ShadCard* dismissCard = addSection([] { return tr("禁止点击外部关闭"); }, [] { return QStringLiteral("setPointerDismissalEnabled(false)"); });
    ShadDrawer* dismissDrawer = createDirectionDrawer([] { return tr("点击外部不关闭"); }, [] { return tr("拖动、Esc 或按钮关闭。"); }, ShadDrawerType::Down, this);
    dismissDrawer->setPointerDismissalEnabled(false);
    ShadButton* dismissButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(dismissButton, &ShadButton::clicked, dismissDrawer, &ShadDrawer::open);
    addItem(dismissCard, dismissButton);

    // 页面上的按钮与页面同寿命
    ExampleI18n::bind(this, [goalButton, handleButton, scrollButton, nestedButton, modelessButton, snapButton, sequentialButton, dismissButton]()
    {
        goalButton->setText(tr("打开抽屉"));
        handleButton->setText(tr("显示把手"));
        scrollButton->setText(tr("可滚动内容"));
        nestedButton->setText(tr("打开第 %1 层").arg(1));
        modelessButton->setText(tr("打开抽屉"));
        snapButton->setText(tr("吸附点"));
        sequentialButton->setText(tr("逐级吸附"));
        dismissButton->setText(tr("打开抽屉"));
    });
}

DrawerPage::~DrawerPage()
{
}
