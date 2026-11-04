#ifndef SHADCOMBOBOXPOPUP_H
#define SHADCOMBOBOXPOPUP_H

#include <QElapsedTimer>
#include <QIcon>
#include <QList>
#include <QMetaObject>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>
#include <QWidget>

class QEvent;
class QHideEvent;
class QKeyEvent;
class QLineEdit;
class QMouseEvent;
class QObject;
class QPaintEvent;
class QVariantAnimation;
class ShadComboBox;
class ShadScrollArea;

// 下拉列表中的一项，row 为 QComboBox 模型中的行号
struct ShadComboBoxPopupItem
{
    int row{-1};
    QString text;
    QIcon icon;
    bool isEnabled{true};
    bool isSeparator{false};
    // 多选模式下的勾选状态，来自模型的 Qt::CheckStateRole
    bool isChecked{false};
};

// 下拉列表，对应 SelectItem / CommandItem：高亮项为 accent 背景；单选时当前选中项右侧显示对勾，多选时每一项左侧显示复选框；不对外导出
class ShadComboBoxList : public QWidget
{
    Q_OBJECT

    public:
        // 对应 py-1.5 + text-sm 行高
        static constexpr int kItemHeight = 32;
        // 对应 p-1
        static constexpr int kPadding = 4;

        explicit ShadComboBoxList(QWidget* parent = nullptr);
        ~ShadComboBoxList() override;

        void setItems(const QList<ShadComboBoxPopupItem>& items);
        const QList<ShadComboBoxPopupItem>& getItems() const;

        // 当前选中项的模型行号，右侧显示对勾；多选模式下不使用
        void setCurrentRow(int row);
        // 多选模式：每一项左侧显示复选框，右侧不显示对勾
        void setMultiSelection(bool isMultiSelection);
        // 没有任何项时居中显示的文字
        void setEmptyText(const QString& text);

        // index 为 getItems() 中的下标，-1 表示没有高亮项
        void setHighlightIndex(int index);
        int getHighlightIndex() const;
        // 从当前高亮项沿 step 的方向移动一项，跳过分隔线与禁用项，到达两端时停住
        void moveHighlight(int step);
        // 高亮第一个或最后一个可选项
        void highlightFirst();
        void highlightLast();
        // 高亮模型行号为 row 的项，找不到或不可选时高亮第一个可选项
        void highlightRow(int row);

        bool isSelectable(int index) const;
        QRect itemRect(int index) const;

        QSize sizeHint() const override;

    Q_SIGNALS:
        // 点击可选项时发出，参数为 getItems() 中的下标
        void itemClicked(int index);
        // 高亮项变化时发出，参数为 getItems() 中的下标
        void highlightChanged(int index);

    protected:
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        int itemHeight(int index) const;
        int indexAt(const QPoint& pos) const;
        void updateItemTops();

        QList<ShadComboBoxPopupItem> _items;
        // 每一项顶部的 y 坐标，最后多一个元素记录内容底部
        QList<int> _itemTops;
        QString _emptyText;
        int _currentRow{-1};
        int _highlightIndex{-1};
        bool _isMultiSelection{false};
};

// ShadComboBox 的弹出层，对应 SelectContent / Popover + Command，不对外导出
// 显示在触发框下方（空间不足时在上方），可选顶部搜索框按模糊匹配过滤，打开时淡入并从触发框方向滑入
class ShadComboBoxPopup : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadComboBoxPopup(ShadComboBox* comboBox);
        ~ShadComboBoxPopup() override;

        // 按下拉框当前的模型、选中项与搜索设置打开弹出层
        void open();

    Q_SIGNALS:
        // 单选模式选中一项时发出，参数为模型行号
        void rowActivated(int row);
        // 多选模式切换一项的勾选状态时发出，参数为模型行号，弹出层保持打开
        void rowToggled(int row);
        // 高亮项变化时发出，参数为模型行号
        void rowHighlighted(int row);
        void popupHidden();

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void hideEvent(QHideEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        // 刷新搜索框的文字、占位符与选中颜色
        void applyThemeColors();
        // 从下拉框的模型读取全部项
        void loadItems();
        // 模型数据变化时重新读取并过滤，保留当前高亮项（多选模式切换勾选时走这里）
        void reloadItems();
        // 打开或清空搜索时初始高亮的模型行号：单选为当前项，多选为第一个已勾选项
        int initialHighlightRow() const;
        // 按搜索文字过滤并排序，然后重新计算弹出层大小
        void applyFilter();
        // 计算弹出层大小与位置，keepSide 为 true 时保持当前的上下方向（搜索过滤后高度变化时使用）
        void updatePopupGeometry(bool keepSide);
        // 按打开动画进度放置内容
        void layoutContent();
        // 处理方向键、回车、Esc 等导航按键，返回 true 表示已处理
        bool handleNavigationKey(QKeyEvent* event);
        // 没有搜索框时，按输入的文字跳到匹配的项
        void handleTypeahead(const QString& text);
        void activateHighlighted();
        void scrollToHighlight();
        // 卡片在弹出层中的区域（含打开动画的位移），四周留给 shadow-md
        QRect cardRect() const;
        int searchBarHeight() const;
        int maxListHeight() const;
        void disconnectModel();

        ShadComboBox* _comboBox{nullptr};
        QWidget* _content{nullptr};
        QLineEdit* _searchEdit{nullptr};
        ShadScrollArea* _scrollArea{nullptr};
        ShadComboBoxList* _list{nullptr};
        QVariantAnimation* _openAnimation{nullptr};
        QList<ShadComboBoxPopupItem> _allItems;
        QList<QMetaObject::Connection> _modelConnections;
        qreal _openProgress{1.0};
        bool _isAbove{false};
        bool _isSearchEnabled{false};
        bool _isMultiSelection{false};
        QString _typeaheadBuffer;
        QElapsedTimer _typeaheadTimer;
};

#endif // SHADCOMBOBOXPOPUP_H
