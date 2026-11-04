#ifndef SHADTREESELECTPANEL_H
#define SHADTREESELECTPANEL_H

#include <QList>
#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QSize>
#include <QString>
#include <QWidget>

class QLineEdit;
class QPaintEvent;
class QResizeEvent;
class ShadTreeView;

// ShadTreeSelect 弹出层的内容（可选搜索栏加树），组件库内部使用
// 过滤直接隐藏树的行而不用代理模型，索引无需转换，勾选联动也能作用到被隐藏的节点
class ShadTreeSelectPanel : public QWidget
{
    Q_OBJECT

    public:
        // 树区域最大高度
        static constexpr int kMaxTreeHeight = 300;
        // 弹出层最小宽度
        static constexpr int kMinWidth = 128;

        explicit ShadTreeSelectPanel(QWidget* parent = nullptr);
        ~ShadTreeSelectPanel() override;

        ShadTreeView* treeView() const;
        QLineEdit* searchEdit() const;

        void setSearchVisible(bool visible);
        bool isSearchVisible() const;
        void setSearchPlaceholderText(const QString& text);
        void setEmptyText(const QString& text);

        // 清空搜索并恢复过滤前的展开状态，不发出 filtered
        void resetFilter();
        // 第一个未被隐藏的顶层节点
        QModelIndex firstVisibleIndex() const;
        // 节点自身与全部祖先都没有被过滤隐藏
        bool isIndexUnhidden(const QModelIndex& index) const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        // 过滤完成后发出，firstMatch 为第一个匹配的节点
        void filtered(const QModelIndex& firstMatch);

    protected:
        void paintEvent(QPaintEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;

    private:
        void applyThemeColors();
        void applyFilter(const QString& query);
        // 递归过滤子节点，返回子树中是否有匹配项
        bool filterRows(const QModelIndex& parent, const QString& query, bool keepAll, QModelIndex* firstMatch);
        // 取消过滤并恢复展开状态
        void restoreRows();
        void showAllRows(const QModelIndex& parent);
        void saveExpandedRows(const QModelIndex& parent);
        int visibleRowCount(const QModelIndex& parent) const;
        bool hasVisibleRows() const;
        int searchBarHeight() const;
        void layoutContent();

        QLineEdit* _searchEdit{nullptr};
        ShadTreeView* _treeView{nullptr};
        QString _emptyText;
        // 开始过滤前展开的节点，取消过滤时恢复
        QList<QPersistentModelIndex> _savedExpandedRows;
        bool _isSearchVisible{false};
        bool _isFiltering{false};
};

#endif // SHADTREESELECTPANEL_H
