#ifndef SHADPROPERTYGRIDROW_H
#define SHADPROPERTYGRIDROW_H

#include <QPointer>
#include <QSize>
#include <QString>
#include <QWidget>

class QEvent;
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;

class ShadPropertyGridRow : public QWidget
{
    Q_OBJECT

    public:
        static constexpr int kRowHeight = 42;
        static constexpr int kMinLabelWidth = 64;
        static constexpr int kMinEditorWidth = 96;

        explicit ShadPropertyGridRow(QWidget* parent = nullptr);
        ~ShadPropertyGridRow() override;

        void setLabel(const QString& label);
        QString getLabel() const;

        void setEditor(QWidget* editor);
        QWidget* getEditor() const;

        void setLabelWidth(int width);
        int effectiveLabelWidth() const;

        void setCurrent(bool current);
        bool isCurrent() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void pressed();
        void labelWidthDragged(int width);

    protected:
        bool event(QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        int logicalX(int x) const;
        bool isNearSplit(int x) const;
        void layoutEditor();

        QString _label;
        QPointer<QWidget> _editor;
        int _labelWidth{160};
        bool _isCurrent{false};
        bool _isDragging{false};
};

#endif // SHADPROPERTYGRIDROW_H
