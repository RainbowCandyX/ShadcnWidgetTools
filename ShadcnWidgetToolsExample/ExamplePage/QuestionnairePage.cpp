#include "QuestionnairePage.h"

#include <QCoreApplication>
#include <QHash>
#include <QLatin1String>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>
#include <QtGlobal>

#include <memory>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadDialog.h"
#include "ShadLabel.h"
#include "ShadProgress.h"
#include "ShadQuestionnaire.h"

namespace
{
    constexpr int kQuestionnaireWidth = 448;
    constexpr int kMaxModules = 2;

    // 题目名与选项值使用与语言无关的 id，切换语言后仍能按 id 查找；结果文字中按当前语言显示
    struct DisplayText
    {
        const char* id;
        const char* text;
    };

    const DisplayText kDisplayTexts[] = {
        {"direction", QT_TRANSLATE_NOOP("QuestionnairePage", "方向")},
        {"timeline", QT_TRANSLATE_NOOP("QuestionnairePage", "时间线")},
        {"approval", QT_TRANSLATE_NOOP("QuestionnairePage", "审批")},
        {"handoff", QT_TRANSLATE_NOOP("QuestionnairePage", "交接")},
        {"report", QT_TRANSLATE_NOOP("QuestionnairePage", "汇报")},
        {"progress", QT_TRANSLATE_NOOP("QuestionnairePage", "进度")},
        {"decision", QT_TRANSLATE_NOOP("QuestionnairePage", "决策")},
        {"risk", QT_TRANSLATE_NOOP("QuestionnairePage", "风险")},
        {"nextStep", QT_TRANSLATE_NOOP("QuestionnairePage", "下一步")},
        {"time", QT_TRANSLATE_NOOP("QuestionnairePage", "时间")},
        {"now", QT_TRANSLATE_NOOP("QuestionnairePage", "现在")},
        {"nextCycle", QT_TRANSLATE_NOOP("QuestionnairePage", "下个周期")},
        {"backlog", QT_TRANSLATE_NOOP("QuestionnairePage", "待办")},
        {"platform", QT_TRANSLATE_NOOP("QuestionnairePage", "平台")},
        {"suggestion", QT_TRANSLATE_NOOP("QuestionnairePage", "建议")},
        {"role", QT_TRANSLATE_NOOP("QuestionnairePage", "角色")},
        {"engineering", QT_TRANSLATE_NOOP("QuestionnairePage", "开发")},
        {"design", QT_TRANSLATE_NOOP("QuestionnairePage", "设计")},
        {"product", QT_TRANSLATE_NOOP("QuestionnairePage", "产品")},
        {"years", QT_TRANSLATE_NOOP("QuestionnairePage", "年限")},
        {"rating", QT_TRANSLATE_NOOP("QuestionnairePage", "评分")},
        {"good", QT_TRANSLATE_NOOP("QuestionnairePage", "好")},
        {"okay", QT_TRANSLATE_NOOP("QuestionnairePage", "一般")},
        {"poor", QT_TRANSLATE_NOOP("QuestionnairePage", "差")},
        {"modules", QT_TRANSLATE_NOOP("QuestionnairePage", "模块")},
        {"account", QT_TRANSLATE_NOOP("QuestionnairePage", "账号")},
        {"payment", QT_TRANSLATE_NOOP("QuestionnairePage", "支付")},
        {"notification", QT_TRANSLATE_NOOP("QuestionnairePage", "通知")},
        {"reporting", QT_TRANSLATE_NOOP("QuestionnairePage", "报表")},
        {"invoice", QT_TRANSLATE_NOOP("QuestionnairePage", "发票")},
        {"need", QT_TRANSLATE_NOOP("QuestionnairePage", "需要")},
        {"noNeed", QT_TRANSLATE_NOOP("QuestionnairePage", "不需要")},
        {"invoiceTitle", QT_TRANSLATE_NOOP("QuestionnairePage", "抬头")},
        {"note", QT_TRANSLATE_NOOP("QuestionnairePage", "备注")},
    };

    QString displayText(const QString& id)
    {
        for (const DisplayText& entry : kDisplayTexts)
        {
            if (id == QLatin1String(entry.id))
            {
                return QCoreApplication::translate("QuestionnairePage", entry.text);
            }
        }
        // 输入框中的回答与没有显示文字的选项值原样显示
        return id;
    }

    // 最近一次提交的答案，切换语言时据此重新生成结果文字
    struct SubmittedResult
    {
        bool isSubmitted{false};
        QStringList names;
        QHash<QString, QStringList> answers;
    };

    ShadQuestionnaire* createQuestionnaire(QWidget* parent)
    {
        ShadQuestionnaire* questionnaire = new ShadQuestionnaire(parent);
        questionnaire->setMaximumWidth(kQuestionnaireWidth);
        return questionnaire;
    }

    void updateResultText(ShadLabel* label, const SubmittedResult& result)
    {
        if (!result.isSubmitted)
        {
            label->setText(QCoreApplication::translate("QuestionnairePage", "尚未提交"));
            return;
        }
        QStringList parts;
        for (const QString& name : result.names)
        {
            QStringList values;
            for (const QString& value : result.answers.value(name))
            {
                values.append(displayText(value));
            }
            parts.append(QCoreApplication::translate("QuestionnairePage", "%1：%2").arg(displayText(name), values.join(QCoreApplication::translate("QuestionnairePage", "、"))));
        }
        label->setText(QCoreApplication::translate("QuestionnairePage", "已提交 %1").arg(parts.join(QCoreApplication::translate("QuestionnairePage", "；"))));
    }

    ShadLabel* createResultLabel(QWidget* parent, const std::shared_ptr<SubmittedResult>& result)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        label->setWordWrap(true);
        ExampleI18n::bind(label, [label, result]() { updateResultText(label, *result); });
        return label;
    }

    void showResult(ShadQuestionnaire* questionnaire, ShadLabel* label, const std::shared_ptr<SubmittedResult>& result)
    {
        QObject::connect(questionnaire, &ShadQuestionnaire::submitted, label, [questionnaire, label, result]()
        {
            result->isSubmitted = true;
            result->names = questionnaire->getItems();
            result->answers = questionnaire->getAllAnswers();
            updateResultText(label, *result);
        });
    }

    // 创建结果标签并在问卷提交时更新
    ShadLabel* createResultFor(ShadQuestionnaire* questionnaire, QWidget* parent)
    {
        std::shared_ptr<SubmittedResult> result = std::make_shared<SubmittedResult>();
        ShadLabel* label = createResultLabel(parent, result);
        showResult(questionnaire, label, result);
        return label;
    }

    // 添加题目，标题与说明随语言刷新；description 为空时不设置说明
    void addQuestion(ShadQuestionnaire* questionnaire, const QString& name, const ExampleI18n::TextFunction& title, const ExampleI18n::TextFunction& description = ExampleI18n::TextFunction())
    {
        questionnaire->addItem(name, title(), description ? description() : QString());
        ExampleI18n::bind(questionnaire, [questionnaire, name, title, description]()
        {
            questionnaire->setItemTitle(name, title());
            if (description)
            {
                questionnaire->setItemDescription(name, description());
            }
        });
    }

    // 添加选项，文字与说明随语言刷新；description 为空时不设置说明
    void addOption(ShadQuestionnaire* questionnaire, const QString& name, const QString& value, const ExampleI18n::TextFunction& label, const ExampleI18n::TextFunction& description = ExampleI18n::TextFunction())
    {
        questionnaire->addChoice(name, value, label(), description ? description() : QString());
        ExampleI18n::bind(questionnaire, [questionnaire, name, value, label, description]()
        {
            questionnaire->setChoiceLabel(name, value, label());
            if (description)
            {
                questionnaire->setChoiceDescription(name, value, description());
            }
        });
    }

    void addProjectItems(ShadQuestionnaire* questionnaire)
    {
        const QString direction = QStringLiteral("direction");
        const QString report = QStringLiteral("report");
        const QString time = QStringLiteral("time");
        addQuestion(questionnaire, direction, [] { return QCoreApplication::translate("QuestionnairePage", "接下来先做什么？"); }, [] { return QCoreApplication::translate("QuestionnairePage", "也可以写下其他想法。"); });
        questionnaire->setItemRequired(direction, true);
        addOption(questionnaire, direction, QStringLiteral("timeline"), [] { return QCoreApplication::translate("QuestionnairePage", "工具调用时间线"); }, [] { return QCoreApplication::translate("QuestionnairePage", "展示操作与结果。"); });
        addOption(questionnaire, direction, QStringLiteral("approval"), [] { return QCoreApplication::translate("QuestionnairePage", "审批节点"); }, [] { return QCoreApplication::translate("QuestionnairePage", "敏感操作前确认。"); });
        addOption(questionnaire, direction, QStringLiteral("handoff"), [] { return QCoreApplication::translate("QuestionnairePage", "任务交接"); }, [] { return QCoreApplication::translate("QuestionnairePage", "跟踪委派的工作。"); });
        ExampleI18n::bind(questionnaire, [questionnaire, direction]() { questionnaire->setItemInput(direction, QCoreApplication::translate("QuestionnairePage", "其他想法…")); });

        addQuestion(questionnaire, report, [] { return QCoreApplication::translate("QuestionnairePage", "汇报包含哪些内容？"); }, [] { return QCoreApplication::translate("QuestionnairePage", "可多选。"); });
        questionnaire->setItemMultiple(report, true);
        addOption(questionnaire, report, QStringLiteral("progress"), [] { return QCoreApplication::translate("QuestionnairePage", "进度"); });
        addOption(questionnaire, report, QStringLiteral("decision"), [] { return QCoreApplication::translate("QuestionnairePage", "决策"); });
        addOption(questionnaire, report, QStringLiteral("risk"), [] { return QCoreApplication::translate("QuestionnairePage", "风险"); });
        addOption(questionnaire, report, QStringLiteral("nextStep"), [] { return QCoreApplication::translate("QuestionnairePage", "下一步"); });

        addQuestion(questionnaire, time, [] { return QCoreApplication::translate("QuestionnairePage", "什么时候开始？"); });
        questionnaire->setItemRequired(time, true);
        addOption(questionnaire, time, QStringLiteral("now"), [] { return QCoreApplication::translate("QuestionnairePage", "现在开始"); });
        addOption(questionnaire, time, QStringLiteral("nextCycle"), [] { return QCoreApplication::translate("QuestionnairePage", "下个周期"); });
        addOption(questionnaire, time, QStringLiteral("backlog"), [] { return QCoreApplication::translate("QuestionnairePage", "放进待办"); });
    }
} // namespace

QuestionnairePage::QuestionnairePage(QWidget* parent)
    : ExampleBasePage([] { return tr("问卷"); }, [] { return tr("ShadQuestionnaire：多步问卷。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("按字母键快速选择"); });
    ShadQuestionnaire* basic = createQuestionnaire(this);
    addProjectItems(basic);
    basic->setShortcuts(ShadQuestionnaireType::Letters);
    ExampleI18n::bind(basic, [basic]() { basic->setSubmitText(tr("保存计划")); });
    addStretchItem(basicCard, basic);
    addStretchItem(basicCard, createResultFor(basic, this));

    ShadCard* multipleCard = addSection([] { return tr("多选"); }, [] { return QStringLiteral("setItemMultiple"); });
    ShadQuestionnaire* multiple = createQuestionnaire(this);
    const QString platform = QStringLiteral("platform");
    addQuestion(multiple, platform, [] { return tr("需要支持哪些平台？"); }, [] { return tr("至少选一项。"); });
    multiple->setItemRequired(platform, true);
    multiple->setItemMultiple(platform, true);
    multiple->addChoice(platform, QStringLiteral("windows"), QStringLiteral("Windows"));
    multiple->addChoice(platform, QStringLiteral("macos"), QStringLiteral("macOS"));
    multiple->addChoice(platform, QStringLiteral("linux"), QStringLiteral("Linux"));
    addStretchItem(multipleCard, multiple);
    addStretchItem(multipleCard, createResultFor(multiple, this));

    ShadCard* freeformCard = addSection([] { return tr("自由输入"); }, [] { return QStringLiteral("setItemInput"); });
    ShadQuestionnaire* freeform = createQuestionnaire(this);
    const QString suggestion = QStringLiteral("suggestion");
    addQuestion(freeform, suggestion, [] { return tr("还有什么建议？"); });
    freeform->setItemRequired(suggestion, true);
    ExampleI18n::bind(freeform, [freeform, suggestion]() { freeform->setItemInput(suggestion, tr("写下你的建议…")); });
    addStretchItem(freeformCard, freeform);
    addStretchItem(freeformCard, createResultFor(freeform, this));

    ShadCard* skipCard = addSection([] { return tr("跳过"); }, [] { return tr("非必答题可跳过"); });
    ShadQuestionnaire* skippable = createQuestionnaire(this);
    const QString role = QStringLiteral("role");
    const QString years = QStringLiteral("years");
    addQuestion(skippable, role, [] { return tr("你的角色是？"); }, [] { return tr("可以跳过。"); });
    addOption(skippable, role, QStringLiteral("engineering"), [] { return tr("开发"); });
    addOption(skippable, role, QStringLiteral("design"), [] { return tr("设计"); });
    addOption(skippable, role, QStringLiteral("product"), [] { return tr("产品"); });
    addQuestion(skippable, years, [] { return tr("工作几年了？"); });
    addOption(skippable, years, QStringLiteral("1"), [] { return tr("1 年以内"); });
    addOption(skippable, years, QStringLiteral("3"), [] { return tr("1 到 3 年"); });
    addOption(skippable, years, QStringLiteral("5"), [] { return tr("3 年以上"); });
    addStretchItem(skipCard, skippable);
    addStretchItem(skipCard, createResultFor(skippable, this));

    ShadCard* shortcutCard = addSection([] { return tr("数字快捷键"); }, [] { return tr("按数字键选择"); });
    ShadQuestionnaire* numbers = createQuestionnaire(this);
    numbers->setShortcuts(ShadQuestionnaireType::Numbers);
    const QString rating = QStringLiteral("rating");
    addQuestion(numbers, rating, [] { return tr("整体体验如何？"); });
    numbers->setItemRequired(rating, true);
    addOption(numbers, rating, QStringLiteral("good"), [] { return tr("很好"); });
    addOption(numbers, rating, QStringLiteral("okay"), [] { return tr("一般"); });
    addOption(numbers, rating, QStringLiteral("poor"), [] { return tr("不太好"); });
    addStretchItem(shortcutCard, numbers);
    addStretchItem(shortcutCard, createResultFor(numbers, this));

    ShadCard* validationCard = addSection([] { return tr("自定义校验"); }, [] { return QStringLiteral("setItemError"); });
    ShadQuestionnaire* validated = createQuestionnaire(this);
    const QString modules = QStringLiteral("modules");
    addQuestion(validated, modules, [] { return tr("先启用哪些模块？"); }, [] { return tr("最多选两项。"); });
    validated->setItemRequired(modules, true);
    validated->setItemMultiple(modules, true);
    addOption(validated, modules, QStringLiteral("account"), [] { return tr("账号"); });
    addOption(validated, modules, QStringLiteral("payment"), [] { return tr("支付"); });
    addOption(validated, modules, QStringLiteral("notification"), [] { return tr("通知"); });
    addOption(validated, modules, QStringLiteral("reporting"), [] { return tr("报表"); });
    // 按当前答案设置错误信息，切换语言时也重新设置
    const auto updateModuleError = [validated, modules]()
    {
        const bool isTooMany = validated->getAnswers(modules).size() > kMaxModules;
        validated->setItemError(modules, isTooMany ? tr("最多选两项。") : QString());
    };
    connect(validated, &ShadQuestionnaire::answersChanged, validated, [modules, updateModuleError](const QString& name)
    {
        if (name == modules)
        {
            updateModuleError();
        }
    });
    ExampleI18n::bind(validated, updateModuleError);
    addStretchItem(validationCard, validated);
    addStretchItem(validationCard, createResultFor(validated, this));

    ShadCard* controlCard = addSection([] { return tr("外部控制"); }, [] { return tr("切换、恢复与重置"); });
    ShadQuestionnaire* controlled = createQuestionnaire(this);
    addProjectItems(controlled);
    ShadButton* jumpButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(jumpButton, &ShadButton::clicked, controlled, [controlled]() { controlled->setCurrentItem(QStringLiteral("time")); });
    ShadButton* restoreButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(restoreButton, &ShadButton::clicked, controlled, [controlled]()
    {
        controlled->setAnswers(QStringLiteral("direction"), {QStringLiteral("approval")});
        controlled->setAnswers(QStringLiteral("report"), {QStringLiteral("progress"), QStringLiteral("risk")});
        controlled->setCurrentItem(QStringLiteral("time"));
    });
    ShadButton* resetButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(resetButton, &ShadButton::clicked, controlled, &ShadQuestionnaire::reset);
    ExampleI18n::bind(this, [jumpButton, restoreButton, resetButton]()
    {
        jumpButton->setText(tr("最后一题"));
        restoreButton->setText(tr("恢复答案"));
        resetButton->setText(tr("重置"));
    });
    addRow(controlCard, {jumpButton, restoreButton, resetButton});
    addStretchItem(controlCard, controlled);

    ShadCard* conditionCard = addSection([] { return tr("条件题目"); }, [] { return QStringLiteral("setItemDisabled"); });
    ShadQuestionnaire* conditional = createQuestionnaire(this);
    const QString invoice = QStringLiteral("invoice");
    const QString invoiceTitle = QStringLiteral("invoiceTitle");
    const QString note = QStringLiteral("note");
    const QString noInvoice = QStringLiteral("noNeed");
    addQuestion(conditional, invoice, [] { return tr("需要发票吗？"); });
    conditional->setItemRequired(invoice, true);
    addOption(conditional, invoice, QStringLiteral("need"), [] { return tr("需要"); });
    addOption(conditional, invoice, noInvoice, [] { return tr("不需要"); });
    addQuestion(conditional, invoiceTitle, [] { return tr("发票抬头"); });
    conditional->setItemRequired(invoiceTitle, true);
    addQuestion(conditional, note, [] { return tr("订单备注"); });
    ExampleI18n::bind(conditional, [conditional, invoiceTitle, note]()
    {
        conditional->setItemInput(invoiceTitle, tr("公司名称"));
        conditional->setItemInput(note, tr("选填"));
    });
    connect(conditional, &ShadQuestionnaire::answersChanged, conditional, [conditional, invoice, invoiceTitle, noInvoice](const QString& name)
    {
        if (name == invoice)
        {
            conditional->setItemDisabled(invoiceTitle, conditional->getAnswers(name) == QStringList{noInvoice});
        }
    });
    addStretchItem(conditionCard, conditional);
    addStretchItem(conditionCard, createResultFor(conditional, this));

    ShadCard* progressCard = addSection([] { return tr("自定义进度"); }, [] { return tr("用进度条显示进度"); });
    ShadQuestionnaire* progressed = createQuestionnaire(this);
    progressed->setProgressVisible(false);
    addProjectItems(progressed);
    ShadProgress* progress = new ShadProgress(this);
    progress->setMaximumWidth(kQuestionnaireWidth);
    const auto updateProgress = [progressed, progress]()
    {
        const int count = progressed->getItemCount();
        progress->setValue(count > 0 ? (progressed->getCurrentIndex() + 1) * 100 / count : 0);
    };
    connect(progressed, &ShadQuestionnaire::currentItemChanged, progress, updateProgress);
    updateProgress();
    addStretchItem(progressCard, progress);
    addStretchItem(progressCard, progressed);

    ShadCard* dialogCard = addSection([] { return tr("对话框"); }, [] { return tr("提交后关闭"); });
    ShadButton* dialogButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ExampleI18n::bind(dialogButton, [dialogButton]() { dialogButton->setText(tr("开始问卷")); });
    std::shared_ptr<SubmittedResult> dialogSubmitted = std::make_shared<SubmittedResult>();
    ShadLabel* dialogResult = createResultLabel(this, dialogSubmitted);
    connect(dialogButton, &ShadButton::clicked, this, [this, dialogResult, dialogSubmitted]()
    {
        ShadDialog* dialog = new ShadDialog(QString(), QString(), this);
        // 对话框打开期间切换语言时刷新标题
        ExampleI18n::bind(dialog, [dialog]() { dialog->setTitle(tr("项目问卷")); });
        ShadQuestionnaire* questionnaire = new ShadQuestionnaire(dialog);
        addProjectItems(questionnaire);
        dialog->contentLayout()->addWidget(questionnaire);
        showResult(questionnaire, dialogResult, dialogSubmitted);
        connect(questionnaire, &ShadQuestionnaire::submitted, dialog, &ShadDialog::accept);
        ExampleLayout::openTransientDialog(dialog);
    });
    addItem(dialogCard, dialogButton);
    addStretchItem(dialogCard, dialogResult);
}

QuestionnairePage::~QuestionnairePage()
{
}
