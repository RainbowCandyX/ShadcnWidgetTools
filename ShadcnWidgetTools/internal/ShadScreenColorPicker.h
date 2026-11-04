#ifndef SHADSCREENCOLORPICKER_H
#define SHADSCREENCOLORPICKER_H

#include <QColor>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPointer>

#include "ShadScreenCapture.h"

class QTimer;
class QWidget;
class ShadScreenPickerOverlay;

// 屏幕取色控制器，组件库内部使用：截取所有屏幕，在每块屏幕上显示 ShadScreenPickerOverlay，结束后发出 colorPicked 或 canceled
// 覆盖窗口以 host 所在的窗口为父窗口，host 位于模态对话框中时同样可以操作
// 平台差异：Wayland 下 Qt 无法截屏，isSupported 返回 false；macOS 需要屏幕录制权限，没有权限时请求授权并不开始取色
// 取色期间切换到其他程序视为取消
class ShadScreenColorPicker : public QObject
{
    Q_OBJECT

    public:
        explicit ShadScreenColorPicker(QWidget* host);
        ~ShadScreenColorPicker() override;

        // 当前平台能否截屏取色
        static bool isSupported();

        // 截图中是否包含本程序的窗口，默认 true
        // 为 false 时用透明度而不是 hide：隐藏 exec 打开的对话框会结束 exec
        void setOwnWindowsVisible(bool visible);
        bool isOwnWindowsVisible() const;

        // 开始取色；平台不支持、没有屏幕录制权限或截屏失败时返回 false，不发出信号
        bool start();
        void cancel();
        bool isActive() const;

    Q_SIGNALS:
        void colorPicked(const QColor& color);
        void canceled();

    private:
        bool captureAndShowOverlays();
        // 关闭全部覆盖窗口，把激活状态还给发起取色的窗口
        void closeOverlays();

        QPointer<QWidget> _host;
        QList<QPointer<ShadScreenPickerOverlay>> _overlays;
        QMetaObject::Connection _applicationStateConnection;
        bool _isOwnWindowsVisible{true};
        bool _isWaitingForCapture{false};
        QTimer* _captureTimer{nullptr};
        ShadScreenCapture _capture;
};

#endif // SHADSCREENCOLORPICKER_H
