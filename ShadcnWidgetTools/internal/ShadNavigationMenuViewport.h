#ifndef SHADNAVIGATIONMENUVIEWPORT_H
#define SHADNAVIGATIONMENUVIEWPORT_H

#include <QPixmap>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QWidget>
#include <QtGlobal>

class QEvent;
class QHideEvent;
class QMouseEvent;
class QObject;
class QPaintEvent;
class QVariantAnimation;

// ShadNavigationMenu 的下拉内容窗口
class ShadNavigationMenuViewport : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadNavigationMenuViewport(QWidget* parent = nullptr);
        ~ShadNavigationMenuViewport() override;

        // 内容的父组件
        QWidget* body() const;
        QWidget* content() const;
        QRect cardRect() const;

        // 已打开时切换内容，step 大于 0 表示新内容在后面
        void showContent(QWidget* content, int step, const QRect& barRect, bool focusContent);

    Q_SIGNALS:
        void closed();

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void hideEvent(QHideEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        QSize contentSize(QWidget* content) const;
        QSize cardSize(QWidget* content) const;
        // 内容在 body 中的位置，offset 为水平位移
        QRect contentRect(int offset) const;
        void finishAnimations();
        void focusFirstChild();

        QWidget* _body{nullptr};
        QPointer<QWidget> _content;
        QPixmap _openPixmap;
        QPixmap _outgoingPixmap;
        QRect _outgoingRect;
        QVariantAnimation* _openAnimation{nullptr};
        QVariantAnimation* _switchAnimation{nullptr};
        qreal _openProgress{1.0};
        qreal _switchProgress{1.0};
        int _switchSign{1};
        bool _isFocusPending{false};
};

#endif // SHADNAVIGATIONMENUVIEWPORT_H
