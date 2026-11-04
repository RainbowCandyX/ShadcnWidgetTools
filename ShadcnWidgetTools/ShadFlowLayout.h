#ifndef SHADFLOWLAYOUT_H
#define SHADFLOWLAYOUT_H

#include <QLayout>
#include <QRect>
#include <QSize>

#include "ShadProperty.h"

class QLayoutItem;
class QWidget;

// 流式布局：组件按 sizeHint 从左到右排列，一行放不下时自动换行，同一行内垂直居中
// 通过 heightForWidth 向外层报告换行后的高度，可直接放进 QScrollArea（widgetResizable）的内容组件中
class ShadFlowLayoutPrivate;

class SHADCN_EXPORT ShadFlowLayout : public QLayout
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadFlowLayout)

    public:
        explicit ShadFlowLayout(QWidget* parent = nullptr);
        ~ShadFlowLayout() override;

        // 同一行内组件的水平间距，默认 8px
        void setHorizontalSpacing(int spacing);
        int getHorizontalSpacing() const;

        // 行与行之间的垂直间距，默认 8px
        void setVerticalSpacing(int spacing);
        int getVerticalSpacing() const;

        void addItem(QLayoutItem* item) override;
        int count() const override;
        QLayoutItem* itemAt(int index) const override;
        QLayoutItem* takeAt(int index) override;

        Qt::Orientations expandingDirections() const override;
        bool hasHeightForWidth() const override;
        int heightForWidth(int width) const override;
        // 最小宽度为最宽组件的宽度，保证每个组件至少能完整显示在单独一行
        QSize minimumSize() const override;
        QSize sizeHint() const override;
        void setGeometry(const QRect& rect) override;
};

#endif // SHADFLOWLAYOUT_H
