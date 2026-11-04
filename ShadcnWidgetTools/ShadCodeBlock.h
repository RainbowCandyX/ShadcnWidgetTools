#ifndef SHADCODEBLOCK_H
#define SHADCODEBLOCK_H

#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QEnterEvent;
class QEvent;
class QHBoxLayout;
class QObject;
class QPaintEvent;
class QResizeEvent;

// 代码块，对应 shadcn CodeBlock：显示代码、终端输出、日志与 diff
// 外框 rounded-lg border-input（暗色 bg-input/30）；代码区 font-mono leading-relaxed px-4 py-4，内容超出时滚动并在可滚动的一侧边缘渐隐
// 设置标题后显示标题栏（CodeBlockHeader），操作按钮位于标题栏结束侧；没有标题时操作按钮悬浮在右上角，悬停或聚焦时淡入
// 标题栏与操作区跟随布局方向，RTL 时镜像；代码区始终从左到右排列（dir="ltr"）
// 代码区可通过 Tab 聚焦（tabIndex=0），聚焦后可用方向键滚动，支持拖动选择、双击选词、全选与复制，行号与 +/− 标记不会被复制
class ShadCodeBlockPrivate;

class SHADCN_EXPORT ShadCodeBlock : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadCodeBlock)
    Q_PROPERTY(ShadCodeBlockType::Variant variant READ getVariant WRITE setVariant)
    Q_PROPERTY(ShadCodeBlockType::Size codeBlockSize READ getCodeBlockSize WRITE setCodeBlockSize)
    Q_PROPERTY(QString code READ getCode WRITE setCode)
    Q_PROPERTY(QString language READ getLanguage WRITE setLanguage)
    Q_PROPERTY(QString title READ getTitle WRITE setTitle)
    Q_PROPERTY(bool lineNumbersVisible READ isLineNumbersVisible WRITE setLineNumbersVisible)
    Q_PROPERTY(int startLine READ getStartLine WRITE setStartLine)
    Q_PROPERTY(bool wordWrap READ isWordWrap WRITE setWordWrap NOTIFY wordWrapChanged)
    Q_PROPERTY(int maximumContentHeight READ getMaximumContentHeight WRITE setMaximumContentHeight)
    Q_PROPERTY(bool copyButtonVisible READ isCopyButtonVisible WRITE setCopyButtonVisible)

    public:
        explicit ShadCodeBlock(QWidget* parent = nullptr);
        explicit ShadCodeBlock(const QString& code, QWidget* parent = nullptr);
        ~ShadCodeBlock() override;

        // Destructive 用于错误输出：边框 destructive/30，文字为 destructive 色
        void setVariant(ShadCodeBlockType::Variant variant);
        ShadCodeBlockType::Variant getVariant() const;

        // Small 对应 size="sm"：内边距更小，代码为 text-xs
        void setCodeBlockSize(ShadCodeBlockType::Size size);
        ShadCodeBlockType::Size getCodeBlockSize() const;

        // 代码按 "\n" 分行，末尾的一个换行不产生空行
        void setCode(const QString& code);
        QString getCode() const;
        // 追加到末尾（流式输出），只重新解析和排版变化的行
        void appendCode(const QString& text);
        // 显示的文字：语言为 "ansi" 时已去掉转义码；复制按钮复制的就是这段文字
        QString toPlainText() const;
        int lineCount() const;

        // 语法高亮语言，不区分大小写：bash（sh、shell）、css、cpp（c++、c）、json、javascript（js、jsx）、typescript（ts、tsx）、python（py）
        // "ansi" 解析终端颜色转义码；空字符串或其他语言按纯文本显示
        void setLanguage(const QString& language);
        QString getLanguage() const;

        // 单行样式，lineIndex 从 0 开始（与 setStartLine 无关）；对应 CodeBlockLine 的 variant
        void setLineVariant(int lineIndex, ShadCodeBlockType::LineVariant variant);
        ShadCodeBlockType::LineVariant getLineVariant(int lineIndex) const;
        void clearLineVariants();

        // 行号，对应 lineNumbers；startLine 为第一行的行号，默认 1
        void setLineNumbersVisible(bool visible);
        bool isLineNumbersVisible() const;
        void setStartLine(int line);
        int getStartLine() const;

        // 自动换行，对应 wrap：在单词边界换行，单词过长时断开；默认关闭，超出宽度时横向滚动
        void setWordWrap(bool wrap);
        bool isWordWrap() const;

        // 代码区的最大高度（含代码区上下内边距），对应 CodeBlockContent 的 max-h-*；超出后纵向滚动，0 表示不限制
        void setMaximumContentHeight(int height);
        int getMaximumContentHeight() const;

        // 标题（文件名或说明），超长时省略；标题或图标不为空时显示标题栏
        void setTitle(const QString& title);
        QString getTitle() const;
        void setTitleIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getTitleIcon() const;

        // 复制按钮（CodeBlockCopyButton）：Ghost 样式，点击后复制全部代码，图标变为对勾 2 秒；默认不显示
        void setCopyButtonVisible(bool visible);
        bool isCopyButtonVisible() const;
        // 在操作区添加组件（通常为 IconExtraSmall 尺寸的 ShadButton），位于复制按钮之前，代码块接管其父子关系
        void addActionWidget(QWidget* widget);

        // 底栏（CodeBlockFooter）的布局，例如"展开更多"按钮；添加组件后显示底栏
        QHBoxLayout* footerLayout();

        // 代码区中选中的文字
        QString selectedText() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        // 点击复制按钮并写入剪贴板后发出
        void copied();
        void wordWrapChanged(bool wrap);

    protected:
        bool event(QEvent* event) override;
        bool eventFilter(QObject* watched, QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADCODEBLOCK_H
