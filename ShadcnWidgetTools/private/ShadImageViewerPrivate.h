#ifndef SHADIMAGEVIEWERPRIVATE_H
#define SHADIMAGEVIEWERPRIVATE_H

#include <QCache>
#include <QImage>
#include <QList>
#include <QObject>
#include <QPixmap>
#include <QPointF>
#include <QPointer>
#include <QRectF>
#include <QSet>
#include <QSizeF>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include "ShadProperty.h"

class QAction;
class QEvent;
class QGraphicsOpacityEffect;
class QLabel;
class QWidget;
class ShadContextMenu;
class ShadFrameAnimation;
class ShadImageViewer;
class ShadImageViewerButton;
class ShadImageViewerToolbar;
class ShadSpinner;

// 只用于过滤顶层窗口的事件，不需要 Q_OBJECT
class ShadImageViewerPrivate : public QObject
{
    SHAD_D_CREATE(ShadImageViewer)

    public:
        ShadImageViewerPrivate();
        ~ShadImageViewerPrivate() override;

        static constexpr qreal kMaskAlpha = 0.8;
        static constexpr int kOpenDurationMs = 200;
        static constexpr int kCloseDurationMs = 150;
        static constexpr int kSnapDurationMs = 200;
        // 打开动画的起始缩放
        static constexpr qreal kInitialScale = 0.95;
        static constexpr qreal kMinScale = 0.1;
        static constexpr qreal kMaxScale = 10.0;
        static constexpr qreal kZoomStep = 1.25;
        static constexpr int kEdgeMargin = 16;
        static constexpr int kRoundButtonSize = 40;
        static constexpr int kToolbarBottom = 24;
        // 图片区域为按钮与工具栏留出的边距
        static constexpr int kVerticalInset = 64;
        static constexpr int kNavigationInset = 72;
        static constexpr int kSpinnerSize = 32;
        static constexpr int kErrorIconSize = 48;
        static constexpr int kErrorGap = 12;
        static constexpr int kCacheCostKb = 256 * 1024;

        enum class ImageState
        {
            Empty = 0,
            Loading,
            Ready,
            Failed,
        };

        // 一张图片的多级缩小版本与实际大小
        struct ImageEntry
        {
            QList<QPixmap> levels;
            QSizeF size;
        };

        QWidget* resolveHost() const;
        void attachHost(QWidget* host);
        void detachHost();
        // 预览放到顶层窗口的最上层，窗口圆角保持在预览之上
        void restack();
        // 自绘标题栏的高度，顶部按钮避开标题栏
        int topInset() const;

        int imageCount() const;
        // 清空缓存与加载状态，丢弃未完成的加载结果
        void resetSource();
        // 切换到 index 并复位变换，force 时强制重新加载
        void showIndex(int index, bool force);
        void requestImage(int index);
        void onImageLoaded(quint64 serial, int index, const QList<QImage>& levels, const QString& errorString);
        static ImageEntry createEntry(const QList<QImage>& levels, const QSizeF& size);
        static int entryCost(const ImageEntry& entry);

        // 图片可以显示的区域
        QRectF contentRect() const;
        // 旋转后的实际大小
        QSizeF rotatedSize() const;
        qreal fitScale() const;
        // 当前缩放与平移下图片的外接矩形，不含打开动画
        QRectF imageRect() const;
        bool isImageAt(const QPointF& pos) const;
        void resetTransform();
        void applyFit();
        void zoomTo(qreal scale, const QPointF& anchor);
        void zoomBy(qreal factor, const QPointF& anchor);
        // 适应窗口与实际大小之间切换
        void toggleFit(const QPointF& anchor);
        void rotateBy(int degrees);
        QPointF clampedOffset(const QPointF& offset) const;
        void animateOffsetTo(const QPointF& target);
        void navigate(int step);
        // 方向键对应的切换步进，RTL 时方向互换
        int keyStep(int key) const;

        void layoutChrome();
        void updateChrome();
        void updateCursor(const QPointF& pos);
        void setAppear(qreal appear);
        void requestClose();
        void finishClose();
        QImage currentImage() const;
        void copyImage();
        void saveImage();

        QPointer<QWidget> _anchor;
        QPointer<QWidget> _host;
        QPointer<QWidget> _focusBeforeOpen;

        QStringList _paths;
        QList<QPixmap> _pixmaps;
        int _currentIndex{-1};
        bool _isToolbarVisible{true};
        bool _isOpen{false};
        bool _isClosing{false};
        bool _isDeleteOnClose{false};

        // 图片来源版本号，用于丢弃过期的加载结果
        quint64 _sourceSerial{0};
        QCache<int, ImageEntry> _cache;
        QSet<int> _pending;
        QSet<int> _failed;
        ImageState _state{ImageState::Empty};
        ImageEntry _current;

        qreal _scale{1.0};
        int _rotation{0};
        bool _isFlippedHorizontally{false};
        bool _isFlippedVertically{false};
        QPointF _offset;
        // 处于适应窗口状态时，窗口尺寸变化后重新计算缩放
        bool _isFitMode{true};

        bool _isPressed{false};
        bool _isDragging{false};
        QPointF _pressPos;
        QPointF _pressOffset;

        qreal _appear{0.0};
        ShadFrameAnimation* _appearAnimation{nullptr};
        ShadFrameAnimation* _offsetAnimation{nullptr};
        QPointF _offsetFrom;
        QPointF _offsetTo;

        ShadImageViewerButton* _closeButton{nullptr};
        ShadImageViewerButton* _prevButton{nullptr};
        ShadImageViewerButton* _nextButton{nullptr};
        ShadImageViewerToolbar* _toolbar{nullptr};
        ShadImageViewerButton* _zoomOutButton{nullptr};
        QLabel* _zoomLabel{nullptr};
        ShadImageViewerButton* _zoomInButton{nullptr};
        ShadImageViewerButton* _fitButton{nullptr};
        ShadImageViewerButton* _rotateLeftButton{nullptr};
        ShadImageViewerButton* _rotateRightButton{nullptr};
        ShadImageViewerButton* _flipHorizontalButton{nullptr};
        ShadImageViewerButton* _flipVerticalButton{nullptr};
        ShadImageViewerButton* _saveButton{nullptr};
        ShadSpinner* _spinner{nullptr};
        ShadContextMenu* _contextMenu{nullptr};
        QAction* _copyAction{nullptr};
        QAction* _saveAction{nullptr};
        QList<QGraphicsOpacityEffect*> _chromeEffects;

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
};

#endif // SHADIMAGEVIEWERPRIVATE_H
