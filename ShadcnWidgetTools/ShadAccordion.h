#ifndef SHADACCORDION_H
#define SHADACCORDION_H

#include <QString>
#include <QStringList>
#include <QWidget>

#include "ShadDef.h"
#include "ShadProperty.h"

class QKeyEvent;
class QPaintEvent;
class ShadLabel;

// 手风琴，各项按 value 标识
class ShadAccordionPrivate;

class SHADCN_EXPORT ShadAccordion : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadAccordion)
    Q_PROPERTY(bool multiple READ isMultiple WRITE setMultiple)
    Q_PROPERTY(ShadAccordionType::Variant variant READ getVariant WRITE setVariant)
    Q_PROPERTY(int animationDuration READ getAnimationDuration WRITE setAnimationDuration)
    Q_PROPERTY(QStringList expandedItems READ getExpandedItems WRITE setExpandedItems NOTIFY expandedItemsChanged)

    public:
        explicit ShadAccordion(QWidget* parent = nullptr);
        ~ShadAccordion() override;

        // 接管 content；value 重复时返回 false，不接管
        bool addItem(const QString& value, const QString& title, QWidget* content);
        // value 重复时返回 nullptr
        ShadLabel* addItem(const QString& value, const QString& title, const QString& text);
        void removeItem(const QString& value);
        QStringList getItems() const;

        void setItemTitle(const QString& value, const QString& title);
        QString getItemTitle(const QString& value) const;
        void setItemDisabled(const QString& value, bool disabled);
        bool isItemDisabled(const QString& value) const;

        // 默认同时只展开一项
        void setMultiple(bool multiple);
        bool isMultiple() const;

        void setVariant(ShadAccordionType::Variant variant);
        ShadAccordionType::Variant getVariant() const;

        // 0 到 1000，默认 200
        void setAnimationDuration(int msecs);
        int getAnimationDuration() const;

        void setItemExpanded(const QString& value, bool expanded);
        bool isItemExpanded(const QString& value) const;
        void setExpandedItems(const QStringList& values);
        QStringList getExpandedItems() const;

    Q_SIGNALS:
        void expandedItemsChanged(const QStringList& values);

    protected:
        void keyPressEvent(QKeyEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADACCORDION_H
