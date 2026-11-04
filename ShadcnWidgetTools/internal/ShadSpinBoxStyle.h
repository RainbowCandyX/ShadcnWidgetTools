#ifndef SHADSPINBOXSTYLE_H
#define SHADSPINBOXSTYLE_H

#include <QAbstractSpinBox>
#include <QPointer>
#include <QProxyStyle>
#include <QRect>
#include <QSize>
#include <QStringList>

#include "ShadDef.h"

class QLineEdit;
class QPainter;
class QPoint;
class QStyleOption;
class QStyleOptionComplex;
class QWidget;

// ShadSpinBox 与 ShadDoubleSpinBox 共用的样式，组件库内部使用
// QAbstractSpinBox 按样式的 subControlRect 放置输入区并判断按钮点击，这里按按钮布局计算区域，并自绘外框与按钮
// 每个数字输入框各有一个实例（父对象为输入框），记录该输入框的按钮布局与错误态
// 外观与 ShadInput 一致：可视高度 36，border-input rounded-md shadow-xs，获得焦点时 border-ring 与 3px ring，四周预留 3px 焦点环边距
class ShadSpinBoxStyle : public QProxyStyle
{
    Q_OBJECT

    public:
        // 设置输入框的字体、内部输入框的样式与焦点策略，并跟随主题更新颜色；调用方随后用 setStyle 应用本样式
        explicit ShadSpinBoxStyle(QAbstractSpinBox* spinBox);
        ~ShadSpinBoxStyle() override;

        void setButtonLayout(ShadSpinBoxType::ButtonLayout layout);
        ShadSpinBoxType::ButtonLayout getButtonLayout() const;
        void setInvalid(bool invalid);
        bool isInvalid() const;

        // 按取值范围的文字计算尺寸提示：texts 为最小值、最大值等需要完整显示的文字（含前后缀）
        QSize sizeHintForTexts(const QStringList& texts) const;

        // 右键菜单：撤销、恢复、剪切、复制、粘贴、删除、全选、增加、减少；lineEdit 与 stepEnabled 为 QAbstractSpinBox 的受保护成员，由调用方传入
        static void showContextMenu(QAbstractSpinBox* spinBox, QLineEdit* lineEdit, QAbstractSpinBox::StepEnabled stepEnabled, const QPoint& globalPos);

        void drawComplexControl(ComplexControl control, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget = nullptr) const override;
        QRect subControlRect(ComplexControl control, const QStyleOptionComplex* option, SubControl subControl, const QWidget* widget = nullptr) const override;

    private:
        // 内部输入框的文字、选中与禁用颜色
        void applyThemeColors();
        // 按钮布局变化后重新放置内部输入框
        void relayout();
        // 按 LTR 计算的区域
        QRect logicalSubControlRect(const QRect& rect, SubControl subControl) const;
        void drawButton(QPainter* painter, const QStyleOptionComplex* option, SubControl subControl, bool isStepEnabled) const;

        QPointer<QAbstractSpinBox> _spinBox;
        QLineEdit* _lineEdit{nullptr};
        ShadSpinBoxType::ButtonLayout _layout{ShadSpinBoxType::Inline};
        bool _isInvalid{false};
};

#endif // SHADSPINBOXSTYLE_H
