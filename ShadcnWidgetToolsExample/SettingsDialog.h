#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "ShadDialog.h"

class QWidget;
class ShadSidebar;
class ShadToaster;

// 示例程序设置：修改立即生效，不保存
class SettingsDialog : public ShadDialog
{
    Q_OBJECT

    public:
        SettingsDialog(ShadSidebar* sidebar, ShadToaster* toaster, QWidget* parent = nullptr);
        ~SettingsDialog() override;
};

#endif // SETTINGSDIALOG_H
