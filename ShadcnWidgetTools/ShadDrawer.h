#ifndef SHADDRAWER_H
#define SHADDRAWER_H

#include <QList>
#include <QString>
#include <QWidget>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadProperty.h"

class QCloseEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QVBoxLayout;

// 抽屉：从窗口边缘滑出，可拖动关闭
// 内容放入 contentLayout() / footerLayout()，不要调用 setLayout
class ShadDrawerPrivate;

class SHADCN_EXPORT ShadDrawer : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadDrawer)
    Q_PROPERTY(QString title READ getTitle WRITE setTitle)
    Q_PROPERTY(QString description READ getDescription WRITE setDescription)
    Q_PROPERTY(ShadDrawerType::SwipeDirection swipeDirection READ getSwipeDirection WRITE setSwipeDirection)
    Q_PROPERTY(bool swipeHandleVisible READ isSwipeHandleVisible WRITE setSwipeHandleVisible)
    Q_PROPERTY(bool modal READ isModal WRITE setModal)
    Q_PROPERTY(bool pointerDismissalEnabled READ isPointerDismissalEnabled WRITE setPointerDismissalEnabled)
    Q_PROPERTY(int preferredSize READ getPreferredSize WRITE setPreferredSize)
    Q_PROPERTY(QList<qreal> snapPoints READ getSnapPoints WRITE setSnapPoints)
    Q_PROPERTY(qreal snapPoint READ getSnapPoint WRITE setSnapPoint NOTIFY snapPointChanged)
    Q_PROPERTY(bool sequentialSnapEnabled READ isSequentialSnapEnabled WRITE setSequentialSnapEnabled)

    friend class ShadDrawerPrivate;

    public:
        explicit ShadDrawer(QWidget* parent = nullptr);
        ShadDrawer(const QString& title, const QString& description, QWidget* parent = nullptr);
        ~ShadDrawer() override;

        void setTitle(const QString& title);
        QString getTitle() const;
        void setDescription(const QString& description);
        QString getDescription() const;

        void setSwipeDirection(ShadDrawerType::SwipeDirection direction);
        ShadDrawerType::SwipeDirection getSwipeDirection() const;

        void setSwipeHandleVisible(bool visible);
        bool isSwipeHandleVisible() const;

        // 打开前设置
        void setModal(bool modal);
        bool isModal() const;

        void setPointerDismissalEnabled(bool enabled);
        bool isPointerDismissalEnabled() const;

        // 0 表示自动
        void setPreferredSize(int size);
        int getPreferredSize() const;

        // 只用于上下方向：不大于 1 为窗口高度比例，否则为像素
        void setSnapPoints(const QList<qreal>& snapPoints);
        QList<qreal> getSnapPoints() const;
        void setSnapPoint(qreal snapPoint);
        qreal getSnapPoint() const;
        void setSequentialSnapEnabled(bool enabled);
        bool isSequentialSnapEnabled() const;

        QVBoxLayout* contentLayout() const;
        // 第一次调用时显示页脚
        QVBoxLayout* footerLayout();

        bool isOpen() const;

    public Q_SLOTS:
        void open();

    Q_SIGNALS:
        void openChanged(bool open);
        void openChangeComplete(bool open);
        void snapPointChanged(qreal snapPoint);

    protected:
        // close() 播放关闭动画
        void closeEvent(QCloseEvent* event) override;
        bool focusNextPrevChild(bool next) override;
        void keyPressEvent(QKeyEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADDRAWER_H
