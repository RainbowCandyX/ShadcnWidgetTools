#ifndef SHADALERTDIALOGPRIVATE_H
#define SHADALERTDIALOGPRIVATE_H

#include "ShadDef.h"

class ShadAlertDialogMedia;
class ShadButton;

class ShadAlertDialogPrivate
{
    public:
        ShadAlertDialogPrivate();
        ~ShadAlertDialogPrivate();

        // max-w-lg / max-w-xs
        static constexpr int kMediumWidth = 512;
        static constexpr int kSmallWidth = 320;
        // text-lg font-medium
        static constexpr int kTitlePixelSize = 18;
        // 标题区 gap-1.5
        static constexpr int kHeaderGap = 6;
        // Medium 有媒体区时媒体区与文字列的间距，gap-x-6
        static constexpr int kMediaColumnGap = 24;

        ShadAlertDialogType::Size _size{ShadAlertDialogType::Medium};
        ShadAlertDialogMedia* _media{nullptr};
        ShadButton* _cancelButton{nullptr};
        ShadButton* _actionButton{nullptr};
};

#endif // SHADALERTDIALOGPRIVATE_H
