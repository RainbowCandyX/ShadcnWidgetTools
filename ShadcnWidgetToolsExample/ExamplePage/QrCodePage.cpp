#include "QrCodePage.h"

#include <QChar>
#include <QColor>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QList>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <memory>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadInput.h"
#include "ShadLabel.h"
#include "ShadQrCode.h"
#include "ShadToggleGroup.h"

namespace
{
    // 输入框与二维码之间的间距
    constexpr int kRowGap = 24;
    // 尺寸示例
    constexpr int kSmallSize = 96;
    constexpr int kLargeSize = 224;
    // 内容过长示例使用的重复次数，High 等级下超出版本 40 的容量
    constexpr int kLongTextRepeat = 200;

    // 页面中二维码的默认内容
    QString defaultText()
    {
        return QStringLiteral("ShadcnWidgetTools");
    }

    ShadLabel* createMutedText(const QString& text, QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(text, parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        return label;
    }

    // 当前版本与格子数，例如 "版本 3 · 29 × 29 格"
    QString describe(const ShadQrCode* qrCode)
    {
        if (!qrCode->isValid())
        {
            return QCoreApplication::translate("QrCodePage", "没有有效的二维码");
        }
        return QCoreApplication::translate("QrCodePage", "版本 %1 · %2 × %2 格").arg(qrCode->getVersion()).arg(qrCode->getModuleCount());
    }

    // 输入框在左、二维码在右的一行，返回容器
    QWidget* createInputRow(ShadInput* input, ShadQrCode* qrCode, QWidget* parent)
    {
        QWidget* row = new QWidget(parent);
        QHBoxLayout* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(kRowGap - ExampleLayout::kFocusRingWidth);
        input->setParent(row);
        qrCode->setParent(row);
        input->setFixedWidth(ExampleLayout::kFieldWidth);
        layout->addWidget(input, 0, Qt::AlignTop);
        layout->addWidget(qrCode, 0, Qt::AlignTop);
        layout->addStretch(1);
        return row;
    }
} // namespace

QrCodePage::QrCodePage(QWidget* parent)
    : ExampleBasePage([] { return tr("二维码"); }, [] { return tr("ShadQrCode：根据输入文本自动生成二维码，不依赖第三方库。"); }, parent)
{
    // 输入文本实时生成
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("修改文本后立即重新编码，支持中文"); });
    ShadInput* basicInput = new ShadInput(this);
    // 只刷新占位文字，输入内容由用户编辑，不重新设置
    ExampleI18n::bind(basicInput, [basicInput]() { basicInput->setPlaceholderText(tr("输入要编码的文本")); });
    basicInput->setText(defaultText());
    ShadQrCode* basicQrCode = new ShadQrCode(basicInput->text(), this);
    connect(basicInput, &ShadInput::textChanged, basicQrCode, &ShadQrCode::setText);
    addItem(basicCard, createInputRow(basicInput, basicQrCode, this));

    // 纠错等级：等级越高，同样的内容需要的版本越大
    ShadCard* levelCard = addSection([] { return tr("纠错等级"); }, [] { return tr("四档纠错等级，等级越高格子越多"); });
    QWidget* levelColumn = new QWidget(this);
    QVBoxLayout* levelLayout = new QVBoxLayout(levelColumn);
    levelLayout->setContentsMargins(0, 0, 0, 0);
    levelLayout->setSpacing(16 - ExampleLayout::kFocusRingWidth);
    ShadToggleGroup* levelGroup = new ShadToggleGroup(ShadToggleGroupType::Single, levelColumn);
    levelGroup->setVariant(ShadToggleType::Outline);
    const QList<QString> levelNames{QStringLiteral("L · 7%"), QStringLiteral("M · 15%"), QStringLiteral("Q · 25%"), QStringLiteral("H · 30%")};
    for (const QString& name : levelNames)
    {
        levelGroup->addItem(name);
    }
    // 输入框修改内容后二维码与版本信息同步更新，便于对比不同内容长度下各等级的版本
    ShadInput* levelInput = new ShadInput(levelColumn);
    levelInput->setFixedWidth(ExampleLayout::kFieldWidth);
    ExampleI18n::bind(levelInput, [levelInput]() { levelInput->setPlaceholderText(tr("输入要编码的文本")); });
    levelInput->setText(defaultText());
    ShadQrCode* levelQrCode = new ShadQrCode(levelInput->text(), levelColumn);
    levelQrCode->setQrSize(kLargeSize);
    ShadLabel* levelInfo = createMutedText(QString(), levelColumn);
    // 按二维码当前的版本与格子数生成文字
    ExampleI18n::bind(levelInfo, [levelQrCode, levelInfo]() { levelInfo->setText(describe(levelQrCode)); });
    connect(levelInput, &ShadInput::textChanged, levelQrCode, [levelQrCode, levelInfo](const QString& text)
    {
        levelQrCode->setText(text);
        levelInfo->setText(describe(levelQrCode));
    });
    levelGroup->setCurrentIndex(static_cast<int>(levelQrCode->getErrorCorrectionLevel()));
    connect(levelGroup, &ShadToggleGroup::currentIndexChanged, levelQrCode, [levelGroup, levelQrCode, levelInfo](int index)
    {
        // 单选模式再次点击已选项会取消选中，这里保持始终有一个等级被选中
        if (index < 0)
        {
            levelGroup->setCurrentIndex(static_cast<int>(levelQrCode->getErrorCorrectionLevel()));
            return;
        }
        levelQrCode->setErrorCorrectionLevel(static_cast<ShadQrCodeType::ErrorCorrection>(index));
        levelInfo->setText(describe(levelQrCode));
    });
    levelLayout->addWidget(levelGroup, 0, Qt::AlignLeft);
    levelLayout->addWidget(levelInput, 0, Qt::AlignLeft);
    levelLayout->addWidget(levelQrCode, 0, Qt::AlignLeft);
    levelLayout->addWidget(levelInfo, 0, Qt::AlignLeft);
    addItem(levelCard, levelColumn);

    // 尺寸、颜色与静区
    ShadCard* styleCard = addSection([] { return tr("尺寸与颜色"); }, [] { return tr("设置边长、配色与四周留白"); });
    ShadQrCode* smallQrCode = new ShadQrCode(defaultText(), this);
    smallQrCode->setQrSize(kSmallSize);
    ShadQrCode* colorQrCode = new ShadQrCode(defaultText(), this);
    colorQrCode->setForegroundColor(QColor(29, 78, 216));
    colorQrCode->setBackgroundColor(QColor(239, 246, 255));
    ShadQrCode* compactQrCode = new ShadQrCode(defaultText(), this);
    compactQrCode->setQuietZone(1);
    addRow(styleCard, {smallQrCode, colorQrCode, compactQrCode});

    // 内容过长：显示错误状态并发出 encodingFailed
    ShadCard* errorCard = addSection([] { return tr("内容过长"); }, [] { return tr("内容超出容量时显示错误状态"); });
    ShadQrCode* errorQrCode = new ShadQrCode(this);
    errorQrCode->setErrorCorrectionLevel(ShadQrCodeType::High);
    ShadLabel* errorInfo = createMutedText(QString(), this);
    // 记录是否已失败与失败原因；原因由组件在编码时生成，切换语言后保持编码时的语言
    const auto hasFailed = std::make_shared<bool>(false);
    const auto failureReason = std::make_shared<QString>();
    const auto updateErrorInfo = [errorInfo, hasFailed, failureReason]() { errorInfo->setText(*hasFailed ? tr("编码失败：%1").arg(*failureReason) : tr("尚未编码")); };
    ExampleI18n::bind(errorInfo, updateErrorInfo);
    connect(errorQrCode, &ShadQrCode::encodingFailed, errorInfo, [hasFailed, failureReason, updateErrorInfo](const QString& reason)
    {
        *hasFailed = true;
        *failureReason = reason;
        updateErrorInfo();
    });
    errorQrCode->setText((defaultText() + QLatin1Char(' ')).repeated(kLongTextRepeat));
    addRow(errorCard, {errorQrCode, errorInfo});
}

QrCodePage::~QrCodePage()
{
}
