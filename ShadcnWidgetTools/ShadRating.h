#ifndef SHADRATING_H
#define SHADRATING_H

#include <QColor>
#include <QSize>
#include <QWidget>

#include "ShadProperty.h"

class QEnterEvent;
class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;

// 评分：只读时不响应鼠标与键盘，也不参与 Tab 顺序
class ShadRatingPrivate;

class SHADCN_EXPORT ShadRating : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadRating)
    Q_PROPERTY(int rating READ getRating WRITE setRating NOTIFY ratingChanged)
    Q_PROPERTY(int maximum READ getMaximum WRITE setMaximum)
    Q_PROPERTY(int starSize READ getStarSize WRITE setStarSize)
    Q_PROPERTY(int spacing READ getSpacing WRITE setSpacing)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
    Q_PROPERTY(QColor activeColor READ getActiveColor WRITE setActiveColor)

    public:
        explicit ShadRating(QWidget* parent = nullptr);
        ~ShadRating() override;

        // 超出范围时限制在 0 到最大星数之间
        void setRating(int rating);
        int getRating() const;

        // 至少为 1，当前评分超出时同时减小
        void setMaximum(int maximum);
        int getMaximum() const;

        void setStarSize(int size);
        int getStarSize() const;

        void setSpacing(int spacing);
        int getSpacing() const;

        void setReadOnly(bool readOnly);
        bool isReadOnly() const;

        // 传入无效颜色恢复默认
        void setActiveColor(const QColor& color);
        QColor getActiveColor() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        // 通过鼠标、键盘或 setRating 改变评分时发出
        void ratingChanged(int rating);

    protected:
        void paintEvent(QPaintEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void changeEvent(QEvent* event) override;
};

#endif // SHADRATING_H
