#include "InputDialogPage.h"

#include <QDialog>
#include <QLineEdit>
#include <QPointer>
#include <QString>
#include <QWidget>
#include <QtGlobal>

#include <memory>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadInputDialog.h"
#include "ShadLabel.h"

namespace
{
    // execInputDialog 的返回值：页面已销毁
    constexpr int kPageDestroyed = -1;

    // 结果文字对应的最近一次操作
    enum class DialogOutcome
    {
        None,
        Accepted,
        Canceled
    };

    // 各示例的结果状态，结果文字按当前状态与当前语言生成
    template <typename Value>
    struct DialogState
    {
        Value value;
        DialogOutcome outcome{DialogOutcome::None};
    };

    ShadLabel* createMutedText(QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        return label;
    }

    // 标题、说明与输入框标签随语言刷新，对话框打开期间切换语言同样生效
    ShadInputDialog* createInputDialog(ExampleI18n::TextFunction title, ExampleI18n::TextFunction description, ExampleI18n::TextFunction label, QWidget* parent)
    {
        ShadInputDialog* dialog = new ShadInputDialog(parent);
        ExampleI18n::bind(dialog, [dialog, title, description, label]()
        {
            dialog->setTitle(title());
            dialog->setDescription(description());
            dialog->setLabelText(label());
        });
        ExampleLayout::addStayOnTopButton(dialog);
        return dialog;
    }

    // exec 打开输入对话框，确定时先用 onAccepted 读取输入值再删除；对话框打开期间页面被销毁时直接返回 kPageDestroyed
    template <typename OnAccepted>
    int execInputDialog(ShadInputDialog* dialog, OnAccepted onAccepted)
    {
        QPointer<ShadInputDialog> guard = dialog;
        const int result = dialog->exec();
        if (guard.isNull())
        {
            return kPageDestroyed;
        }
        if (result == QDialog::Accepted)
        {
            onAccepted(dialog);
        }
        delete dialog;
        return result;
    }

    DialogOutcome toOutcome(int result)
    {
        return result == QDialog::Accepted ? DialogOutcome::Accepted : DialogOutcome::Canceled;
    }
} // namespace

InputDialogPage::InputDialogPage(QWidget* parent)
    : ExampleBasePage([] { return tr("输入对话框"); }, [] { return tr("ShadInputDialog：在对话框中输入一个值，支持文本、密码、多行文本与数值。"); }, parent)
{
    // 单行文本：文件名为空表示尚未修改，使用当前语言的默认文件名
    ShadCard* textCard = addSection([] { return tr("单行文本"); }, [] { return tr("输入单行文本，确定后取回输入值"); });
    ShadButton* renameButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ShadLabel* renameResult = createMutedText(this);
    std::shared_ptr<DialogState<QString>> renameState = std::make_shared<DialogState<QString>>();
    const auto updateRenameResult = [renameResult, renameState]()
    {
        switch (renameState->outcome)
        {
            case DialogOutcome::None:
                renameResult->setText(tr("尚未修改"));
                break;
            case DialogOutcome::Accepted:
                renameResult->setText(tr("新文件名：%1").arg(renameState->value));
                break;
            case DialogOutcome::Canceled:
                renameResult->setText(tr("已取消"));
                break;
        }
    };
    connect(renameButton, &ShadButton::clicked, this, [this, renameState, updateRenameResult]()
    {
        ShadInputDialog* dialog = createInputDialog([] { return tr("重命名文件"); }, [] { return tr("输入新的文件名，完成后点击确定。"); }, [] { return tr("文件名"); }, this);
        dialog->setTextValue(renameState->value.isEmpty() ? tr("季度报告.docx") : renameState->value);
        const int result = execInputDialog(dialog, [renameState](ShadInputDialog* accepted) { renameState->value = accepted->getTextValue(); });
        if (result == kPageDestroyed)
        {
            return;
        }
        renameState->outcome = toOutcome(result);
        updateRenameResult();
    });
    addRow(textCard, {renameButton, renameResult});

    // 密码
    ShadCard* passwordCard = addSection([] { return tr("密码"); }, [] { return tr("隐藏输入内容，尾部按钮切换显示"); });
    ShadButton* passwordButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ShadLabel* passwordResult = createMutedText(this);
    std::shared_ptr<DialogState<qsizetype>> passwordState = std::make_shared<DialogState<qsizetype>>();
    const auto updatePasswordResult = [passwordResult, passwordState]()
    {
        switch (passwordState->outcome)
        {
            case DialogOutcome::None:
                passwordResult->setText(tr("尚未验证"));
                break;
            case DialogOutcome::Accepted:
                passwordResult->setText(tr("已输入 %1 个字符").arg(passwordState->value));
                break;
            case DialogOutcome::Canceled:
                passwordResult->setText(tr("已取消"));
                break;
        }
    };
    connect(passwordButton, &ShadButton::clicked, this, [this, passwordState, updatePasswordResult]()
    {
        ShadInputDialog* dialog = createInputDialog([] { return tr("需要验证"); }, [] { return tr("输入当前账户的密码以继续。"); }, [] { return tr("密码"); }, this);
        dialog->setTextEchoMode(QLineEdit::Password);
        ExampleI18n::bind(dialog, [dialog]()
        {
            dialog->setPlaceholderText(tr("输入密码"));
            dialog->setOkButtonText(tr("验证"));
        });
        const int result = execInputDialog(dialog, [passwordState](ShadInputDialog* accepted) { passwordState->value = accepted->getTextValue().size(); });
        if (result == kPageDestroyed)
        {
            return;
        }
        passwordState->outcome = toOutcome(result);
        updatePasswordResult();
    });
    addRow(passwordCard, {passwordButton, passwordResult});

    // 多行文本
    ShadCard* multiLineCard = addSection([] { return tr("多行文本"); }, [] { return tr("使用多行输入框，内容超出时滚动"); });
    ShadButton* noteButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ShadLabel* noteResult = createMutedText(this);
    std::shared_ptr<DialogState<QString>> noteState = std::make_shared<DialogState<QString>>();
    const auto updateNoteResult = [noteResult, noteState]()
    {
        switch (noteState->outcome)
        {
            case DialogOutcome::None:
                noteResult->setText(tr("暂无备注"));
                break;
            case DialogOutcome::Accepted:
                noteResult->setText(tr("已保存 %1 个字符").arg(noteState->value.size()));
                break;
            case DialogOutcome::Canceled:
                noteResult->setText(tr("已取消"));
                break;
        }
    };
    connect(noteButton, &ShadButton::clicked, this, [this, noteState, updateNoteResult]()
    {
        ShadInputDialog* dialog = createInputDialog([] { return tr("添加备注"); }, [] { return tr("备注会显示在订单详情中。"); }, [] { return tr("备注"); }, this);
        dialog->setInputMode(ShadInputDialogType::MultiLineTextInput);
        ExampleI18n::bind(dialog, [dialog]()
        {
            dialog->setPlaceholderText(tr("写下需要商家注意的事项"));
            dialog->setOkButtonText(tr("保存"));
        });
        dialog->setTextValue(noteState->value);
        const int result = execInputDialog(dialog, [noteState](ShadInputDialog* accepted) { noteState->value = accepted->getTextValue(); });
        if (result == kPageDestroyed)
        {
            return;
        }
        noteState->outcome = toOutcome(result);
        updateNoteResult();
    });
    addRow(multiLineCard, {noteButton, noteResult});

    // 整数：未操作时显示初始数量
    ShadCard* intCard = addSection([] { return tr("整数"); }, [] { return tr("整数输入框，可设置范围和步长"); });
    ShadButton* quantityButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ShadLabel* quantityResult = createMutedText(this);
    std::shared_ptr<DialogState<int>> quantityState = std::make_shared<DialogState<int>>(DialogState<int>{1});
    const auto updateQuantityResult = [quantityResult, quantityState]() { quantityResult->setText(quantityState->outcome == DialogOutcome::Canceled ? tr("已取消") : tr("数量：%1").arg(quantityState->value)); };
    connect(quantityButton, &ShadButton::clicked, this, [this, quantityState, updateQuantityResult]()
    {
        ShadInputDialog* dialog = createInputDialog([] { return tr("购买数量"); }, [] { return tr("每单最多购买 10 件。"); }, [] { return tr("数量"); }, this);
        dialog->setInputMode(ShadInputDialogType::IntInput);
        dialog->setIntRange(1, 10);
        dialog->setIntValue(quantityState->value);
        const int result = execInputDialog(dialog, [quantityState](ShadInputDialog* accepted) { quantityState->value = accepted->getIntValue(); });
        if (result == kPageDestroyed)
        {
            return;
        }
        quantityState->outcome = toOutcome(result);
        updateQuantityResult();
    });
    addRow(intCard, {quantityButton, quantityResult});

    // 小数：未操作时显示初始价格
    ShadCard* doubleCard = addSection([] { return tr("小数"); }, [] { return tr("小数输入框，可设置范围、小数位数和步长"); });
    ShadButton* priceButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ShadLabel* priceResult = createMutedText(this);
    std::shared_ptr<DialogState<double>> priceState = std::make_shared<DialogState<double>>(DialogState<double>{19.9});
    const auto updatePriceResult = [priceResult, priceState]() { priceResult->setText(priceState->outcome == DialogOutcome::Canceled ? tr("已取消") : tr("价格：%1 元").arg(QString::number(priceState->value, 'f', 2))); };
    connect(priceButton, &ShadButton::clicked, this, [this, priceState, updatePriceResult]()
    {
        ShadInputDialog* dialog = createInputDialog([] { return tr("调整价格"); }, [] { return tr("价格保留两位小数，加减按钮每次调整 0.5 元。"); }, [] { return tr("价格（元）"); }, this);
        dialog->setInputMode(ShadInputDialogType::DoubleInput);
        dialog->setDoubleDecimals(2);
        dialog->setDoubleRange(0.0, 9999.99);
        dialog->setDoubleStep(0.5);
        dialog->setDoubleValue(priceState->value);
        const int result = execInputDialog(dialog, [priceState](ShadInputDialog* accepted) { priceState->value = accepted->getDoubleValue(); });
        if (result == kPageDestroyed)
        {
            return;
        }
        priceState->outcome = toOutcome(result);
        updatePriceResult();
    });
    addRow(doubleCard, {priceButton, priceResult});

    ExampleI18n::bind(this, [renameButton, passwordButton, noteButton, quantityButton, priceButton, updateRenameResult, updatePasswordResult, updateNoteResult, updateQuantityResult, updatePriceResult]()
    {
        renameButton->setText(tr("重命名"));
        passwordButton->setText(tr("验证身份"));
        noteButton->setText(tr("添加备注"));
        quantityButton->setText(tr("修改数量"));
        priceButton->setText(tr("调整价格"));
        updateRenameResult();
        updatePasswordResult();
        updateNoteResult();
        updateQuantityResult();
        updatePriceResult();
    });
}

InputDialogPage::~InputDialogPage()
{
}
