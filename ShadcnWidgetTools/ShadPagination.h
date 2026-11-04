#ifndef SHADPAGINATION_H
#define SHADPAGINATION_H

#include <QSize>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ShadProperty.h"

class QEvent;
class QResizeEvent;

// 分页，对应 shadcn Pagination：上一页、页码、省略号、下一页，按页数与当前页自动生成
// 页码为 ghost 图标按钮（36x36），当前页为 outline；上一页 / 下一页为 ghost default 尺寸，带 ChevronLeft / ChevronRight；省略号不可点击
// 页码列表：首尾各保留 boundaryCount 页，当前页两侧各保留 siblingCount 页，中间不连续处显示省略号
// 第一页时上一页禁用，最后一页时下一页禁用；元素间距 gap-1，内容在组件宽度内按 alignment 排列（默认居中，对应 mx-auto justify-center）
// 页码跳转（setJumpVisible）："跳至 [输入框] 页"，按 Enter 跳转；页码超出范围时输入框显示错误态，不跳转
// 页码从 1 开始计数，与界面显示一致；RTL 下整体镜像，箭头方向翻转
class ShadPaginationPrivate;

class SHADCN_EXPORT ShadPagination : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadPagination)
    Q_PROPERTY(int pageCount READ getPageCount WRITE setPageCount)
    Q_PROPERTY(int currentPage READ getCurrentPage WRITE setCurrentPage NOTIFY currentPageChanged)
    Q_PROPERTY(int siblingCount READ getSiblingCount WRITE setSiblingCount)
    Q_PROPERTY(int boundaryCount READ getBoundaryCount WRITE setBoundaryCount)
    Q_PROPERTY(bool navigationVisible READ isNavigationVisible WRITE setNavigationVisible)
    Q_PROPERTY(bool navigationTextVisible READ isNavigationTextVisible WRITE setNavigationTextVisible)
    Q_PROPERTY(bool pageButtonsVisible READ isPageButtonsVisible WRITE setPageButtonsVisible)
    Q_PROPERTY(bool jumpVisible READ isJumpVisible WRITE setJumpVisible)

    public:
        explicit ShadPagination(QWidget* parent = nullptr);
        ShadPagination(int pageCount, QWidget* parent = nullptr);
        ~ShadPagination() override;

        // 总页数，默认 1；小于 1 时不显示页码，上一页与下一页都禁用
        void setPageCount(int pageCount);
        int getPageCount() const;
        // 当前页，限制在 1 到总页数之间；变化时发出 currentPageChanged
        void setCurrentPage(int page);
        int getCurrentPage() const;

        // 当前页两侧显示的页数，默认 1
        void setSiblingCount(int count);
        int getSiblingCount() const;
        // 首尾显示的页数，默认 1
        void setBoundaryCount(int count);
        int getBoundaryCount() const;

        // 上一页 / 下一页按钮，默认显示；关闭后只有页码（对应 Simple 示例）
        void setNavigationVisible(bool visible);
        bool isNavigationVisible() const;
        // 上一页 / 下一页的文字，默认显示；关闭后只显示箭头，文字作为悬停提示
        void setNavigationTextVisible(bool visible);
        bool isNavigationTextVisible() const;
        // 默认"上一页" / "下一页"
        void setPreviousText(const QString& text);
        QString getPreviousText() const;
        void setNextText(const QString& text);
        QString getNextText() const;

        // 页码按钮，默认显示；关闭后只有上一页 / 下一页
        void setPageButtonsVisible(bool visible);
        bool isPageButtonsVisible() const;

        // 页码跳转，默认关闭；位于下一页之后，间距 gap-4
        void setJumpVisible(bool visible);
        bool isJumpVisible() const;
        // 输入框前后的文字，默认"跳至" / "页"
        void setJumpTexts(const QString& prefix, const QString& suffix);

        // 内容在组件宽度内的水平对齐：AlignHCenter（默认）、AlignLeft（起始侧）或 AlignRight（结束侧），RTL 下左右对调
        void setAlignment(Qt::Alignment alignment);
        Qt::Alignment getAlignment() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        // 点击页码、上一页、下一页、页码跳转或调用 setCurrentPage 改变当前页时发出
        void currentPageChanged(int page);

    protected:
        void changeEvent(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
};

#endif // SHADPAGINATION_H
