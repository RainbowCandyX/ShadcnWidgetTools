#ifndef SHADAUTOCOMPLETE_H
#define SHADAUTOCOMPLETE_H

#include <QScopedPointer>
#include <QString>
#include <QStringList>

#include "ShadDef.h"
#include "ShadInput.h"
#include "ShadProperty.h"

class QAbstractItemModel;
class QEvent;
class QFocusEvent;
class QHideEvent;
class QKeyEvent;
class QObject;
class QWidget;
class ShadAutoCompletePrivate;

// 自动补全输入框，对应 Ant Design AutoComplete：继承 ShadInput，输入时在下方弹出建议列表，外观与 ShadComboBox 的弹出层一致
// 弹出层不抢占焦点：上下方向键切换高亮项，回车选中，Esc 关闭；点击建议项同样选中，选中后把文字填入输入框并发出 optionSelected
// 建议来自 setOptions 的字符串列表或 setModel 的模型（第 0 列的 DisplayRole 与 DecorationRole，禁用的行不可选）
// 远程搜索：把 filterMode 设为 NoFilter，在 searchTextChanged 中请求数据后调用 setOptions，输入框有焦点时建议列表自动刷新
class SHADCN_EXPORT ShadAutoComplete : public ShadInput
{
    Q_OBJECT
    Q_PROPERTY(QStringList options READ getOptions WRITE setOptions)
    Q_PROPERTY(ShadAutoCompleteType::FilterMode filterMode READ getFilterMode WRITE setFilterMode)
    Q_PROPERTY(int maxVisibleItems READ getMaxVisibleItems WRITE setMaxVisibleItems)

    public:
        explicit ShadAutoComplete(QWidget* parent = nullptr);
        ~ShadAutoComplete() override;

        // 建议选项，使用内置的字符串模型；之前通过 setModel 设置的模型不再使用
        void setOptions(const QStringList& options);
        QStringList getOptions() const;

        // 自定义模型，不接管所有权；传 nullptr 恢复内置的字符串模型
        void setModel(QAbstractItemModel* model);
        QAbstractItemModel* model() const;

        // 过滤方式，默认 Fuzzy
        void setFilterMode(ShadAutoCompleteType::FilterMode mode);
        ShadAutoCompleteType::FilterMode getFilterMode() const;

        // 建议列表最多同时显示的行数，超出时滚动，默认 8
        void setMaxVisibleItems(int count);
        int getMaxVisibleItems() const;

        bool isPopupVisible() const;
        // 按当前文字过滤后显示建议列表，没有匹配项时不显示
        void showPopup();
        void hidePopup();

    Q_SIGNALS:
        // 用户输入改变文字时发出，不包括 setText 与选中建议
        void searchTextChanged(const QString& text);
        // 选中建议时发出，row 为模型中的行号
        void optionSelected(const QString& text, int row);

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void hideEvent(QHideEvent* event) override;

    private:
        QScopedPointer<ShadAutoCompletePrivate> _private;
};

#endif // SHADAUTOCOMPLETE_H
