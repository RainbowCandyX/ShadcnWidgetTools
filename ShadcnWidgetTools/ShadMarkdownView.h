#ifndef SHADMARKDOWNVIEW_H
#define SHADMARKDOWNVIEW_H

#include <QSize>
#include <QString>
#include <QUrl>
#include <QWidget>

#include "ShadProperty.h"

class QEvent;
class QResizeEvent;
class ShadMarkdownViewPrivate;

class SHADCN_EXPORT ShadMarkdownView : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadMarkdownView)
    Q_PROPERTY(QString markdown READ getMarkdown WRITE setMarkdown)
    Q_PROPERTY(bool openLinks READ isOpenLinks WRITE setOpenLinks)

    public:
        explicit ShadMarkdownView(QWidget* parent = nullptr);
        ~ShadMarkdownView() override;

        void setMarkdown(const QString& markdown);
        QString getMarkdown() const;
        void appendMarkdown(const QString& markdown);
        void clear();

        void setOpenLinks(bool open);
        bool isOpenLinks() const;

        void setBaseUrl(const QUrl& url);
        QUrl getBaseUrl() const;

        int heightForWidth(int width) const override;
        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void linkActivated(const QUrl& url);

    protected:
        bool event(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
};

#endif // SHADMARKDOWNVIEW_H
