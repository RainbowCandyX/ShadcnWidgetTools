#ifndef SHADMARKDOWNTEXT_H
#define SHADMARKDOWNTEXT_H

#include <QSize>
#include <QString>
#include <QTextBrowser>
#include <QUrl>

class QEvent;
class QPaintEvent;
class QResizeEvent;
class QWheelEvent;

class ShadMarkdownText : public QTextBrowser
{
    Q_OBJECT

    public:
        explicit ShadMarkdownText(QWidget* parent = nullptr);
        ~ShadMarkdownText() override;

        void setMarkdownText(const QString& markdown);
        QString getMarkdownText() const;
        void setBaseUrl(const QUrl& url);

        int heightForWidth(int width) const override;
        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void linkActivated(const QUrl& url);

    protected:
        void paintEvent(QPaintEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
        void changeEvent(QEvent* event) override;

    private:
        void applyPalette();
        void rebuildDocument();
        void applyStyle();

        QString _markdown;
        bool _applying{false};
        bool _hasImages{false};
        int _styledWidth{-1};
        mutable int _cachedWidth{-1};
        mutable int _cachedHeight{0};
};

#endif // SHADMARKDOWNTEXT_H
