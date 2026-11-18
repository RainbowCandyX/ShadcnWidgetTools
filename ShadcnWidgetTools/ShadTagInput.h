#ifndef SHADTAGINPUT_H
#define SHADTAGINPUT_H

#include <QSize>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "ShadProperty.h"

class QEvent;
class QLineEdit;
class QMouseEvent;
class QObject;
class QPaintEvent;
class QResizeEvent;

class ShadTagInputPrivate;

class SHADCN_EXPORT ShadTagInput : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadTagInput)
    Q_PROPERTY(QStringList tags READ getTags WRITE setTags NOTIFY tagsChanged USER true)
    Q_PROPERTY(QString placeholderText READ getPlaceholderText WRITE setPlaceholderText)
    Q_PROPERTY(QString separators READ getSeparators WRITE setSeparators)
    Q_PROPERTY(int maxTags READ getMaxTags WRITE setMaxTags)
    Q_PROPERTY(bool duplicatesAllowed READ isDuplicatesAllowed WRITE setDuplicatesAllowed)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
    Q_PROPERTY(bool invalid READ isInvalid WRITE setInvalid NOTIFY invalidChanged)

    public:
        explicit ShadTagInput(QWidget* parent = nullptr);
        ~ShadTagInput() override;

        void setTags(const QStringList& tags);
        QStringList getTags() const;

        bool addTag(const QString& tag);
        bool removeTag(const QString& tag);
        void removeTagAt(int index);
        QString tagAt(int index) const;
        int count() const;

        void setPlaceholderText(const QString& text);
        QString getPlaceholderText() const;

        void setSeparators(const QString& separators);
        QString getSeparators() const;

        void setMaxTags(int maxTags);
        int getMaxTags() const;

        void setDuplicatesAllowed(bool allowed);
        bool isDuplicatesAllowed() const;

        void setReadOnly(bool readOnly);
        bool isReadOnly() const;

        void setInvalid(bool invalid);
        bool isInvalid() const;

        QLineEdit* lineEdit() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    public Q_SLOTS:
        void clear();

    Q_SIGNALS:
        void tagsChanged(const QStringList& tags);
        void tagAdded(const QString& tag);
        void tagRemoved(const QString& tag);
        void tagRejected(const QString& tag);
        void invalidChanged();

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADTAGINPUT_H
