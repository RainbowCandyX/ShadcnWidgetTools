#ifndef SHADCODEEDITOR_H
#define SHADCODEEDITOR_H

#include <QPlainTextEdit>
#include <QString>

#include "ShadProperty.h"

class QEvent;
class QFocusEvent;
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;
class ShadCodeEditorGutter;
class ShadCodeEditorPrivate;

// 代码编辑器：继承 QPlainTextEdit，带行号栏、语法高亮与当前行高亮，外框样式与 ShadTextarea 一致
// 行高依赖自带文档的排版，不要用 setDocument 替换文档；布局始终从左到右
class SHADCN_EXPORT ShadCodeEditor : public QPlainTextEdit
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadCodeEditor)
    Q_PROPERTY(QString language READ getLanguage WRITE setLanguage)
    Q_PROPERTY(int tabSize READ getTabSize WRITE setTabSize)
    Q_PROPERTY(bool lineNumbersVisible READ isLineNumbersVisible WRITE setLineNumbersVisible)
    Q_PROPERTY(bool invalid READ isInvalid WRITE setInvalid NOTIFY invalidChanged)

    public:
        explicit ShadCodeEditor(QWidget* parent = nullptr);
        explicit ShadCodeEditor(const QString& code, QWidget* parent = nullptr);
        ~ShadCodeEditor() override;

        // 语法高亮语言（bash、css、cpp、json、javascript、typescript、python 及常见别名），其他按纯文本显示
        void setLanguage(const QString& language);
        QString getLanguage() const;

        // Tab 显示宽度（空格数），默认 4
        void setTabSize(int size);
        int getTabSize() const;

        // 行号栏，默认显示
        void setLineNumbersVisible(bool visible);
        bool isLineNumbersVisible() const;

        // 错误态，边框与焦点环变为 destructive 色
        void setInvalid(bool invalid);
        bool isInvalid() const;

    Q_SIGNALS:
        void invalidChanged();

    protected:
        // paintEvent 只负责视口，外框在 event 中拦截绘制
        bool event(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        // 基类选区只按文字高度绘制，因此先自绘整行高度的当前行与选区背景
        void paintEvent(QPaintEvent* event) override;
        // 行内文字上下的空白不属于任何文字行，鼠标坐标先映射到文字行再交给基类
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;

    private:
        friend class ShadCodeEditorGutter;
        friend class ShadCodeEditorPrivate;
};

#endif // SHADCODEEDITOR_H
