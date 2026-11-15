#include "ShadDiffViewPrivate.h"

#include <QPainterPath>
#include <QPolygon>
#include <QPolygonF>
#include <QRect>
#include <QRectF>
#include <QRegion>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadDiffContent.h"
#include "ShadDiffView.h"
#include "ShadScrollArea.h"
#include "ShadScrollBar.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadDiffViewPrivate::ShadDiffViewPrivate()
{
}

ShadDiffViewPrivate::~ShadDiffViewPrivate()
{
}

void ShadDiffViewPrivate::updateLayout()
{
    Q_Q(ShadDiffView);
    // 避免 contentSizeChanged 导致重入
    if (_isUpdatingLayout)
    {
        return;
    }
    _isUpdatingLayout = true;
    const QRect innerRect = q->rect().adjusted(kBorderWidth, kBorderWidth, -kBorderWidth, -kBorderWidth);
    const int innerWidth = qMax(0, innerRect.width());
    _content->setViewportWidth(innerWidth);
    const int contentHeight = _content->contentHeight();
    const int visibleHeight = _maximumContentHeight > 0 ? qMin(contentHeight, _maximumContentHeight) : contentHeight;
    _scrollArea->setGeometry(innerRect.left(), innerRect.top(), innerWidth, visibleHeight);
    _preferredHeight = visibleHeight + 2 * kBorderWidth;

    const int radius = qMax(0, shadTheme->getRadius(ShadThemeType::Large) - kBorderWidth);
    const QRectF maskRect(0.0, 0.0, innerWidth, visibleHeight);
    _scrollArea->setMask(QRegion(ShadStyleHelper::roundedRectPath(maskRect, radius).toFillPolygon().toPolygon()));
    _scrollArea->getOverlayScrollBar(Qt::Vertical)->setTrackInset(radius);
    _scrollArea->getOverlayScrollBar(Qt::Horizontal)->setTrackInset(radius);
    _isUpdatingLayout = false;

    const QSize hint = q->sizeHint();
    if (hint != _lastSizeHint)
    {
        _lastSizeHint = hint;
        q->updateGeometry();
    }
    q->update();
}
