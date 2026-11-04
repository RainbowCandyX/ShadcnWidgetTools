#ifndef SHADDOCKCONTAINER_H
#define SHADDOCKCONTAINER_H

#include <QList>
#include <QPoint>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ShadDef.h"

class QResizeEvent;
class QVBoxLayout;
class ShadDockGroup;
class ShadDockOverlay;
class ShadResizable;

class ShadDockContainer : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadDockContainer(QWidget* parent = nullptr);
        ~ShadDockContainer() override;

        ShadResizable* rootSplitter() const;
        QList<ShadDockGroup*> groups() const;
        ShadDockGroup* groupAt(const QPoint& pos) const;
        bool isEmpty() const;

        // target 为空时相对整个容器
        void insertNode(QWidget* node, QWidget* target, ShadDockType::DockArea area, int extent);
        // 不删除节点
        void removeNode(QWidget* node);
        // 取出全部内容作为一个节点
        QWidget* takeContent();
        void setContent(QWidget* node);
        // 组内面板需先取出
        void clear();
        void updateStretch();

        ShadDockOverlay* overlay();
        void hideOverlay();

        static ShadResizable* parentSplitter(const QWidget* node);
        static ShadResizable* createSplitter(Qt::Orientation orientation);
        static void moveInto(ShadResizable* splitter, int index, QWidget* widget);
        static void applySizes(ShadResizable* splitter, const QList<int>& sizes);

    protected:
        void resizeEvent(QResizeEvent* event) override;

    private:
        void collectGroups(QWidget* node, QList<ShadDockGroup*>* result) const;
        bool containsCentral(QWidget* node) const;
        void updateStretch(ShadResizable* splitter);
        void simplify(ShadResizable* splitter);
        static void insertSized(ShadResizable* splitter, int index, QWidget* node, int extent, QWidget* donor);

        QVBoxLayout* _layout{nullptr};
        ShadResizable* _root{nullptr};
        ShadDockOverlay* _overlay{nullptr};
};

#endif // SHADDOCKCONTAINER_H
