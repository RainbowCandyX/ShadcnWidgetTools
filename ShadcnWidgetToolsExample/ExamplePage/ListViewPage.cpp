#include "ListViewPage.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAbstractListModel>
#include <QColor>
#include <QEvent>
#include <QFont>
#include <QItemSelectionModel>
#include <QList>
#include <QModelIndex>
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <initializer_list>
#include <memory>

#include "ExampleI18n.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadEmpty.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadListView.h"

namespace
{
    // 示例列表的尺寸
    constexpr int kListHeight = 360;
    constexpr int kSizeListWidth = 300;
    constexpr int kSizeListHeight = 260;
    constexpr int kLargeRowCount = 100000;
    // 滚动加载的分页大小、总行数与模拟延迟
    constexpr int kPageSize = 50;
    constexpr int kMaxLoadedRows = 500;
    constexpr int kLoadDelayMs = 800;
    constexpr int kTaskBatchSize = 5;
    constexpr int kAvatarSize = 80;
    constexpr int kRepaintSampleMs = 1000;

    const ShadIconType::IconName kContactIcons[] = {ShadIconType::User, ShadIconType::Mail, ShadIconType::Phone, ShadIconType::MessageSquare};

    // 按行号即时生成数据的大列表模型
    class LargeListModel : public QAbstractListModel
    {
        public:
            explicit LargeListModel(QObject* parent = nullptr)
                : QAbstractListModel(parent)
            {
            }

            int rowCount(const QModelIndex& parent = QModelIndex()) const override
            {
                return parent.isValid() ? 0 : kLargeRowCount;
            }

            QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
            {
                if (!index.isValid() || index.row() < 0 || index.row() >= kLargeRowCount)
                {
                    return QVariant();
                }
                const int row = index.row();
                switch (role)
                {
                    case Qt::DisplayRole:
                        return ListViewPage::tr("联系人 %1").arg(row + 1);
                    case ShadListView::DescriptionRole:
                        return QStringLiteral("contact%1@example.com").arg(row + 1);
                    case ShadListView::TrailingTextRole:
                        return ListViewPage::tr("%1 分钟前").arg(row % 59 + 1);
                    case ShadListView::IconRole:
                        return static_cast<int>(kContactIcons[row % 4]);
                    default:
                        break;
                }
                return QVariant();
            }

            // 切换语言后通知视图重新读取全部文字
            void refreshTexts()
            {
                Q_EMIT dataChanged(index(0), index(kLargeRowCount - 1));
            }
    };

    // 统计视口重绘次数
    class RepaintCounter : public QObject
    {
        public:
            explicit RepaintCounter(QObject* parent = nullptr)
                : QObject(parent)
            {
            }

            // 返回上次读取以来的重绘次数并清零
            int takeCount()
            {
                const int count = _count;
                _count = 0;
                return count;
            }

        protected:
            bool eventFilter(QObject* watched, QEvent* event) override
            {
                if (event->type() == QEvent::Paint)
                {
                    ++_count;
                }
                return QObject::eventFilter(watched, event);
            }

        private:
            int _count{0};
    };

    struct Member
    {
        QString name;
        QString email;
        QString initials;
        QColor color;
        ExampleI18n::TextFunction role;
    };

    // 生成首字母头像
    QPixmap createAvatar(const QString& initials, const QColor& color)
    {
        QPixmap pixmap(kAvatarSize, kAvatarSize);
        pixmap.fill(color);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        QFont font = painter.font();
        font.setPixelSize(kAvatarSize * 2 / 5);
        font.setWeight(QFont::DemiBold);
        painter.setFont(font);
        painter.setPen(QColor(255, 255, 255));
        painter.drawText(pixmap.rect(), Qt::AlignCenter, initials);
        return pixmap;
    }

    // 团队成员模型，角色文字随语言刷新
    QStandardItemModel* createMemberModel(QObject* parent)
    {
        const QList<Member> members{
            {QStringLiteral("shadcn"), QStringLiteral("shadcn@example.com"), QStringLiteral("SC"), QColor(0x18, 0x18, 0x1b), [] { return ListViewPage::tr("所有者"); }},
            {QStringLiteral("maxleiter"), QStringLiteral("maxleiter@example.com"), QStringLiteral("ML"), QColor(0x25, 0x63, 0xeb), [] { return ListViewPage::tr("管理员"); }},
            {QStringLiteral("evilrabbit"), QStringLiteral("evilrabbit@example.com"), QStringLiteral("ER"), QColor(0xdc, 0x26, 0x26), [] { return ListViewPage::tr("成员"); }},
            {QStringLiteral("leerob"), QStringLiteral("leerob@example.com"), QStringLiteral("LR"), QColor(0x16, 0xa3, 0x4a), [] { return ListViewPage::tr("成员"); }},
            {QStringLiteral("rauchg"), QStringLiteral("rauchg@example.com"), QStringLiteral("GR"), QColor(0x93, 0x33, 0xea), [] { return ListViewPage::tr("管理员"); }},
            {QStringLiteral("delba"), QStringLiteral("delba@example.com"), QStringLiteral("DB"), QColor(0xea, 0x58, 0x0c), [] { return ListViewPage::tr("成员"); }},
            {QStringLiteral("steventey"), QStringLiteral("steventey@example.com"), QStringLiteral("ST"), QColor(0x08, 0x91, 0xb2), [] { return ListViewPage::tr("访客"); }},
            {QStringLiteral("shuding"), QStringLiteral("shuding@example.com"), QStringLiteral("SD"), QColor(0xdb, 0x27, 0x77), [] { return ListViewPage::tr("访客"); }},
        };
        QStandardItemModel* model = new QStandardItemModel(parent);
        for (const Member& member : members)
        {
            QStandardItem* item = new QStandardItem(member.name);
            item->setEditable(false);
            item->setData(member.email, ShadListView::DescriptionRole);
            item->setData(createAvatar(member.initials, member.color), Qt::DecorationRole);
            model->appendRow(item);
        }
        ExampleI18n::bind(model, [model, members]()
        {
            for (int row = 0; row < members.size(); ++row)
            {
                model->item(row)->setData(members.at(row).role(), ShadListView::TrailingTextRole);
            }
        });
        return model;
    }

    // 滚动加载示例的消息文字
    void applyMessageTexts(QStandardItem* item, int row)
    {
        item->setText(ListViewPage::tr("第 %1 条消息").arg(row + 1));
        item->setData(ListViewPage::tr("这是第 %1 条消息的摘要内容").arg(row + 1), ShadListView::DescriptionRole);
        item->setData(ListViewPage::tr("%1 分钟前").arg(row % 59 + 1), ShadListView::TrailingTextRole);
    }

    void appendMessages(QStandardItemModel* model, int count)
    {
        for (int index = 0; index < count; ++index)
        {
            QStandardItem* item = new QStandardItem();
            item->setEditable(false);
            item->setData(static_cast<int>(ShadIconType::Bell), ShadListView::IconRole);
            applyMessageTexts(item, model->rowCount());
            model->appendRow(item);
        }
    }

    // 空状态示例的任务文字
    void applyTaskText(QStandardItem* item, int row)
    {
        item->setText(ListViewPage::tr("任务 %1").arg(row + 1));
    }

    void appendTasks(QStandardItemModel* model, int count)
    {
        for (int index = 0; index < count; ++index)
        {
            QStandardItem* item = new QStandardItem();
            item->setEditable(false);
            item->setData(static_cast<int>(ShadIconType::ListTodo), ShadListView::IconRole);
            applyTaskText(item, model->rowCount());
            model->appendRow(item);
        }
    }

    ShadListView* createList(QAbstractItemModel* model, int height, QWidget* parent)
    {
        ShadListView* list = new ShadListView(parent);
        list->setModel(model);
        list->setFixedHeight(height);
        return list;
    }

    // 文字由调用方按当前状态绑定
    ShadLabel* createMutedText(QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        return label;
    }

    ShadButton* createToolButton(ExampleI18n::TextFunction text, ShadIconType::IconName icon, QWidget* parent)
    {
        ShadButton* button = new ShadButton(QString(), ShadButtonType::Outline, parent);
        ExampleI18n::bind(button, [button, text]() { button->setText(text()); });
        button->setButtonSize(ShadButtonType::Small);
        button->setShadIcon(icon);
        return button;
    }
} // namespace

ListViewPage::ListViewPage(QWidget* parent)
    : ExampleBasePage([] { return tr("虚拟列表"); }, [] { return tr("ShadListView：继承 QListView，只绘制可见行，所有行等高，大量数据也能流畅滚动；行外观与 Item 一致。"); }, parent)
{
    ShadCard* largeCard = addSection([] { return tr("十万行数据"); }, [] { return tr("模型在 data() 中按行号即时生成数据，不预先创建；滚动时只读取并绘制可见行"); });
    LargeListModel* largeModel = new LargeListModel(this);
    ShadListView* largeList = createList(largeModel, kListHeight, this);
    addStretchItem(largeCard, largeList);
    ShadLabel* largeCountLabel = createMutedText(this);
    ShadLabel* repaintLabel = createMutedText(this);
    addRow(largeCard, {largeCountLabel, repaintLabel});
    // 定时统计视口重绘次数，用于观察静止时不重绘
    RepaintCounter* repaintCounter = new RepaintCounter(largeList);
    largeList->viewport()->installEventFilter(repaintCounter);
    std::shared_ptr<int> repaintCount = std::make_shared<int>(0);
    auto updateRepaintLabel = [repaintLabel, repaintCount]()
    {
        repaintLabel->setText(*repaintCount > 0 ? tr("视口重绘：%1 次/秒").arg(*repaintCount) : tr("视口重绘：静止"));
    };
    QTimer* repaintTimer = new QTimer(this);
    repaintTimer->setInterval(kRepaintSampleMs);
    connect(repaintTimer, &QTimer::timeout, repaintLabel, [repaintCounter, repaintCount, updateRepaintLabel]()
    {
        *repaintCount = repaintCounter->takeCount();
        updateRepaintLabel();
    });
    repaintTimer->start();
    ExampleI18n::bind(largeList, [largeModel, largeCountLabel, updateRepaintLabel]()
    {
        largeModel->refreshTexts();
        largeCountLabel->setText(tr("共 %1 行").arg(kLargeRowCount));
        updateRepaintLabel();
    });

    ShadCard* sizeCard = addSection([] { return tr("尺寸与头像"); }, [] { return tr("从左到右依次为 Medium、Small、ExtraSmall；三个列表共用同一个模型，图片按 setImageShape(Circle) 裁剪为圆形"); });
    QStandardItemModel* memberModel = createMemberModel(this);
    QList<QWidget*> sizeLists;
    for (const ShadItemType::Size size : {ShadItemType::Medium, ShadItemType::Small, ShadItemType::ExtraSmall})
    {
        ShadListView* sizeList = createList(memberModel, kSizeListHeight, this);
        sizeList->setFixedWidth(kSizeListWidth);
        sizeList->setItemSize(size);
        sizeList->setImageShape(ShadTableType::Circle);
        sizeLists.append(sizeList);
    }
    addRow(sizeCard, sizeLists);

    ShadCard* checkCard = addSection([] { return tr("多选与复选框"); }, [] { return tr("带 Qt::CheckStateRole 的行显示复选框，点击复选框或按空格切换；Ctrl / Shift 多选行，与复选框相互独立"); });
    const QList<ExampleI18n::TextFunction> tasks{
        [] { return tr("设计评审"); },
        [] { return tr("编写接口文档"); },
        [] { return tr("修复登录问题"); },
        [] { return tr("更新依赖"); },
        [] { return tr("准备发布说明"); },
        [] { return tr("整理测试用例"); },
        [] { return tr("优化启动速度"); },
        [] { return tr("回复用户反馈"); },
    };
    QStandardItemModel* checkModel = new QStandardItemModel(this);
    for (int row = 0; row < tasks.size(); ++row)
    {
        QStandardItem* item = new QStandardItem();
        item->setEditable(false);
        item->setCheckable(true);
        item->setCheckState(row < 2 ? Qt::Checked : Qt::Unchecked);
        item->setData(static_cast<int>(ShadIconType::ClipboardList), ShadListView::IconRole);
        checkModel->appendRow(item);
    }
    ShadListView* checkList = createList(checkModel, kSizeListHeight, this);
    checkList->setItemSize(ShadItemType::Small);
    checkList->setDescriptionVisible(false);
    checkList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    addStretchItem(checkCard, checkList);
    ShadLabel* checkLabel = createMutedText(this);
    addItem(checkCard, checkLabel);
    auto updateCheckLabel = [checkModel, checkList, checkLabel]()
    {
        int checkedCount = 0;
        for (int row = 0; row < checkModel->rowCount(); ++row)
        {
            if (checkModel->item(row)->checkState() == Qt::Checked)
            {
                ++checkedCount;
            }
        }
        const int selectedCount = static_cast<int>(checkList->selectionModel()->selectedRows().size());
        checkLabel->setText(tr("已勾选 %1 项，已选中 %2 行").arg(checkedCount).arg(selectedCount));
    };
    connect(checkModel, &QStandardItemModel::itemChanged, checkLabel, updateCheckLabel);
    connect(checkList->selectionModel(), &QItemSelectionModel::selectionChanged, checkLabel, updateCheckLabel);
    ExampleI18n::bind(checkModel, [checkModel, tasks, updateCheckLabel]()
    {
        for (int row = 0; row < tasks.size(); ++row)
        {
            checkModel->item(row)->setText(tasks.at(row)());
        }
        updateCheckLabel();
    });

    ShadCard* loadCard = addSection([] { return tr("滚动加载"); }, [] { return tr("滚动到距底部 200px 以内时发出 reachedEnd，setLoadingMore(true) 在底部显示加载指示；每次加载 50 行，共 500 行"); });
    QStandardItemModel* loadModel = new QStandardItemModel(this);
    appendMessages(loadModel, kPageSize);
    ShadListView* loadList = createList(loadModel, kListHeight, this);
    addStretchItem(loadCard, loadList);
    ShadLabel* loadLabel = createMutedText(this);
    addItem(loadCard, loadLabel);
    auto updateLoadLabel = [loadModel, loadLabel]()
    {
        const int rowCount = loadModel->rowCount();
        loadLabel->setText(rowCount >= kMaxLoadedRows ? tr("已全部加载 %1 行").arg(rowCount) : tr("已加载 %1 / %2 行").arg(rowCount).arg(kMaxLoadedRows));
    };
    connect(loadList, &ShadListView::reachedEnd, loadList, [loadList, loadModel, updateLoadLabel]()
    {
        if (loadList->isLoadingMore() || loadModel->rowCount() >= kMaxLoadedRows)
        {
            return;
        }
        loadList->setLoadingMore(true);
        // 模拟网络请求，列表销毁后定时器随之取消
        QTimer::singleShot(kLoadDelayMs, loadList, [loadList, loadModel, updateLoadLabel]()
        {
            appendMessages(loadModel, qMin(kPageSize, kMaxLoadedRows - loadModel->rowCount()));
            loadList->setLoadingMore(false);
            updateLoadLabel();
        });
    });
    ExampleI18n::bind(loadModel, [loadModel, updateLoadLabel]()
    {
        for (int row = 0; row < loadModel->rowCount(); ++row)
        {
            applyMessageTexts(loadModel->item(row), row);
        }
        updateLoadLabel();
    });

    ShadCard* emptyCard = addSection([] { return tr("空列表"); }, [] { return tr("模型没有行时显示 emptyState()，可设置图标、标题、描述与操作按钮"); });
    QStandardItemModel* taskModel = new QStandardItemModel(this);
    ShadButton* addTaskButton = createToolButton([] { return tr("添加任务"); }, ShadIconType::Plus, this);
    ShadButton* clearTaskButton = createToolButton([] { return tr("清空"); }, ShadIconType::Trash2, this);
    addRow(emptyCard, {addTaskButton, clearTaskButton});
    ShadListView* taskList = createList(taskModel, kSizeListHeight, this);
    taskList->setItemSize(ShadItemType::Small);
    taskList->setDescriptionVisible(false);
    ShadEmpty* emptyState = taskList->emptyState();
    emptyState->setShadIcon(ShadIconType::ClipboardList);
    ShadButton* emptyAddButton = new ShadButton(QString(), ShadButtonType::Default, emptyState);
    emptyAddButton->setButtonSize(ShadButtonType::Small);
    emptyAddButton->setShadIcon(ShadIconType::Plus);
    emptyState->contentLayout()->addWidget(emptyAddButton);
    addStretchItem(emptyCard, taskList);
    connect(addTaskButton, &ShadButton::clicked, taskModel, [taskModel]() { appendTasks(taskModel, kTaskBatchSize); });
    connect(emptyAddButton, &ShadButton::clicked, taskModel, [taskModel]() { appendTasks(taskModel, kTaskBatchSize); });
    connect(clearTaskButton, &ShadButton::clicked, taskModel, [taskModel]() { taskModel->removeRows(0, taskModel->rowCount()); });
    ExampleI18n::bind(taskList, [taskModel, emptyState, emptyAddButton]()
    {
        emptyState->setTitle(tr("暂无任务"));
        emptyState->setDescription(tr("添加的任务会显示在这里。"));
        emptyAddButton->setText(tr("添加任务"));
        for (int row = 0; row < taskModel->rowCount(); ++row)
        {
            applyTaskText(taskModel->item(row), row);
        }
    });
}

ListViewPage::~ListViewPage()
{
}
