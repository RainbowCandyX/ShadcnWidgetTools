#ifndef SHADACCORDIONPANEL_H
#define SHADACCORDIONPANEL_H

#include <QSize>
#include <QWidget>
#include <QtGlobal>

class QEvent;
class QObject;
class QResizeEvent;

// ShadAccordion 的正文容器，按进度裁剪
class ShadAccordionPanel : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadAccordionPanel(QWidget* parent = nullptr);
        ~ShadAccordionPanel() override;

        // 接管 content
        void setContent(QWidget* content);
        QWidget* getContent() const;

        void setProgress(qreal progress);
        qreal progress() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;
        bool hasHeightForWidth() const override;
        int heightForWidth(int width) const override;

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;

    private:
        int contentHeight(int width) const;
        void layoutContent();
        void syncAncestorLayouts();

        QWidget* _content{nullptr};
        qreal _progress{0.0};
};

#endif // SHADACCORDIONPANEL_H
