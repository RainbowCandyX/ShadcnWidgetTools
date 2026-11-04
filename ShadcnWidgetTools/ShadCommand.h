#ifndef SHADCOMMAND_H
#define SHADCOMMAND_H

#include <QSize>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "ShadIconType.h"
#include "ShadProperty.h"

class QAction;
class QEvent;
class QObject;
class QPaintEvent;
class QResizeEvent;

// 命令面板，搜索框加分组列表，按模糊匹配过滤
class ShadCommandPrivate;

class SHADCN_EXPORT ShadCommand : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadCommand)
    friend class ShadCommandDialog;
    Q_PROPERTY(QString placeholderText READ getPlaceholderText WRITE setPlaceholderText)
    Q_PROPERTY(QString emptyText READ getEmptyText WRITE setEmptyText)
    Q_PROPERTY(QString searchText READ getSearchText WRITE setSearchText)

    public:
        explicit ShadCommand(QWidget* parent = nullptr);
        ~ShadCommand() override;

        // 之后添加的项归入该分组；返回它是第几个分组（从 0 开始，clear 后重新计数），用于 setGroupHeading
        int addGroup(const QString& heading);
        // 修改第 groupIndex 个分组的标题，例如切换语言时；序号无效时不处理
        void setGroupHeading(int groupIndex, const QString& heading);
        // shortcutText 只用于显示
        QAction* addItem(const QString& text, ShadIconType::IconName icon = ShadIconType::None, const QString& shortcutText = QString());
        // 搜索时额外匹配的关键词
        void setItemKeywords(QAction* action, const QStringList& keywords);
        void addSeparator();
        void clear();

        void setPlaceholderText(const QString& text);
        QString getPlaceholderText() const;
        void setEmptyText(const QString& text);
        QString getEmptyText() const;
        void setSearchText(const QString& text);
        QString getSearchText() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void itemTriggered(QAction* action);

    protected:
        bool event(QEvent* event) override;
        bool eventFilter(QObject* watched, QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADCOMMAND_H
