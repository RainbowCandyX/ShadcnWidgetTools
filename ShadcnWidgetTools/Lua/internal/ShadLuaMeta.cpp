#include "ShadLuaMeta.h"

#include <QByteArray>
#include <QList>
#include <QMetaEnum>
#include <QMetaMethod>
#include <QMetaObject>
#include <QMetaProperty>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QtCore/qnamespace.h>

#include <stdexcept>
#include <string>

#include "ShadDef.h"
#include "ShadLuaBinder.h"
#include "ShadLuaEngine.h"
#include "ShadLuaInclude.h"
#include "ShadLuaObject.h"
#include "ShadLuaStack.h"
#include "ShadTheme.h"

namespace
{
    // 与 Qt 元对象调用的参数个数上限一致
    constexpr int kMaxInvokeArguments = 10;

    bool isInvokable(const QMetaMethod& method)
    {
        return method.access() == QMetaMethod::Public && (method.methodType() == QMetaMethod::Slot || method.methodType() == QMetaMethod::Method);
    }

    bool hasInvokable(const QMetaObject* metaObject, const QByteArray& name)
    {
        for (int index = metaObject->methodCount() - 1; index >= 0; --index)
        {
            const QMetaMethod method = metaObject->method(index);
            if (method.name() == name && isInvokable(method))
            {
                return true;
            }
        }
        return false;
    }

    // 没有对应属性时返回 -1
    int accessorPropertyIndex(const QMetaObject* metaObject, const QByteArray& name, int prefixLength)
    {
        if (name.size() <= prefixLength)
        {
            return -1;
        }
        const QByteArray propertyName = name.mid(prefixLength, 1).toLower() + name.mid(prefixLength + 1);
        return metaObject->indexOfProperty(propertyName.constData());
    }

    QByteArray upvalueName(lua_State* L)
    {
        size_t length = 0;
        const char* text = lua_tolstring(L, lua_upvalueindex(1), &length);
        return QByteArray(text, qsizetype(length));
    }

    QObject* selfObject(lua_State* L)
    {
        return ShadLuaObject::get(L, 1);
    }

    int invalidObjectError(lua_State* L)
    {
        return ShadLuaStack::error(L, QStringLiteral("对象无效或已销毁；方法需要用冒号调用，例如 object:method()"));
    }

    int invokeMethod(lua_State* L, QObject* object, const QMetaMethod& method)
    {
        const int count = method.parameterCount();
        if (count > kMaxInvokeArguments)
        {
            return ShadLuaStack::error(L, QStringLiteral("方法 %1 的参数超过 %2 个").arg(QString::fromUtf8(method.name())).arg(kMaxInvokeArguments));
        }
        QVariant returnValue;
        const QMetaType returnType = method.returnMetaType();
        void* arguments[kMaxInvokeArguments + 1] = {nullptr};
        if (returnType.id() == QMetaType::QVariant)
        {
            arguments[0] = &returnValue;
        }
        else if (returnType.isValid() && returnType.id() != QMetaType::Void)
        {
            returnValue = QVariant(returnType, nullptr);
            arguments[0] = returnValue.data();
        }
        QList<QVariant> values(count);
        for (int index = 0; index < count; ++index)
        {
            const QMetaType type = method.parameterMetaType(index);
            if (!type.isValid())
            {
                return ShadLuaStack::error(L, QStringLiteral("方法 %1 的第 %2 个参数的类型没有注册到元对象系统").arg(QString::fromUtf8(method.name())).arg(index + 1));
            }
            QVariant value = ShadLuaStack::getVariant(L, index + 2, type);
            if (type.id() == QMetaType::QVariant)
            {
                values[index] = value;
                arguments[index + 1] = &values[index];
                continue;
            }
            if (!value.isValid() && type.flags().testFlag(QMetaType::PointerToQObject))
            {
                value = QVariant(type, nullptr);
            }
            if (value.metaType() != type && !value.convert(type))
            {
                return ShadLuaStack::argumentError(L, index + 1);
            }
            values[index] = value;
            arguments[index + 1] = values[index].data();
        }
        object->qt_metacall(QMetaObject::InvokeMetaMethod, method.methodIndex(), arguments);
        if (!returnValue.isValid())
        {
            return 0;
        }
        ShadLuaStack::pushVariant(L, returnValue);
        return 1;
    }

    int callInvokable(lua_State* L)
    {
        QObject* object = selfObject(L);
        if (object == nullptr)
        {
            return invalidObjectError(L);
        }
        const QByteArray name = upvalueName(L);
        const int argumentCount = lua_gettop(L) - 1;
        const QMetaObject* metaObject = object->metaObject();
        for (int index = metaObject->methodCount() - 1; index >= 0; --index)
        {
            const QMetaMethod method = metaObject->method(index);
            if (method.name() == name && method.parameterCount() == argumentCount && isInvokable(method))
            {
                return invokeMethod(L, object, method);
            }
        }
        return ShadLuaStack::error(L, QStringLiteral("方法 %1 不接受 %2 个参数").arg(QString::fromUtf8(name)).arg(argumentCount));
    }

    int invokableEntry(lua_State* L)
    {
        const int count = callInvokable(L);
        return count < 0 ? ShadLuaMeta::raise(L) : count;
    }

    int callPropertySetter(lua_State* L)
    {
        QObject* object = selfObject(L);
        if (object == nullptr)
        {
            return invalidObjectError(L);
        }
        const QByteArray name = upvalueName(L);
        const QMetaObject* metaObject = object->metaObject();
        const QMetaProperty property = metaObject->property(metaObject->indexOfProperty(name.constData()));
        if (!property.isValid() || !property.isWritable() || !property.write(object, ShadLuaStack::getVariant(L, 2, property.metaType())))
        {
            return ShadLuaStack::error(L, QStringLiteral("无法设置属性 %1，属性只读或值的类型不匹配").arg(QString::fromUtf8(name)));
        }
        return 0;
    }

    int propertySetterEntry(lua_State* L)
    {
        const int count = callPropertySetter(L);
        return count < 0 ? ShadLuaMeta::raise(L) : count;
    }

    int callPropertyGetter(lua_State* L)
    {
        QObject* object = selfObject(L);
        if (object == nullptr)
        {
            return invalidObjectError(L);
        }
        const QByteArray name = upvalueName(L);
        const QMetaObject* metaObject = object->metaObject();
        const QMetaProperty property = metaObject->property(metaObject->indexOfProperty(name.constData()));
        if (!property.isValid())
        {
            return ShadLuaStack::error(L, QStringLiteral("对象没有属性 %1").arg(QString::fromUtf8(name)));
        }
        ShadLuaStack::pushVariant(L, property.read(object));
        return 1;
    }

    int propertyGetterEntry(lua_State* L)
    {
        const int count = callPropertyGetter(L);
        return count < 0 ? ShadLuaMeta::raise(L) : count;
    }

    // 句柄失效后仍可调用
    int isValidEntry(lua_State* L)
    {
        lua_pushboolean(L, ShadLuaObject::get(L, 1) != nullptr ? 1 : 0);
        return 1;
    }

    luabridge::LuaRef namedClosure(lua_State* L, const QByteArray& name, lua_CFunction function)
    {
        lua_pushlstring(L, name.constData(), size_t(name.size()));
        lua_pushcclosure(L, function, 1);
        return luabridge::LuaRef::fromStack(L);
    }

    // 依次查找显式绑定的方法、属性与槽，都没有时返回 nil
    luabridge::LuaRef indexObject(ShadLuaObject& self, const luabridge::LuaRef& key, lua_State* L)
    {
        if (!key.isString())
        {
            return luabridge::LuaRef(L);
        }
        const std::string keyText = key.unsafe_cast<std::string>();
        const QByteArray name(keyText.data(), qsizetype(keyText.size()));
        if (name == "isValid")
        {
            lua_pushcfunction(L, isValidEntry);
            return luabridge::LuaRef::fromStack(L);
        }
        QObject* object = self.object();
        if (object == nullptr)
        {
            return luabridge::LuaRef(L);
        }
        const QMetaObject* metaObject = object->metaObject();
        const ShadLuaMethod* method = ShadLuaRegistry::findMethod(metaObject, name);
        if (method != nullptr)
        {
            ShadLuaRegistry::pushMethod(L, method);
            return luabridge::LuaRef::fromStack(L);
        }
        const int propertyIndex = metaObject->indexOfProperty(name.constData());
        if (propertyIndex >= 0)
        {
            ShadLuaStack::pushVariant(L, metaObject->property(propertyIndex).read(object));
            return luabridge::LuaRef::fromStack(L);
        }
        if (hasInvokable(metaObject, name))
        {
            return namedClosure(L, name, invokableEntry);
        }
        if (name.startsWith("set"))
        {
            const int index = accessorPropertyIndex(metaObject, name, 3);
            if (index >= 0)
            {
                return namedClosure(L, QByteArray(metaObject->property(index).name()), propertySetterEntry);
            }
        }
        if (name.startsWith("get") || name.startsWith("is"))
        {
            const int index = accessorPropertyIndex(metaObject, name, name.startsWith("get") ? 3 : 2);
            if (index >= 0)
            {
                return namedClosure(L, QByteArray(metaObject->property(index).name()), propertyGetterEntry);
            }
        }
        return luabridge::LuaRef(L);
    }

    // 失败时抛出异常，由 LuaBridge 转换为 Lua 错误
    luabridge::LuaRef newIndexObject(ShadLuaObject& self, const luabridge::LuaRef& key, const luabridge::LuaRef& value, lua_State* L)
    {
        QObject* object = self.object();
        if (object == nullptr)
        {
            throw std::runtime_error("对象已销毁，无法设置属性");
        }
        const std::string keyText = key.isString() ? key.unsafe_cast<std::string>() : std::string();
        const QMetaObject* metaObject = object->metaObject();
        const int propertyIndex = metaObject->indexOfProperty(keyText.c_str());
        if (propertyIndex < 0)
        {
            throw std::runtime_error("对象没有属性 " + keyText);
        }
        const QMetaProperty property = metaObject->property(propertyIndex);
        value.push(L);
        const QVariant variant = ShadLuaStack::getVariant(L, -1, property.metaType());
        lua_pop(L, 1);
        if (!property.isWritable() || !property.write(object, variant))
        {
            throw std::runtime_error("无法设置属性 " + keyText + "，属性只读或值的类型不匹配");
        }
        return luabridge::LuaRef(L);
    }

    void installEnums(lua_State* L, const QMetaObject& metaObject)
    {
        QByteArray name(metaObject.className());
        if (name.startsWith("Shad"))
        {
            name.remove(0, 4);
        }
        lua_pushstring(L, name.constData());
        lua_createtable(L, 0, 0);
        for (int enumIndex = 0; enumIndex < metaObject.enumeratorCount(); ++enumIndex)
        {
            const QMetaEnum metaEnum = metaObject.enumerator(enumIndex);
            for (int keyIndex = 0; keyIndex < metaEnum.keyCount(); ++keyIndex)
            {
                lua_pushinteger(L, lua_Integer(metaEnum.value(keyIndex)));
                lua_setfield(L, -2, metaEnum.key(keyIndex));
            }
        }
        lua_rawset(L, -3);
    }
} // namespace

namespace ShadLuaMeta
{
    void install(lua_State* L)
    {
        luabridge::getGlobalNamespace(L).beginNamespace("shad").beginClass<ShadLuaObject>("Object").addIndexMetaMethod(&indexObject).addNewIndexMetaMethod(&newIndexObject).endClass().endNamespace();

        // shad 带有 __newindex 元方法，用 rawset 直接写入
        lua_getglobal(L, "shad");
        const QList<const QMetaObject*> enumNamespaces = {&Qt::staticMetaObject, &ShadThemeType::staticMetaObject, &ShadButtonType::staticMetaObject, &ShadBadgeType::staticMetaObject, &ShadToggleType::staticMetaObject, &ShadToggleGroupType::staticMetaObject, &ShadInputType::staticMetaObject, &ShadToastType::staticMetaObject, &ShadNotificationType::staticMetaObject, &ShadRadialProgressType::staticMetaObject, &ShadStatisticType::staticMetaObject, &ShadSegmentedType::staticMetaObject, &ShadImageCropperType::staticMetaObject, &ShadMarkerType::staticMetaObject, &ShadAlertType::staticMetaObject, &ShadPlacementType::staticMetaObject, &ShadInputGroupType::staticMetaObject, &ShadAttachmentType::staticMetaObject, &ShadCodeBlockType::staticMetaObject, &ShadTraceType::staticMetaObject, &ShadTabsType::staticMetaObject, &ShadTabViewType::staticMetaObject, &ShadDockType::staticMetaObject, &ShadDrawerType::staticMetaObject, &ShadQuestionnaireType::staticMetaObject, &ShadAccordionType::staticMetaObject, &ShadComboBoxType::staticMetaObject, &ShadWindowType::staticMetaObject, &ShadQrCodeType::staticMetaObject, &ShadCalendarType::staticMetaObject, &ShadDatePickerType::staticMetaObject, &ShadSidebarType::staticMetaObject, &ShadSidebarMenuButtonType::staticMetaObject, &ShadCarouselType::staticMetaObject, &ShadAvatarType::staticMetaObject, &ShadSpinBoxType::staticMetaObject, &ShadFieldType::staticMetaObject, &ShadInputDialogType::staticMetaObject, &ShadAlertDialogType::staticMetaObject, &ShadItemType::staticMetaObject, &ShadColorPickerType::staticMetaObject, &ShadTableType::staticMetaObject, &ShadChartType::staticMetaObject, &ShadEmptyType::staticMetaObject, &ShadTreeSelectType::staticMetaObject, &ShadDescriptionsType::staticMetaObject, &ShadToolbarType::staticMetaObject, &ShadPasskeyType::staticMetaObject, &ShadBiometricType::staticMetaObject, &ShadDiffViewType::staticMetaObject, &ShadTimelineType::staticMetaObject, &ShadAutoCompleteType::staticMetaObject, &ShadTransferType::staticMetaObject, &ShadStepsType::staticMetaObject};
        for (const QMetaObject* metaObject : enumNamespaces)
        {
            installEnums(L, *metaObject);
        }
        ShadLuaRegistry::installConstructors(L);
        lua_pushstring(L, "theme");
        ShadLuaObject::push(L, ShadTheme::getInstance());
        lua_rawset(L, -3);
        lua_pushstring(L, "runtime");
        ShadLuaStack::pushString(L, ShadLuaEngine::runtimeName());
        lua_rawset(L, -3);
        lua_pop(L, 1);
    }

    int raise(lua_State* L)
    {
        luaL_where(L, 1);
        lua_insert(L, -2);
        lua_concat(L, 2);
        return lua_error(L);
    }
} // namespace ShadLuaMeta
