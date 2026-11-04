#ifndef SHADTREEVIEWSTYLE_H
#define SHADTREEVIEWSTYLE_H

#include <QProxyStyle>

class QPainter;
class QStyleOption;
class QWidget;

// ShadTreeView 的样式，组件库内部使用（父对象为树形视图）
// 行背景、选中与悬停、焦点框、展开箭头由 ShadTreeView 自行绘制，这里不再绘制系统样式的对应部分
// 拖放指示：插入位置为 2px 的 primary 色横线，放到节点上时为该节点的 primary 色圆角描边
class ShadTreeViewStyle : public QProxyStyle
{
    Q_OBJECT

    public:
        explicit ShadTreeViewStyle(QWidget* treeView);
        ~ShadTreeViewStyle() override;

        void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override;
};

#endif // SHADTREEVIEWSTYLE_H
