#ifndef SHADALERTDIALOG_H
#define SHADALERTDIALOG_H

#include <QPixmap>
#include <QScopedPointer>
#include <QString>

#include "ShadDef.h"
#include "ShadDialog.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QEvent;
class QWidget;
class ShadButton;

// 警告对话框，对应 shadcn AlertDialog：打断用户并要求回应的模态对话框
// 继承 ShadDialog，标题与描述使用 setTitle / setDescription，遮罩、动画、RTL 与 exec 返回值和 ShadDialog 相同；标题为 text-lg font-medium
// 页脚内置取消（outline，调用 reject）与确认（default，调用 accept）两个按钮，通过 cancelButton() / actionButton() 修改文字、变体或隐藏
// 打开时焦点在取消按钮上，按 Enter 触发取消，避免误操作；Esc 与系统关闭按取消处理
// 标题栏与 ShadDialog 相同，默认只显示关闭按钮，点击按取消处理
// 尺寸不能调整：宽度由 setSize 决定，每次打开时高度按内容计算，显示后固定
// 媒体区（对应 AlertDialogMedia）位于标题区内，标题与描述都为空时标题区不显示，媒体区也随之不显示
class ShadAlertDialogPrivate;

class SHADCN_EXPORT ShadAlertDialog : public ShadDialog
{
    Q_OBJECT
    Q_PROPERTY(ShadAlertDialogType::Size size READ getSize WRITE setSize)
    Q_PROPERTY(ShadAlertDialogType::MediaVariant mediaVariant READ getMediaVariant WRITE setMediaVariant)

    public:
        explicit ShadAlertDialog(QWidget* parent = nullptr);
        ShadAlertDialog(const QString& title, const QString& description, QWidget* parent = nullptr);
        ~ShadAlertDialog() override;

        // 默认 Medium
        void setSize(ShadAlertDialogType::Size size);
        ShadAlertDialogType::Size getSize() const;

        // 媒体区图标，传入 None 移除；与图片同时设置时显示图片
        void setMediaIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getMediaIcon() const;
        // 媒体区图片，按 object-cover 缩放裁剪；传入空图移除
        void setMediaPixmap(const QPixmap& pixmap);
        QPixmap getMediaPixmap() const;
        // 媒体区配色，默认 Default
        void setMediaVariant(ShadAlertDialogType::MediaVariant variant);
        ShadAlertDialogType::MediaVariant getMediaVariant() const;

        // 确认按钮，对应 AlertDialogAction，默认文字为 "继续"、变体为 Default
        ShadButton* actionButton() const;
        // 取消按钮，对应 AlertDialogCancel，默认文字为 "取消"、变体为 Outline
        ShadButton* cancelButton() const;

        // 以应用模态打开并阻塞，点击确认按钮返回 true；取消、Esc 或对话框被销毁时返回 false，关闭后删除对话框
        // actionText / cancelText 为空时使用默认文字
        static bool confirm(QWidget* parent, const QString& title, const QString& description, const QString& actionText = QString(), const QString& cancelText = QString(), ShadButtonType::Variant actionVariant = ShadButtonType::Default);

        // 打开时按尺寸设置宽度、按内容计算高度并固定
        void setVisible(bool visible) override;

    protected:
        void changeEvent(QEvent* event) override;

    private:
        // 设置按钮的默认文字，使用者改过的文字保持不变
        void retranslate();
        // 按尺寸与是否有媒体区重建标题区布局
        void rebuildHeader();
        // 按尺寸设置页脚按钮的排列
        void updateFooter();

        QScopedPointer<ShadAlertDialogPrivate> _private;
};

#endif // SHADALERTDIALOG_H
