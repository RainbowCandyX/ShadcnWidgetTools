#ifndef SHADTABVIEWPRIVATE_H
#define SHADTABVIEWPRIVATE_H

#include <QList>
#include <QPoint>
#include <QPointer>
#include <QString>

#include <functional>

#include "ShadProperty.h"

class QEvent;
class QMimeData;
class QStackedLayout;
class QVBoxLayout;
class QWidget;
class ShadPageTransition;
class ShadSeparator;
class ShadTabView;
class ShadTabViewBar;

class ShadTabViewPrivate
{
    SHAD_D_CREATE(ShadTabView)

    public:
        ShadTabViewPrivate();
        ~ShadTabViewPrivate();

        void updatePages();
        void stepCurrent(int step);
        void startDrag(int index, const QPoint& grabOffset);
        // 不接受时返回 nullptr
        ShadTabView* acceptedSource(const QMimeData* mimeData) const;
        bool handleDragEvent(QEvent* event);
        static void transferTab(ShadTabView* source, int index, ShadTabView* target, int targetIndex);

        QVBoxLayout* _layout{nullptr};
        // 放进标题栏后由标题栏管理生命周期
        QPointer<ShadTabViewBar> _bar;
        ShadSeparator* _separator{nullptr};
        QWidget* _pageArea{nullptr};
        QStackedLayout* _pageStack{nullptr};
        QList<QPointer<QWidget>> _pages;
        QPointer<QWidget> _shownPage;
        ShadPageTransition* _transition{nullptr};
        QString _dragGroup;
        std::function<ShadTabView*(ShadTabView*)> _detachHandler;
};

#endif // SHADTABVIEWPRIVATE_H
