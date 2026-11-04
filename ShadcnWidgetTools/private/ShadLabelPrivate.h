#ifndef SHADLABELPRIVATE_H
#define SHADLABELPRIVATE_H

#include <QPointer>
#include <QWidget>

#include "ShadDef.h"

class ShadLabel;

class ShadLabelPrivate
{
    SHAD_D_CREATE(ShadLabel)

    public:
        ShadLabelPrivate();
        ~ShadLabelPrivate();

        void applyThemeColors();

        ShadThemeType::ThemeColor _colorRole{ShadThemeType::Foreground};
        // 已安装事件过滤器的 buddy，被外部删除后自动置空
        QPointer<QWidget> _observedBuddy;
};

#endif // SHADLABELPRIVATE_H
