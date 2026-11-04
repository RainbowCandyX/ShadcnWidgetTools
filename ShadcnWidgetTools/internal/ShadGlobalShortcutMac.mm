#include "ShadGlobalShortcut.h"

#include <QCoreApplication>
#include <QKeyCombination>
#include <QKeySequence>
#include <QMetaObject>
#include <QObject>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <atomic>
#include <functional>
#include <memory>
#include <utility>

#include <Carbon/Carbon.h>
#import <AppKit/AppKit.h>

namespace
{
    constexpr OSType kHotKeySignature = (static_cast<OSType>('S') << 24) | (static_cast<OSType>('H') << 16) | (static_cast<OSType>('S') << 8) | static_cast<OSType>('C');
    constexpr UInt32 kInvalidKeyCode = 0xFFFF;

    UInt32 nextHotKeyId()
    {
        static std::atomic<UInt32> counter{1};
        return counter.fetch_add(1);
    }

    UInt32 toKeyCode(Qt::Key key)
    {
        switch (key)
        {
            case Qt::Key_A: return kVK_ANSI_A;
            case Qt::Key_B: return kVK_ANSI_B;
            case Qt::Key_C: return kVK_ANSI_C;
            case Qt::Key_D: return kVK_ANSI_D;
            case Qt::Key_E: return kVK_ANSI_E;
            case Qt::Key_F: return kVK_ANSI_F;
            case Qt::Key_G: return kVK_ANSI_G;
            case Qt::Key_H: return kVK_ANSI_H;
            case Qt::Key_I: return kVK_ANSI_I;
            case Qt::Key_J: return kVK_ANSI_J;
            case Qt::Key_K: return kVK_ANSI_K;
            case Qt::Key_L: return kVK_ANSI_L;
            case Qt::Key_M: return kVK_ANSI_M;
            case Qt::Key_N: return kVK_ANSI_N;
            case Qt::Key_O: return kVK_ANSI_O;
            case Qt::Key_P: return kVK_ANSI_P;
            case Qt::Key_Q: return kVK_ANSI_Q;
            case Qt::Key_R: return kVK_ANSI_R;
            case Qt::Key_S: return kVK_ANSI_S;
            case Qt::Key_T: return kVK_ANSI_T;
            case Qt::Key_U: return kVK_ANSI_U;
            case Qt::Key_V: return kVK_ANSI_V;
            case Qt::Key_W: return kVK_ANSI_W;
            case Qt::Key_X: return kVK_ANSI_X;
            case Qt::Key_Y: return kVK_ANSI_Y;
            case Qt::Key_Z: return kVK_ANSI_Z;
            case Qt::Key_0: return kVK_ANSI_0;
            case Qt::Key_1: return kVK_ANSI_1;
            case Qt::Key_2: return kVK_ANSI_2;
            case Qt::Key_3: return kVK_ANSI_3;
            case Qt::Key_4: return kVK_ANSI_4;
            case Qt::Key_5: return kVK_ANSI_5;
            case Qt::Key_6: return kVK_ANSI_6;
            case Qt::Key_7: return kVK_ANSI_7;
            case Qt::Key_8: return kVK_ANSI_8;
            case Qt::Key_9: return kVK_ANSI_9;
            case Qt::Key_F1: return kVK_F1;
            case Qt::Key_F2: return kVK_F2;
            case Qt::Key_F3: return kVK_F3;
            case Qt::Key_F4: return kVK_F4;
            case Qt::Key_F5: return kVK_F5;
            case Qt::Key_F6: return kVK_F6;
            case Qt::Key_F7: return kVK_F7;
            case Qt::Key_F8: return kVK_F8;
            case Qt::Key_F9: return kVK_F9;
            case Qt::Key_F10: return kVK_F10;
            case Qt::Key_F11: return kVK_F11;
            case Qt::Key_F12: return kVK_F12;
            case Qt::Key_F13: return kVK_F13;
            case Qt::Key_F14: return kVK_F14;
            case Qt::Key_F15: return kVK_F15;
            case Qt::Key_F16: return kVK_F16;
            case Qt::Key_F17: return kVK_F17;
            case Qt::Key_F18: return kVK_F18;
            case Qt::Key_F19: return kVK_F19;
            case Qt::Key_F20: return kVK_F20;
            case Qt::Key_Space: return kVK_Space;
            case Qt::Key_Return: return kVK_Return;
            case Qt::Key_Enter: return kVK_ANSI_KeypadEnter;
            case Qt::Key_Tab: return kVK_Tab;
            case Qt::Key_Escape: return kVK_Escape;
            case Qt::Key_Backspace: return kVK_Delete;
            case Qt::Key_Delete: return kVK_ForwardDelete;
            case Qt::Key_Home: return kVK_Home;
            case Qt::Key_End: return kVK_End;
            case Qt::Key_PageUp: return kVK_PageUp;
            case Qt::Key_PageDown: return kVK_PageDown;
            case Qt::Key_Left: return kVK_LeftArrow;
            case Qt::Key_Right: return kVK_RightArrow;
            case Qt::Key_Up: return kVK_UpArrow;
            case Qt::Key_Down: return kVK_DownArrow;
            case Qt::Key_Minus: return kVK_ANSI_Minus;
            case Qt::Key_Equal: return kVK_ANSI_Equal;
            case Qt::Key_Comma: return kVK_ANSI_Comma;
            case Qt::Key_Period: return kVK_ANSI_Period;
            case Qt::Key_Slash: return kVK_ANSI_Slash;
            case Qt::Key_Semicolon: return kVK_ANSI_Semicolon;
            case Qt::Key_Apostrophe: return kVK_ANSI_Quote;
            case Qt::Key_BracketLeft: return kVK_ANSI_LeftBracket;
            case Qt::Key_BracketRight: return kVK_ANSI_RightBracket;
            case Qt::Key_Backslash: return kVK_ANSI_Backslash;
            case Qt::Key_QuoteLeft: return kVK_ANSI_Grave;
            default: break;
        }
        return kInvalidKeyCode;
    }

    UInt32 toModifiers(Qt::KeyboardModifiers modifiers)
    {
        const bool isSwapped = !QCoreApplication::testAttribute(Qt::AA_MacDontSwapCtrlAndMeta);
        UInt32 result = 0;
        if (modifiers.testFlag(Qt::ControlModifier))
        {
            result |= isSwapped ? cmdKey : controlKey;
        }
        if (modifiers.testFlag(Qt::MetaModifier))
        {
            result |= isSwapped ? controlKey : cmdKey;
        }
        if (modifiers.testFlag(Qt::ShiftModifier))
        {
            result |= shiftKey;
        }
        if (modifiers.testFlag(Qt::AltModifier))
        {
            result |= optionKey;
        }
        return result;
    }

    struct HotKeyReceiver
    {
        UInt32 id{0};
        std::function<void()> callback;
    };

    OSStatus handleHotKey(EventHandlerCallRef handlerCall, EventRef event, void* userData)
    {
        Q_UNUSED(handlerCall);
        HotKeyReceiver* receiver = static_cast<HotKeyReceiver*>(userData);
        EventHotKeyID hotKeyId;
        if (GetEventParameter(event, kEventParamDirectObject, typeEventHotKeyID, nullptr, sizeof(hotKeyId), nullptr, &hotKeyId) != noErr)
        {
            return eventNotHandledErr;
        }
        if (hotKeyId.signature != kHotKeySignature || hotKeyId.id != receiver->id || receiver->id == 0)
        {
            return eventNotHandledErr;
        }
        receiver->callback();
        return noErr;
    }
}

struct ShadGlobalShortcut::Impl
{
    HotKeyReceiver receiver;
    EventHandlerRef handler{nullptr};
    EventHotKeyRef hotKey{nullptr};

    void unregister()
    {
        if (hotKey != nullptr)
        {
            UnregisterEventHotKey(hotKey);
            hotKey = nullptr;
        }
        receiver.id = 0;
    }
};

ShadGlobalShortcut::ShadGlobalShortcut(QObject* parent)
    : QObject(parent), _impl(std::make_unique<Impl>())
{
    _impl->receiver.callback = [this]()
    {
        QMetaObject::invokeMethod(this, [this]() { Q_EMIT activated(); }, Qt::QueuedConnection);
    };
    const EventTypeSpec eventType{kEventClassKeyboard, kEventHotKeyPressed};
    InstallApplicationEventHandler(&handleHotKey, 1, &eventType, &_impl->receiver, &_impl->handler);
}

ShadGlobalShortcut::~ShadGlobalShortcut()
{
    _impl->unregister();
    if (_impl->handler != nullptr)
    {
        RemoveEventHandler(_impl->handler);
        _impl->handler = nullptr;
    }
}

bool ShadGlobalShortcut::isSupported()
{
    return true;
}

void ShadGlobalShortcut::activateApplication()
{
    if (@available(macOS 14.0, *))
    {
        [NSApp activate];
        return;
    }
    [NSApp activateIgnoringOtherApps:YES];
}

bool ShadGlobalShortcut::setShortcut(const QKeySequence& sequence)
{
    if (sequence.isEmpty())
    {
        _impl->unregister();
        _sequence = QKeySequence();
        return true;
    }
    const QKeyCombination combination = sequence[0];
    const QKeySequence normalized(combination);
    if (_impl->hotKey != nullptr && normalized == _sequence)
    {
        return true;
    }
    const UInt32 keyCode = toKeyCode(combination.key());
    if (keyCode == kInvalidKeyCode || _impl->handler == nullptr)
    {
        return false;
    }
    const UInt32 id = nextHotKeyId();
    const EventHotKeyID hotKeyId{kHotKeySignature, id};
    EventHotKeyRef hotKey = nullptr;
    if (RegisterEventHotKey(keyCode, toModifiers(combination.keyboardModifiers()), hotKeyId, GetApplicationEventTarget(), kEventHotKeyExclusive, &hotKey) != noErr)
    {
        return false;
    }
    _impl->unregister();
    _impl->hotKey = hotKey;
    _impl->receiver.id = id;
    _sequence = normalized;
    return true;
}

QKeySequence ShadGlobalShortcut::getShortcut() const
{
    return _sequence;
}
