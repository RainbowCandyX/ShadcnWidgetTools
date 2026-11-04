#ifndef SHADINPUTDIALOGPRIVATE_H
#define SHADINPUTDIALOGPRIVATE_H

#include <QLineEdit>
#include <QString>

#include "ShadDef.h"

class ShadButton;
class ShadDoubleSpinBox;
class ShadField;
class ShadInput;
class ShadSpinBox;
class ShadTextarea;

class ShadInputDialogPrivate
{
    public:
        ShadInputDialogPrivate();
        ~ShadInputDialogPrivate();

        // 多行模式输入框的最小可视高度
        static constexpr int kMultiLineMinimumHeight = 120;

        ShadInputDialogType::InputMode _inputMode{ShadInputDialogType::TextInput};
        ShadField* _field{nullptr};
        // 当前模式的输入控件，其余为空；切换模式时旧控件由 ShadField 删除
        ShadInput* _lineEdit{nullptr};
        ShadTextarea* _textarea{nullptr};
        ShadSpinBox* _spinBox{nullptr};
        ShadDoubleSpinBox* _doubleSpinBox{nullptr};
        ShadButton* _cancelButton{nullptr};
        ShadButton* _okButton{nullptr};

        QString _textValue;
        QString _placeholderText;
        QLineEdit::EchoMode _echoMode{QLineEdit::Normal};

        int _intValue{0};
        int _intMinimum{-2147483647};
        int _intMaximum{2147483647};
        int _intStep{1};

        double _doubleValue{0.0};
        double _doubleMinimum{-2147483647.0};
        double _doubleMaximum{2147483647.0};
        int _doubleDecimals{2};
        double _doubleStep{1.0};
};

#endif // SHADINPUTDIALOGPRIVATE_H
