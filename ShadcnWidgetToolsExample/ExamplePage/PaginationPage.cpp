#include "PaginationPage.h"

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QObject>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadCard.h"
#include "ShadComboBox.h"
#include "ShadDef.h"
#include "ShadLabel.h"
#include "ShadPagination.h"

namespace
{
    // 每页行数示例的总行数
    constexpr int kTotalRows = 237;
    // 每页行数下拉框的可视宽度（w-20）
    constexpr int kRowsComboWidth = 80;

    ShadLabel* createMutedText(const QString& text, QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(text, parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        return label;
    }

    // 显示当前页的文字，随分页与语言变化
    ShadLabel* createPageLabel(ShadPagination* pagination, QWidget* parent)
    {
        ShadLabel* label = createMutedText(QString(), parent);
        const auto updateText = [label, pagination]() { label->setText(QCoreApplication::translate("PaginationPage", "第 %1 / %2 页").arg(pagination->getCurrentPage()).arg(pagination->getPageCount())); };
        QObject::connect(pagination, &ShadPagination::currentPageChanged, label, updateText);
        ExampleI18n::bind(label, updateText);
        return label;
    }

    int pageCountFor(int rowsPerPage)
    {
        return (kTotalRows + rowsPerPage - 1) / rowsPerPage;
    }
} // namespace

PaginationPage::PaginationPage(QWidget* parent)
    : ExampleBasePage([] { return tr("分页"); }, [] { return tr("ShadPagination：带上一页、页码与下一页的分页，支持输入页码跳转。"); }, parent)
{
    // 基础用法
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("页数较多时中间显示省略号，当前页为描边样式"); });
    ShadPagination* basicPagination = new ShadPagination(20, this);
    basicPagination->setCurrentPage(5);
    addStretchItem(basicCard, basicPagination);
    addItem(basicCard, createPageLabel(basicPagination, this));

    // 简单
    ShadCard* simpleCard = addSection([] { return tr("简单"); }, [] { return tr("setNavigationVisible(false)：只显示页码"); });
    ShadPagination* simplePagination = new ShadPagination(5, this);
    simplePagination->setNavigationVisible(false);
    simplePagination->setCurrentPage(2);
    addStretchItem(simpleCard, simplePagination);
    addItem(simpleCard, createPageLabel(simplePagination, this));

    // 每页行数：左侧下拉框，右侧只有上一页 / 下一页
    ShadCard* rowsCard = addSection([] { return tr("每页行数"); }, [] { return tr("只显示上一页与下一页，切换每页行数后重新计算页数"); });
    QWidget* rowsBar = new QWidget(this);
    QHBoxLayout* rowsLayout = new QHBoxLayout(rowsBar);
    rowsLayout->setContentsMargins(0, 0, 0, 0);
    rowsLayout->setSpacing(8 - ExampleLayout::kFocusRingWidth);
    ShadLabel* rowsLabel = new ShadLabel(rowsBar);
    ExampleI18n::bind(rowsLabel, [rowsLabel]() { rowsLabel->setText(tr("每页行数")); });
    ShadComboBox* rowsComboBox = new ShadComboBox(rowsBar);
    rowsComboBox->addItems({QStringLiteral("10"), QStringLiteral("25"), QStringLiteral("50"), QStringLiteral("100")});
    rowsComboBox->setCurrentIndex(1);
    rowsComboBox->setFixedWidth(kRowsComboWidth + 2 * ExampleLayout::kFocusRingWidth);
    ShadPagination* rowsPagination = new ShadPagination(pageCountFor(25), rowsBar);
    rowsPagination->setPageButtonsVisible(false);
    rowsPagination->setAlignment(Qt::AlignRight);
    rowsLayout->addWidget(rowsLabel);
    rowsLayout->addWidget(rowsComboBox);
    rowsLayout->addWidget(rowsPagination, 1);
    ShadLabel* rowsResult = createMutedText(QString(), this);
    const auto updateRowsResult = [rowsResult, rowsPagination, rowsComboBox]()
    {
        const int rowsPerPage = rowsComboBox->currentText().toInt();
        const int first = (rowsPagination->getCurrentPage() - 1) * rowsPerPage + 1;
        const int last = qMin(kTotalRows, first + rowsPerPage - 1);
        rowsResult->setText(tr("第 %1 - %2 条，共 %3 条").arg(first).arg(last).arg(kTotalRows));
    };
    connect(rowsComboBox, &ShadComboBox::currentIndexChanged, rowsPagination, [rowsPagination, rowsComboBox, updateRowsResult]()
    {
        rowsPagination->setPageCount(pageCountFor(rowsComboBox->currentText().toInt()));
        rowsPagination->setCurrentPage(1);
        updateRowsResult();
    });
    connect(rowsPagination, &ShadPagination::currentPageChanged, rowsResult, updateRowsResult);
    // 按当前页与每页行数生成文字，切换语言时重新生成
    ExampleI18n::bind(rowsResult, updateRowsResult);
    addStretchItem(rowsCard, rowsBar);
    addItem(rowsCard, rowsResult);

    // 页码跳转
    ShadCard* jumpCard = addSection([] { return tr("页码跳转"); }, [] { return tr("setJumpVisible(true)：输入页码后按 Enter 跳转，超出范围时显示错误态"); });
    ShadPagination* jumpPagination = new ShadPagination(50, this);
    jumpPagination->setJumpVisible(true);
    addStretchItem(jumpCard, jumpPagination);
    addItem(jumpCard, createPageLabel(jumpPagination, this));

    // 只显示图标
    ShadCard* iconCard = addSection([] { return tr("只显示图标"); }, [] { return tr("setNavigationTextVisible(false)：上一页与下一页只显示箭头"); });
    ShadPagination* iconPagination = new ShadPagination(10, this);
    iconPagination->setNavigationTextVisible(false);
    iconPagination->setCurrentPage(3);
    addStretchItem(iconCard, iconPagination);

    // RTL
    ShadCard* rtlCard = addSection([] { return QStringLiteral("RTL"); }, [] { return tr("从右到左时整体镜像，箭头方向翻转"); });
    ShadPagination* rtlPagination = new ShadPagination(20, this);
    rtlPagination->setLayoutDirection(Qt::RightToLeft);
    rtlPagination->setCurrentPage(10);
    rtlPagination->setJumpVisible(true);
    addStretchItem(rtlCard, rtlPagination);
}

PaginationPage::~PaginationPage()
{
}
