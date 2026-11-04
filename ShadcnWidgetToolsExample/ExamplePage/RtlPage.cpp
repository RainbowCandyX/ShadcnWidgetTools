#include "RtlPage.h"

#include <QAction>
#include <QBoxLayout>
#include <QFont>
#include <QKeySequence>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QList>
#include <QLocale>
#include <QObject>
#include <QPair>
#include <QSizePolicy>
#include <QString>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include "ExampleLayout.h"
#include "ExampleMutedItem.h"
#include "ShadAlert.h"
#include "ShadBadge.h"
#include "ShadBreadcrumb.h"
#include "ShadButton.h"
#include "ShadButtonGroup.h"
#include "ShadCard.h"
#include "ShadCheckbox.h"
#include "ShadCodeBlock.h"
#include "ShadComboBox.h"
#include "ShadDatePicker.h"
#include "ShadDialog.h"
#include "ShadDropdownMenu.h"
#include "ShadFlowLayout.h"
#include "ShadInput.h"
#include "ShadInputGroup.h"
#include "ShadKbd.h"
#include "ShadLabel.h"
#include "ShadPopover.h"
#include "ShadProgress.h"
#include "ShadQrCode.h"
#include "ShadRadioButton.h"
#include "ShadSidebar.h"
#include "ShadSidebarGroup.h"
#include "ShadSidebarLayout.h"
#include "ShadSidebarMenuButton.h"
#include "ShadSidebarTrigger.h"
#include "ShadSlider.h"
#include "ShadSpinner.h"
#include "ShadSwitch.h"
#include "ShadTabs.h"
#include "ShadTimePicker.h"
#include "ShadToggleGroup.h"
#include "ShadTooltip.h"
#include "ShadTrace.h"
#include "ShadWidget.h"

namespace
{
    constexpr int kLoginCardWidth = 400;
    // 对话框宽度 sm:max-w-sm
    constexpr int kRtlDialogWidth = 384;
    // 独立窗口尺寸
    constexpr int kRtlWindowWidth = 420;
    constexpr int kRtlWindowHeight = 300;
    // 日期选择触发框 w-[212px]，加上左右焦点环边距
    constexpr int kRtlDatePickerWidth = 212 + 2 * ExampleLayout::kFocusRingWidth;
    // 时间选择触发框 w-36，加上左右焦点环边距
    constexpr int kRtlTimePickerWidth = 144 + 2 * ExampleLayout::kFocusRingWidth;
    // 侧栏示例的高度
    constexpr int kRtlSidebarHeight = 360;
    // 下拉框与搜索框的可视宽度，加上左右焦点环边距
    constexpr int kCompactFieldWidth = 200 + 2 * ExampleLayout::kFocusRingWidth;

    // 固定为从右到左的演示区域，子组件全部继承该方向
    QWidget* createRtlArea(QWidget* parent)
    {
        QWidget* area = new QWidget(parent);
        area->setLayoutDirection(Qt::RightToLeft);
        return area;
    }

    // 表单字段标签：左右各留焦点环边距，与输入框可视边缘对齐（对称设置，两种方向下都成立）
    ShadLabel* createFieldLabel(const QString& text, QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(text, parent);
        label->setContentsMargins(ExampleLayout::kFocusRingWidth, 0, ExampleLayout::kFocusRingWidth, 0);
        return label;
    }

    // 编辑资料对话框，方向继承自 RTL 区域内的父组件
    ShadDialog* createRtlDialog(QWidget* parent)
    {
        ShadDialog* dialog = new ShadDialog(QStringLiteral("تعديل الملف الشخصي"), QStringLiteral("قم بإجراء تغييرات على ملفك الشخصي هنا. انقر فوق حفظ عند الانتهاء."), parent);
        dialog->resize(kRtlDialogWidth, dialog->height());
        ExampleLayout::addStayOnTopButton(dialog);
        // 字段之间 gap-7，标签与输入框 gap-3，均扣除输入框的焦点环边距
        dialog->contentLayout()->setSpacing(28 - ExampleLayout::kFocusRingWidth);
        const QList<QPair<QString, QString>> fields{
            {QStringLiteral("الاسم"), QStringLiteral("Pedro Duarte")},
            {QStringLiteral("اسم المستخدم"), QStringLiteral("@peduarte")},
        };
        for (const QPair<QString, QString>& field : fields)
        {
            QVBoxLayout* fieldLayout = new QVBoxLayout();
            fieldLayout->setContentsMargins(0, 0, 0, 0);
            fieldLayout->setSpacing(12 - ExampleLayout::kFocusRingWidth);
            ShadLabel* fieldLabel = createFieldLabel(field.first, dialog);
            ShadInput* fieldInput = new ShadInput(dialog);
            fieldInput->setText(field.second);
            fieldLabel->setBuddy(fieldInput);
            fieldLayout->addWidget(fieldLabel);
            fieldLayout->addWidget(fieldInput);
            dialog->contentLayout()->addLayout(fieldLayout);
        }
        ShadButton* cancelButton = new ShadButton(QStringLiteral("إلغاء"), ShadButtonType::Outline, dialog);
        ShadButton* saveButton = new ShadButton(QStringLiteral("حفظ التغييرات"), dialog);
        saveButton->setDefault(true);
        dialog->footerLayout()->addWidget(cancelButton);
        dialog->footerLayout()->addWidget(saveButton);
        QObject::connect(cancelButton, &ShadButton::clicked, dialog, &ShadDialog::reject);
        QObject::connect(saveButton, &ShadButton::clicked, dialog, &ShadDialog::accept);
        return dialog;
    }

    // 独立窗口：不设父组件，否则在 Windows 上会始终位于主窗口之上；顶层窗口不继承方向，需显式设置 RTL
    ShadWidget* createRtlWindow()
    {
        ShadWidget* window = new ShadWidget();
        window->setAttribute(Qt::WA_QuitOnClose, false);
        ExampleLayout::addStayOnTopButton(window);
        window->setLayoutDirection(Qt::RightToLeft);
        window->setWindowTitle(QStringLiteral("الإعدادات"));
        window->resize(kRtlWindowWidth, kRtlWindowHeight);
        QVBoxLayout* layout = new QVBoxLayout(window);
        layout->setContentsMargins(24, 24, 24, 24);
        layout->setSpacing(16);
        ShadLabel* heading = new ShadLabel(QStringLiteral("الإشعارات"), window);
        QFont headingFont = heading->font();
        headingFont.setPixelSize(16);
        headingFont.setWeight(QFont::DemiBold);
        heading->setFont(headingFont);
        ShadLabel* description = new ShadLabel(QStringLiteral("اختر الإشعارات التي تريد تلقيها."), window);
        description->setColorRole(ShadThemeType::MutedForeground);
        description->setWordWrap(true);
        ShadSwitch* emailSwitch = new ShadSwitch(QStringLiteral("إشعارات البريد الإلكتروني"), window);
        emailSwitch->setChecked(true);
        ShadSwitch* pushSwitch = new ShadSwitch(QStringLiteral("الإشعارات الفورية"), window);
        ShadButton* doneButton = new ShadButton(QStringLiteral("تم"), window);
        layout->addWidget(heading);
        layout->addWidget(description);
        layout->addWidget(emailSwitch, 0, Qt::AlignLeft);
        layout->addWidget(pushSwitch, 0, Qt::AlignLeft);
        layout->addStretch(1);
        layout->addWidget(doneButton, 0, Qt::AlignRight);
        QObject::connect(doneButton, &ShadButton::clicked, window, &ShadWidget::close);
        return window;
    }
} // namespace

RtlPage::RtlPage(QWidget* parent)
    : ExampleBasePage([] { return tr("RTL 布局"); }, [] { return tr("组件跟随 Qt 的 layoutDirection，从右到左时绘制、交互与弹出层位置全部镜像。"); }, parent)
{
    // 登录卡片，对应 shadcn 文档 RTL 示例
    ShadCard* loginSection = addSection([] { return tr("阿拉伯语登录卡片"); }, [] { return tr("操作按钮位于左上角，输入框图标与按钮组镜像"); });
    QWidget* loginArea = createRtlArea(this);
    QVBoxLayout* loginAreaLayout = new QVBoxLayout(loginArea);
    loginAreaLayout->setContentsMargins(0, 0, 0, 0);

    ShadCard* loginCard = new ShadCard(QStringLiteral("تسجيل الدخول إلى حسابك"), QStringLiteral("أدخل بريدك الإلكتروني أدناه لتسجيل الدخول إلى حسابك"), loginArea);
    loginCard->setFixedWidth(kLoginCardWidth);
    loginCard->setHeaderAction(new ShadButton(QStringLiteral("إنشاء حساب"), ShadButtonType::Link, loginCard));

    QVBoxLayout* contentLayout = loginCard->contentLayout();
    // 字段之间 gap-6，标签与输入框之间 gap-2，均扣除输入框 3px 焦点环边距
    contentLayout->setSpacing(24 - 2 * ExampleLayout::kFocusRingWidth);
    ShadLabel* emailLabel = createFieldLabel(QStringLiteral("البريد الإلكتروني"), loginCard);
    ShadInput* emailInput = new ShadInput(loginCard);
    emailInput->setLeadingIcon(ShadIconType::Mail);
    emailInput->setPlaceholderText(QStringLiteral("m@example.com"));
    emailLabel->setBuddy(emailInput);
    QVBoxLayout* emailLayout = new QVBoxLayout();
    emailLayout->setSpacing(8 - ExampleLayout::kFocusRingWidth);
    emailLayout->addWidget(emailLabel);
    emailLayout->addWidget(emailInput);

    // 密码标签与“忘记密码”链接分列两端（ml-auto）
    ShadLabel* passwordLabel = createFieldLabel(QStringLiteral("كلمة المرور"), loginCard);
    ShadButton* forgotButton = new ShadButton(QStringLiteral("نسيت كلمة المرور؟"), ShadButtonType::Link, loginCard);
    forgotButton->setButtonSize(ShadButtonType::Small);
    QHBoxLayout* passwordHeaderLayout = new QHBoxLayout();
    passwordHeaderLayout->setSpacing(0);
    passwordHeaderLayout->addWidget(passwordLabel);
    passwordHeaderLayout->addStretch(1);
    passwordHeaderLayout->addWidget(forgotButton);
    ShadInput* passwordInput = new ShadInput(loginCard);
    passwordInput->setEchoMode(QLineEdit::Password);
    passwordInput->setTrailingAction(ShadInputType::PasswordToggleAction);
    passwordLabel->setBuddy(passwordInput);
    QVBoxLayout* passwordLayout = new QVBoxLayout();
    passwordLayout->setSpacing(8 - ExampleLayout::kFocusRingWidth);
    passwordLayout->addLayout(passwordHeaderLayout);
    passwordLayout->addWidget(passwordInput);
    contentLayout->addLayout(emailLayout);
    contentLayout->addLayout(passwordLayout);

    // 底部按钮纵向排列（flex-col gap-2），扣除相邻按钮的焦点环边距
    QHBoxLayout* footerLayout = loginCard->footerLayout();
    footerLayout->setDirection(QBoxLayout::TopToBottom);
    footerLayout->setSpacing(8 - 2 * ExampleLayout::kFocusRingWidth);
    ShadButton* loginButton = new ShadButton(QStringLiteral("تسجيل الدخول"), ShadButtonType::Default, loginCard);
    loginButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ShadButton* googleButton = new ShadButton(QStringLiteral("تسجيل الدخول باستخدام Google"), ShadButtonType::Outline, loginCard);
    googleButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    footerLayout->addWidget(loginButton);
    footerLayout->addWidget(googleButton);

    // AlignLeft 为逻辑对齐，RTL 下卡片靠右；演示区域占满整行
    loginAreaLayout->addWidget(loginCard, 0, Qt::AlignLeft);
    loginSection->contentLayout()->addWidget(loginArea);

    // 面包屑，对应 shadcn breadcrumb-rtl 示例：从右向左排列，下拉菜单与触发项的起始侧（右侧）对齐
    ShadCard* breadcrumbSection = addSection([] { return tr("面包屑"); }, [] { return tr("项从右向左排列，下拉菜单右对齐"); });
    QWidget* breadcrumbArea = createRtlArea(this);
    QVBoxLayout* breadcrumbAreaLayout = new QVBoxLayout(breadcrumbArea);
    breadcrumbAreaLayout->setContentsMargins(0, 0, 0, 0);
    ShadBreadcrumb* breadcrumb = new ShadBreadcrumb(breadcrumbArea);
    breadcrumb->setSeparatorIcon(ShadIconType::Dot);
    ShadDropdownMenu* componentsMenu = new ShadDropdownMenu(breadcrumbArea);
    componentsMenu->addItem(QStringLiteral("التوثيق"));
    componentsMenu->addItem(QStringLiteral("السمات"));
    componentsMenu->addItem(QStringLiteral("جيت هاب"));
    breadcrumb->addLink(QStringLiteral("الرئيسية"));
    breadcrumb->addDropdown(QStringLiteral("المكونات"), componentsMenu);
    breadcrumb->addPage(QStringLiteral("مسار التنقل"));
    // 第二条使用默认的 ChevronRight 分隔符，RTL 下显示为向左的箭头
    ShadBreadcrumb* chevronBreadcrumb = new ShadBreadcrumb(breadcrumbArea);
    chevronBreadcrumb->setPath({QStringLiteral("الرئيسية"), QStringLiteral("المكونات"), QStringLiteral("مسار التنقل")});
    breadcrumbAreaLayout->addWidget(breadcrumb, 0, Qt::AlignLeft);
    breadcrumbAreaLayout->addWidget(chevronBreadcrumb, 0, Qt::AlignLeft);
    breadcrumbSection->contentLayout()->addWidget(breadcrumbArea);

    // 按钮组：从右向左排列，第一个按钮的右侧为圆角，拼接处的边框在左侧
    ShadCard* buttonGroupSection = addSection([] { return tr("按钮组"); }, [] { return tr("从右向左排列，圆角与拼接边框随方向镜像"); });
    QWidget* buttonGroupArea = createRtlArea(this);
    QVBoxLayout* buttonGroupAreaLayout = new QVBoxLayout(buttonGroupArea);
    buttonGroupAreaLayout->setContentsMargins(0, 0, 0, 0);
    ShadButtonGroup* mailGroup = new ShadButtonGroup(buttonGroupArea);
    mailGroup->addWidget(new ShadButton(QStringLiteral("أرشفة"), ShadButtonType::Outline, mailGroup));
    mailGroup->addWidget(new ShadButton(QStringLiteral("إبلاغ"), ShadButtonType::Outline, mailGroup));
    mailGroup->addWidget(new ShadButton(QStringLiteral("تأجيل"), ShadButtonType::Outline, mailGroup));
    ShadButton* mailMoreButton = new ShadButton(QString(), ShadButtonType::Outline, mailGroup);
    mailMoreButton->setButtonSize(ShadButtonType::Icon);
    mailMoreButton->setShadIcon(ShadIconType::Ellipsis);
    mailGroup->addWidget(mailMoreButton);
    ShadButtonGroup* searchGroup = new ShadButtonGroup(buttonGroupArea);
    ShadInput* rtlSearchInput = new ShadInput(searchGroup);
    rtlSearchInput->setPlaceholderText(QStringLiteral("بحث..."));
    ShadButton* rtlSearchButton = new ShadButton(QString(), ShadButtonType::Outline, searchGroup);
    rtlSearchButton->setButtonSize(ShadButtonType::Icon);
    rtlSearchButton->setShadIcon(ShadIconType::Search);
    searchGroup->addWidget(rtlSearchInput);
    searchGroup->addWidget(rtlSearchButton);
    buttonGroupAreaLayout->addWidget(mailGroup, 0, Qt::AlignLeft);
    buttonGroupAreaLayout->addWidget(searchGroup, 0, Qt::AlignLeft);
    buttonGroupSection->contentLayout()->addWidget(buttonGroupArea);

    // 浮层、提示与输入组：浮层的起始对齐为右对齐，提示的 Left / Right 互换，输入组的附加区镜像
    ShadCard* overlaySection = addSection([] { return tr("浮层、提示与输入组"); }, [] { return tr("浮层右对齐，提示左右互换，输入组附加区镜像"); });
    QWidget* overlayArea = createRtlArea(this);
    QVBoxLayout* overlayAreaLayout = new QVBoxLayout(overlayArea);
    overlayAreaLayout->setContentsMargins(0, 0, 0, 0);
    overlayAreaLayout->setSpacing(16);
    ShadButton* popoverButton = new ShadButton(QStringLiteral("فتح النافذة"), ShadButtonType::Outline, overlayArea);
    ShadPopover* popover = new ShadPopover(overlayArea);
    popover->setTitle(QStringLiteral("الأبعاد"));
    popover->setDescription(QStringLiteral("قم بتعيين أبعاد الطبقة."));
    ShadInput* widthInput = new ShadInput(popover);
    widthInput->setText(QStringLiteral("100%"));
    popover->contentLayout()->addWidget(widthInput);
    connect(popoverButton, &ShadButton::clicked, popover, [popover, popoverButton]() { popover->toggle(popoverButton, ShadPlacementType::Bottom, ShadPlacementType::Start); });
    ShadButton* tooltipButton = new ShadButton(QStringLiteral("تمرير"), ShadButtonType::Outline, overlayArea);
    tooltipButton->setToolTip(QStringLiteral("إضافة إلى المكتبة"));
    ShadTooltip::setSide(tooltipButton, ShadPlacementType::Right);
    ShadFlowLayout* overlayRow = new ShadFlowLayout();
    overlayRow->addWidget(popoverButton);
    overlayRow->addWidget(tooltipButton);
    overlayAreaLayout->addLayout(overlayRow);
    ShadInputGroup* rtlInputGroup = new ShadInputGroup(overlayArea);
    rtlInputGroup->setFixedWidth(kLoginCardWidth);
    rtlInputGroup->getInput()->setPlaceholderText(QStringLiteral("بحث..."));
    rtlInputGroup->addIcon(ShadInputGroupType::InlineStart, ShadIconType::Search);
    // Ctrl+K（macOS 为 ⌘K）聚焦搜索框
    ShadKbdGroup* rtlSearchKeys = new ShadKbdGroup(rtlInputGroup);
    rtlSearchKeys->setKeySequence(QKeySequence(Qt::CTRL | Qt::Key_K));
    rtlSearchKeys->setShortcutEnabled(true);
    connect(rtlSearchKeys, &ShadKbdGroup::activated, rtlInputGroup, [rtlInputGroup]() { rtlInputGroup->getInput()->setFocus(Qt::ShortcutFocusReason); });
    rtlInputGroup->addWidget(ShadInputGroupType::InlineEnd, rtlSearchKeys);
    overlayAreaLayout->addWidget(rtlInputGroup, 0, Qt::AlignLeft);
    overlaySection->contentLayout()->addWidget(overlayArea);

    // 常用组件：从右向左排列，弹出层跟随锚点的方向
    ShadCard* widgetSection = addSection([] { return tr("常用组件"); }, [] { return tr("复选框、开关、滑块等常用组件在 RTL 下的效果"); });
    QWidget* widgetArea = createRtlArea(this);
    QVBoxLayout* widgetAreaLayout = new QVBoxLayout(widgetArea);
    widgetAreaLayout->setContentsMargins(0, 0, 0, 0);
    widgetAreaLayout->setSpacing(16);

    ShadCheckbox* rememberCheckbox = new ShadCheckbox(QStringLiteral("تذكرني"), widgetArea);
    rememberCheckbox->setChecked(true);
    ShadSwitch* notifySwitch = new ShadSwitch(QStringLiteral("الإشعارات"), widgetArea);
    notifySwitch->setChecked(true);
    // 同一父组件下的单选框自动互斥
    ShadRadioButton* monthlyRadio = new ShadRadioButton(QStringLiteral("شهري"), widgetArea);
    monthlyRadio->setChecked(true);
    ShadRadioButton* yearlyRadio = new ShadRadioButton(QStringLiteral("سنوي"), widgetArea);
    ShadFlowLayout* choiceRow = new ShadFlowLayout();
    choiceRow->addWidget(rememberCheckbox);
    choiceRow->addWidget(notifySwitch);
    choiceRow->addWidget(monthlyRadio);
    choiceRow->addWidget(yearlyRadio);
    widgetAreaLayout->addLayout(choiceRow);

    ShadComboBox* cityComboBox = new ShadComboBox(widgetArea);
    cityComboBox->setFixedWidth(kCompactFieldWidth);
    cityComboBox->setPlaceholderText(QStringLiteral("اختر مدينة"));
    cityComboBox->setSearchEnabled(true);
    cityComboBox->setSearchPlaceholderText(QStringLiteral("بحث..."));
    cityComboBox->setEmptyText(QStringLiteral("لا توجد نتائج"));
    cityComboBox->addItems({QStringLiteral("الرياض"), QStringLiteral("دبي"), QStringLiteral("القاهرة"), QStringLiteral("عمّان"), QStringLiteral("الدوحة")});
    ShadInput* searchInput = new ShadInput(widgetArea);
    searchInput->setFixedWidth(kCompactFieldWidth);
    searchInput->setLeadingIcon(ShadIconType::Search);
    searchInput->setTrailingAction(ShadInputType::ClearAction);
    searchInput->setPlaceholderText(QStringLiteral("بحث..."));
    ShadButton* menuButton = new ShadButton(QStringLiteral("الحساب"), ShadButtonType::Outline, widgetArea);
    menuButton->setShadIcon(ShadIconType::User);
    ShadDropdownMenu* accountMenu = new ShadDropdownMenu(this);
    accountMenu->addLabel(QStringLiteral("حسابي"));
    accountMenu->addItem(QStringLiteral("الملف الشخصي"), ShadIconType::User, ShadKbdGroup::shortcutText(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P)));
    accountMenu->addItem(QStringLiteral("الإعدادات"), ShadIconType::Settings, ShadKbdGroup::shortcutText(QKeySequence(Qt::CTRL | Qt::Key_S)));
    accountMenu->addCheckableItem(QStringLiteral("الوضع المضغوط"), true);
    accountMenu->addSeparator();
    QAction* deleteAction = accountMenu->addItem(QStringLiteral("حذف الحساب"), ShadIconType::Trash2);
    accountMenu->setItemDestructive(deleteAction, true);
    // 菜单使用按钮的布局方向，默认与按钮的起始侧（右侧）对齐
    connect(menuButton, &ShadButton::clicked, accountMenu, [accountMenu, menuButton]() { accountMenu->popup(menuButton); });
    ShadFlowLayout* fieldRow = new ShadFlowLayout();
    fieldRow->addWidget(cityComboBox);
    fieldRow->addWidget(searchInput);
    fieldRow->addWidget(menuButton);
    widgetAreaLayout->addLayout(fieldRow);

    ShadToggleGroup* formatGroup = new ShadToggleGroup(ShadToggleGroupType::Multiple, widgetArea);
    formatGroup->addItem(ShadIconType::Bold);
    formatGroup->addItem(ShadIconType::Italic);
    formatGroup->addItem(ShadIconType::Underline);
    formatGroup->setItemChecked(0, true);
    ShadBadge* newBadge = new ShadBadge(QStringLiteral("جديد"), ShadBadgeType::Default, widgetArea);
    ShadProgress* progress = new ShadProgress(widgetArea);
    progress->setValue(60);
    ShadSlider* slider = new ShadSlider(Qt::Horizontal, widgetArea);
    slider->setRange(0, 100);
    slider->setValue(40);
    ShadFlowLayout* valueRow = new ShadFlowLayout();
    valueRow->addWidget(formatGroup);
    valueRow->addWidget(newBadge);
    widgetAreaLayout->addLayout(valueRow);
    // 进度条、滑块与提示占满整行，进度从右侧开始
    widgetAreaLayout->addWidget(progress);
    widgetAreaLayout->addWidget(slider);

    ShadAlert* alert = new ShadAlert(ShadIconType::CircleCheck, QStringLiteral("تم حفظ التغييرات"), QStringLiteral("يمكنك تعديل إعدادات حسابك في أي وقت من صفحة الإعدادات."), ShadAlertType::Default, widgetArea);
    widgetAreaLayout->addWidget(alert);

    // 演示区域占满整行，流式布局才能按可用宽度换行
    widgetSection->contentLayout()->addWidget(widgetArea);

    // 代码块：标题栏、操作按钮与底栏跟随方向镜像，代码区保持从左到右
    ShadCard* codeSection = addSection([] { return tr("代码块"); }, [] { return tr("标题靠右、复制按钮在左，代码保持从左到右"); });
    QWidget* codeArea = createRtlArea(this);
    QVBoxLayout* codeAreaLayout = new QVBoxLayout(codeArea);
    codeAreaLayout->setContentsMargins(0, 0, 0, 0);
    codeAreaLayout->setSpacing(16);
    const QString formatDateCode = QStringLiteral("// تنسيق التاريخ للعرض في الواجهة\n"
        "export function formatDate(date: Date) {\n"
        "  return date.toLocaleDateString(\"en-US\")\n"
        "  return date.toLocaleDateString(\"ar-SA\")\n"
        "}\n");
    ShadCodeBlock* diffBlock = new ShadCodeBlock(formatDateCode, codeArea);
    diffBlock->setTitle(QStringLiteral("تنسيق التاريخ"));
    diffBlock->setTitleIcon(ShadIconType::FileCode);
    diffBlock->setLanguage(QStringLiteral("ts"));
    diffBlock->setLineNumbersVisible(true);
    diffBlock->setLineVariant(2, ShadCodeBlockType::RemovedLine);
    diffBlock->setLineVariant(3, ShadCodeBlockType::AddedLine);
    diffBlock->setCopyButtonVisible(true);
    ShadCodeBlock* floatingBlock = new ShadCodeBlock(QStringLiteral("$ pnpm build\n ✓ Compiled successfully in 4.2s\n"), codeArea);
    floatingBlock->setLanguage(QStringLiteral("bash"));
    floatingBlock->setCopyButtonVisible(true);
    codeAreaLayout->addWidget(diffBlock);
    codeAreaLayout->addWidget(floatingBlock);
    codeSection->contentLayout()->addWidget(codeArea);

    // 执行记录：图标、描述与箭头镜像，详情区的缩进与竖线位于右侧，扫光从右向左
    ShadCard* traceSection = addSection([] { return tr("执行记录"); }, [] { return tr("图标在右、描述在左，箭头朝左，详情区缩进在右"); });
    QWidget* traceArea = createRtlArea(this);
    QVBoxLayout* traceAreaLayout = new QVBoxLayout(traceArea);
    traceAreaLayout->setContentsMargins(0, 0, 0, 0);
    ShadTrace* rtlTrace = new ShadTrace(traceArea);
    ShadTraceCollapsible* rtlTurn = rtlTrace->addCollapsible(ShadIconType::Terminal, QStringLiteral("تم تشغيل أمرين، فشل أحدهما"));
    rtlTurn->getTriggerLine()->setStatus(ShadTraceType::Error);
    ShadTrace* rtlSteps = new ShadTrace(rtlTurn);
    rtlSteps->addLine(ShadIconType::CircleCheck, QStringLiteral("تم تشغيل pnpm install"))->setDescription(QStringLiteral("9s"));
    rtlSteps->addLine(ShadIconType::CircleAlert, QStringLiteral("فشل تشغيل pnpm lint"))->setStatus(ShadTraceType::Error);
    rtlSteps->addLine(ShadIconType::LoaderCircle, QStringLiteral("جارٍ تشغيل pnpm build"))->setStatus(ShadTraceType::Running);
    ShadTraceLine* rtlLink = rtlSteps->addLine(ShadIconType::Globe, QStringLiteral("تثبيت Astro | docs.astro.build"));
    rtlLink->setIndicatorIcon(ShadIconType::ArrowUpRight);
    rtlLink->setUrl(QUrl(QStringLiteral("https://docs.astro.build/en/install-and-setup/")));
    rtlTurn->contentLayout()->addWidget(rtlSteps);
    rtlTurn->addText(QStringLiteral("تم تثبيت الحزم، لكن فحص الشيفرة فشل بسبب خطأ في الإعدادات."));
    rtlTurn->setExpanded(true);
    rtlTrace->addLine(ShadIconType::FilePen, QStringLiteral("تم تعديل components/login-form.tsx"))->setDescription(QStringLiteral("+12 −3"));
    traceAreaLayout->addWidget(rtlTrace);
    traceSection->contentLayout()->addWidget(traceArea);

    // 标签页：标签从右向左排列，左右方向键对调；纵向时标签栏在右侧，Line 的纵向指示线在左侧
    ShadCard* tabsSection = addSection([] { return tr("标签页"); }, [] { return tr("标签从右向左排列，纵向时标签栏位于右侧"); });
    QWidget* tabsArea = createRtlArea(this);
    QVBoxLayout* tabsAreaLayout = new QVBoxLayout(tabsArea);
    tabsAreaLayout->setContentsMargins(0, 0, 0, 0);
    tabsAreaLayout->setSpacing(16);
    ShadTabs* rtlTabs = new ShadTabs(tabsArea);
    rtlTabs->addTab(QStringLiteral("الحساب"), new ShadCard(QStringLiteral("الحساب"), QStringLiteral("قم بإجراء تغييرات على حسابك هنا."), rtlTabs));
    rtlTabs->addTab(QStringLiteral("كلمة المرور"), new ShadCard(QStringLiteral("كلمة المرور"), QStringLiteral("قم بتغيير كلمة المرور الخاصة بك هنا."), rtlTabs));
    rtlTabs->addTab(QStringLiteral("الإشعارات"), new ShadCard(QStringLiteral("الإشعارات"), QStringLiteral("اختر طريقة تلقي الإشعارات."), rtlTabs));
    ShadTabs* rtlLineTabs = new ShadTabs(tabsArea);
    rtlLineTabs->setVariant(ShadTabsType::Line);
    rtlLineTabs->addTab(ShadIconType::AppWindow, QStringLiteral("معاينة"));
    rtlLineTabs->addTab(ShadIconType::Code, QStringLiteral("الشيفرة"));
    ShadTabs* rtlVerticalTabs = new ShadTabs(tabsArea);
    rtlVerticalTabs->setVariant(ShadTabsType::Line);
    rtlVerticalTabs->setOrientation(Qt::Vertical);
    rtlVerticalTabs->addTab(QStringLiteral("الحساب"));
    rtlVerticalTabs->addTab(QStringLiteral("كلمة المرور"));
    rtlVerticalTabs->addTab(QStringLiteral("الإشعارات"));
    // 第一个标签页占满整行，其余按内容宽度靠起始侧（AlignLeft 为逻辑对齐，RTL 下靠右）
    tabsAreaLayout->addWidget(rtlTabs);
    tabsAreaLayout->addWidget(rtlLineTabs, 0, Qt::AlignLeft);
    tabsAreaLayout->addWidget(rtlVerticalTabs, 0, Qt::AlignLeft);
    tabsSection->contentLayout()->addWidget(tabsArea);

    // 对话框：打开时同步父组件的 RTL 方向，标题栏与正文一起镜像，页脚按钮靠左
    ShadCard* dialogSection = addSection([] { return tr("对话框"); }, [] { return tr("跟随父组件方向，窗口按钮在左，标题与描述靠右"); });
    QWidget* dialogArea = createRtlArea(this);
    QVBoxLayout* dialogAreaLayout = new QVBoxLayout(dialogArea);
    dialogAreaLayout->setContentsMargins(0, 0, 0, 0);
    ShadButton* dialogButton = new ShadButton(QStringLiteral("فتح الحوار"), ShadButtonType::Outline, dialogArea);
    connect(dialogButton, &ShadButton::clicked, this, [dialogButton]() { ExampleLayout::openTransientDialog(createRtlDialog(dialogButton)); });
    dialogAreaLayout->addWidget(dialogButton, 0, Qt::AlignLeft);
    dialogSection->contentLayout()->addWidget(dialogArea);

    // 独立窗口：顶层窗口不继承父组件的布局方向，显式设置为从右到左
    ShadCard* windowSection = addSection([] { return tr("独立窗口"); }, [] { return tr("标题栏镜像，窗口按钮在左，内容从右向左排列"); });
    QWidget* windowArea = createRtlArea(this);
    QVBoxLayout* windowAreaLayout = new QVBoxLayout(windowArea);
    windowAreaLayout->setContentsMargins(0, 0, 0, 0);
    ShadButton* widgetButton = new ShadButton(QStringLiteral("فتح النافذة"), ShadButtonType::Outline, windowArea);
    connect(widgetButton, &ShadButton::clicked, this, [this]() { ExampleLayout::presentTransientWindow(_rtlWindow, createRtlWindow); });
    windowAreaLayout->addWidget(widgetButton, 0, Qt::AlignLeft);
    windowSection->contentLayout()->addWidget(windowArea);

    // 加载指示：条目中 Spinner 位于右侧（起始侧），金额位于左侧；旋转方向不变
    ShadCard* spinnerSection = addSection([] { return tr("加载指示"); }, [] { return tr("Spinner 在右、金额在左，旋转方向不变"); });
    QWidget* spinnerArea = createRtlArea(this);
    QVBoxLayout* spinnerAreaLayout = new QVBoxLayout(spinnerArea);
    spinnerAreaLayout->setContentsMargins(0, 0, 0, 0);
    ExampleMutedItem* rtlPaymentItem = new ExampleMutedItem(spinnerArea);
    ShadLabel* rtlPaymentTitle = new ShadLabel(QStringLiteral("جاري معالجة الدفع..."), rtlPaymentItem);
    QFont rtlPaymentFont = rtlPaymentTitle->font();
    rtlPaymentFont.setWeight(QFont::Medium);
    rtlPaymentTitle->setFont(rtlPaymentFont);
    ShadLabel* rtlPaymentAmount = new ShadLabel(QStringLiteral("١٠٠.٠٠ دولار"), rtlPaymentItem);
    rtlPaymentItem->itemLayout()->addWidget(new ShadSpinner(rtlPaymentItem));
    rtlPaymentItem->itemLayout()->addWidget(rtlPaymentTitle, 1);
    rtlPaymentItem->itemLayout()->addWidget(rtlPaymentAmount);
    spinnerAreaLayout->addWidget(rtlPaymentItem);
    spinnerSection->contentLayout()->addWidget(spinnerArea);

    // 二维码：RTL 下不镜像，镜像后的二维码无法识别；说明文字从右向左排列
    ShadCard* qrCodeSection = addSection([] { return tr("二维码"); }, [] { return tr("二维码不镜像，位于右侧，说明文字从右向左"); });
    QWidget* qrCodeArea = createRtlArea(this);
    QHBoxLayout* qrCodeAreaLayout = new QHBoxLayout(qrCodeArea);
    qrCodeAreaLayout->setContentsMargins(0, 0, 0, 0);
    qrCodeAreaLayout->setSpacing(16);
    ShadQrCode* rtlQrCode = new ShadQrCode(QStringLiteral("ShadcnWidgetTools"), qrCodeArea);
    ShadLabel* rtlQrCodeLabel = new ShadLabel(QStringLiteral("امسح الرمز لعرض النص."), qrCodeArea);
    rtlQrCodeLabel->setColorRole(ShadThemeType::MutedForeground);
    qrCodeAreaLayout->addWidget(rtlQrCode);
    qrCodeAreaLayout->addWidget(rtlQrCodeLabel, 0, Qt::AlignVCenter);
    qrCodeAreaLayout->addStretch(1);
    qrCodeSection->contentLayout()->addWidget(qrCodeArea);

    // 日期选择：阿拉伯语 locale，月名、星期名与日期数字本地化，日历网格从右向左排列
    ShadCard* datePickerSection = addSection([] { return tr("日期与时间选择"); }, [] { return tr("下拉箭头在左、浮层右对齐，日历从右向左排列"); });
    QWidget* datePickerArea = createRtlArea(this);
    QHBoxLayout* datePickerAreaLayout = new QHBoxLayout(datePickerArea);
    datePickerAreaLayout->setContentsMargins(0, 0, 0, 0);
    datePickerAreaLayout->setSpacing(16 - 2 * ExampleLayout::kFocusRingWidth);
    ShadDatePicker* rtlDatePicker = new ShadDatePicker(datePickerArea);
    rtlDatePicker->setLocale(QLocale(QLocale::Arabic, QLocale::SaudiArabia));
    rtlDatePicker->setPlaceholderText(QStringLiteral("اختر تاريخًا"));
    rtlDatePicker->setChevronVisible(true);
    rtlDatePicker->setFixedWidth(kRtlDatePickerWidth);
    // 时间选择：触发框镜像（时钟图标在右、下拉箭头在左），面板中的列仍按"时 分 秒"从左到右
    ShadTimePicker* rtlTimePicker = new ShadTimePicker(datePickerArea);
    rtlTimePicker->setPlaceholderText(QStringLiteral("اختر وقتًا"));
    rtlTimePicker->setClockIconVisible(true);
    rtlTimePicker->setChevronVisible(true);
    rtlTimePicker->setFixedWidth(kRtlTimePickerWidth);
    datePickerAreaLayout->addWidget(rtlDatePicker);
    datePickerAreaLayout->addWidget(rtlTimePicker);
    datePickerAreaLayout->addStretch(1);
    datePickerSection->contentLayout()->addWidget(datePickerArea);

    // 侧栏：起始侧在右，边框与 Rail 在左；子菜单竖线、徽章与箭头镜像；切换按钮图标朝右
    ShadCard* sidebarSection = addSection([] { return tr("侧栏"); }, [] { return tr("侧栏位于右侧，徽章、操作按钮与箭头在左侧"); });
    QWidget* rtlSidebarArea = createRtlArea(this);
    QVBoxLayout* rtlSidebarAreaLayout = new QVBoxLayout(rtlSidebarArea);
    rtlSidebarAreaLayout->setContentsMargins(0, 0, 0, 0);
    ShadSidebarLayout* rtlSidebarLayout = new ShadSidebarLayout(rtlSidebarArea);
    rtlSidebarLayout->setFixedHeight(kRtlSidebarHeight);
    rtlSidebarLayout->setShortcutEnabled(false);
    ShadSidebar* rtlSidebar = new ShadSidebar(rtlSidebarLayout);
    rtlSidebar->setCollapsible(ShadSidebarType::Icon);
    ShadSidebarGroup* rtlSidebarGroup = rtlSidebar->addGroup(QStringLiteral("المنصة"));
    ShadSidebarMenuButton* rtlPlayground = rtlSidebarGroup->addItem(ShadIconType::SquareTerminal, QStringLiteral("ساحة التجربة"));
    rtlPlayground->addSubItem(QStringLiteral("السجل"));
    rtlPlayground->addSubItem(QStringLiteral("المفضلة"));
    rtlPlayground->setSubMenuExpanded(true);
    rtlSidebarGroup->addItem(ShadIconType::Bot, QStringLiteral("النماذج"))->setBadgeText(QStringLiteral("٣"));
    rtlSidebarGroup->addItem(ShadIconType::BookOpen, QStringLiteral("التوثيق"))->setActionIcon(ShadIconType::Ellipsis);
    rtlSidebarLayout->setSidebar(rtlSidebar);
    QWidget* rtlSidebarContent = new QWidget(rtlSidebarLayout);
    QHBoxLayout* rtlSidebarContentLayout = new QHBoxLayout(rtlSidebarContent);
    rtlSidebarContentLayout->setContentsMargins(16, 16, 16, 16);
    rtlSidebarContentLayout->addWidget(new ShadSidebarTrigger(rtlSidebar, rtlSidebarContent), 0, Qt::AlignTop);
    rtlSidebarContentLayout->addStretch(1);
    rtlSidebarLayout->setContentWidget(rtlSidebarContent);
    rtlSidebarAreaLayout->addWidget(rtlSidebarLayout);
    sidebarSection->contentLayout()->addWidget(rtlSidebarArea);
}

RtlPage::~RtlPage()
{
    // 独立窗口没有父组件，不会随页面释放
    delete _rtlWindow.data();
}
