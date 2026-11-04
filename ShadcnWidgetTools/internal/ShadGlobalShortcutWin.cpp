#include "ShadGlobalShortcut.h"

#include <QAbstractNativeEventFilter>
#include <QByteArray>
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

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace
{
    constexpr int kFirstHotKeyId = 0x5300;
    constexpr int kLastHotKeyId = 0xBFFF;

    int nextHotKeyId()
    {
        static std::atomic<int> counter{kFirstHotKeyId};
        const int id = counter.fetch_add(1);
        return kFirstHotKeyId + (id - kFirstHotKeyId) % (kLastHotKeyId - kFirstHotKeyId + 1);
    }

    UINT toVirtualKey(Qt::Key key)
    {
        if (key >= Qt::Key_A && key <= Qt::Key_Z)
        {
            return static_cast<UINT>('A' + (key - Qt::Key_A));
        }
        if (key >= Qt::Key_0 && key <= Qt::Key_9)
        {
            return static_cast<UINT>('0' + (key - Qt::Key_0));
        }
        if (key >= Qt::Key_F1 && key <= Qt::Key_F24)
        {
            return static_cast<UINT>(VK_F1 + (key - Qt::Key_F1));
        }
        switch (key)
        {
            case Qt::Key_Space:
                return VK_SPACE;
            case Qt::Key_Return:
            case Qt::Key_Enter:
                return VK_RETURN;
            case Qt::Key_Tab:
                return VK_TAB;
            case Qt::Key_Escape:
                return VK_ESCAPE;
            case Qt::Key_Backspace:
                return VK_BACK;
            case Qt::Key_Insert:
                return VK_INSERT;
            case Qt::Key_Delete:
                return VK_DELETE;
            case Qt::Key_Home:
                return VK_HOME;
            case Qt::Key_End:
                return VK_END;
            case Qt::Key_PageUp:
                return VK_PRIOR;
            case Qt::Key_PageDown:
                return VK_NEXT;
            case Qt::Key_Left:
                return VK_LEFT;
            case Qt::Key_Right:
                return VK_RIGHT;
            case Qt::Key_Up:
                return VK_UP;
            case Qt::Key_Down:
                return VK_DOWN;
            case Qt::Key_Print:
                return VK_SNAPSHOT;
            case Qt::Key_Pause:
                return VK_PAUSE;
            case Qt::Key_Minus:
                return VK_OEM_MINUS;
            case Qt::Key_Equal:
                return VK_OEM_PLUS;
            case Qt::Key_Comma:
                return VK_OEM_COMMA;
            case Qt::Key_Period:
                return VK_OEM_PERIOD;
            case Qt::Key_Slash:
                return VK_OEM_2;
            case Qt::Key_Semicolon:
                return VK_OEM_1;
            case Qt::Key_Apostrophe:
                return VK_OEM_7;
            case Qt::Key_BracketLeft:
                return VK_OEM_4;
            case Qt::Key_BracketRight:
                return VK_OEM_6;
            case Qt::Key_Backslash:
                return VK_OEM_5;
            case Qt::Key_QuoteLeft:
                return VK_OEM_3;
            default:
                break;
        }
        return 0;
    }

    UINT toModifiers(Qt::KeyboardModifiers modifiers)
    {
        UINT result = MOD_NOREPEAT;
        if (modifiers.testFlag(Qt::ControlModifier))
        {
            result |= MOD_CONTROL;
        }
        if (modifiers.testFlag(Qt::ShiftModifier))
        {
            result |= MOD_SHIFT;
        }
        if (modifiers.testFlag(Qt::AltModifier))
        {
            result |= MOD_ALT;
        }
        if (modifiers.testFlag(Qt::MetaModifier))
        {
            result |= MOD_WIN;
        }
        return result;
    }

    class HotKeyFilter : public QAbstractNativeEventFilter
    {
        public:
            explicit HotKeyFilter(std::function<void(int)> callback)
                : _callback(std::move(callback))
            {
            }

            bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override
            {
                Q_UNUSED(result);
                if (eventType != "windows_generic_MSG" && eventType != "windows_dispatcher_MSG")
                {
                    return false;
                }
                const MSG* msg = static_cast<const MSG*>(message);
                if (msg->message != WM_HOTKEY)
                {
                    return false;
                }
                _callback(static_cast<int>(msg->wParam));
                return false;
            }

        private:
            std::function<void(int)> _callback;
    };
}

struct ShadGlobalShortcut::Impl
{
    std::unique_ptr<HotKeyFilter> filter;
    int id{0};
    bool isRegistered{false};

    void unregister()
    {
        if (isRegistered)
        {
            UnregisterHotKey(nullptr, id);
            isRegistered = false;
        }
    }
};

ShadGlobalShortcut::ShadGlobalShortcut(QObject* parent)
    : QObject(parent), _impl(std::make_unique<Impl>())
{
    Impl* impl = _impl.get();
    _impl->filter = std::make_unique<HotKeyFilter>([this, impl](int id)
    {
        if (impl->isRegistered && id == impl->id)
        {
            QMetaObject::invokeMethod(this, [this]() { Q_EMIT activated(); }, Qt::QueuedConnection);
        }
    });
    QCoreApplication::instance()->installNativeEventFilter(_impl->filter.get());
}

ShadGlobalShortcut::~ShadGlobalShortcut()
{
    _impl->unregister();
    if (QCoreApplication::instance() != nullptr)
    {
        QCoreApplication::instance()->removeNativeEventFilter(_impl->filter.get());
    }
}

bool ShadGlobalShortcut::isSupported()
{
    return true;
}

void ShadGlobalShortcut::activateApplication()
{
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
    if (_impl->isRegistered && normalized == _sequence)
    {
        return true;
    }
    const UINT virtualKey = toVirtualKey(combination.key());
    if (virtualKey == 0)
    {
        return false;
    }
    const int id = nextHotKeyId();
    if (!RegisterHotKey(nullptr, id, toModifiers(combination.keyboardModifiers()), virtualKey))
    {
        return false;
    }
    _impl->unregister();
    _impl->id = id;
    _impl->isRegistered = true;
    _sequence = normalized;
    return true;
}

QKeySequence ShadGlobalShortcut::getShortcut() const
{
    return _sequence;
}
