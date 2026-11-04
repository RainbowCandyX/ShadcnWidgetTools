#include "DataTablePage.h"

#include <QAction>
#include <QClipboard>
#include <QColor>
#include <QFont>
#include <QGuiApplication>
#include <QList>
#include <QLocale>
#include <QPainter>
#include <QPixmap>
#include <QSize>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include <QObject>
#include <QVBoxLayout>

#include <memory>

#include "ExampleI18n.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDataTable.h"
#include "ShadDef.h"
#include "ShadDropdownMenu.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadTable.h"

namespace
{
    // 一条付款记录，对应 shadcn Data Table 示例中的 Payment
    struct Payment
    {
        QString id;
        double amount;
        QString status;
        QString email;
    };

    // 源模型的列
    enum PaymentColumn
    {
        StatusColumn = 0,
        EmailColumn,
        AmountColumn,
    };

    // 排序使用 Qt::UserRole：邮箱按小写排序，金额按数值排序
    constexpr int kSortRole = Qt::UserRole;
    // 付款 ID 不显示，保存在第一列的 Qt::UserRole + 1 中
    constexpr int kPaymentIdRole = Qt::UserRole + 1;
    // 生成头像的边长，表格中按 32px 缩放
    constexpr int kAvatarImageSize = 96;

    // 示例头像：没有使用网络图片，纯色背景上写姓名的第一个字
    QPixmap createAvatar(const QString& name, const QColor& color)
    {
        QPixmap avatar(kAvatarImageSize, kAvatarImageSize);
        avatar.fill(color);
        QPainter painter(&avatar);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        QFont font = painter.font();
        font.setPixelSize(kAvatarImageSize / 2);
        font.setWeight(QFont::Medium);
        painter.setFont(font);
        painter.setPen(QColor(Qt::white));
        painter.drawText(avatar.rect(), Qt::AlignCenter, name.left(1));
        return avatar;
    }

    // 行操作菜单中的操作类型
    enum class RowAction
    {
        None,
        CopyId,
        ViewCustomer,
        ViewDetails,
    };

    // 最近一次行操作；detail 为付款 ID 或邮箱，不随语言变化
    struct LastRowAction
    {
        RowAction action{RowAction::None};
        QString detail;
    };
} // namespace

DataTablePage::DataTablePage(QWidget* parent)
    : ExampleBasePage([] { return tr("数据表格"); }, [] { return tr("ShadDataTable：基于 ShadTable 的数据表格，支持筛选、排序、行选择与分页。"); }, parent)
{
    const QList<Payment> payments = {
        {QStringLiteral("m5gr84i9"), 316, QStringLiteral("success"), QStringLiteral("ken99@example.com")},
        {QStringLiteral("3u1reuv4"), 242, QStringLiteral("success"), QStringLiteral("Abe45@example.com")},
        {QStringLiteral("derv1ws0"), 837, QStringLiteral("processing"), QStringLiteral("Monserrat44@example.com")},
        {QStringLiteral("5kma53ae"), 874, QStringLiteral("success"), QStringLiteral("Silas22@example.com")},
        {QStringLiteral("bhqecj4p"), 721, QStringLiteral("failed"), QStringLiteral("carmella@example.com")},
        {QStringLiteral("p9xk2l7d"), 129, QStringLiteral("pending"), QStringLiteral("olivia.martin@example.com")},
        {QStringLiteral("a7ty3mq1"), 560, QStringLiteral("success"), QStringLiteral("jackson.lee@example.com")},
        {QStringLiteral("c2nv8bz5"), 45, QStringLiteral("failed"), QStringLiteral("isabella.nguyen@example.com")},
        {QStringLiteral("q8we4rt6"), 999, QStringLiteral("processing"), QStringLiteral("william.kim@example.com")},
        {QStringLiteral("z1lo9pm3"), 380, QStringLiteral("success"), QStringLiteral("sofia.davis@example.com")},
        {QStringLiteral("h6jk2nb8"), 215, QStringLiteral("pending"), QStringLiteral("liam.johnson@example.com")},
        {QStringLiteral("t4ys7vc0"), 672, QStringLiteral("success"), QStringLiteral("emma.brown@example.com")},
        {QStringLiteral("e3ui5op2"), 88, QStringLiteral("failed"), QStringLiteral("noah.wilson@example.com")},
        {QStringLiteral("r7df1gh4"), 431, QStringLiteral("processing"), QStringLiteral("ava.taylor@example.com")},
        {QStringLiteral("y5bn3mk9"), 750, QStringLiteral("success"), QStringLiteral("lucas.anderson@example.com")},
        {QStringLiteral("u2cx6za1"), 199, QStringLiteral("pending"), QStringLiteral("mia.thomas@example.com")},
        {QStringLiteral("i9qw8er3"), 523, QStringLiteral("success"), QStringLiteral("ethan.moore@example.com")},
        {QStringLiteral("o4as2df6"), 64, QStringLiteral("failed"), QStringLiteral("amelia.jackson@example.com")},
        {QStringLiteral("k8gh5jl2"), 915, QStringLiteral("success"), QStringLiteral("logan.white@example.com")},
        {QStringLiteral("n1vb7cx4"), 307, QStringLiteral("processing"), QStringLiteral("harper.harris@example.com")},
        {QStringLiteral("w6er9ty1"), 482, QStringLiteral("success"), QStringLiteral("elijah.clark@example.com")},
        {QStringLiteral("s3df8gh5"), 156, QStringLiteral("pending"), QStringLiteral("evelyn.lewis@example.com")},
    };

    QStandardItemModel* paymentModel = new QStandardItemModel(0, 3, this);
    paymentModel->setHorizontalHeaderLabels({QStringLiteral("Status"), QStringLiteral("Email"), QStringLiteral("Amount")});
    // 金额列表头与单元格右对齐（text-right）
    paymentModel->setHeaderData(AmountColumn, Qt::Horizontal, static_cast<int>(Qt::AlignRight | Qt::AlignVCenter), Qt::TextAlignmentRole);
    const QLocale usLocale(QLocale::English, QLocale::UnitedStates);
    QFont mediumFont;
    mediumFont.setWeight(QFont::Medium);
    for (const Payment& payment : payments)
    {
        // 状态首字母大写（对应 capitalize）
        QString statusText = payment.status;
        statusText[0] = statusText.at(0).toUpper();
        QStandardItem* statusItem = new QStandardItem(statusText);
        statusItem->setData(payment.status, kSortRole);
        statusItem->setData(payment.id, kPaymentIdRole);
        QStandardItem* emailItem = new QStandardItem(payment.email);
        emailItem->setData(payment.email.toLower(), kSortRole);
        // 金额按 en-US 货币格式显示，右对齐并加粗（text-right font-medium）
        QStandardItem* amountItem = new QStandardItem(usLocale.toCurrencyString(payment.amount, QStringLiteral("$")));
        amountItem->setData(payment.amount, kSortRole);
        amountItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        amountItem->setFont(mediumFont);
        paymentModel->appendRow({statusItem, emailItem, amountItem});
    }

    ShadCard* tableCard = addSection([] { return tr("付款记录"); }, [] { return tr("支持筛选、排序、列显示、行选择、分页与调整列宽"); });
    ShadDataTable* dataTable = new ShadDataTable(this);
    dataTable->setSourceModel(paymentModel);
    dataTable->setFilterColumn(EmailColumn);
    dataTable->setFilterPlaceholderText(QStringLiteral("Filter emails..."));
    dataTable->setSortRole(kSortRole);
    dataTable->setColumnSortable(EmailColumn, true);
    dataTable->setColumnSortable(AmountColumn, true);
    dataTable->setColumnHideable(StatusColumn, true);
    dataTable->setColumnHideable(EmailColumn, true);
    dataTable->setColumnHideable(AmountColumn, true);
    dataTable->setSelectionEnabled(true);

    // 最近一次行操作，切换语言时据此重新生成提示文字
    ShadLabel* actionLabel = new ShadLabel(this);
    actionLabel->setColorRole(ShadThemeType::MutedForeground);
    const std::shared_ptr<LastRowAction> lastAction = std::make_shared<LastRowAction>();
    const auto updateActionLabel = [actionLabel, lastAction]()
    {
        switch (lastAction->action)
        {
            case RowAction::CopyId:
                actionLabel->setText(tr("最近操作: 已复制付款 ID %1").arg(lastAction->detail));
                break;
            case RowAction::ViewCustomer:
                actionLabel->setText(tr("最近操作: %1 %2").arg(tr("查看客户"), lastAction->detail));
                break;
            case RowAction::ViewDetails:
                actionLabel->setText(tr("最近操作: %1 %2").arg(tr("查看付款详情"), lastAction->detail));
                break;
            case RowAction::None:
                actionLabel->setText(tr("最近操作: 无"));
                break;
        }
    };
    ExampleI18n::bind(actionLabel, updateActionLabel);
    const auto recordRowAction = [dataTable, paymentModel, lastAction, updateActionLabel](RowAction action)
    {
        const int row = dataTable->getActionRow();
        if (row < 0)
        {
            return;
        }
        if (action == RowAction::CopyId)
        {
            const QString paymentId = paymentModel->item(row, StatusColumn)->data(kPaymentIdRole).toString();
            QGuiApplication::clipboard()->setText(paymentId);
            lastAction->detail = paymentId;
        }
        else
        {
            lastAction->detail = paymentModel->item(row, EmailColumn)->text();
        }
        lastAction->action = action;
        updateActionLabel();
    };

    // 行操作菜单，对应示例中的 Actions / Copy payment ID / View customer / View payment details
    ShadDropdownMenu* actionMenu = new ShadDropdownMenu(this);
    const int actionLabelIndex = actionMenu->addLabel(QString());
    QAction* copyAction = actionMenu->addItem(QString());
    actionMenu->addSeparator();
    QAction* customerAction = actionMenu->addItem(QString());
    QAction* detailsAction = actionMenu->addItem(QString());
    ExampleI18n::bind(actionMenu, [actionMenu, actionLabelIndex, copyAction, customerAction, detailsAction]()
    {
        actionMenu->setLabelText(actionLabelIndex, tr("操作"));
        copyAction->setText(tr("复制付款 ID"));
        customerAction->setText(tr("查看客户"));
        detailsAction->setText(tr("查看付款详情"));
    });
    connect(copyAction, &QAction::triggered, actionLabel, [recordRowAction]() { recordRowAction(RowAction::CopyId); });
    connect(customerAction, &QAction::triggered, actionLabel, [recordRowAction]() { recordRowAction(RowAction::ViewCustomer); });
    connect(detailsAction, &QAction::triggered, actionLabel, [recordRowAction]() { recordRowAction(RowAction::ViewDetails); });
    dataTable->setRowActionMenu(actionMenu);

    // 拖动过的列恢复为自动宽度
    ShadButton* resetWidthButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ExampleI18n::bind(resetWidthButton, [resetWidthButton]() { resetWidthButton->setText(tr("重置列宽")); });
    resetWidthButton->setButtonSize(ShadButtonType::Small);
    resetWidthButton->setShadIcon(ShadIconType::RotateCcw);
    connect(resetWidthButton, &ShadButton::clicked, dataTable, &ShadDataTable::resetColumnWidths);

    tableCard->contentLayout()->addWidget(dataTable);
    addRow(tableCard, {resetWidthButton, actionLabel});

    // 头像列：32px 圆形头像 + 姓名，第二行为邮箱
    struct Member
    {
        ExampleI18n::TextFunction name;
        QString email;
        ExampleI18n::TextFunction role;
        QColor color;
    };
    const QList<Member> members = {
        {[] { return tr("陈娜"); }, QStringLiteral("chen.na@example.com"), [] { return tr("设计"); }, QColor(0x3B, 0x82, 0xF6)},
        {[] { return tr("李瑞"); }, QStringLiteral("li.rui@example.com"), [] { return tr("前端"); }, QColor(0xEC, 0x48, 0x99)},
        {[] { return tr("王悦"); }, QStringLiteral("wang.yue@example.com"), [] { return tr("后端"); }, QColor(0x22, 0xC5, 0x5E)},
        {[] { return tr("张一"); }, QStringLiteral("zhang.yi@example.com"), [] { return tr("产品"); }, QColor(0xF9, 0x73, 0x16)},
        {[] { return tr("刘洋"); }, QStringLiteral("liu.yang@example.com"), [] { return tr("测试"); }, QColor(0x8B, 0x5C, 0xF6)},
        {[] { return tr("赵敏"); }, QStringLiteral("zhao.min@example.com"), [] { return tr("运营"); }, QColor(0x14, 0xB8, 0xA6)},
        {[] { return tr("孙磊"); }, QString(), [] { return tr("实习"); }, QColor()},
    };
    QStandardItemModel* memberModel = new QStandardItemModel(0, 2, this);
    for (const Member& member : members)
    {
        QStandardItem* memberItem = new QStandardItem();
        if (!member.email.isEmpty())
        {
            memberItem->setData(member.email, ShadTable::DescriptionRole);
        }
        memberModel->appendRow({memberItem, new QStandardItem()});
    }
    // 按源模型行号原地更新姓名、角色与头像，排序、筛选、分页与选中状态保留
    ExampleI18n::bind(memberModel, [memberModel, members]()
    {
        memberModel->setHeaderData(0, Qt::Horizontal, tr("成员"));
        memberModel->setHeaderData(1, Qt::Horizontal, tr("角色"));
        for (int row = 0; row < members.size(); ++row)
        {
            const Member& member = members.at(row);
            const QString name = member.name();
            const QString role = member.role();
            QStandardItem* memberItem = memberModel->item(row, 0);
            memberItem->setText(name);
            memberItem->setData(name, kSortRole);
            // 最后一位成员没有头像，显示占位块
            if (member.color.isValid())
            {
                memberItem->setData(createAvatar(name, member.color), Qt::DecorationRole);
            }
            QStandardItem* roleItem = memberModel->item(row, 1);
            roleItem->setText(role);
            roleItem->setData(role, kSortRole);
        }
    });
    ShadCard* avatarCard = addSection([] { return tr("头像列"); }, [] { return tr("setColumnImage 设为圆形头像，DescriptionRole 显示第二行"); });
    ShadDataTable* memberTable = new ShadDataTable(this);
    memberTable->setSourceModel(memberModel);
    memberTable->setFilterColumn(0);
    ExampleI18n::bind(memberTable, [memberTable]() { memberTable->setFilterPlaceholderText(tr("筛选成员...")); });
    memberTable->setSortRole(kSortRole);
    memberTable->setColumnSortable(0, true);
    memberTable->setColumnImage(0, QSize(32, 32), ShadTableType::Circle);
    memberTable->setPageSize(5);
    avatarCard->contentLayout()->addWidget(memberTable);
}

DataTablePage::~DataTablePage()
{
}
