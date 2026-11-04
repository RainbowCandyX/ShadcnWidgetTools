#include "ShadPopupEffect.h"

#include <QColor>
#include <QEvent>
#include <QPainter>
#include <QWidget>

#include <utility>

#include "ShadDef.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadWindowEffect.h"

ShadPopupEffect::ShadPopupEffect(QWidget* popup, std::function<QRect()> cardRect)
    : QObject(popup), _popup(popup), _cardRect(std::move(cardRect))
{
    popup->installEventFilter(this);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { apply(); });
}

ShadPopupEffect::~ShadPopupEffect()
{
}

void ShadPopupEffect::paintBackdrop(QPainter* painter, const QWidget* popup, const QRectF& card, qreal radius)
{
    using namespace ShadStyleHelper;
    const ShadWindowType::Effect effect = shadTheme->getWindowEffect();
    if (effect == ShadWindowType::NoEffect)
    {
        return;
    }
    const QColor background = shadTheme->getColor(shadTheme->getThemeMode(), ShadThemeType::Background);
    const qreal opacity = shadTheme->getWindowEffectOpacity(effect);
    if (effect == ShadWindowType::Wallpaper && !ShadWindowEffect::wallpaper().isNull())
    {
        ShadWindowEffect::paintWallpaper(painter, popup, card, radius);
        fillRoundedRect(painter, card, radius, withAlpha(background, opacity));
        return;
    }
#ifdef Q_OS_MACOS
    if (effect == ShadWindowType::Blur)
    {
        fillRoundedRect(painter, card, radius, withAlpha(background, opacity));
        return;
    }
#endif
    // 没有可用特效时保持不透明
    fillRoundedRect(painter, card, radius, shadTheme->getColor(shadTheme->getThemeMode(), ShadThemeType::Popover));
}

bool ShadPopupEffect::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == _popup && (event->type() == QEvent::Show || event->type() == QEvent::Resize))
    {
        apply();
    }
    return QObject::eventFilter(watched, event);
}

void ShadPopupEffect::apply()
{
#ifdef Q_OS_MACOS
    if (_popup->internalWinId() == 0)
    {
        return;
    }
    const ShadWindowType::Effect effect = shadTheme->getWindowEffect() == ShadWindowType::Blur ? ShadWindowType::Blur : ShadWindowType::NoEffect;
    ShadWindowEffect::apply(_popup, effect, shadTheme->getRadius(ShadThemeType::Medium), _cardRect());
#endif
}
