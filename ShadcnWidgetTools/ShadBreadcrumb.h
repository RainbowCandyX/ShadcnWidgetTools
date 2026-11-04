#ifndef SHADBREADCRUMB_H
#define SHADBREADCRUMB_H

#include <QSize>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "ShadIconType.h"
#include "ShadProperty.h"

class QEvent;
class QResizeEvent;
class ShadDropdownMenu;

// 面包屑，对应 shadcn Breadcrumb：显示当前页面在层级中的路径，项之间用分隔符隔开（默认 ChevronRight）
// 文字为 text-sm text-muted-foreground，链接悬停变为 foreground，当前页为 foreground；宽度不够时整体换行，单项文字放不下时尾部省略
// RTL 下从右向左排列，方向性分隔符图标水平翻转
class ShadBreadcrumbPrivate;

class SHADCN_EXPORT ShadBreadcrumb : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadBreadcrumb)
    Q_PROPERTY(int maximumVisibleItems READ getMaximumVisibleItems WRITE setMaximumVisibleItems)
    Q_PROPERTY(int maximumItemWidth READ getMaximumItemWidth WRITE setMaximumItemWidth)

    public:
        explicit ShadBreadcrumb(QWidget* parent = nullptr);
        ~ShadBreadcrumb() override;

        // 以下 add 函数按顺序追加一项并返回其序号
        // 链接（BreadcrumbLink），点击或回车 / 空格发出 linkClicked
        int addLink(const QString& text);
        // 当前页（BreadcrumbPage），不可点击
        int addPage(const QString& text);
        // 省略号（BreadcrumbEllipsis）；传入菜单时显示为 ghost 图标按钮，点击在下方弹出菜单
        int addEllipsis(ShadDropdownMenu* menu = nullptr);
        // 文字 + ChevronDown，点击在下方弹出菜单，同时发出 linkClicked
        // 菜单由调用方持有，面包屑不会删除
        int addDropdown(const QString& text, ShadDropdownMenu* menu);
        // 便捷接口：清空后前面的项生成链接，最后一项为当前页
        void setPath(const QStringList& texts);
        void clear();
        int count() const;

        void setItemText(int index, const QString& text);
        QString getItemText(int index) const;

        // 分隔符图标，默认 ChevronRight；例如 Dot 对应 shadcn 的自定义分隔符示例
        void setSeparatorIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getSeparatorIcon() const;

        // 项数超过 n 时只保留第一项和最后 n - 1 项，中间的项收进省略号下拉菜单；默认 0 表示不折叠，小于 2 时按 2 处理
        // 菜单中的项点击后发出原序号的 linkClicked（当前页与省略号不进入菜单）
        void setMaximumVisibleItems(int count);
        int getMaximumVisibleItems() const;

        // 单项文字的最大可视宽度（像素），超出时尾部省略并在悬停提示中显示完整文字；默认 0 表示只受面包屑宽度限制
        void setMaximumItemWidth(int width);
        int getMaximumItemWidth() const;

        bool hasHeightForWidth() const override;
        int heightForWidth(int width) const override;
        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        // 链接或下拉项被点击，index 为 add 时返回的序号
        void linkClicked(int index);

    protected:
        void changeEvent(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
};

#endif // SHADBREADCRUMB_H
