#ifndef SHADTOASTITEM_H
#define SHADTOASTITEM_H

#include <QFont>
#include <QRect>
#include <QRectF>
#include <QWidget>

#include "ShadToaster.h"

class QEvent;
class QPaintEvent;
class QResizeEvent;
class QTimer;
class QVariantAnimation;
class ShadButton;
class ShadTransition;

// 单条 toast 卡片，由 ShadToaster 创建并负责摆放；卡片阴影由 ShadToaster 统一绘制
class ShadToastItem : public QWidget
{
    Q_OBJECT

    public:
        ShadToastItem(int id, QWidget* parent);
        ~ShadToastItem() override;

        int toastId() const;
        // 设置内容并重新开始关闭计时
        void setOptions(const ShadToastOptions& options, int defaultDuration);
        // 给定卡片宽度下内容完整显示所需的高度
        int naturalHeight(int width) const;

        // 移动到目标几何位置；animate 为 false 时立即到位
        void moveTo(const QRect& target, bool animate);
        // 是否已经被摆放过（新建的 toast 需要先放到入场起点）
        bool isPlaced() const;
        // 收起堆叠时后面的 toast 隐藏内容（对应 data-behind:opacity-0）
        void setContentVisible(bool visible);
        bool isExiting() const;
        // 移动到离场位置并同时淡出，结束后发出 exitFinished
        void startExit(const QRect& target);
        // 整体不透明度，离场时由 1 渐变到 0，ShadToaster 绘制阴影时同步使用
        qreal opacity() const;

        // rounded-2xl，随主题基础圆角变化
        static int cardRadius();

    Q_SIGNALS:
        void dismissRequested(int id);
        void actionTriggered(int id);
        void exitFinished(int id);
        // 位移动画每一帧发出，ShadToaster 据此刷新阴影与鼠标区域
        void geometryAnimating();

    protected:
        void paintEvent(QPaintEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void changeEvent(QEvent* event) override;

    private:
        // 卡片内各元素的位置（卡片坐标系）
        struct ContentLayout
        {
            QRectF iconRect;
            QRectF titleRect;
            QRectF descriptionRect;
            QRectF actionRect;
            QRectF closeRect;
        };

        ContentLayout computeLayout(int width, int height) const;
        int textWidth(int width) const;
        int titleHeight() const;
        int descriptionHeight(int width) const;
        void layoutChildren();
        // 关闭计时与倒计时动画同步开始，悬停展开时不暂停
        void startDismissTimer();

        int _id{0};
        ShadToastOptions _options;
        int _effectiveDuration{0};
        bool _isPlaced{false};
        bool _isContentVisible{true};
        bool _isExiting{false};
        qreal _spinAngle{0.0};
        // 剩余时间占总时长的比例，用于绘制底部倒计时条
        qreal _countdownProgress{1.0};
        QFont _titleFont;
        QFont _descriptionFont;
        ShadButton* _actionButton{nullptr};
        ShadButton* _closeButton{nullptr};
        QTimer* _dismissTimer{nullptr};
        QVariantAnimation* _geometryAnimation{nullptr};
        QVariantAnimation* _spinAnimation{nullptr};
        QVariantAnimation* _countdownAnimation{nullptr};
        ShadTransition* _contentTransition{nullptr};
        ShadTransition* _opacityTransition{nullptr};
};

#endif // SHADTOASTITEM_H
