#ifndef SHADCOLORDIALOG_H
#define SHADCOLORDIALOG_H

#include <QColor>
#include <QScopedPointer>
#include <QString>

#include "ShadDialog.h"
#include "ShadProperty.h"

class QEvent;
class QWidget;
class ShadButton;
class ShadColorPicker;

// 颜色对话框：继承 ShadDialog，正文为 ShadColorPicker，页脚为取消（outline）与确定按钮
// 标题默认为"选择颜色"，可用 setTitle 修改；标题栏、遮罩、动画与 RTL 行为和 ShadDialog 相同
// 每次打开时把当前颜色记为原始颜色，预览条下半显示原始颜色，点击后恢复
// 点击确定或在十六进制输入框中按 Enter 时发出 colorSelected 并接受；Esc 与关闭按钮取消
// 尺寸不能调整：宽度 520，每次打开时高度按内容计算，显示后固定
class ShadColorDialogPrivate;

class SHADCN_EXPORT ShadColorDialog : public ShadDialog
{
    Q_OBJECT
    Q_PROPERTY(QColor currentColor READ getCurrentColor WRITE setCurrentColor NOTIFY currentColorChanged)

    public:
        explicit ShadColorDialog(QWidget* parent = nullptr);
        explicit ShadColorDialog(const QColor& initial, QWidget* parent = nullptr);
        ~ShadColorDialog() override;

        void setCurrentColor(const QColor& color);
        QColor getCurrentColor() const;

        // 内部的取色面板，用于修改基本颜色、自定义颜色或数值模式
        ShadColorPicker* colorPicker() const;
        ShadButton* okButton() const;
        ShadButton* cancelButton() const;

        // 以应用模态打开并阻塞，确定时返回选中的颜色；取消或对话框被销毁时返回无效颜色，关闭后删除对话框
        // title 为空时使用默认标题
        static QColor getColor(const QColor& initial = QColor(Qt::white), QWidget* parent = nullptr, const QString& title = QString());

        // 打开时记录原始颜色，并按内容计算尺寸后固定
        void setVisible(bool visible) override;

    Q_SIGNALS:
        void currentColorChanged(const QColor& color);
        // 点击确定时发出，随后对话框以 Accepted 关闭
        void colorSelected(const QColor& color);

    protected:
        void changeEvent(QEvent* event) override;

    private:
        // 设置标题与按钮的默认文字，使用者改过的文字保持不变
        void retranslate();

        QScopedPointer<ShadColorDialogPrivate> _private;
};

#endif // SHADCOLORDIALOG_H
