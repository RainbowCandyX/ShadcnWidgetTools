#ifndef SHADFIELDSET_H
#define SHADFIELDSET_H

#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadProperty.h"

class QVBoxLayout;

// 表单分组，对应 shadcn FieldSet / FieldLegend：标题（legend）、标题下方的说明与一组内容
// 标题 font-medium，Legend 样式为 text-base、Label 样式为 text-sm，下方留 12px（mb-3）；说明 text-sm text-muted-foreground，与标题间距 6px
// 内容通过 addWidget 添加（通常为 ShadFieldGroup 或多个 ShadField），与上方间距 24（gap-6），内容之间同样为 24
class ShadFieldSetPrivate;

class SHADCN_EXPORT ShadFieldSet : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadFieldSet)
    Q_PROPERTY(QString legend READ getLegend WRITE setLegend)
    Q_PROPERTY(QString description READ getDescription WRITE setDescription)
    Q_PROPERTY(ShadFieldType::LegendVariant legendVariant READ getLegendVariant WRITE setLegendVariant)

    public:
        explicit ShadFieldSet(QWidget* parent = nullptr);
        ShadFieldSet(const QString& legend, const QString& description, QWidget* parent = nullptr);
        ~ShadFieldSet() override;

        // 为空时不显示
        void setLegend(const QString& legend);
        QString getLegend() const;
        // 默认 Legend
        void setLegendVariant(ShadFieldType::LegendVariant variant);
        ShadFieldType::LegendVariant getLegendVariant() const;
        // 为空时不显示
        void setDescription(const QString& description);
        QString getDescription() const;

        // 添加内容，分组接管父子关系
        void addWidget(QWidget* widget);
        // 内容区布局，也可以直接操作
        QVBoxLayout* contentLayout() const;
};

#endif // SHADFIELDSET_H
