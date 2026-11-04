#ifndef SHADCOMMANDDIALOG_H
#define SHADCOMMANDDIALOG_H

#include <QKeySequence>

#include "ShadDialog.h"
#include "ShadProperty.h"

class QEvent;
class QObject;
class ShadCommand;

// 对话框中的命令面板，触发任一项、点击外部或按 Esc 后关闭
class ShadCommandDialogPrivate;

class SHADCN_EXPORT ShadCommandDialog : public ShadDialog
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadCommandDialog)
    Q_PROPERTY(bool resizable READ isResizable WRITE setResizable)
    Q_PROPERTY(QKeySequence shortcut READ getShortcut WRITE setShortcut)

    public:
        explicit ShadCommandDialog(QWidget* parent = nullptr);
        ~ShadCommandDialog() override;

        ShadCommand* command() const;

        // 默认 false
        void setResizable(bool resizable);
        bool isResizable() const;

        // 程序内全局快捷键，再按一次关闭；空序列表示不注册
        void setShortcut(const QKeySequence& sequence);
        QKeySequence getShortcut() const;
        // 之后创建的对话框使用，默认 Ctrl+K
        static void setDefaultShortcut(const QKeySequence& sequence);
        static QKeySequence getDefaultShortcut();

        // 打开时清空搜索
        void setVisible(bool visible) override;

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void changeEvent(QEvent* event) override;
};

#endif // SHADCOMMANDDIALOG_H
