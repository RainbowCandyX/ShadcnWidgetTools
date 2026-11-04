#ifndef SHADPOPCONFIRMPRIVATE_H
#define SHADPOPCONFIRMPRIVATE_H

class ShadButton;
class ShadPopconfirmIcon;

class ShadPopconfirmPrivate
{
    public:
        ShadPopconfirmPrivate();
        ~ShadPopconfirmPrivate();

        static constexpr int kDefaultWidth = 256;
        static constexpr int kIconGap = 8;
        static constexpr int kButtonGap = 8;

        ShadPopconfirmIcon* _iconWidget{nullptr};
        ShadButton* _cancelButton{nullptr};
        ShadButton* _confirmButton{nullptr};
        bool _isDestructive{false};
        bool _isResolved{true};
};

#endif
