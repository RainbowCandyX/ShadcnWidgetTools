#ifndef SHADTRACE_H
#define SHADTRACE_H

#include <QSize>
#include <QString>
#include <QUrl>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QEnterEvent;
class QEvent;
class QFocusEvent;
class QHideEvent;
class QKeyEvent;
class QMouseEvent;
class QObject;
class QPaintEvent;
class QResizeEvent;
class QShowEvent;
class QVBoxLayout;
class ShadLabel;
class ShadTraceCollapsiblePrivate;

// 执行记录中的一行，对应 shadcn TraceLine（含 TraceIcon / TraceTitle / TraceDescription）：用于显示 agent 的推理、工具调用、来源、计划与任务
// 图标 size-4、标题 text-sm text-muted-foreground，间距 gap-2；标题只占一行，超长时尾部省略；描述（数量、耗时）不省略，为 muted-foreground/70
// 默认只是文字，不可聚焦；setClickable 或 setUrl 后整行可点击（对应包在 button / a 中）：悬停变为 foreground 色，Tab 聚焦时显示焦点环
// 四周预留焦点环边距，与组件库其他可聚焦组件一致
class ShadTraceLinePrivate;

class SHADCN_EXPORT ShadTraceLine : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadTraceLine)
    // 可展开的行需要在触发行上显示箭头并设置旋转进度
    friend class ShadTraceCollapsiblePrivate;
    Q_PROPERTY(QString title READ getTitle WRITE setTitle)
    Q_PROPERTY(QString description READ getDescription WRITE setDescription)
    Q_PROPERTY(ShadTraceType::Status status READ getStatus WRITE setStatus)
    Q_PROPERTY(bool clickable READ isClickable WRITE setClickable)
    Q_PROPERTY(QUrl url READ getUrl WRITE setUrl)

    public:
        explicit ShadTraceLine(QWidget* parent = nullptr);
        ShadTraceLine(ShadIconType::IconName icon, const QString& title, QWidget* parent = nullptr);
        ~ShadTraceLine() override;

        // 标题前的图标，说明这一步做了什么；LoaderCircle 自动旋转
        void setIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getIcon() const;

        // 一句话的标题，建议写成完整的句子并写明状态，例如"已编辑 trace.tsx"
        void setTitle(const QString& title);
        QString getTitle() const;

        // 标题后的次要文字，例如"14s"、"8 个结果"
        void setDescription(const QString& description);
        QString getDescription() const;
        // 描述使用等宽字体（font-mono），用于 "+48 −12" 这类改动统计
        void setDescriptionMonospace(bool monospace);
        bool isDescriptionMonospace() const;

        // 标题后的指示图标，例如链接的 ArrowUpRight、打开浮层的 ChevronRight；RTL 下水平翻转
        void setIndicatorIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getIndicatorIcon() const;

        // 状态：Running 标题扫光，Error 为 destructive 色，Pending 为 60% 不透明度
        void setStatus(ShadTraceType::Status status);
        ShadTraceType::Status getStatus() const;

        // 整行可点击：点击、回车或空格时发出 clicked
        void setClickable(bool clickable);
        bool isClickable() const;

        // 设为链接：同时开启 setClickable，点击时用系统默认程序打开；传空 QUrl 取消链接（保留可点击）
        void setUrl(const QUrl& url);
        QUrl getUrl() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void clicked();

    protected:
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void hideEvent(QHideEvent* event) override;
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

// 可展开的一行，对应 TraceCollapsible + TraceCollapsibleTrigger + TraceCollapsibleContent：
// 触发行后带 ChevronRight，展开时旋转 90 度；详情区 mt-2 mb-1，起始侧 1px 竖线与 pl-4 缩进，文字为 muted 色，高度变化 200ms
// 默认折叠；详情区可以放任意组件，包括嵌套的 ShadTrace，组成"回合 → 步骤 → 输出"的层级
class ShadTraceCollapsiblePrivate;

class SHADCN_EXPORT ShadTraceCollapsible : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadTraceCollapsible)
    Q_PROPERTY(bool expanded READ isExpanded WRITE setExpanded NOTIFY expandedChanged)

    public:
        explicit ShadTraceCollapsible(QWidget* parent = nullptr);
        ShadTraceCollapsible(ShadIconType::IconName icon, const QString& title, QWidget* parent = nullptr);
        ~ShadTraceCollapsible() override;

        // 触发行，用于设置图标、标题、描述与状态；子行出错时将触发行设为 Error，箭头一起变为 destructive 色
        ShadTraceLine* getTriggerLine() const;

        // 展开或折叠；组件可见时播放高度动画
        void setExpanded(bool expanded);
        bool isExpanded() const;

        // 详情区布局，组件之间 gap-2；放入的组件应与组件库一致地四周预留焦点环边距（3px），addText 添加的文字已按此对齐
        QVBoxLayout* contentLayout() const;
        // 添加一段说明文字：muted 色、自动换行
        ShadLabel* addText(const QString& text);

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        // 展开状态变化时发出，包括点击触发行与调用 setExpanded
        void expandedChanged(bool expanded);

    protected:
        bool event(QEvent* event) override;
        bool eventFilter(QObject* watched, QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

// 执行记录列表，对应 shadcn Trace：行纵向排列，间距 gap-2；行按内容宽度显示（w-fit），空间不够时标题省略
// 可展开的行占满宽度，详情区中可以再放 ShadTrace 实现嵌套
class ShadTracePrivate;

class SHADCN_EXPORT ShadTrace : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadTrace)

    public:
        explicit ShadTrace(QWidget* parent = nullptr);
        ~ShadTrace() override;

        // 添加一行，返回的行由列表管理
        ShadTraceLine* addLine(ShadIconType::IconName icon, const QString& title);
        // 添加可展开的行
        ShadTraceCollapsible* addCollapsible(ShadIconType::IconName icon, const QString& title);
        // 添加任意组件；ShadTraceLine 按内容宽度起始侧对齐，其他组件占满宽度
        void addWidget(QWidget* widget);
        // 删除全部行
        void clear();
        int count() const;
};

#endif // SHADTRACE_H
