#include "ShadLuaModules.h"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QList>
#include <QString>
#include <QUrl>
#include <QUrlQuery>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QXmlStreamAttribute>
#include <QXmlStreamReader>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <utility>

#include "ShadLuaEngine.h"
#include "ShadLuaEnginePrivate.h"
#include "ShadLuaHttp.h"
#include "ShadLuaInclude.h"
#include "ShadLuaMeta.h"
#include "ShadLuaStack.h"

namespace
{
    constexpr int kDefaultTimeoutMs = 30000;

    // C++ 局部对象已在 Body 中析构，可安全抛出 Lua 错误
    template <int (*Body)(lua_State*)>
    int entry(lua_State* L)
    {
        const int count = Body(L);
        return count < 0 ? ShadLuaMeta::raise(L) : count;
    }

    bool readBytes(lua_State* L, int index, QByteArray& bytes)
    {
        if (lua_type(L, index) != LUA_TSTRING)
        {
            return false;
        }
        size_t length = 0;
        const char* text = lua_tolstring(L, index, &length);
        bytes = QByteArray(text, qsizetype(length));
        return true;
    }

    int pushFailure(lua_State* L, const QString& message)
    {
        lua_pushnil(L);
        ShadLuaStack::pushString(L, message);
        return 2;
    }

    int jsonDecode(lua_State* L)
    {
        QByteArray text;
        if (!readBytes(L, 1, text))
        {
            return ShadLuaStack::error(L, QStringLiteral("json.decode 需要一个字符串"));
        }
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(text, &parseError);
        if (parseError.error != QJsonParseError::NoError)
        {
            return pushFailure(L, QStringLiteral("第 %1 个字节处解析失败：%2").arg(parseError.offset).arg(parseError.errorString()));
        }
        ShadLuaStack::pushVariant(L, document.toVariant());
        return 1;
    }

    QByteArray toJson(const QVariant& variant, QJsonDocument::JsonFormat format)
    {
        const QJsonValue value = QJsonValue::fromVariant(variant);
        QByteArray json;
        if (value.isObject())
        {
            json = QJsonDocument(value.toObject()).toJson(format);
        }
        else if (value.isArray())
        {
            json = QJsonDocument(value.toArray()).toJson(format);
        }
        else
        {
            // QJsonDocument 只能保存对象或数组，单个值需包进数组再去掉方括号
            json = QJsonDocument(QJsonArray{value}).toJson(QJsonDocument::Compact);
            json = json.mid(1, json.size() - 2);
        }
        return json.trimmed();
    }

    int jsonEncode(lua_State* L)
    {
        const QByteArray json = toJson(ShadLuaStack::getVariant(L, 1), lua_toboolean(L, 2) != 0 ? QJsonDocument::Indented : QJsonDocument::Compact);
        lua_pushlstring(L, json.constData(), size_t(json.size()));
        return 1;
    }

    struct XmlNode
    {
        QString name;
        QVariantMap attributes;
        QString text;
        QVariantList children;
    };

    QVariantMap toVariantMap(const XmlNode& node)
    {
        QVariantMap map;
        map.insert(QStringLiteral("name"), node.name);
        map.insert(QStringLiteral("attributes"), node.attributes);
        map.insert(QStringLiteral("text"), node.text);
        map.insert(QStringLiteral("children"), node.children);
        return map;
    }

    int xmlDecode(lua_State* L)
    {
        QByteArray text;
        if (!readBytes(L, 1, text))
        {
            return ShadLuaStack::error(L, QStringLiteral("xml.decode 需要一个字符串"));
        }
        QXmlStreamReader reader(text);
        QList<XmlNode> openNodes;
        QVariantMap root;
        while (!reader.atEnd())
        {
            const QXmlStreamReader::TokenType token = reader.readNext();
            if (token == QXmlStreamReader::StartElement)
            {
                XmlNode node;
                node.name = reader.qualifiedName().toString();
                const QXmlStreamAttributes attributes = reader.attributes();
                for (const QXmlStreamAttribute& attribute : attributes)
                {
                    node.attributes.insert(attribute.qualifiedName().toString(), attribute.value().toString());
                }
                openNodes.append(node);
            }
            else if (token == QXmlStreamReader::Characters && !reader.isWhitespace() && !openNodes.isEmpty())
            {
                openNodes.last().text += reader.text();
            }
            else if (token == QXmlStreamReader::EndElement && !openNodes.isEmpty())
            {
                const QVariantMap node = toVariantMap(openNodes.takeLast());
                if (openNodes.isEmpty())
                {
                    root = node;
                }
                else
                {
                    openNodes.last().children.append(node);
                }
            }
        }
        if (reader.hasError())
        {
            return pushFailure(L, QStringLiteral("第 %1 行第 %2 列解析失败：%3").arg(reader.lineNumber()).arg(reader.columnNumber()).arg(reader.errorString()));
        }
        if (root.isEmpty())
        {
            return pushFailure(L, QStringLiteral("没有找到根元素"));
        }
        ShadLuaStack::pushVariant(L, root);
        return 1;
    }

    // 只允许 http 与 https，避免通过其他协议读取本机文件
    bool readUrl(lua_State* L, int index, QUrl& url)
    {
        QString text;
        if (lua_type(L, index) != LUA_TSTRING || !ShadLuaStack::getString(L, index, text))
        {
            return false;
        }
        url = QUrl(text);
        return url.isValid() && (url.scheme() == QStringLiteral("http") || url.scheme() == QStringLiteral("https"));
    }

    int invalidUrlError(lua_State* L)
    {
        return ShadLuaStack::error(L, QStringLiteral("url 无效，只支持 http 与 https 地址"));
    }

    // 方法名只能由字母组成，避免把其他内容带进请求行；省略时为 GET
    bool readMethod(lua_State* L, int index, QByteArray& method)
    {
        if (lua_isnoneornil(L, index))
        {
            method = QByteArrayLiteral("GET");
            return true;
        }
        QString text;
        if (lua_type(L, index) != LUA_TSTRING || !ShadLuaStack::getString(L, index, text) || text.isEmpty())
        {
            return false;
        }
        const QByteArray upper = text.toUpper().toLatin1();
        for (const char character : upper)
        {
            if (character < 'A' || character > 'Z')
            {
                return false;
            }
        }
        method = upper;
        return true;
    }

    // 字符串原样发送，表按 JSON 发送，nil 表示没有正文
    bool readBody(lua_State* L, int index, QByteArray& body)
    {
        if (lua_isnoneornil(L, index))
        {
            return true;
        }
        if (lua_type(L, index) == LUA_TTABLE)
        {
            body = toJson(ShadLuaStack::getVariant(L, index), QJsonDocument::Compact);
            return true;
        }
        return readBytes(L, index, body);
    }

    // 读取 headers、query 与 timeout；index 必须是正的栈位置
    void readOptions(lua_State* L, int index, ShadLuaHttpRequest& request)
    {
        lua_getfield(L, index, "headers");
        const QVariantMap headers = ShadLuaStack::getVariant(L, -1).toMap();
        lua_pop(L, 1);
        for (auto it = headers.cbegin(); it != headers.cend(); ++it)
        {
            request.headers.append(std::make_pair(it.key().toUtf8(), it.value().toString().toUtf8()));
        }
        lua_getfield(L, index, "query");
        const QVariantMap query = ShadLuaStack::getVariant(L, -1).toMap();
        lua_pop(L, 1);
        if (!query.isEmpty())
        {
            QUrlQuery urlQuery(request.url);
            for (auto it = query.cbegin(); it != query.cend(); ++it)
            {
                urlQuery.addQueryItem(it.key(), it.value().toString());
            }
            request.url.setQuery(urlQuery);
        }
        qint64 timeout = 0;
        lua_getfield(L, index, "timeout");
        ShadLuaStack::getInteger(L, -1, timeout);
        lua_pop(L, 1);
        if (timeout > 0)
        {
            request.timeoutMs = int(timeout);
        }
    }

    // 有正文但没有指定 Content-Type 时按 JSON 发送
    void applyDefaultContentType(ShadLuaHttpRequest& request)
    {
        if (request.body.isEmpty())
        {
            return;
        }
        for (const std::pair<QByteArray, QByteArray>& header : std::as_const(request.headers))
        {
            if (header.first.compare("Content-Type", Qt::CaseInsensitive) == 0)
            {
                return;
            }
        }
        request.headers.append(std::make_pair(QByteArrayLiteral("Content-Type"), QByteArrayLiteral("application/json")));
    }

    int sendRequest(lua_State* L, ShadLuaHttpRequest request, int callbackIndex)
    {
        if (lua_type(L, callbackIndex) != LUA_TFUNCTION)
        {
            return ShadLuaStack::error(L, QStringLiteral("需要一个回调函数，请求结束后以响应为参数调用"));
        }
        ShadLuaEngine* engine = ShadLuaEngine::fromState(L);
        if (engine == nullptr)
        {
            return ShadLuaStack::error(L, QStringLiteral("当前 Lua 状态不属于 ShadLuaEngine，无法发起请求"));
        }
        applyDefaultContentType(request);
        lua_pushvalue(L, callbackIndex);
        ShadLuaEnginePrivate::get(engine)->http()->request(request, luaL_ref(L, LUA_REGISTRYINDEX));
        return 0;
    }

    // request(options, callback)：method 可以是任意 HTTP 方法
    int httpRequest(lua_State* L)
    {
        if (lua_type(L, 1) != LUA_TTABLE)
        {
            return ShadLuaStack::error(L, QStringLiteral("http.request 的第一个参数需要是表，例如 {url = \"https://example.com\"}"));
        }
        ShadLuaHttpRequest request;
        request.timeoutMs = kDefaultTimeoutMs;
        lua_getfield(L, 1, "url");
        const bool isUrlValid = readUrl(L, -1, request.url);
        lua_pop(L, 1);
        if (!isUrlValid)
        {
            return invalidUrlError(L);
        }
        lua_getfield(L, 1, "method");
        const bool isMethodValid = readMethod(L, -1, request.method);
        lua_pop(L, 1);
        if (!isMethodValid)
        {
            return ShadLuaStack::error(L, QStringLiteral("method 需要是只含字母的字符串，例如 \"PUT\""));
        }
        lua_getfield(L, 1, "body");
        const bool isBodyValid = readBody(L, -1, request.body);
        lua_pop(L, 1);
        if (!isBodyValid)
        {
            return ShadLuaStack::error(L, QStringLiteral("body 需要是字符串或表"));
        }
        readOptions(L, 1, request);
        return sendRequest(L, request, 2);
    }

    // 无正文：(url, callback) 或 (url, options, callback)；有正文：(url, body, callback) 或 (url, body, options, callback)
    int sendMethod(lua_State* L, const QByteArray& method, bool hasBody)
    {
        ShadLuaHttpRequest request;
        request.method = method;
        request.timeoutMs = kDefaultTimeoutMs;
        if (!readUrl(L, 1, request.url))
        {
            return invalidUrlError(L);
        }
        int index = 2;
        if (hasBody)
        {
            if (!readBody(L, index, request.body))
            {
                return ShadLuaStack::error(L, QStringLiteral("正文需要是字符串或表，没有正文时传 nil"));
            }
            ++index;
        }
        if (lua_type(L, index) == LUA_TTABLE)
        {
            readOptions(L, index, request);
            ++index;
        }
        return sendRequest(L, request, index);
    }

    int httpGet(lua_State* L)
    {
        return sendMethod(L, QByteArrayLiteral("GET"), false);
    }

    int httpHead(lua_State* L)
    {
        return sendMethod(L, QByteArrayLiteral("HEAD"), false);
    }

    int httpDelete(lua_State* L)
    {
        return sendMethod(L, QByteArrayLiteral("DELETE"), false);
    }

    int httpPost(lua_State* L)
    {
        return sendMethod(L, QByteArrayLiteral("POST"), true);
    }

    int httpPut(lua_State* L)
    {
        return sendMethod(L, QByteArrayLiteral("PUT"), true);
    }

    int httpPatch(lua_State* L)
    {
        return sendMethod(L, QByteArrayLiteral("PATCH"), true);
    }

    void addFunction(lua_State* L, const char* name, lua_CFunction function)
    {
        lua_pushcfunction(L, function);
        lua_setfield(L, -2, name);
    }
} // namespace

namespace ShadLuaModules
{
    void install(lua_State* L, bool isHttpEnabled)
    {
        // shad 带有 __newindex 元方法，用 rawset 直接写入
        lua_getglobal(L, "shad");
        lua_pushstring(L, "json");
        lua_createtable(L, 0, 2);
        addFunction(L, "decode", &entry<jsonDecode>);
        addFunction(L, "encode", &entry<jsonEncode>);
        lua_rawset(L, -3);

        lua_pushstring(L, "xml");
        lua_createtable(L, 0, 1);
        addFunction(L, "decode", &entry<xmlDecode>);
        lua_rawset(L, -3);

        if (isHttpEnabled)
        {
            lua_pushstring(L, "http");
            lua_createtable(L, 0, 7);
            addFunction(L, "request", &entry<httpRequest>);
            addFunction(L, "get", &entry<httpGet>);
            addFunction(L, "head", &entry<httpHead>);
            addFunction(L, "delete", &entry<httpDelete>);
            addFunction(L, "post", &entry<httpPost>);
            addFunction(L, "put", &entry<httpPut>);
            addFunction(L, "patch", &entry<httpPatch>);
            lua_rawset(L, -3);
        }
        lua_pop(L, 1);
    }
} // namespace ShadLuaModules
