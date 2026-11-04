#ifndef SHADCOMBOBOX_H
#define SHADCOMBOBOX_H

#include <QComboBox>
#include <QList>
#include <QSize>
#include <QString>
#include <QStringList>

#include "ShadDef.h"
#include "ShadProperty.h"

class QEnterEvent;
class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QWheelEvent;

// 下拉选择框，对应 shadcn Select；开启搜索后对应 shadcn Combobox（弹出层顶部搜索框，按模糊匹配过滤并按相关度排序）
// 保留 QComboBox 的模型与接口（addItem、setCurrentIndex、insertSeparator、currentIndexChanged 等），触发框与弹出层全部自绘
// 不支持 setEditable(true)；关闭状态下滚轮不切换选项；未选中时显示 placeholderText
// 多选模式下勾选状态保存在模型的 Qt::CheckStateRole 中，currentIndex 不参与显示；直接修改模型的勾选状态不会发出 checkedRowsChanged
class ShadComboBoxPrivate;

class SHADCN_EXPORT ShadComboBox : public QComboBox
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadComboBox)
    Q_PROPERTY(bool invalid READ isInvalid WRITE setInvalid NOTIFY invalidChanged)
    Q_PROPERTY(ShadComboBoxType::Size comboBoxSize READ getComboBoxSize WRITE setComboBoxSize)
    Q_PROPERTY(bool searchEnabled READ isSearchEnabled WRITE setSearchEnabled)
    Q_PROPERTY(QString searchPlaceholderText READ getSearchPlaceholderText WRITE setSearchPlaceholderText)
    Q_PROPERTY(QString emptyText READ getEmptyText WRITE setEmptyText)
    Q_PROPERTY(ShadComboBoxType::SelectionMode selectionMode READ getSelectionMode WRITE setSelectionMode)

    public:
        explicit ShadComboBox(QWidget* parent = nullptr);
        ~ShadComboBox() override;

        // 错误态，对应 aria-invalid：边框变为 destructive 色，键盘聚焦时焦点环同为 destructive 色
        void setInvalid(bool invalid);
        bool isInvalid() const;

        // 触发框尺寸，默认 Medium（36px），Small 为 24px
        void setComboBoxSize(ShadComboBoxType::Size size);
        ShadComboBoxType::Size getComboBoxSize() const;

        // 弹出层顶部的搜索框，默认关闭；开启后按模糊匹配过滤选项，搜索时隐藏分隔线
        void setSearchEnabled(bool enabled);
        bool isSearchEnabled() const;

        // 搜索框占位文字，默认"搜索..."
        void setSearchPlaceholderText(const QString& text);
        QString getSearchPlaceholderText() const;

        // 搜索没有匹配项时显示的文字，默认"未找到结果"
        void setEmptyText(const QString& text);
        QString getEmptyText() const;

        // 选择模式，默认 SingleSelection；多选模式下点击选项切换勾选状态，弹出层保持打开
        void setSelectionMode(ShadComboBoxType::SelectionMode mode);
        ShadComboBoxType::SelectionMode getSelectionMode() const;

        // 多选模式的勾选状态，行号为模型中的行，分隔线始终不勾选；状态变化时发出 checkedRowsChanged
        void setCheckedRows(const QList<int>& rows);
        QList<int> getCheckedRows() const;
        QStringList getCheckedTexts() const;
        void setRowChecked(int row, bool checked);
        bool isRowChecked(int row) const;
        void clearChecked();

        bool isPopupVisible() const;

        void showPopup() override;
        void hidePopup() override;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void invalidChanged();
        // 通过本类接口或弹出层改变多选勾选状态时发出
        void checkedRowsChanged();

    protected:
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void changeEvent(QEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADCOMBOBOX_H
