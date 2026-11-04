#ifndef SHADTREESELECT_H
#define SHADTREESELECT_H

#include <QList>
#include <QModelIndex>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "ShadDef.h"
#include "ShadProperty.h"

class QAbstractItemModel;
class QEnterEvent;
class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QObject;
class QPaintEvent;
class QStandardItem;
class ShadTreeView;

// 树形下拉选择：点击触发框弹出树，支持单选与多选（多选以复选框勾选并显示为徽章），可选搜索过滤
// 默认使用内部模型，通过 addItem 添加节点；setModel 使用自己的模型时只使用第 0 列，
// 多选需自行给节点设置 Qt::ItemIsUserCheckable、Qt::CheckStateRole 初值与 Qt::ItemIsAutoTristate
class ShadTreeSelectPrivate;

class SHADCN_EXPORT ShadTreeSelect : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadTreeSelect)
    Q_PROPERTY(bool invalid READ isInvalid WRITE setInvalid)
    Q_PROPERTY(ShadComboBoxType::Size treeSelectSize READ getTreeSelectSize WRITE setTreeSelectSize)
    Q_PROPERTY(ShadComboBoxType::SelectionMode selectionMode READ getSelectionMode WRITE setSelectionMode)
    Q_PROPERTY(ShadTreeSelectType::CheckedStrategy checkedStrategy READ getCheckedStrategy WRITE setCheckedStrategy)
    Q_PROPERTY(bool leafOnly READ isLeafOnly WRITE setLeafOnly)
    Q_PROPERTY(bool fullPathVisible READ isFullPathVisible WRITE setFullPathVisible)
    Q_PROPERTY(bool searchEnabled READ isSearchEnabled WRITE setSearchEnabled)
    Q_PROPERTY(QString placeholderText READ getPlaceholderText WRITE setPlaceholderText)
    Q_PROPERTY(QString searchPlaceholderText READ getSearchPlaceholderText WRITE setSearchPlaceholderText)
    Q_PROPERTY(QString emptyText READ getEmptyText WRITE setEmptyText)

    public:
        explicit ShadTreeSelect(QWidget* parent = nullptr);
        explicit ShadTreeSelect(ShadComboBoxType::SelectionMode mode, QWidget* parent = nullptr);
        ~ShadTreeSelect() override;

        // 使用自己的模型，不转移所有权；传 nullptr 恢复内部模型
        void setModel(QAbstractItemModel* model);
        QAbstractItemModel* model() const;
        // 弹出层中的树，用于调整外观，不要修改其模型与选择模式
        ShadTreeView* treeView() const;

        // 向内部模型添加节点，使用自己的模型时返回 nullptr
        QStandardItem* addItem(const QString& text, QStandardItem* parent = nullptr);
        // 删除内部模型的全部节点
        void clearItems();

        // 错误态，边框显示为警示色
        void setInvalid(bool invalid);
        bool isInvalid() const;
        // 触发框尺寸，默认 Medium
        void setTreeSelectSize(ShadComboBoxType::Size size);
        ShadComboBoxType::Size getTreeSelectSize() const;

        // 选择模式，默认 SingleSelection
        void setSelectionMode(ShadComboBoxType::SelectionMode mode);
        ShadComboBoxType::SelectionMode getSelectionMode() const;
        // 多选模式下回显与返回哪些勾选节点，默认 ShowChild
        void setCheckedStrategy(ShadTreeSelectType::CheckedStrategy strategy);
        ShadTreeSelectType::CheckedStrategy getCheckedStrategy() const;
        // 单选模式只允许选择叶子节点，默认 false
        void setLeafOnly(bool leafOnly);
        bool isLeafOnly() const;
        // 单选模式在触发框中显示完整路径，默认 false
        void setFullPathVisible(bool visible);
        bool isFullPathVisible() const;

        // 弹出层顶部的搜索框，默认关闭
        void setSearchEnabled(bool enabled);
        bool isSearchEnabled() const;
        // 未选择时的占位文字，默认"请选择"
        void setPlaceholderText(const QString& text);
        QString getPlaceholderText() const;
        // 搜索框占位文字，默认"搜索..."
        void setSearchPlaceholderText(const QString& text);
        QString getSearchPlaceholderText() const;
        // 搜索没有匹配项时显示的文字，默认"未找到结果"
        void setEmptyText(const QString& text);
        QString getEmptyText() const;

        // 单选模式的当前项，传无效索引清空
        void setCurrentIndex(const QModelIndex& index);
        QModelIndex getCurrentIndex() const;
        // 触发框显示的文字，多选模式下为空
        QString getCurrentText() const;

        // 多选模式的勾选状态，父子节点联动
        void setIndexChecked(const QModelIndex& index, bool checked);
        bool isIndexChecked(const QModelIndex& index) const;
        // 按 CheckedStrategy 返回勾选的节点
        QList<QModelIndex> getCheckedIndexes() const;
        QStringList getCheckedTexts() const;
        void clearChecked();

        void showPopup();
        void hidePopup();
        bool isPopupVisible() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        // 当前项变化时发出，当前项被删除时也会发出
        void currentIndexChanged(const QModelIndex& index);
        // 勾选状态变化时发出，一次联动只发出一次
        void checkedIndexesChanged();

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADTREESELECT_H
