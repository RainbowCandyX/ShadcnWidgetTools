#include "TablePage.h"

#include <QAction>
#include <QColor>
#include <QFont>
#include <QLinearGradient>
#include <QList>
#include <QPainter>
#include <QPixmap>
#include <QPointF>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>

#include <memory>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadDropdownMenu.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadTable.h"

namespace
{
    // 一张发票：发票号、状态、支付方式、金额
    struct Invoice
    {
        QString invoice;
        QString status;
        QString method;
        QString amount;
    };

    QStandardItem* createItem(const QString& text, Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter)
    {
        QStandardItem* item = new QStandardItem(text);
        item->setTextAlignment(alignment);
        return item;
    }

    // 示例图片：没有使用网络图片，按两种颜色生成对角渐变；宽高不同，用于演示按比例裁剪
    QPixmap createImage(const QColor& from, const QColor& to, const QSize& size)
    {
        QPixmap image(size);
        QPainter painter(&image);
        QLinearGradient gradient(QPointF(0, 0), QPointF(size.width(), size.height()));
        gradient.setColorAt(0.0, from);
        gradient.setColorAt(1.0, to);
        painter.fillRect(image.rect(), gradient);
        return image;
    }

    // 图片列单元格：DecorationRole 为图片，可选 DescriptionRole 为第二行说明
    QStandardItem* createImageItem(const QString& text, const QPixmap& image, const QString& description = QString())
    {
        QStandardItem* item = createItem(text);
        if (!image.isNull())
        {
            item->setData(image, Qt::DecorationRole);
        }
        if (!description.isEmpty())
        {
            item->setData(description, ShadTable::DescriptionRole);
        }
        return item;
    }
} // namespace

TablePage::TablePage(QWidget* parent)
    : ExampleBasePage([] { return tr("表格"); }, [] { return tr("ShadTable：接受任意 QAbstractItemModel 的表格组件，对齐与字重由模型决定。"); }, parent)
{
    const QList<Invoice> invoices = {
        {QStringLiteral("INV001"), QStringLiteral("Paid"), QStringLiteral("Credit Card"), QStringLiteral("$250.00")},
        {QStringLiteral("INV002"), QStringLiteral("Pending"), QStringLiteral("PayPal"), QStringLiteral("$150.00")},
        {QStringLiteral("INV003"), QStringLiteral("Unpaid"), QStringLiteral("Bank Transfer"), QStringLiteral("$350.00")},
        {QStringLiteral("INV004"), QStringLiteral("Paid"), QStringLiteral("Credit Card"), QStringLiteral("$450.00")},
        {QStringLiteral("INV005"), QStringLiteral("Paid"), QStringLiteral("PayPal"), QStringLiteral("$550.00")},
        {QStringLiteral("INV006"), QStringLiteral("Pending"), QStringLiteral("Bank Transfer"), QStringLiteral("$200.00")},
        {QStringLiteral("INV007"), QStringLiteral("Unpaid"), QStringLiteral("Credit Card"), QStringLiteral("$300.00")},
    };

    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("拖动表头边缘调整列宽，双击恢复；带表尾合计"); });
    QStandardItemModel* invoiceModel = new QStandardItemModel(0, 4, this);
    invoiceModel->setHorizontalHeaderLabels({QStringLiteral("Invoice"), QStringLiteral("Status"), QStringLiteral("Method"), QStringLiteral("Amount")});
    // 表头与单元格的 text-right 都通过 TextAlignmentRole 指定
    invoiceModel->setHeaderData(3, Qt::Horizontal, static_cast<int>(Qt::AlignRight | Qt::AlignVCenter), Qt::TextAlignmentRole);
    QFont mediumFont;
    mediumFont.setWeight(QFont::Medium);
    for (const Invoice& invoice : invoices)
    {
        QStandardItem* invoiceItem = createItem(invoice.invoice);
        invoiceItem->setFont(mediumFont);
        invoiceModel->appendRow({invoiceItem, createItem(invoice.status), createItem(invoice.method), createItem(invoice.amount, Qt::AlignRight | Qt::AlignVCenter)});
    }
    ShadTable* invoiceTable = new ShadTable(this);
    invoiceTable->setModel(invoiceModel);
    invoiceTable->setColumnFixedWidth(0, 100);
    invoiceTable->setFooterText(0, QStringLiteral("Total"), Qt::AlignLeft | Qt::AlignVCenter, 3);
    invoiceTable->setFooterText(3, QStringLiteral("$2,250.00"), Qt::AlignRight | Qt::AlignVCenter);
    invoiceTable->setCaption(QStringLiteral("A list of your recent invoices."));
    basicCard->contentLayout()->addWidget(invoiceTable);

    ShadCard* actionCard = addSection([] { return tr("行操作"); }, [] { return tr("setActionColumn：指定行操作列，点击可弹出菜单"); });
    QStandardItemModel* productModel = new QStandardItemModel(0, 3, this);
    productModel->setHorizontalHeaderLabels({QStringLiteral("Product"), QStringLiteral("Price"), QStringLiteral("Actions")});
    productModel->setHeaderData(2, Qt::Horizontal, static_cast<int>(Qt::AlignRight | Qt::AlignVCenter), Qt::TextAlignmentRole);
    const QStringList products = {QStringLiteral("Wireless Mouse"), QStringLiteral("Mechanical Keyboard"), QStringLiteral("USB-C Hub")};
    const QStringList prices = {QStringLiteral("$29.99"), QStringLiteral("$129.99"), QStringLiteral("$49.99")};
    for (int index = 0; index < int(products.size()); ++index)
    {
        QStandardItem* productItem = createItem(products.at(index));
        productItem->setFont(mediumFont);
        // 行操作列的按钮跟随该列的 TextAlignmentRole 靠右
        productModel->appendRow({productItem, createItem(prices.at(index)), createItem(QString(), Qt::AlignRight | Qt::AlignVCenter)});
    }
    ShadTable* productTable = new ShadTable(this);
    productTable->setModel(productModel);
    productTable->setActionColumn(2);
    ShadDropdownMenu* productMenu = new ShadDropdownMenu(this);
    QAction* editAction = productMenu->addItem(QString(), ShadIconType::Pencil);
    QAction* copyAction = productMenu->addItem(QString(), ShadIconType::Copy);
    productMenu->addSeparator();
    QAction* deleteAction = productMenu->addItem(QString(), ShadIconType::Trash2);
    productMenu->setItemDestructive(deleteAction, true);
    ExampleI18n::bind(productMenu, [editAction, copyAction, deleteAction]()
    {
        editAction->setText(tr("编辑"));
        copyAction->setText(tr("复制"));
        deleteAction->setText(tr("删除"));
    });
    ShadLabel* productLabel = new ShadLabel(this);
    productLabel->setColorRole(ShadThemeType::MutedForeground);
    // 记录弹出菜单时对应的商品名，菜单项触发时显示
    const auto activeProduct = std::make_shared<QString>();
    // 最近触发的菜单项与对应商品名，切换语言时按菜单项的当前文字重新生成；在菜单项之后绑定，保证读到新语言的文字
    const auto lastAction = std::make_shared<QPointer<QAction>>();
    const auto lastProduct = std::make_shared<QString>();
    const auto updateProductLabel = [productLabel, lastAction, lastProduct]()
    {
        const QAction* action = lastAction->data();
        productLabel->setText(action != nullptr ? tr("最近操作: %1 %2").arg(action->text(), *lastProduct) : tr("最近操作: 无"));
    };
    ExampleI18n::bind(productLabel, updateProductLabel);
    connect(productTable, &ShadTable::actionRequested, productMenu, [productMenu, productModel, activeProduct](int row, const QRect& buttonRect)
    {
        *activeProduct = productModel->item(row, 0)->text();
        productMenu->popup(buttonRect, Qt::AlignRight);
    });
    connect(productMenu, &ShadDropdownMenu::itemTriggered, productLabel, [activeProduct, lastAction, lastProduct, updateProductLabel](QAction* action)
    {
        *lastAction = action;
        *lastProduct = *activeProduct;
        updateProductLabel();
    });
    actionCard->contentLayout()->addWidget(productTable);
    addItem(actionCard, productLabel);

    // 图片列：同一表格中 40px 圆角缩略图与 24px 圆形头像，行高按最大的图片自动计算
    ShadCard* imageCard = addSection([] { return tr("图片列"); }, [] { return tr("setColumnImage：每列单独设置图片尺寸与形状"); });
    QStandardItemModel* goodsModel = new QStandardItemModel(0, 4, this);
    goodsModel->setHeaderData(3, Qt::Horizontal, static_cast<int>(Qt::AlignRight | Qt::AlignVCenter), Qt::TextAlignmentRole);
    struct Goods
    {
        ExampleI18n::TextFunction name;
        QString sku;
        QPixmap image;
        ExampleI18n::TextFunction seller;
        QPixmap avatar;
        QString stock;
        QString price;
    };
    const QSize photoSize(240, 160);
    const QSize avatarSize(96, 96);
    const QList<Goods> goodsList = {
        {[] { return tr("无线鼠标"); }, QStringLiteral("SKU-1024"), createImage(QColor(0x60, 0xA5, 0xFA), QColor(0x1E, 0x3A, 0x8A), photoSize), [] { return tr("数码旗舰店"); }, createImage(QColor(0xF9, 0xA8, 0xD4), QColor(0x9D, 0x17, 0x4D), avatarSize), QStringLiteral("128"), QStringLiteral("¥129.00")},
        {[] { return tr("机械键盘"); }, QStringLiteral("SKU-2048"), createImage(QColor(0x86, 0xEF, 0xAC), QColor(0x16, 0x65, 0x34), photoSize), [] { return tr("外设专营店"); }, createImage(QColor(0xFD, 0xBA, 0x74), QColor(0x9A, 0x34, 0x12), avatarSize), QStringLiteral("56"), QStringLiteral("¥499.00")},
        {[] { return tr("USB-C 扩展坞"); }, QStringLiteral("SKU-4096"), QPixmap(), [] { return tr("数码旗舰店"); }, createImage(QColor(0xF9, 0xA8, 0xD4), QColor(0x9D, 0x17, 0x4D), avatarSize), QStringLiteral("0"), QStringLiteral("¥259.00")},
    };
    for (const Goods& goods : goodsList)
    {
        // 第三个商品没有图片，显示占位块
        QStandardItem* nameItem = createImageItem(QString(), goods.image, goods.sku);
        QStandardItem* sellerItem = createImageItem(QString(), goods.avatar);
        goodsModel->appendRow({nameItem, sellerItem, createItem(goods.stock), createItem(goods.price, Qt::AlignRight | Qt::AlignVCenter)});
        // 只改单元格文字，图片、说明与行顺序不变
        const ExampleI18n::TextFunction name = goods.name;
        const ExampleI18n::TextFunction seller = goods.seller;
        ExampleI18n::bind(goodsModel, [nameItem, sellerItem, name, seller]()
        {
            nameItem->setText(name());
            sellerItem->setText(seller());
        });
    }
    ExampleI18n::bind(goodsModel, [goodsModel]()
    {
        goodsModel->setHeaderData(0, Qt::Horizontal, tr("商品"), Qt::DisplayRole);
        goodsModel->setHeaderData(1, Qt::Horizontal, tr("卖家"), Qt::DisplayRole);
        goodsModel->setHeaderData(2, Qt::Horizontal, tr("库存"), Qt::DisplayRole);
        goodsModel->setHeaderData(3, Qt::Horizontal, tr("价格"), Qt::DisplayRole);
    });
    ShadTable* goodsTable = new ShadTable(this);
    goodsTable->setModel(goodsModel);
    goodsTable->setColumnImage(0, QSize(40, 40), ShadTableType::Rounded);
    goodsTable->setColumnImage(1, QSize(24, 24), ShadTableType::Circle);
    imageCard->contentLayout()->addWidget(goodsTable);
}

TablePage::~TablePage()
{
}
