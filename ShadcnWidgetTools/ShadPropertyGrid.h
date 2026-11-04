#ifndef SHADPROPERTYGRID_H
#define SHADPROPERTYGRID_H

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QWidget>

#include "ShadProperty.h"

class QEvent;

class ShadPropertyGridPrivate;

class SHADCN_EXPORT ShadPropertyGrid : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadPropertyGrid)
    Q_PROPERTY(int labelWidth READ getLabelWidth WRITE setLabelWidth)
    Q_PROPERTY(bool descriptionVisible READ isDescriptionVisible WRITE setDescriptionVisible)

    public:
        explicit ShadPropertyGrid(QWidget* parent = nullptr);
        ~ShadPropertyGrid() override;

        bool addProperty(const QString& key, const QString& label, const QVariant& value, const QString& group = QString());
        bool addEnumProperty(const QString& key, const QString& label, const QStringList& options, int currentIndex, const QString& group = QString());
        bool addWidgetProperty(const QString& key, const QString& label, QWidget* editor, const QString& group = QString());
        void removeProperty(const QString& key);
        void clear();
        bool hasProperty(const QString& key) const;
        QStringList getKeys() const;

        void setValue(const QString& key, const QVariant& value);
        QVariant getValue(const QString& key) const;

        void setPropertyLabel(const QString& key, const QString& label);
        QString getPropertyLabel(const QString& key) const;

        void setPropertyDescription(const QString& key, const QString& description);
        QString getPropertyDescription(const QString& key) const;

        void setPropertyReadOnly(const QString& key, bool readOnly);
        bool isPropertyReadOnly(const QString& key) const;

        void setPropertyRange(const QString& key, const QVariant& minimum, const QVariant& maximum);
        void setEnumOptions(const QString& key, const QStringList& options);
        QWidget* getEditor(const QString& key) const;

        void setGroupTitle(const QString& group, const QString& title);
        QString getGroupTitle(const QString& group) const;
        void setGroupExpanded(const QString& group, bool expanded);
        bool isGroupExpanded(const QString& group) const;

        void setLabelWidth(int width);
        int getLabelWidth() const;

        void setDescriptionVisible(bool visible);
        bool isDescriptionVisible() const;

        QString getCurrentKey() const;

    Q_SIGNALS:
        void valueChanged(const QString& key, const QVariant& value);
        void currentKeyChanged(const QString& key);

    protected:
        bool event(QEvent* event) override;
};

#endif // SHADPROPERTYGRID_H
