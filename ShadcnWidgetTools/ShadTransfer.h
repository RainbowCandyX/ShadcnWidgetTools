#ifndef SHADTRANSFER_H
#define SHADTRANSFER_H

#include <QList>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QEvent;

// 穿梭框中的一项，key 在全部数据中唯一
struct ShadTransferItem
{
    QString key;
    QString title;
    QString description;
    ShadIconType::IconName icon{ShadIconType::None};
    // 禁用的项不能勾选与移动
    bool isDisabled{false};
};

// 穿梭框，对应 Ant Design Transfer：左右两个带边框的列表与中间的移动按钮，在两个列表之间移动勾选的项
// 列表头部为全选复选框、标题与"已选/总数"计数；列表使用 ShadListView，点击整行切换勾选，空格键同样切换；没有项时显示空状态
// 可选的搜索框按标题过滤（忽略大小写），全选只作用于过滤后可见的项；RTL 时左右互换，移动按钮的箭头随之翻转
class ShadTransferPrivate;

class SHADCN_EXPORT ShadTransfer : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadTransfer)
    Q_PROPERTY(QStringList targetKeys READ getTargetKeys WRITE setTargetKeys)
    Q_PROPERTY(bool searchEnabled READ isSearchEnabled WRITE setSearchEnabled)

    public:
        explicit ShadTransfer(QWidget* parent = nullptr);
        ~ShadTransfer() override;

        // 全部数据，源列表显示不在 targetKeys 中的项，目标列表显示其余项，两边都按数据顺序排列
        void setItems(const QList<ShadTransferItem>& items);
        QList<ShadTransferItem> getItems() const;

        // 目标列表中的项
        void setTargetKeys(const QStringList& keys);
        QStringList getTargetKeys() const;

        // 两个列表的标题，默认"可选项"与"已选项"
        void setTitles(const QString& sourceTitle, const QString& targetTitle);
        QString getSourceTitle() const;
        QString getTargetTitle() const;

        // 列表顶部的搜索框，默认关闭
        void setSearchEnabled(bool enabled);
        bool isSearchEnabled() const;

        // 两个列表中当前勾选的项
        QStringList getSourceCheckedKeys() const;
        QStringList getTargetCheckedKeys() const;

    public Q_SLOTS:
        // 把源列表中勾选的项移到目标列表，与点击移动按钮相同
        void moveToTarget();
        // 把目标列表中勾选的项移回源列表
        void moveToSource();

    Q_SIGNALS:
        // 通过移动按钮或 moveToTarget / moveToSource 移动项后发出，movedKeys 为本次移动的项
        void targetKeysChanged(const QStringList& targetKeys, ShadTransferType::Direction direction, const QStringList& movedKeys);

    protected:
        void changeEvent(QEvent* event) override;
};

#endif // SHADTRANSFER_H
