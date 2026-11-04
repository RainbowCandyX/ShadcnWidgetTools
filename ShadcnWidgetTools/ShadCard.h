#ifndef SHADCARD_H
#define SHADCARD_H

#include <QString>
#include <QWidget>

#include "ShadProperty.h"

class QHBoxLayout;
class QPaintEvent;
class QVBoxLayout;

// 卡片容器：Header（标题、描述、右上角操作）+ Content + Footer
class ShadCardPrivate;

class SHADCN_EXPORT ShadCard : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadCard)
    Q_PROPERTY(QString title READ getTitle WRITE setTitle)
    Q_PROPERTY(QString description READ getDescription WRITE setDescription)

    public:
        explicit ShadCard(QWidget* parent = nullptr);
        ShadCard(const QString& title, const QString& description, QWidget* parent = nullptr);
        ~ShadCard() override;

        void setTitle(const QString& title);
        QString getTitle() const;

        void setDescription(const QString& description);
        QString getDescription() const;

        // 对应 CardAction，放在 Header 右上角；传入 nullptr 移除，旧组件会被删除
        void setHeaderAction(QWidget* widget);
        QWidget* getHeaderAction() const;

        // 对应 CardContent
        QVBoxLayout* contentLayout() const;
        // 对应 CardFooter，首次获取时显示 Footer 区域
        QHBoxLayout* footerLayout();

    protected:
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADCARD_H
