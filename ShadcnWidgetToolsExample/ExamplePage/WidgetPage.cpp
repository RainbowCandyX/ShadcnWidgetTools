#include "WidgetPage.h"

#include <QCoreApplication>
#include <QDialog>
#include <QFont>
#include <QHBoxLayout>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <initializer_list>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadBadge.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadDialog.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadSwitch.h"
#include "ShadWidget.h"

namespace
{
    // 窗口内容的内边距与间距
    constexpr int kBodyPadding = 24;
    constexpr int kBodyGap = 16;
    // 固定尺寸示例的窗口尺寸
    constexpr int kFixedWidth = 360;
    constexpr int kFixedHeight = 220;
    // 标题栏示例的窗口尺寸
    constexpr int kCompactWidth = 420;
    constexpr int kCompactHeight = 260;
    // 关闭确认框的宽度
    constexpr int kConfirmDialogWidth = 320;

    // 窗口内容：标题、说明与纵向布局，返回的布局已设置到窗口上；窗口标题、标题与说明随语言刷新
    QVBoxLayout* createBody(ShadWidget* window, ExampleI18n::TextFunction windowTitle, ExampleI18n::TextFunction heading, ExampleI18n::TextFunction description)
    {
        QVBoxLayout* layout = new QVBoxLayout(window);
        layout->setContentsMargins(kBodyPadding, kBodyPadding, kBodyPadding, kBodyPadding);
        layout->setSpacing(kBodyGap);
        ShadLabel* headingLabel = new ShadLabel(window);
        QFont headingFont = headingLabel->font();
        headingFont.setPixelSize(16);
        headingFont.setWeight(QFont::DemiBold);
        headingLabel->setFont(headingFont);
        ShadLabel* descriptionLabel = new ShadLabel(window);
        descriptionLabel->setColorRole(ShadThemeType::MutedForeground);
        descriptionLabel->setWordWrap(true);
        layout->addWidget(headingLabel);
        layout->addWidget(descriptionLabel);
        ExampleI18n::bind(window, [window, headingLabel, descriptionLabel, windowTitle, heading, description]()
        {
            window->setWindowTitle(windowTitle());
            headingLabel->setText(heading());
            descriptionLabel->setText(description());
        });
        return layout;
    }

    // 不设父组件的示例窗口，与主窗口平级
    ShadWidget* createWindow()
    {
        ShadWidget* window = new ShadWidget();
        ExampleLayout::addStayOnTopButton(window);
        // 关闭主窗口时不被这些窗口阻止退出
        window->setAttribute(Qt::WA_QuitOnClose, false);
        return window;
    }

    // 基础用法：直接 setLayout
    ShadWidget* createBasicWindow()
    {
        ShadWidget* window = createWindow();
        QVBoxLayout* layout = createBody(window, [] { return QCoreApplication::translate("WidgetPage", "通知设置"); }, [] { return QCoreApplication::translate("WidgetPage", "通知"); }, [] { return QCoreApplication::translate("WidgetPage", "选择你希望收到的通知类型，修改会立即生效。"); });
        // 开关之间扣除上下的焦点环边距
        QVBoxLayout* switchLayout = new QVBoxLayout();
        switchLayout->setContentsMargins(0, 0, 0, 0);
        switchLayout->setSpacing(12 - 2 * ExampleLayout::kFocusRingWidth);
        ShadSwitch* emailSwitch = new ShadSwitch(window);
        emailSwitch->setChecked(true);
        ShadSwitch* pushSwitch = new ShadSwitch(window);
        ShadSwitch* weeklySwitch = new ShadSwitch(window);
        weeklySwitch->setChecked(true);
        switchLayout->addWidget(emailSwitch);
        switchLayout->addWidget(pushSwitch);
        switchLayout->addWidget(weeklySwitch);
        layout->addLayout(switchLayout);
        layout->addStretch(1);
        QHBoxLayout* footer = new QHBoxLayout();
        footer->setContentsMargins(0, 0, 0, 0);
        footer->setSpacing(8 - 2 * ExampleLayout::kFocusRingWidth);
        ShadButton* centerButton = new ShadButton(QString(), ShadButtonType::Outline, window);
        ShadButton* doneButton = new ShadButton(window);
        footer->addWidget(centerButton);
        footer->addStretch(1);
        footer->addWidget(doneButton);
        layout->addLayout(footer);
        QObject::connect(centerButton, &ShadButton::clicked, window, &ShadWidget::moveToCenter);
        QObject::connect(doneButton, &ShadButton::clicked, window, &ShadWidget::close);
        ExampleI18n::bind(window, [emailSwitch, pushSwitch, weeklySwitch, centerButton, doneButton]()
        {
            emailSwitch->setText(QCoreApplication::translate("WidgetPage", "邮件通知"));
            pushSwitch->setText(QCoreApplication::translate("WidgetPage", "推送通知"));
            weeklySwitch->setText(QCoreApplication::translate("WidgetPage", "每周摘要"));
            centerButton->setText(QCoreApplication::translate("WidgetPage", "移到中央"));
            doneButton->setText(QCoreApplication::translate("WidgetPage", "完成"));
        });
        return window;
    }

    // 标题栏按钮与自定义组件
    ShadWidget* createTitleBarWindow()
    {
        ShadWidget* window = createWindow();
        window->resize(kCompactWidth, kCompactHeight);
        window->setButtonFlags(ShadWindowType::MinimizeButton | ShadWindowType::CloseButton);
        ExampleLayout::addStayOnTopButton(window);
        window->setTitleBarWidget(ShadWindowType::LeftArea, new ShadBadge(QStringLiteral("Pro"), ShadBadgeType::Secondary, window));
        ShadButton* settingsButton = new ShadButton(window);
        settingsButton->setVariant(ShadButtonType::Ghost);
        settingsButton->setButtonSize(ShadButtonType::IconSmall);
        settingsButton->setShadIcon(ShadIconType::Settings);
        ExampleI18n::bind(settingsButton, [settingsButton]() { settingsButton->setToolTip(QCoreApplication::translate("WidgetPage", "设置")); });
        window->setTitleBarWidget(ShadWindowType::RightArea, settingsButton);
        QVBoxLayout* layout = createBody(window, [] { return QCoreApplication::translate("WidgetPage", "云同步"); }, [] { return QCoreApplication::translate("WidgetPage", "云同步"); }, [] { return QCoreApplication::translate("WidgetPage", "这个窗口没有最大化按钮，双击标题栏不会最大化；标题后的徽章与右侧的设置按钮通过 setTitleBarWidget 放入。"); });
        layout->addStretch(1);
        return window;
    }

#ifdef Q_OS_MACOS
    ShadWidget* createButtonPositionWindow()
    {
        ShadWidget* window = createWindow();
        window->resize(kCompactWidth, kCompactHeight);
        QVBoxLayout* layout = createBody(window, [] { return QCoreApplication::translate("WidgetPage", "按钮位置"); }, [] { return QCoreApplication::translate("WidgetPage", "按钮位置"); }, [] { return QCoreApplication::translate("WidgetPage", "窗口按钮默认在右侧，打开下方开关后移到左侧。"); });
        ShadSwitch* leftSwitch = new ShadSwitch(window);
        ExampleI18n::bind(leftSwitch, [leftSwitch]() { leftSwitch->setText(QCoreApplication::translate("WidgetPage", "按钮放在左侧")); });
        layout->addWidget(leftSwitch, 0, Qt::AlignLeft);
        layout->addStretch(1);
        QObject::connect(leftSwitch, &ShadSwitch::toggled, window, [window](bool checked) { window->setButtonPosition(checked ? ShadWindowType::ButtonsOnLeft : ShadWindowType::ButtonsOnRight); });
        return window;
    }
#endif

    // 固定尺寸
    ShadWidget* createFixedWindow()
    {
        ShadWidget* window = createWindow();
        window->setButtonFlags(ShadWindowType::CloseButton);
        ExampleLayout::addStayOnTopButton(window);
        window->setFixedSize(kFixedWidth, kFixedHeight);
        QVBoxLayout* layout = createBody(window, [] { return QCoreApplication::translate("WidgetPage", "固定尺寸"); }, [] { return QCoreApplication::translate("WidgetPage", "固定尺寸"); }, [] { return QCoreApplication::translate("WidgetPage", "这个窗口的尺寸固定为 360 × 220。"); });
        layout->addStretch(1);
        return window;
    }

    // 置顶：开关与标题栏图钉按钮同步
    ShadWidget* createStayOnTopWindow()
    {
        ShadWidget* window = createWindow();
        window->resize(kCompactWidth, kCompactHeight);
        QVBoxLayout* layout = createBody(window, [] { return QCoreApplication::translate("WidgetPage", "便签"); }, [] { return QCoreApplication::translate("WidgetPage", "便签"); }, [] { return QCoreApplication::translate("WidgetPage", "打开下方开关或点击标题栏的图钉按钮后，这个窗口会保持在其他窗口之上。"); });
        ShadSwitch* stayOnTopSwitch = new ShadSwitch(window);
        ExampleI18n::bind(stayOnTopSwitch, [stayOnTopSwitch]() { stayOnTopSwitch->setText(QCoreApplication::translate("WidgetPage", "保持在最上层")); });
        layout->addWidget(stayOnTopSwitch, 0, Qt::AlignLeft);
        layout->addStretch(1);
        QObject::connect(stayOnTopSwitch, &ShadSwitch::toggled, window, &ShadWidget::setStayOnTop);
        // 标题栏置顶按钮反向同步开关
        QObject::connect(window, &ShadWidget::stayOnTopChanged, stayOnTopSwitch, &ShadSwitch::setChecked);
        return window;
    }

    // 放弃修改的确认框，exec 返回后由调用方删除；打开期间切换语言同样刷新文字
    ShadDialog* createDiscardDialog(QWidget* parent)
    {
        ShadDialog* dialog = new ShadDialog(parent);
        ExampleLayout::addStayOnTopButton(dialog);
        ShadButton* keepButton = new ShadButton(QString(), ShadButtonType::Outline, dialog);
        ShadButton* discardButton = new ShadButton(QString(), ShadButtonType::Destructive, dialog);
        dialog->footerLayout()->addWidget(keepButton);
        dialog->footerLayout()->addWidget(discardButton);
        QObject::connect(keepButton, &ShadButton::clicked, dialog, &ShadDialog::reject);
        QObject::connect(discardButton, &ShadButton::clicked, dialog, &ShadDialog::accept);
        ExampleI18n::bind(dialog, [dialog, keepButton, discardButton]()
        {
            dialog->setTitle(QCoreApplication::translate("WidgetPage", "放弃修改？"));
            dialog->setDescription(QCoreApplication::translate("WidgetPage", "草稿还没有保存，关闭后修改会丢失。"));
            keepButton->setText(QCoreApplication::translate("WidgetPage", "继续编辑"));
            discardButton->setText(QCoreApplication::translate("WidgetPage", "放弃并关闭"));
        });
        // 较窄宽度，对应 AlertDialog 的 size="sm"
        dialog->resize(kConfirmDialogWidth, dialog->height());
        return dialog;
    }

    // 拦截关闭按钮：确认后再关闭
    ShadWidget* createEditorWindow()
    {
        ShadWidget* window = createWindow();
        window->resize(kCompactWidth, kCompactHeight);
        window->setDefaultCloseEnabled(false);
        QVBoxLayout* layout = createBody(window, [] { return QCoreApplication::translate("WidgetPage", "未保存的草稿"); }, [] { return QCoreApplication::translate("WidgetPage", "草稿"); }, [] { return QCoreApplication::translate("WidgetPage", "点击标题栏的关闭按钮或按 Alt+F4 时会先询问是否放弃修改。"); });
        layout->addStretch(1);
        QObject::connect(window, &ShadWidget::closeRequested, window, [window]()
        {
            QPointer<ShadDialog> dialog = createDiscardDialog(window);
            const bool isAccepted = dialog->exec() == QDialog::Accepted;
            // 对话框打开期间窗口被销毁时，对话框已随窗口删除
            if (dialog.isNull())
            {
                return;
            }
            delete dialog.data();
            if (isAccepted)
            {
                window->close();
            }
        });
        return window;
    }
} // namespace

WidgetPage::WidgetPage(QWidget* parent)
    : ExampleBasePage([] { return tr("独立窗口"); }, [] { return tr("ShadWidget：外观与 ShadWindow 相同的独立窗口，标题栏不占用布局，可以直接 setLayout。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("直接 setLayout，内容从标题栏下方开始"); });
    ShadButton* basicButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(basicButton, &ShadButton::clicked, this, [this]() { ExampleLayout::presentTransientWindow(_basicWindow, createBasicWindow); });
    addItem(basicCard, basicButton);

    ShadCard* titleBarCard = addSection([] { return tr("标题栏按钮与自定义组件"); }, [] { return tr("精简窗口按钮，并在标题栏放入自定义组件"); });
    ShadButton* titleBarButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(titleBarButton, &ShadButton::clicked, this, [this]() { ExampleLayout::presentTransientWindow(_titleBarWindow, createTitleBarWindow); });
    addItem(titleBarCard, titleBarButton);

    ShadButton* buttonPositionButton = nullptr;
#ifdef Q_OS_MACOS
    ShadCard* buttonPositionCard = addSection([] { return tr("按钮位置"); }, [] { return tr("窗口按钮可放在标题栏左侧或右侧"); });
    buttonPositionButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(buttonPositionButton, &ShadButton::clicked, this, [this]() { ExampleLayout::presentTransientWindow(_buttonPositionWindow, createButtonPositionWindow); });
    addItem(buttonPositionCard, buttonPositionButton);
#endif

    ShadCard* fixedCard = addSection([] { return tr("固定尺寸"); }, [] { return tr("固定尺寸后不能拖动缩放，也不能双击最大化"); });
    ShadButton* fixedButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(fixedButton, &ShadButton::clicked, this, [this]() { ExampleLayout::presentTransientWindow(_fixedWindow, createFixedWindow); });
    addItem(fixedCard, fixedButton);

    ShadCard* stayOnTopCard = addSection([] { return tr("置顶"); }, [] { return tr("保持在其他窗口之上，标题栏可显示置顶按钮"); });
    ShadButton* stayOnTopButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(stayOnTopButton, &ShadButton::clicked, this, [this]() { ExampleLayout::presentTransientWindow(_stayOnTopWindow, createStayOnTopWindow); });
    addItem(stayOnTopCard, stayOnTopButton);

    ShadCard* closeCard = addSection([] { return tr("拦截关闭按钮"); }, [] { return tr("关闭请求只发出 closeRequested，确认后再关闭"); });
    ShadButton* closeButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(closeButton, &ShadButton::clicked, this, [this]() { ExampleLayout::presentTransientWindow(_editorWindow, createEditorWindow); });
    addItem(closeCard, closeButton);

    ExampleI18n::bind(this, [basicButton, titleBarButton, buttonPositionButton, fixedButton, stayOnTopButton, closeButton]()
    {
        for (ShadButton* button : {basicButton, titleBarButton, buttonPositionButton, fixedButton, stayOnTopButton, closeButton})
        {
            if (button != nullptr)
            {
                button->setText(tr("打开窗口"));
            }
        }
    });
}

WidgetPage::~WidgetPage()
{
    // 示例窗口没有父组件，不会随页面释放
    for (ShadWidget* window : {_basicWindow.data(), _titleBarWindow.data(), _buttonPositionWindow.data(), _fixedWindow.data(), _stayOnTopWindow.data(), _editorWindow.data()})
    {
        delete window;
    }
}
