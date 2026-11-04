#ifndef SHADDIALOG_H
#define SHADDIALOG_H

#include <QByteArray>
#include <QDialog>
#include <QString>

#include "ShadDef.h"
#include "ShadProperty.h"

class QHBoxLayout;
class QHideEvent;
class QPaintEvent;
class QVBoxLayout;

// 对话框：布局由 ShadDialog 管理，不要调用 setLayout，内容放入 contentLayout() / footerLayout()
class ShadDialogPrivate;

class SHADCN_EXPORT ShadDialog : public QDialog
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadDialog)
    Q_PROPERTY(QString title READ getTitle WRITE setTitle)
    Q_PROPERTY(QString description READ getDescription WRITE setDescription)
    Q_PROPERTY(int titleBarHeight READ getTitleBarHeight WRITE setTitleBarHeight)
    Q_PROPERTY(ShadWindowType::ButtonFlags buttonFlags READ getButtonFlags WRITE setButtonFlags)
    Q_PROPERTY(ShadWindowType::ButtonPosition buttonPosition READ getButtonPosition WRITE setButtonPosition)
    Q_PROPERTY(bool overlayVisible READ isOverlayVisible WRITE setOverlayVisible)

    public:
        explicit ShadDialog(QWidget* parent = nullptr);
        ShadDialog(const QString& title, const QString& description, QWidget* parent = nullptr);
        ~ShadDialog() override;

        // 正文标题与描述，与 windowTitle 相互独立，为空时不显示对应行
        void setTitle(const QString& title);
        QString getTitle() const;
        void setDescription(const QString& description);
        QString getDescription() const;

        void setTitleBarHeight(int height);
        int getTitleBarHeight() const;

        // 应在显示前设置
        void setButtonFlags(ShadWindowType::ButtonFlags buttonFlags);
        ShadWindowType::ButtonFlags getButtonFlags() const;

        void setButtonPosition(ShadWindowType::ButtonPosition position);
        ShadWindowType::ButtonPosition getButtonPosition() const;

        void setOverlayVisible(bool visible);
        bool isOverlayVisible() const;

        QVBoxLayout* contentLayout() const;
        // 第一次调用时显示页脚
        QHBoxLayout* footerLayout();

        // 最大化或全屏时不移动
        void moveToCenter();

        // macOS 上显示前恢复被 QDialog::open 改成 Qt::Sheet 的窗口标志
        void open() override;
        // 关闭动画结束后才调用 QDialog::done，动画期间再次调用被忽略
        void done(int result) override;
        void setVisible(bool visible) override;

    protected:
        bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
        void hideEvent(QHideEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADDIALOG_H
