#ifndef SHADCOLLAPSEPANEL_H
#define SHADCOLLAPSEPANEL_H

#include <QSize>
#include <QWidget>
#include <QtGlobal>

class QEvent;
class QObject;
class QResizeEvent;

// 按进度裁剪内容的容器
class ShadCollapsePanel : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadCollapsePanel(QWidget* parent = nullptr);
        ~ShadCollapsePanel() override;

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

#endif // SHADCOLLAPSEPANEL_H
