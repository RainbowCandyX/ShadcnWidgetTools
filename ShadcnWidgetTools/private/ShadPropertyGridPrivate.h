#ifndef SHADPROPERTYGRIDPRIVATE_H
#define SHADPROPERTYGRIDPRIVATE_H

#include <QColor>
#include <QList>
#include <QMetaObject>
#include <QPixmap>
#include <QString>
#include <QVariant>

#include "ShadProperty.h"

class QVBoxLayout;
class QWidget;
class ShadButton;
class ShadLabel;
class ShadPropertyGrid;
class ShadPropertyGridGroupHeader;
class ShadPropertyGridRow;
class ShadScrollArea;
class ShadSeparator;

class ShadPropertyGridPrivate
{
    SHAD_D_CREATE(ShadPropertyGrid)

    public:
        ShadPropertyGridPrivate();
        ~ShadPropertyGridPrivate();

        static constexpr int kDefaultLabelWidth = 160;
        static constexpr int kDescriptionHeight = 80;
        static constexpr int kDescriptionPadding = 12;
        static constexpr int kDescriptionSpacing = 4;
        static constexpr int kSwatchSize = 16;
        static constexpr qreal kSwatchRadius = 4.0;
        static constexpr double kDoubleLimit = 1e9;

        enum class PropertyType
        {
            String = 0,
            Int,
            Double,
            Bool,
            Color,
            Enum,
            Widget,
        };

        struct Property
        {
            QString key;
            QString group;
            QString description;
            PropertyType type{PropertyType::String};
            QWidget* editor{nullptr};
            ShadPropertyGridRow* row{nullptr};
            bool isReadOnly{false};
            QColor color;
        };

        struct Group
        {
            QString key;
            QWidget* container{nullptr};
            ShadPropertyGridGroupHeader* header{nullptr};
            QWidget* body{nullptr};
            QVBoxLayout* bodyLayout{nullptr};
        };

        int propertyIndex(const QString& key) const;
        int groupIndex(const QString& group) const;
        int ensureGroup(const QString& group);
        void removeGroupAt(int index);
        bool insertProperty(const QString& key, const QString& label, PropertyType type, QWidget* editor, const QString& group);
        QWidget* createEditor(PropertyType type, const QVariant& value);
        void connectEditor(const QString& key, PropertyType type, QWidget* editor);
        void applyReadOnly(const Property& property);
        void updateColorButton(ShadButton* button, const QColor& color) const;
        QPixmap swatchPixmap(const QColor& color) const;
        void refreshColorButtons();
        void pickColor(const QString& key);
        void setCurrentKey(const QString& key);
        void updateDescription();
        void handleFocusChanged(QWidget* now);
        void disconnectChildren();

        QList<Property> _properties;
        QList<Group> _groups;
        QString _currentKey;
        int _labelWidth{kDefaultLabelWidth};
        bool _isDescriptionVisible{true};
        QMetaObject::Connection _focusConnection;

        ShadScrollArea* _scrollArea{nullptr};
        QWidget* _contentWidget{nullptr};
        QVBoxLayout* _contentLayout{nullptr};
        ShadSeparator* _descriptionSeparator{nullptr};
        QWidget* _descriptionArea{nullptr};
        ShadLabel* _descriptionTitle{nullptr};
        ShadLabel* _descriptionText{nullptr};
};

#endif // SHADPROPERTYGRIDPRIVATE_H
