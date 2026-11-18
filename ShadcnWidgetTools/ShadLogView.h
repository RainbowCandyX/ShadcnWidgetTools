#ifndef SHADLOGVIEW_H
#define SHADLOGVIEW_H

#include <QDateTime>
#include <QPlainTextEdit>
#include <QSize>
#include <QString>

#include "ShadDef.h"
#include "ShadProperty.h"

class QContextMenuEvent;
class QEvent;
class QFocusEvent;
class QResizeEvent;
class QShowEvent;

class ShadLogViewPrivate;

class SHADCN_EXPORT ShadLogView : public QPlainTextEdit
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadLogView)
    Q_PROPERTY(int maxEntryCount READ getMaxEntryCount WRITE setMaxEntryCount)
    Q_PROPERTY(bool autoScroll READ isAutoScroll WRITE setAutoScroll)
    Q_PROPERTY(bool wordWrap READ isWordWrap WRITE setWordWrap)
    Q_PROPERTY(bool timestampVisible READ isTimestampVisible WRITE setTimestampVisible)
    Q_PROPERTY(bool levelVisible READ isLevelVisible WRITE setLevelVisible)
    Q_PROPERTY(QString timestampFormat READ getTimestampFormat WRITE setTimestampFormat)
    Q_PROPERTY(ShadLogViewType::Level minimumLevel READ getMinimumLevel WRITE setMinimumLevel)
    Q_PROPERTY(QString filterText READ getFilterText WRITE setFilterText)

    public:
        explicit ShadLogView(QWidget* parent = nullptr);
        ~ShadLogView() override;

        void append(ShadLogViewType::Level level, const QString& message);
        void append(ShadLogViewType::Level level, const QString& message, const QDateTime& time);

        int count() const;
        int visibleCount() const;

        void setMaxEntryCount(int count);
        int getMaxEntryCount() const;

        void setAutoScroll(bool autoScroll);
        bool isAutoScroll() const;

        void setWordWrap(bool wordWrap);
        bool isWordWrap() const;

        void setTimestampVisible(bool visible);
        bool isTimestampVisible() const;

        void setLevelVisible(bool visible);
        bool isLevelVisible() const;

        void setTimestampFormat(const QString& format);
        QString getTimestampFormat() const;

        void setMinimumLevel(ShadLogViewType::Level level);
        ShadLogViewType::Level getMinimumLevel() const;

        void setFilterText(const QString& text);
        QString getFilterText() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    public Q_SLOTS:
        void debug(const QString& message);
        void info(const QString& message);
        void success(const QString& message);
        void warning(const QString& message);
        void error(const QString& message);
        void clear();
        void scrollToBottom();

    Q_SIGNALS:
        void countChanged(int count);

    protected:
        bool event(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void contextMenuEvent(QContextMenuEvent* event) override;
};

#endif // SHADLOGVIEW_H
