#ifndef SHADPROPERTYGRIDGROUPHEADER_H
#define SHADPROPERTYGRIDGROUPHEADER_H

#include <QAbstractButton>
#include <QSize>

class QEnterEvent;
class QEvent;
class QPaintEvent;

class ShadPropertyGridGroupHeader : public QAbstractButton
{
    Q_OBJECT

    public:
        explicit ShadPropertyGridGroupHeader(QWidget* parent = nullptr);
        ~ShadPropertyGridGroupHeader() override;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADPROPERTYGRIDGROUPHEADER_H
