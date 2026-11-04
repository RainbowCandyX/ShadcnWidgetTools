#include "FieldPage.h"

#include <QButtonGroup>
#include <QChar>
#include <QHBoxLayout>
#include <QList>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <functional>
#include <initializer_list>
#include <memory>

#include "ExampleI18n.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadCheckbox.h"
#include "ShadComboBox.h"
#include "ShadDef.h"
#include "ShadField.h"
#include "ShadFieldGroup.h"
#include "ShadFieldSeparator.h"
#include "ShadFieldSet.h"
#include "ShadInput.h"
#include "ShadLabel.h"
#include "ShadRadioButton.h"
#include "ShadRadioGroup.h"
#include "ShadSlider.h"
#include "ShadSwitch.h"
#include "ShadTextarea.h"

namespace
{
    // max-w-md 与 max-w-sm
    constexpr int kWideFormWidth = 448;
    constexpr int kNarrowFormWidth = 384;
    // 嵌套在分组中的表单项组 gap-4，复选框组 gap-3
    constexpr int kNestedGroupSpacing = 16;
    constexpr int kCheckboxGroupSpacing = 12;
    constexpr int kMinPasswordLength = 8;

    // 最近一次提交时的校验结果，切换语言时据此重新生成错误信息
    struct ValidationResult
    {
        bool isSubmitted{false};
        bool isNameValid{true};
        bool isPasswordEmpty{false};
        bool isPasswordShort{false};
        bool hasDigit{true};
    };

    ShadInput* createInput(const ExampleI18n::TextFunction& placeholder, QWidget* parent)
    {
        ShadInput* input = new ShadInput(parent);
        ExampleI18n::bind(input, [input, placeholder]() { input->setPlaceholderText(placeholder()); });
        return input;
    }

    // 切换语言时按序号更新选项文字，保留当前选中项
    ShadComboBox* createComboBox(const ExampleI18n::TextFunction& placeholder, const std::function<QStringList()>& items, QWidget* parent)
    {
        ShadComboBox* comboBox = new ShadComboBox(parent);
        comboBox->addItems(items());
        comboBox->setCurrentIndex(-1);
        ExampleI18n::bind(comboBox, [comboBox, placeholder, items]()
        {
            comboBox->setPlaceholderText(placeholder());
            const QStringList texts = items();
            for (int index = 0; index < texts.size(); ++index)
            {
                comboBox->setItemText(index, texts.at(index));
            }
        });
        return comboBox;
    }

    // 横排、控件在起始侧的复选框表单项
    ShadField* createCheckboxField(const ExampleI18n::TextFunction& label, bool checked, QWidget* parent)
    {
        ShadCheckbox* checkbox = new ShadCheckbox(parent);
        checkbox->setChecked(checked);
        ShadField* field = new ShadField(QString(), checkbox, parent);
        field->setOrientation(ShadFieldType::Horizontal);
        field->setControlPosition(ShadFieldType::Start);
        ExampleI18n::bind(field, [field, label]() { field->setLabel(label()); });
        return field;
    }

    // 多个表单项横向并排，对应 grid grid-cols-*
    QWidget* createRow(const QList<QWidget*>& widgets, QWidget* parent)
    {
        QWidget* row = new QWidget(parent);
        QHBoxLayout* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(16);
        for (QWidget* widget : widgets)
        {
            layout->addWidget(widget, 1, Qt::AlignTop);
        }
        return row;
    }
} // namespace

FieldPage::FieldPage(QWidget* parent)
    : ExampleBasePage([] { return tr("表单项"); }, [] { return tr("ShadField / ShadFieldSet / ShadFieldGroup：把标签、控件、说明与错误信息组合成表单项。"); }, parent)
{
    // 基础用法：付款表单
    ShadCard* paymentSection = addSection([] { return tr("基础用法"); }, [] { return tr("多个字段集纵向排列，组之间用分隔线隔开"); });
    ShadFieldGroup* paymentForm = new ShadFieldGroup(this);
    paymentForm->setFixedWidth(kWideFormWidth);
    ShadFieldSet* paymentSet = new ShadFieldSet(paymentForm);
    ExampleI18n::bind(paymentSet, [paymentSet]()
    {
        paymentSet->setLegend(tr("付款方式"));
        paymentSet->setDescription(tr("所有交易均已加密，安全可靠。"));
    });
    ShadFieldGroup* paymentFields = new ShadFieldGroup(paymentSet);
    ShadField* cardNameField = new ShadField(QString(), createInput([] { return tr("张三"); }, paymentFields), paymentFields);
    ExampleI18n::bind(cardNameField, [cardNameField]() { cardNameField->setLabel(tr("持卡人姓名")); });
    paymentFields->addWidget(cardNameField);
    ShadField* cardNumberField = new ShadField(QString(), createInput([] { return QStringLiteral("1234 5678 9012 3456"); }, paymentFields), paymentFields);
    ExampleI18n::bind(cardNumberField, [cardNumberField]()
    {
        cardNumberField->setLabel(tr("卡号"));
        cardNumberField->setDescription(tr("请输入 16 位卡号。"));
    });
    paymentFields->addWidget(cardNumberField);
    QStringList months;
    for (int month = 1; month <= 12; ++month)
    {
        months.append(QStringLiteral("%1").arg(month, 2, 10, QLatin1Char('0')));
    }
    const QStringList years{QStringLiteral("2025"), QStringLiteral("2026"), QStringLiteral("2027"), QStringLiteral("2028"), QStringLiteral("2029"), QStringLiteral("2030")};
    ShadField* monthField = new ShadField(QString(), createComboBox([] { return QStringLiteral("MM"); }, [months] { return months; }, paymentFields), paymentFields);
    ShadField* yearField = new ShadField(QString(), createComboBox([] { return QStringLiteral("YYYY"); }, [years] { return years; }, paymentFields), paymentFields);
    ExampleI18n::bind(paymentFields, [monthField, yearField]()
    {
        monthField->setLabel(tr("月份"));
        yearField->setLabel(tr("年份"));
    });
    ShadField* cvvField = new ShadField(QStringLiteral("CVV"), createInput([] { return QStringLiteral("123"); }, paymentFields), paymentFields);
    paymentFields->addWidget(createRow({monthField, yearField, cvvField}, paymentFields));
    paymentSet->addWidget(paymentFields);
    paymentForm->addWidget(paymentSet);
    paymentForm->addSeparator();
    ShadFieldSet* billingSet = new ShadFieldSet(paymentForm);
    ExampleI18n::bind(billingSet, [billingSet]()
    {
        billingSet->setLegend(tr("账单地址"));
        billingSet->setDescription(tr("与付款方式关联的账单地址。"));
    });
    billingSet->addWidget(createCheckboxField([] { return tr("与收货地址相同"); }, true, billingSet));
    paymentForm->addWidget(billingSet);
    ShadFieldSet* commentSet = new ShadFieldSet(paymentForm);
    ShadTextarea* commentTextarea = new ShadTextarea(commentSet);
    ShadField* commentField = new ShadField(QString(), commentTextarea, commentSet);
    ExampleI18n::bind(commentField, [commentTextarea, commentField]()
    {
        commentTextarea->setPlaceholderText(tr("添加其他备注"));
        commentField->setLabel(tr("备注"));
    });
    commentSet->addWidget(commentField);
    paymentForm->addWidget(commentSet);
    QWidget* paymentButtons = new QWidget(paymentForm);
    QHBoxLayout* paymentButtonsLayout = new QHBoxLayout(paymentButtons);
    paymentButtonsLayout->setContentsMargins(0, 0, 0, 0);
    paymentButtonsLayout->setSpacing(8);
    ShadButton* paymentSubmitButton = new ShadButton(paymentButtons);
    ShadButton* paymentCancelButton = new ShadButton(QString(), ShadButtonType::Outline, paymentButtons);
    ExampleI18n::bind(paymentButtons, [paymentSubmitButton, paymentCancelButton]()
    {
        paymentSubmitButton->setText(tr("提交"));
        paymentCancelButton->setText(tr("取消"));
    });
    paymentButtonsLayout->addWidget(paymentSubmitButton);
    paymentButtonsLayout->addWidget(paymentCancelButton);
    paymentButtonsLayout->addStretch(1);
    paymentForm->addWidget(paymentButtons);
    addItem(paymentSection, paymentForm);

    // 输入框
    ShadCard* inputSection = addSection([] { return tr("输入框"); }, [] { return tr("在控件下方显示说明文字"); });
    ShadFieldGroup* inputGroup = new ShadFieldGroup(this);
    inputGroup->setFixedWidth(kNarrowFormWidth);
    ShadField* usernameField = new ShadField(QString(), createInput([] { return QStringLiteral("ShadcnWidgetTools"); }, inputGroup), inputGroup);
    ShadInput* passwordInput = createInput([] { return tr("请输入密码"); }, inputGroup);
    passwordInput->setTrailingAction(ShadInputType::PasswordToggleAction);
    ShadField* passwordField = new ShadField(QString(), passwordInput, inputGroup);
    ExampleI18n::bind(inputGroup, [usernameField, passwordField]()
    {
        usernameField->setLabel(tr("用户名"));
        usernameField->setDescription(tr("请选择一个唯一的用户名。"));
        passwordField->setLabel(tr("密码"));
        passwordField->setDescription(tr("至少 8 个字符。"));
    });
    inputGroup->addWidget(usernameField);
    inputGroup->addWidget(passwordField);
    addItem(inputSection, inputGroup);

    // 多行输入
    ShadCard* textareaSection = addSection([] { return tr("多行输入"); }, [] { return tr("控件为 ShadTextarea"); });
    ShadTextarea* feedbackTextarea = new ShadTextarea(this);
    ShadField* feedbackField = new ShadField(QString(), feedbackTextarea, this);
    ExampleI18n::bind(feedbackField, [feedbackTextarea, feedbackField]()
    {
        feedbackTextarea->setPlaceholderText(tr("你的反馈能帮助我们改进……"));
        feedbackField->setLabel(tr("反馈"));
        feedbackField->setDescription(tr("告诉我们你的想法。"));
    });
    feedbackField->setFixedWidth(kNarrowFormWidth);
    addItem(textareaSection, feedbackField);

    // 下拉选择
    ShadCard* selectSection = addSection([] { return tr("下拉选择"); }, [] { return tr("控件为 ShadComboBox"); });
    ShadComboBox* departmentComboBox = createComboBox([] { return tr("选择部门"); }, [] { return QStringList{tr("工程"), tr("设计"), tr("市场"), tr("销售"), tr("客服")}; }, this);
    ShadField* departmentField = new ShadField(QString(), departmentComboBox, this);
    ExampleI18n::bind(departmentField, [departmentField]()
    {
        departmentField->setLabel(tr("部门"));
        departmentField->setDescription(tr("选择你所在的部门。"));
    });
    departmentField->setFixedWidth(kNarrowFormWidth);
    addItem(selectSection, departmentField);

    // 滑块
    ShadCard* sliderSection = addSection([] { return tr("滑块"); }, [] { return tr("控件为范围滑块，说明文字随取值更新"); });
    ShadSlider* priceSlider = new ShadSlider(Qt::Horizontal, this);
    priceSlider->setRange(0, 1000);
    priceSlider->setSingleStep(10);
    priceSlider->setRangeMode(true);
    priceSlider->setRangeValues(200, 800);
    ShadField* priceField = new ShadField(QString(), priceSlider, this);
    priceField->setFixedWidth(kNarrowFormWidth);
    const auto updatePriceDescription = [priceField, priceSlider]() { priceField->setDescription(tr("预算范围：¥%1 - ¥%2").arg(priceSlider->getLowerValue()).arg(priceSlider->value())); };
    connect(priceSlider, &ShadSlider::rangeValuesChanged, priceField, updatePriceDescription);
    ExampleI18n::bind(priceField, [priceField, updatePriceDescription]()
    {
        priceField->setLabel(tr("价格范围"));
        updatePriceDescription();
    });
    addItem(sliderSection, priceField);

    // 分组
    ShadCard* fieldSetSection = addSection([] { return tr("分组"); }, [] { return tr("把相关的表单项放在同一个标题下"); });
    ShadFieldSet* addressSet = new ShadFieldSet(this);
    addressSet->setFixedWidth(kWideFormWidth);
    ShadFieldGroup* addressFields = new ShadFieldGroup(addressSet);
    addressFields->setSpacing(kNestedGroupSpacing);
    ShadField* streetField = new ShadField(QString(), createInput([] { return tr("人民路 123 号"); }, addressFields), addressFields);
    ShadField* cityField = new ShadField(QString(), createInput([] { return tr("上海"); }, addressFields), addressFields);
    ShadField* zipField = new ShadField(QString(), createInput([] { return QStringLiteral("200000"); }, addressFields), addressFields);
    ExampleI18n::bind(addressSet, [addressSet, streetField, cityField, zipField]()
    {
        addressSet->setLegend(tr("地址信息"));
        addressSet->setDescription(tr("我们需要你的地址来配送订单。"));
        streetField->setLabel(tr("街道地址"));
        cityField->setLabel(tr("城市"));
        zipField->setLabel(tr("邮编"));
    });
    addressFields->addWidget(streetField);
    addressFields->addWidget(createRow({cityField, zipField}, addressFields));
    addressSet->addWidget(addressFields);
    addItem(fieldSetSection, addressSet);

    // 复选框
    ShadCard* checkboxSection = addSection([] { return tr("复选框"); }, [] { return tr("横排，复选框位于起始侧"); });
    ShadFieldGroup* checkboxForm = new ShadFieldGroup(this);
    checkboxForm->setFixedWidth(kNarrowFormWidth);
    ShadFieldSet* desktopSet = new ShadFieldSet(checkboxForm);
    ExampleI18n::bind(desktopSet, [desktopSet]()
    {
        desktopSet->setLegend(tr("在桌面上显示这些项目"));
        desktopSet->setDescription(tr("选择要在桌面上显示的项目。"));
    });
    desktopSet->setLegendVariant(ShadFieldType::Label);
    ShadFieldGroup* desktopItems = new ShadFieldGroup(desktopSet);
    desktopItems->setSpacing(kCheckboxGroupSpacing);
    desktopItems->addWidget(createCheckboxField([] { return tr("硬盘"); }, true, desktopItems));
    desktopItems->addWidget(createCheckboxField([] { return tr("外置磁盘"); }, true, desktopItems));
    desktopItems->addWidget(createCheckboxField([] { return tr("CD、DVD 与 iPod"); }, false, desktopItems));
    desktopItems->addWidget(createCheckboxField([] { return tr("已连接的服务器"); }, false, desktopItems));
    desktopSet->addWidget(desktopItems);
    checkboxForm->addWidget(desktopSet);
    checkboxForm->addSeparator();
    ShadField* syncField = createCheckboxField([] { return tr("同步桌面与文稿文件夹"); }, true, checkboxForm);
    ExampleI18n::bind(syncField, [syncField]() { syncField->setDescription(tr("桌面与文稿文件夹会同步到云端，可以在其他设备上访问。")); });
    checkboxForm->addWidget(syncField);
    addItem(checkboxSection, checkboxForm);

    // 单选组
    ShadCard* radioSection = addSection([] { return tr("单选组"); }, [] { return tr("ShadFieldSet 中直接放 ShadRadioGroup"); });
    ShadFieldSet* planSet = new ShadFieldSet(this);
    planSet->setLegendVariant(ShadFieldType::Label);
    planSet->setFixedWidth(kNarrowFormWidth);
    ShadRadioGroup* planGroup = new ShadRadioGroup(planSet);
    ShadRadioButton* monthlyPlan = planGroup->addItem(QString());
    ShadRadioButton* yearlyPlan = planGroup->addItem(QString());
    ShadRadioButton* lifetimePlan = planGroup->addItem(QString());
    ExampleI18n::bind(planSet, [planSet, monthlyPlan, yearlyPlan, lifetimePlan]()
    {
        planSet->setLegend(tr("订阅方案"));
        planSet->setDescription(tr("年付与终身方案可享受大幅折扣。"));
        monthlyPlan->setText(tr("月付（¥9/月）"));
        yearlyPlan->setText(tr("年付（¥99/年）"));
        lifetimePlan->setText(tr("终身（¥299）"));
    });
    planGroup->setCurrentIndex(0);
    planSet->addWidget(planGroup);
    addItem(radioSection, planSet);

    // 开关
    ShadCard* switchSection = addSection([] { return tr("开关"); }, [] { return tr("横排，开关位于结束侧"); });
    ShadField* mfaField = new ShadField(QString(), new ShadSwitch(this), this);
    mfaField->setOrientation(ShadFieldType::Horizontal);
    ExampleI18n::bind(mfaField, [mfaField]()
    {
        mfaField->setLabel(tr("多重身份验证"));
        mfaField->setDescription(tr("开启后需要通过验证码或安全密钥完成登录。"));
    });
    mfaField->setFixedWidth(kWideFormWidth);
    addItem(switchSection, mfaField);

    // 选择卡片
    ShadCard* choiceSection = addSection([] { return tr("选择卡片"); }, [] { return tr("setChoiceCard(true)：整个表单项为卡片，点击任意位置切换"); });
    ShadFieldSet* computeSet = new ShadFieldSet(this);
    ExampleI18n::bind(computeSet, [computeSet]()
    {
        computeSet->setLegend(tr("计算环境"));
        computeSet->setDescription(tr("选择集群的计算环境。"));
    });
    computeSet->setLegendVariant(ShadFieldType::Label);
    computeSet->setFixedWidth(kWideFormWidth);
    computeSet->contentLayout()->setSpacing(12);
    QButtonGroup* computeButtons = new QButtonGroup(computeSet);
    struct ChoiceDemo
    {
        ExampleI18n::TextFunction title;
        ExampleI18n::TextFunction description;
    };
    const QList<ChoiceDemo> choiceDemos{
        {[] { return QStringLiteral("Kubernetes"); }, [] { return tr("在 Kubernetes 集群上运行 GPU 工作负载。"); }},
        {[] { return tr("虚拟机"); }, [] { return tr("使用虚拟机集群运行 GPU 工作负载（即将推出）。"); }},
    };
    for (int index = 0; index < choiceDemos.size(); ++index)
    {
        ShadRadioButton* radio = new ShadRadioButton(computeSet);
        computeButtons->addButton(radio);
        radio->setChecked(index == 0);
        ShadField* choiceField = new ShadField(QString(), radio, computeSet);
        choiceField->setOrientation(ShadFieldType::Horizontal);
        const ChoiceDemo demo = choiceDemos.at(index);
        ExampleI18n::bind(choiceField, [choiceField, demo]()
        {
            choiceField->setLabel(demo.title());
            choiceField->setDescription(demo.description());
        });
        choiceField->setChoiceCard(true);
        computeSet->addWidget(choiceField);
    }
    addItem(choiceSection, computeSet);

    // 字段组与分隔线
    ShadCard* groupSection = addSection([] { return tr("字段组与分隔线"); }, [] { return tr("在两组之间插入分隔线，可以带文字"); });
    ShadFieldGroup* notificationForm = new ShadFieldGroup(this);
    notificationForm->setFixedWidth(kNarrowFormWidth);
    // 标题与说明各自完整翻译，避免拼接句子
    struct NotificationDemo
    {
        ExampleI18n::TextFunction legend;
        ExampleI18n::TextFunction description;
    };
    const QList<NotificationDemo> notificationDemos{
        {[] { return tr("回复与评论"); }, [] { return tr("选择接收回复与评论通知的方式。"); }},
        {[] { return tr("任务"); }, [] { return tr("选择接收任务通知的方式。"); }},
    };
    bool isFirstSet = true;
    for (const NotificationDemo& demo : notificationDemos)
    {
        ShadFieldSet* notificationSet = new ShadFieldSet(notificationForm);
        ExampleI18n::bind(notificationSet, [notificationSet, demo]()
        {
            notificationSet->setLegend(demo.legend());
            notificationSet->setDescription(demo.description());
        });
        notificationSet->setLegendVariant(ShadFieldType::Label);
        ShadFieldGroup* options = new ShadFieldGroup(notificationSet);
        options->setSpacing(kCheckboxGroupSpacing);
        options->addWidget(createCheckboxField([] { return tr("推送通知"); }, true, options));
        options->addWidget(createCheckboxField([] { return tr("邮件通知"); }, false, options));
        notificationSet->addWidget(options);
        if (!isFirstSet)
        {
            ShadFieldSeparator* separator = notificationForm->addSeparator();
            ExampleI18n::bind(separator, [separator]() { separator->setText(tr("以及")); });
        }
        notificationForm->addWidget(notificationSet);
        isFirstSet = false;
    }
    addItem(groupSection, notificationForm);

    // 三种方向
    ShadCard* orientationSection = addSection([] { return tr("三种方向"); }, [] { return tr("竖排、横排，以及按宽度自动切换的响应式"); });
    ShadField* verticalField = new ShadField(QString(), createInput([] { return tr("请输入姓名"); }, this), this);
    verticalField->setFixedWidth(kNarrowFormWidth);
    ShadField* horizontalField = new ShadField(QString(), createInput([] { return tr("请输入姓名"); }, this), this);
    horizontalField->setOrientation(ShadFieldType::Horizontal);
    horizontalField->setFixedWidth(560);
    ShadField* wideResponsiveField = new ShadField(QString(), createInput([] { return tr("请输入姓名"); }, this), this);
    wideResponsiveField->setOrientation(ShadFieldType::Responsive);
    wideResponsiveField->setFixedWidth(560);
    ShadField* narrowResponsiveField = new ShadField(QString(), createInput([] { return tr("请输入姓名"); }, this), this);
    narrowResponsiveField->setOrientation(ShadFieldType::Responsive);
    narrowResponsiveField->setFixedWidth(320);
    ExampleI18n::bind(this, [verticalField, horizontalField, wideResponsiveField, narrowResponsiveField]()
    {
        for (ShadField* field : {verticalField, horizontalField, wideResponsiveField, narrowResponsiveField})
        {
            field->setLabel(tr("姓名"));
        }
        verticalField->setDescription(tr("Vertical：标签、控件、说明依次排列。"));
        horizontalField->setDescription(tr("Horizontal：文字与控件并排。"));
        wideResponsiveField->setDescription(tr("Responsive，宽度 560：横排。"));
        narrowResponsiveField->setDescription(tr("Responsive，宽度 320：竖排。"));
    });
    for (ShadField* field : {verticalField, horizontalField, wideResponsiveField, narrowResponsiveField})
    {
        addItem(orientationSection, field);
    }

    // 校验与错误
    ShadCard* validationSection = addSection([] { return tr("校验与错误"); }, [] { return tr("出错时标签与控件变色，并显示错误信息"); });
    ShadFieldGroup* validationForm = new ShadFieldGroup(this);
    validationForm->setFixedWidth(kNarrowFormWidth);
    ShadInput* nameInput = createInput([] { return QStringLiteral("ShadcnWidgetTools"); }, validationForm);
    ShadField* nameField = new ShadField(QString(), nameInput, validationForm);
    ShadInput* secretInput = createInput([] { return tr("请输入密码"); }, validationForm);
    secretInput->setTrailingAction(ShadInputType::PasswordToggleAction);
    ShadField* secretField = new ShadField(QString(), secretInput, validationForm);
    QWidget* validationButtons = new QWidget(validationForm);
    QHBoxLayout* validationButtonsLayout = new QHBoxLayout(validationButtons);
    validationButtonsLayout->setContentsMargins(0, 0, 0, 0);
    ShadButton* submitButton = new ShadButton(validationButtons);
    ShadLabel* resultLabel = new ShadLabel(validationButtons);
    resultLabel->setColorRole(ShadThemeType::MutedForeground);
    validationButtonsLayout->addWidget(submitButton);
    validationButtonsLayout->addWidget(resultLabel, 1);
    validationForm->addWidget(nameField);
    validationForm->addWidget(secretField);
    validationForm->addWidget(validationButtons);
    std::shared_ptr<ValidationResult> validation = std::make_shared<ValidationResult>();
    // 按最近一次提交的校验结果显示错误与结果文字，未提交时不显示
    const auto showValidation = [nameField, secretField, resultLabel, validation]()
    {
        if (!validation->isSubmitted)
        {
            return;
        }
        nameField->setInvalid(!validation->isNameValid);
        nameField->setError(validation->isNameValid ? QString() : tr("请输入用户名。"));
        QStringList passwordErrors;
        if (validation->isPasswordEmpty)
        {
            passwordErrors.append(tr("请输入密码。"));
        }
        else
        {
            if (validation->isPasswordShort)
            {
                passwordErrors.append(tr("密码至少需要 8 个字符。"));
            }
            if (!validation->hasDigit)
            {
                passwordErrors.append(tr("密码需要包含至少一个数字。"));
            }
        }
        secretField->setInvalid(!passwordErrors.isEmpty());
        secretField->setErrors(passwordErrors);
        resultLabel->setText(validation->isNameValid && passwordErrors.isEmpty() ? tr("提交成功。") : QString());
    };
    connect(submitButton, &ShadButton::clicked, this, [nameInput, secretInput, validation, showValidation]()
    {
        const QString password = secretInput->text();
        bool hasDigit = false;
        for (const QChar character : password)
        {
            hasDigit = hasDigit || character.isDigit();
        }
        validation->isSubmitted = true;
        validation->isNameValid = !nameInput->text().trimmed().isEmpty();
        validation->isPasswordEmpty = password.isEmpty();
        validation->isPasswordShort = password.size() < kMinPasswordLength;
        validation->hasDigit = hasDigit;
        showValidation();
    });
    ExampleI18n::bind(validationForm, [nameField, secretField, submitButton, showValidation]()
    {
        nameField->setLabel(tr("用户名"));
        secretField->setLabel(tr("密码"));
        secretField->setDescription(tr("至少 8 个字符，且包含数字。"));
        submitButton->setText(tr("提交"));
        showValidation();
    });
    addItem(validationSection, validationForm);
}

FieldPage::~FieldPage()
{
}
