#ifndef SHADANCHOR_H
#define SHADANCHOR_H

#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadProperty.h"

class QAbstractScrollArea;
class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QObject;
class QPaintEvent;

// 锚点导航，对应 Ant Design Anchor 与文档页的"本页目录"：纵向链接列表，起始侧细轨道上的高亮条指示当前链接，切换时滑动过去
// 链接文字 text-sm muted-foreground，悬停与当前链接为 foreground；level 每级缩进 12px；文字超长时尾部省略
// 点击链接或按回车、空格时平滑滚动到对应组件；手动滚动时按可视区顶部（加 offset）所在的位置自动切换当前链接，滚动到底部时切换到最后一个可见的链接
// 目标组件必须位于滚动区域的可视区内（例如 QScrollArea 的内容组件中）；目标被删除后链接保留，但不再参与定位
class ShadAnchorPrivate;

class SHADCN_EXPORT ShadAnchor : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadAnchor)
    Q_PROPERTY(int currentIndex READ getCurrentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int offset READ getOffset WRITE setOffset)
    Q_PROPERTY(int duration READ getDuration WRITE setDuration)

    public:
        explicit ShadAnchor(QWidget* parent = nullptr);
        ~ShadAnchor() override;

        // 链接所在的滚动区域
        void setScrollArea(QAbstractScrollArea* scrollArea);
        QAbstractScrollArea* getScrollArea() const;

        // 添加链接，返回下标；level 为缩进级别，0 为顶级
        int addLink(const QString& title, QWidget* target, int level = 0);
        void setLinkTitle(int index, const QString& title);
        QString getLinkTitle(int index) const;
        QWidget* getLinkTarget(int index) const;
        int count() const;
        // 删除全部链接
        void clear();

        // 当前链接，-1 表示没有；设置时平滑滚动到对应组件
        void setCurrentIndex(int index);
        int getCurrentIndex() const;

        // 滚动到目标时，目标顶部与可视区顶部的距离，同时作为判断当前链接的位置，默认 0
        void setOffset(int offset);
        int getOffset() const;

        // 滚动动画时长（毫秒），默认 450，0 表示直接跳转
        void setDuration(int durationMs);
        int getDuration() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        // 当前链接变化时发出，包括点击与滚动
        void currentIndexChanged(int index);
        // 点击链接或按回车、空格时发出
        void linkClicked(int index);

    protected:
        bool event(QEvent* event) override;
        bool eventFilter(QObject* watched, QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADANCHOR_H
