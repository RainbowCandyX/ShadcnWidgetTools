#ifndef SHADCOLORDIALOGPRIVATE_H
#define SHADCOLORDIALOGPRIVATE_H

class ShadButton;
class ShadColorPicker;

class ShadColorDialogPrivate
{
    public:
        ShadColorDialogPrivate();
        ~ShadColorDialogPrivate();

        // 对话框宽度：取色面板 472 加左右 p-6
        static constexpr int kDialogWidth = 520;

        ShadColorPicker* _picker{nullptr};
        ShadButton* _cancelButton{nullptr};
        ShadButton* _okButton{nullptr};
};

#endif // SHADCOLORDIALOGPRIVATE_H
