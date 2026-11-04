#include "ToastPage.h"

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <initializer_list>
#include <memory>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadLabel.h"
#include "ShadToaster.h"
#include "ShadToggle.h"
#include "ShadToggleGroup.h"
#include "ShadWidget.h"

namespace
{
    // 子窗口尺寸与间距
    constexpr int kChildWindowWidth = 440;
    constexpr int kChildWindowHeight = 280;
    constexpr int kChildWindowPadding = 24;
    constexpr int kChildWindowGap = 16;

    // 子窗口：分别在本窗口与主窗口显示通知
    ShadWidget* createChildWindow(ShadToaster* mainToaster)
    {
        ShadWidget* window = new ShadWidget();
        window->resize(kChildWindowWidth, kChildWindowHeight);
        ExampleLayout::addStayOnTopButton(window);
        // 不阻止主窗口退出
        window->setAttribute(Qt::WA_QuitOnClose, false);
        QVBoxLayout* layout = new QVBoxLayout(window);
        layout->setContentsMargins(kChildWindowPadding, kChildWindowPadding, kChildWindowPadding, kChildWindowPadding);
        layout->setSpacing(kChildWindowGap);
        ShadLabel* descriptionLabel = new ShadLabel(window);
        descriptionLabel->setColorRole(ShadThemeType::MutedForeground);
        descriptionLabel->setWordWrap(true);
        layout->addWidget(descriptionLabel);
        layout->addStretch(1);
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->setContentsMargins(0, 0, 0, 0);
        buttonLayout->setSpacing(8 - 2 * ExampleLayout::kFocusRingWidth);
        ShadButton* localButton = new ShadButton(window);
        ShadButton* mainButton = new ShadButton(QString(), ShadButtonType::Outline, window);
        QObject::connect(localButton, &ShadButton::clicked, window, [window]() { ShadToaster::forWindow(window)->add(QCoreApplication::translate("ToastPage", "子窗口中的通知"), QCoreApplication::translate("ToastPage", "显示在当前窗口。"), ShadToastType::Success); });
        QObject::connect(mainButton, &ShadButton::clicked, window, [mainToaster]() { mainToaster->add(QCoreApplication::translate("ToastPage", "来自子窗口的通知"), QCoreApplication::translate("ToastPage", "显示在主窗口。")); });
        buttonLayout->addWidget(localButton);
        buttonLayout->addWidget(mainButton);
        buttonLayout->addStretch(1);
        layout->addLayout(buttonLayout);
        ExampleI18n::bind(window, [window, descriptionLabel, localButton, mainButton]()
        {
            window->setWindowTitle(QCoreApplication::translate("ToastPage", "子窗口通知"));
            descriptionLabel->setText(QCoreApplication::translate("ToastPage", "ShadToaster::forWindow(window) 返回本窗口的 toaster。"));
            localButton->setText(QCoreApplication::translate("ToastPage", "在此窗口显示"));
            mainButton->setText(QCoreApplication::translate("ToastPage", "在主窗口显示"));
        });
        return window;
    }
} // namespace

ToastPage::ToastPage(ShadToaster* toaster, QWidget* parent)
    : ExampleBasePage([] { return tr("通知"); }, [] { return tr("ShadToaster：在窗口内短暂显示的通知消息，悬停时展开。"); }, parent)
{
    ShadCard* positionCard = addSection([] { return tr("位置"); }, [] { return tr("切换位置后已显示的通知随之移动"); });
    ShadToggleGroup* positionGroup = new ShadToggleGroup(ShadToggleGroupType::Single, this);
    positionGroup->setVariant(ShadToggleType::Outline);
    // 顺序与 ShadToastType::Position 一致
    const QList<ExampleI18n::TextFunction> positionNames{[] { return tr("左上"); }, [] { return tr("上中"); }, [] { return tr("右上"); }, [] { return tr("左下"); }, [] { return tr("下中"); }, [] { return tr("右下"); }};
    for (int index = 0; index < positionNames.size(); ++index)
    {
        positionGroup->addItem(QString());
    }
    // 按序号刷新选项文字，不影响当前选中项
    ExampleI18n::bind(positionGroup, [positionGroup, positionNames]()
    {
        for (int index = 0; index < positionNames.size(); ++index)
        {
            positionGroup->itemAt(index)->setText(positionNames.at(index)());
        }
    });
    positionGroup->setCurrentIndex(static_cast<int>(toaster->getPosition()));
    connect(positionGroup, &ShadToggleGroup::currentIndexChanged, this, [positionGroup, toaster, positionNames](int index)
    {
        // 不允许取消选中：再次点击当前位置时恢复选中
        if (index < 0)
        {
            positionGroup->setCurrentIndex(static_cast<int>(toaster->getPosition()));
            return;
        }
        toaster->setPosition(static_cast<ShadToastType::Position>(index));
        toaster->add(tr("位置已切换"), tr("通知现在显示在%1").arg(positionNames.at(index)()));
    });
    addItem(positionCard, positionGroup);

    ShadCard* typeCard = addSection([] { return tr("类型"); }, [] { return tr("不同类型显示不同的状态图标"); });
    struct TypeDemo
    {
        ExampleI18n::TextFunction buttonText;
        ExampleI18n::TextFunction title;
        ExampleI18n::TextFunction description;
        ShadToastType::Type type;
    };
    const QList<TypeDemo> typeDemos{
        {[] { return tr("默认"); }, [] { return tr("事件已创建"); }, [] { return tr("星期日，12 月 3 日 上午 9:00"); }, ShadToastType::Default},
        {[] { return tr("成功"); }, [] { return tr("保存成功"); }, [] { return tr("所有更改已同步到云端。"); }, ShadToastType::Success},
        {[] { return tr("信息"); }, [] { return tr("有新版本可用"); }, [] { return tr("版本 0.2 包含 Toast 与 Resizable 组件。"); }, ShadToastType::Info},
        {[] { return tr("警告"); }, [] { return tr("存储空间不足"); }, [] { return tr("剩余空间低于 10%，请及时清理。"); }, ShadToastType::Warning},
        {[] { return tr("错误"); }, [] { return tr("上传失败"); }, [] { return tr("网络连接已断开，请检查后重试。"); }, ShadToastType::Error},
    };
    QList<QWidget*> typeButtons;
    for (const TypeDemo& demo : typeDemos)
    {
        ShadButton* button = new ShadButton(QString(), ShadButtonType::Outline, this);
        const ExampleI18n::TextFunction buttonText = demo.buttonText;
        ExampleI18n::bind(button, [button, buttonText]() { button->setText(buttonText()); });
        connect(button, &ShadButton::clicked, this, [toaster, demo]() { toaster->add(demo.title(), demo.description(), demo.type); });
        typeButtons.append(button);
    }
    addRow(typeCard, typeButtons);

    ShadCard* actionCard = addSection([] { return tr("操作按钮"); }, [] { return tr("通知中显示操作按钮，点击后执行回调"); });
    ShadButton* undoButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(undoButton, &ShadButton::clicked, this, [toaster]()
    {
        // onAction 需要用到 add 返回的 id，先用共享指针占位
        std::shared_ptr<int> toastId = std::make_shared<int>(0);
        ShadToastOptions options;
        options.title = tr("文件已删除");
        options.description = tr("报告.pdf 已移到回收站。");
        options.actionText = tr("撤销");
        options.onAction = [toaster, toastId]()
        {
            toaster->dismiss(*toastId);
            toaster->add(tr("已撤销"), tr("报告.pdf 已恢复。"), ShadToastType::Success);
        };
        *toastId = toaster->add(options);
    });
    addRow(actionCard, {undoButton});

    ShadCard* promiseCard = addSection([] { return tr("异步任务"); }, [] { return tr("先显示加载状态，任务结束后更新为成功或失败"); });
    ShadButton* successButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ShadButton* failureButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    for (ShadButton* button : {successButton, failureButton})
    {
        const bool isFailure = button == failureButton;
        connect(button, &ShadButton::clicked, this, [toaster, isFailure]()
        {
            const int toastId = toaster->add(tr("正在创建事件"), tr("请稍候……"), ShadToastType::Loading);
            QTimer::singleShot(2000, toaster, [toaster, toastId, isFailure]()
            {
                ShadToastOptions result;
                result.title = isFailure ? tr("创建失败") : tr("事件已创建");
                result.description = isFailure ? tr("服务器暂时不可用。") : tr("星期日，12 月 3 日 上午 9:00");
                result.type = isFailure ? ShadToastType::Error : ShadToastType::Success;
                toaster->updateToast(toastId, result);
            });
        });
    }
    addRow(promiseCard, {successButton, failureButton});

    ShadCard* stackCard = addSection([] { return tr("堆叠与关闭"); }, [] { return tr("收起时显示最新 3 条，悬停展开"); });
    ShadButton* burstButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(burstButton, &ShadButton::clicked, this, [toaster]()
    {
        for (int i = 1; i <= 5; ++i)
        {
            toaster->add(tr("第 %1 条通知").arg(i), tr("悬停在通知上可以展开查看全部。"), ShadToastType::Info);
        }
    });
    ShadButton* persistentButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(persistentButton, &ShadButton::clicked, this, [toaster]()
    {
        ShadToastOptions options;
        options.title = tr("需要手动关闭");
        options.description = tr("这条通知不会自动消失，点击右侧的关闭按钮。");
        options.duration = 0;
        toaster->add(options);
    });
    ShadButton* dismissAllButton = new ShadButton(QString(), ShadButtonType::Secondary, this);
    connect(dismissAllButton, &ShadButton::clicked, toaster, &ShadToaster::dismissAll);
    addRow(stackCard, {burstButton, persistentButton, dismissAllButton});

    ShadCard* windowCard = addSection([] { return tr("子窗口"); }, [] { return tr("ShadToaster::forWindow 按窗口显示通知"); });
    ShadButton* windowButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(windowButton, &ShadButton::clicked, this, [this, toaster]() { ExampleLayout::presentTransientWindow(_childWindow, [toaster]() { return createChildWindow(toaster); }); });
    addItem(windowCard, windowButton);

    ExampleI18n::bind(this, [undoButton, successButton, failureButton, burstButton, persistentButton, dismissAllButton, windowButton]()
    {
        undoButton->setText(tr("删除文件"));
        successButton->setText(tr("创建事件"));
        failureButton->setText(tr("创建事件（模拟失败）"));
        burstButton->setText(tr("连续弹出 5 条"));
        persistentButton->setText(tr("不自动关闭"));
        dismissAllButton->setText(tr("全部关闭"));
        windowButton->setText(tr("打开子窗口"));
    });
}

ToastPage::~ToastPage()
{
    // 子窗口无父组件，随页面销毁
    delete _childWindow.data();
}
