#ifndef SHADITEMCONTENT_H
#define SHADITEMCONTENT_H

#include <QFont>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "ShadDef.h"

class QEvent;
class QPaintEvent;

// 列表项的标题与描述，组件库内部使用，对应 ItemContent / ItemTitle / ItemDescription
// 标题 text-sm font-medium leading-snug，单行，超出时尾部省略（line-clamp-1）
// 描述 text-sm（ExtraSmall 为 text-xs）text-muted-foreground leading-normal，最多两行，第二行放不下时尾部省略（line-clamp-2）
// 标题与描述间距 gap-1（ExtraSmall 为 0）；分配的高度大于内容时垂直居中（items-center）
// 高度随宽度变化（heightForWidth），宽度可以压缩到 0，文字按宽度省略
class ShadItemContent : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadItemContent(QWidget* parent = nullptr);
        ~ShadItemContent() override;

        void setTitle(const QString& title);
        QString getTitle() const;
        void setDescription(const QString& description);
        QString getDescription() const;
        void setItemSize(ShadItemType::Size size);

        bool hasHeightForWidth() const override;
        int heightForWidth(int width) const override;
        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        QFont titleFont() const;
        QFont descriptionFont() const;
        int titleLineHeight() const;
        int descriptionLineHeight() const;
        int gap() const;
        // 按宽度把描述排成最多两行，最后一行放不下时尾部省略
        QStringList descriptionLines(int width) const;
        // 按宽度计算的内容高度
        int contentHeight(int width) const;

        QString _title;
        QString _description;
        ShadItemType::Size _size{ShadItemType::Medium};
};

#endif // SHADITEMCONTENT_H
