#ifndef SHADCOMBOBOXPRIVATE_H
#define SHADCOMBOBOXPRIVATE_H

#include <QElapsedTimer>
#include <QRectF>
#include <QString>

#include "ShadDef.h"

class QPainter;
class ShadComboBox;
class ShadComboBoxPopup;
class ShadTransition;

class ShadComboBoxPrivate
{
    SHAD_D_CREATE(ShadComboBox)

    public:
        ShadComboBoxPrivate();
        ~ShadComboBoxPrivate();

        // 触发框可视高度（不含焦点环边距）
        static int bodyHeight(ShadComboBoxType::Size size);
        // 选中一项：更新当前项并发出 activated / textActivated，与用户在 QComboBox 中选择的行为一致
        void activateRow(int row);
        // 关闭状态下按输入的文字直接切换到匹配的项
        void handleTypeahead(const QString& text);
        // 是否处于按键查找中（缓冲未超时），此时空格作为查找文字而不是打开弹出层
        bool isTypeaheadActive() const;
        // 该行可以被选中：不是分隔线且未禁用
        bool isRowSelectable(int row) const;
        bool isSeparatorRow(int row) const;
        bool isMultiSelection() const;
        // 多选模式下切换一行的勾选状态
        void toggleRow(int row);
        // 在 rect 内单行绘制已选项的徽章，放不下的部分合并为 +N 徽章
        void paintCheckedBadges(QPainter* painter, const QRectF& rect) const;

        ShadComboBoxType::Size _size{ShadComboBoxType::Medium};
        ShadComboBoxType::SelectionMode _selectionMode{ShadComboBoxType::SingleSelection};
        bool _isInvalid{false};
        bool _isFocusVisible{false};
        bool _isSearchEnabled{false};
        QString _searchPlaceholderText;
        QString _emptyText;
        ShadTransition* _hoverTransition{nullptr};
        ShadComboBoxPopup* _popup{nullptr};
        QString _typeaheadBuffer;
        QElapsedTimer _typeaheadTimer;
};

#endif // SHADCOMBOBOXPRIVATE_H
