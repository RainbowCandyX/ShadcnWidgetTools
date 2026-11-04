#ifndef SHADCOMMANDLIST_H
#define SHADCOMMANDLIST_H

#include <QList>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadIconType.h"

class QAction;
class QMouseEvent;
class QPaintEvent;

struct ShadCommandRow
{
    enum class Kind
    {
        GroupTop,
        Heading,
        Item,
        GroupBottom,
        Separator,
    };

    Kind kind{Kind::Item};
    QString text;
    QAction* action{nullptr};
    ShadIconType::IconName icon{ShadIconType::None};
    QString shortcutText;
    bool isFirstGroup{false};
};

// ShadCommand 的结果列表
class ShadCommandList : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadCommandList(QWidget* parent = nullptr);
        ~ShadCommandList() override;

        void setRows(const QList<ShadCommandRow>& rows);
        const QList<ShadCommandRow>& getRows() const;
        void setEmptyText(const QString& text);
        // 对话框中使用较大的尺寸
        void setLarge(bool large);

        void setHighlightIndex(int index);
        int getHighlightIndex() const;
        // 到达两端时停住
        void moveHighlight(int step);
        void highlightFirst();
        void highlightLast();
        bool isSelectable(int index) const;
        QRect rowRect(int index) const;
        int itemHeight() const;

        QSize sizeHint() const override;

    Q_SIGNALS:
        void itemClicked(int index);

    protected:
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        bool hasItems() const;
        int rowHeight(int index) const;
        int paddingX() const;
        int iconSize() const;
        int indexAt(const QPoint& pos) const;
        void updateRowTops();

        QList<ShadCommandRow> _rows;
        // 最后多一个元素记录内容底部
        QList<int> _rowTops;
        QString _emptyText;
        int _highlightIndex{-1};
        bool _isLarge{false};
};

#endif // SHADCOMMANDLIST_H
