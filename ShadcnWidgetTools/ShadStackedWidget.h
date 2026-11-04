#ifndef SHADSTACKEDWIDGET_H
#define SHADSTACKEDWIDGET_H

#include <QStackedWidget>

#include "ShadProperty.h"

class QWidget;

// 切换页面时播放过渡动画，配置跟随 ShadTheme
class ShadStackedWidgetPrivate;

class SHADCN_EXPORT ShadStackedWidget : public QStackedWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadStackedWidget)

    public:
        explicit ShadStackedWidget(QWidget* parent = nullptr);
        ~ShadStackedWidget() override;
};

#endif // SHADSTACKEDWIDGET_H
