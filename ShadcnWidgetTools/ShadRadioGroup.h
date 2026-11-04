#ifndef SHADRADIOGROUP_H
#define SHADRADIOGROUP_H

#include <QString>
#include <QStringList>
#include <QWidget>

#include "ShadProperty.h"

class ShadRadioButton;

// 单选组，负责布局与互斥，索引即添加顺序
class ShadRadioGroupPrivate;

class SHADCN_EXPORT ShadRadioGroup : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadRadioGroup)
    Q_PROPERTY(int currentIndex READ getCurrentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(Qt::Orientation orientation READ getOrientation WRITE setOrientation)

    public:
        explicit ShadRadioGroup(QWidget* parent = nullptr);
        explicit ShadRadioGroup(Qt::Orientation orientation, QWidget* parent = nullptr);
        ~ShadRadioGroup() override;

        ShadRadioButton* addItem(const QString& text);
        void addItems(const QStringList& texts);
        ShadRadioButton* itemAt(int index) const;
        int count() const;

        // 传入 -1 清空选中
        void setCurrentIndex(int index);
        int getCurrentIndex() const;

        void setOrientation(Qt::Orientation orientation);
        Qt::Orientation getOrientation() const;

    Q_SIGNALS:
        void currentIndexChanged(int index);
};

#endif // SHADRADIOGROUP_H
