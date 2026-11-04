#ifndef SHADSEGMENTED_H
#define SHADSEGMENTED_H

#include <QFont>
#include <QPoint>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;
class QShowEvent;

class ShadSegmentedPrivate;

class SHADCN_EXPORT ShadSegmented : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadSegmented)
    Q_PROPERTY(int currentIndex READ getCurrentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(ShadSegmentedType::Size segmentedSize READ getSegmentedSize WRITE setSegmentedSize)
    Q_PROPERTY(bool block READ isBlock WRITE setBlock)

    public:
        explicit ShadSegmented(QWidget* parent = nullptr);
        ~ShadSegmented() override;

        int addItem(const QString& text);
        int addItem(ShadIconType::IconName icon, const QString& text = QString());
        int insertItem(int index, const QString& text);
        int insertItem(int index, ShadIconType::IconName icon, const QString& text = QString());
        void removeItem(int index);
        void clear();
        int count() const;

        void setItemText(int index, const QString& text);
        QString getItemText(int index) const;
        void setItemIcon(int index, ShadIconType::IconName icon);
        ShadIconType::IconName getItemIcon(int index) const;
        void setItemEnabled(int index, bool enabled);
        bool isItemEnabled(int index) const;
        void setItemToolTip(int index, const QString& toolTip);
        QString getItemToolTip(int index) const;

        void setCurrentIndex(int index);
        int getCurrentIndex() const;

        void setSegmentedSize(ShadSegmentedType::Size size);
        ShadSegmentedType::Size getSegmentedSize() const;

        void setBlock(bool block);
        bool isBlock() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void currentIndexChanged(int index);

    protected:
        bool event(QEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void changeEvent(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        QFont itemFont() const;
        int itemPaddingX() const;
        int itemHeight() const;
        int contentWidth(int index) const;
        QRectF logicalItemRect(int index) const;
        QRectF itemRect(int index) const;
        int itemAt(const QPoint& position) const;
        bool isSelectable(int index) const;
        int stepSelectable(int from, int step) const;
        void setHoverIndex(int index);
        void syncBlock(bool animate);
        void layoutItems();
};

#endif // SHADSEGMENTED_H
