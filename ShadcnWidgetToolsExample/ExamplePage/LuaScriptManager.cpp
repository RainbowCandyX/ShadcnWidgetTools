#include "LuaScriptManager.h"

#include <QColor>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QObject>
#include <QScopedValueRollback>
#include <QScrollArea>
#include <QScrollBar>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStringList>
#include <QTime>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QVariant>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <utility>

#include "ExampleI18n.h"
#include "ShadButton.h"
#include "ShadCodeBlock.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadListView.h"
#include "ShadLuaEngine.h"
#include "ShadSwitch.h"

namespace
{
    constexpr int kSectionGap = 12;
    constexpr int kRowGap = 8;
    constexpr int kListHeight = 300;
    constexpr QRgb kLoadedRgb = qRgb(0x22, 0xc5, 0x5e);
    constexpr int kLogHeight = 220;
    const QString kBuiltInDirectory = QStringLiteral(":/lua");
    // 脚本窗口上的动态属性，用于窗口关闭时找到脚本
    const char kScriptPathProperty[] = "_luaScriptPath";
    const char kScriptIdProperty[] = "_luaScriptId";
    const QString kAnsiDim = QStringLiteral("\x1b[2m");
    const QString kAnsiRed = QStringLiteral("\x1b[31m");
    const QString kAnsiGreen = QStringLiteral("\x1b[32m");
    const QString kAnsiBlue = QStringLiteral("\x1b[34m");
    const QString kAnsiReset = QStringLiteral("\x1b[0m");

    ShadButton* createButton(ExampleI18n::TextFunction text, QWidget* parent)
    {
        ShadButton* button = new ShadButton(QString(), ShadButtonType::Outline, parent);
        button->setButtonSize(ShadButtonType::Small);
        ExampleI18n::bind(button, [button, text]() { button->setText(text()); });
        return button;
    }
} // namespace

LuaScriptManager::LuaScriptManager(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(kSectionGap);

    _model = new QStandardItemModel(this);
    _list = new ShadListView(this);
    _list->setModel(_model);
    _list->setItemSize(ShadItemType::Small);
    _list->setFixedHeight(kListHeight);
    connect(_model, &QStandardItemModel::itemChanged, this, [this](QStandardItem* item)
    {
        // 只处理用户的勾选操作
        const int index = item->row();
        if (_isUpdatingItems || index < 0 || index >= _scripts.size())
        {
            return;
        }
        const bool isChecked = item->checkState() == Qt::Checked;
        const bool isLoaded = _scripts.at(index).scriptId != 0;
        if (isChecked && !isLoaded)
        {
            loadScript(index);
        }
        else if (!isChecked && isLoaded)
        {
            unloadScript(index);
        }
    });
    ExampleI18n::bind(_list, [this]()
    {
        for (int index = 0; index < _scripts.size(); ++index)
        {
            updateStatus(index);
        }
    });

    _directoryLabel = new ShadLabel(this);
    _directoryLabel->setColorRole(ShadThemeType::MutedForeground);
    _directoryLabel->setWordWrap(true);
    ExampleI18n::bind(_directoryLabel, [this]() { _directoryLabel->setText(tr("脚本目录：%1").arg(QDir::toNativeSeparators(scriptDirectory()))); });

    QWidget* toolbar = new QWidget(this);
    QHBoxLayout* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(kRowGap);
    _sharedSwitch = new ShadSwitch(toolbar);
    ExampleI18n::bind(_sharedSwitch, [this]() { _sharedSwitch->setText(tr("共享状态")); });
    ShadButton* refreshButton = createButton([] { return tr("刷新列表"); }, toolbar);
    ShadButton* reloadButton = createButton([] { return tr("重新加载"); }, toolbar);
    ShadButton* clearButton = createButton([] { return tr("清空日志"); }, toolbar);
    ShadButton* openButton = createButton([] { return tr("打开目录"); }, toolbar);
    toolbarLayout->addWidget(_sharedSwitch);
    toolbarLayout->addWidget(refreshButton);
    toolbarLayout->addWidget(reloadButton);
    toolbarLayout->addWidget(clearButton);
    toolbarLayout->addWidget(openButton);
    toolbarLayout->addStretch(1);

    _log = new ShadCodeBlock(this);
    _log->setLanguage(QStringLiteral("ansi"));
    _log->setWordWrap(true);
    _log->setMaximumContentHeight(kLogHeight);
    ExampleI18n::bind(_log, [this]() { _log->setTitle(tr("日志")); });

    layout->addWidget(_list);
    layout->addWidget(_directoryLabel);
    layout->addWidget(toolbar);
    layout->addWidget(_log);

    connect(refreshButton, &ShadButton::clicked, this, [this]() { refreshList(); });
    connect(reloadButton, &ShadButton::clicked, this, [this]() { reloadScripts(); });
    connect(clearButton, &ShadButton::clicked, this, [this]() { _log->setCode(QString()); });
    connect(openButton, &ShadButton::clicked, this, [this]()
    {
        // 先创建脚本目录再显示路径
        QDir().mkpath(scriptDirectory());
        _directoryLabel->show();
        QDesktopServices::openUrl(QUrl::fromLocalFile(scriptDirectory()));
    });

    connectEngine(ShadLuaEngine::shared());
    refreshList();
}

LuaScriptManager::~LuaScriptManager()
{
    // 共享引擎不随本组件销毁，需手动卸载
    for (const Script& script : std::as_const(_scripts))
    {
        if (script.scriptId != 0 && !script.engine.isNull())
        {
            script.engine->unload(script.scriptId);
        }
    }
}

bool LuaScriptManager::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::Close)
    {
        // 窗口被关闭，事件处理结束后再卸载脚本
        const QString filePath = watched->property(kScriptPathProperty).toString();
        const int scriptId = watched->property(kScriptIdProperty).toInt();
        QTimer::singleShot(0, this, [this, filePath, scriptId]()
        {
            for (int index = 0; index < _scripts.size(); ++index)
            {
                if (_scripts.at(index).filePath == filePath && _scripts.at(index).scriptId == scriptId && scriptId != 0)
                {
                    unloadScript(index);
                    setChecked(index, false);
                    return;
                }
            }
        });
    }
    return QWidget::eventFilter(watched, event);
}

QString LuaScriptManager::scriptDirectory()
{
    QDir directory(QCoreApplication::applicationDirPath());
#ifdef Q_OS_MACOS
    if (directory.absolutePath().endsWith(QStringLiteral(".app/Contents/MacOS")))
    {
        directory.cdUp();
        directory.cdUp();
        directory.cdUp();
    }
#endif
    return directory.filePath(QStringLiteral("lua"));
}

void LuaScriptManager::refreshList()
{
    unloadAll();
    _scripts.clear();
    _model->clear();
    // .ljbc 为 LuaJIT 字节码
    const QStringList filters = {QStringLiteral("*.lua"), QStringLiteral("*.ljbc")};
    const QDir builtInDirectory(kBuiltInDirectory);
    for (const QString& fileName : builtInDirectory.entryList(filters, QDir::Files, QDir::Name))
    {
        addScript(builtInDirectory.filePath(fileName), true);
    }
    const QDir localDirectory(scriptDirectory());
    _directoryLabel->setVisible(localDirectory.exists());
    if (localDirectory.exists())
    {
        for (const QString& fileName : localDirectory.entryList(filters, QDir::Files, QDir::Name))
        {
            addScript(localDirectory.filePath(fileName), false);
        }
    }
}

void LuaScriptManager::addScript(const QString& filePath, bool isBuiltIn)
{
    Script script;
    script.name = QFileInfo(filePath).fileName();
    script.filePath = filePath;
    script.isBuiltIn = isBuiltIn;

    // 追加一行不会触发 itemChanged
    script.item = new QStandardItem(script.name);
    script.item->setEditable(false);
    script.item->setCheckable(true);
    script.item->setCheckState(Qt::Unchecked);
    script.item->setData(static_cast<int>(ShadIconType::FileCode), ShadListView::IconRole);
    _model->appendRow(script.item);
    _scripts.append(script);
    updateStatus(int(_scripts.size()) - 1);
}

void LuaScriptManager::loadScript(int index)
{
    Script& script = _scripts[index];
    if (script.scriptId != 0)
    {
        return;
    }
    const bool isShared = _sharedSwitch->isChecked();
    ShadLuaEngine* engine = isShared ? ShadLuaEngine::shared() : new ShadLuaEngine(this);
    if (!isShared)
    {
        connectEngine(engine);
    }
    _loadingIndex = index;
    const int scriptId = engine->loadFile(script.filePath);
    _loadingIndex = -1;
    if (scriptId == 0)
    {
        if (!isShared)
        {
            engine->deleteLater();
        }
        script.isFailed = true;
        setChecked(index, false);
        updateStatus(index);
        return;
    }
    script.engine = engine;
    script.scriptId = scriptId;
    script.isShared = isShared;
    script.isFailed = false;
    const QObjectList objects = engine->getScriptObjects(scriptId);
    for (QObject* object : objects)
    {
        QWidget* window = qobject_cast<QWidget*>(object);
        if (window != nullptr && window->parentWidget() == nullptr)
        {
            window->setProperty(kScriptPathProperty, script.filePath);
            window->setProperty(kScriptIdProperty, scriptId);
            window->installEventFilter(this);
        }
    }
    // 失败的原因由 errorOccurred 写入日志
    updateStatus(index);
}

void LuaScriptManager::unloadScript(int index)
{
    Script& script = _scripts[index];
    if (script.scriptId == 0)
    {
        return;
    }
    if (!script.engine.isNull())
    {
        script.engine->unload(script.scriptId);
        if (!script.isShared)
        {
            script.engine->deleteLater();
        }
    }
    script.engine = nullptr;
    script.scriptId = 0;
    updateStatus(index);
    appendLog(script.name, tr("已卸载"), LogKind::Status);
}

void LuaScriptManager::unloadAll()
{
    for (int index = 0; index < _scripts.size(); ++index)
    {
        unloadScript(index);
        setChecked(index, false);
    }
}

void LuaScriptManager::reloadScripts()
{
    for (int index = 0; index < _scripts.size(); ++index)
    {
        if (_scripts.at(index).scriptId != 0)
        {
            unloadScript(index);
            loadScript(index);
        }
    }
}

void LuaScriptManager::setChecked(int index, bool checked)
{
    // 只同步勾选状态，不触发加载
    const QScopedValueRollback<bool> guard(_isUpdatingItems, true);
    _scripts.at(index).item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
}

void LuaScriptManager::updateStatus(int index)
{
    const QScopedValueRollback<bool> guard(_isUpdatingItems, true);
    const Script& script = _scripts.at(index);
    QString status = tr("未加载");
    QVariant statusColor;
    if (script.scriptId != 0)
    {
        status = script.isShared ? tr("已加载（共享）") : tr("已加载");
        statusColor = QColor(kLoadedRgb);
    }
    else if (script.isFailed)
    {
        status = tr("加载失败");
        statusColor = static_cast<int>(ShadThemeType::Destructive);
    }
    script.item->setData(script.isBuiltIn ? tr("内置") : tr("本地"), ShadListView::DescriptionRole);
    script.item->setData(status, ShadListView::TrailingTextRole);
    script.item->setData(statusColor, ShadListView::TrailingColorRole);
}

void LuaScriptManager::connectEngine(ShadLuaEngine* engine)
{
    connect(engine, &ShadLuaEngine::messagePrinted, this, [this, engine](const QString& message) { appendLog(sourceOf(engine), message, LogKind::Output); });
    connect(engine, &ShadLuaEngine::errorOccurred, this, [this, engine](const QString& message) { appendLog(sourceOf(engine), message, LogKind::Error); });
}

QString LuaScriptManager::sourceOf(ShadLuaEngine* engine) const
{
    if (_loadingIndex >= 0)
    {
        return _scripts.at(_loadingIndex).name;
    }
    const int currentScript = engine->getCurrentScript();
    for (const Script& script : _scripts)
    {
        if (script.scriptId != 0 && script.engine == engine && (!script.isShared || script.scriptId == currentScript))
        {
            return script.name;
        }
    }
    return tr("共享状态");
}

void LuaScriptManager::appendLog(const QString& source, const QString& message, LogKind kind)
{
    QString contentColor;
    if (kind == LogKind::Error)
    {
        contentColor = kAnsiRed;
    }
    else if (kind == LogKind::Status)
    {
        contentColor = kAnsiGreen;
    }
    // ANSI 颜色不延续到下一行，需逐行着色
    const QString time = QTime::currentTime().toString(QStringLiteral("HH:mm:ss"));
    const QString prefix = kAnsiDim + time + kAnsiReset + QStringLiteral(" ") + kAnsiBlue + QStringLiteral("[") + source + QStringLiteral("]") + kAnsiReset + QStringLiteral(" ");
    QString text;
    const QStringList lines = message.split(u'\n');
    for (const QString& line : lines)
    {
        text += prefix + contentColor + line + kAnsiReset + QStringLiteral("\n");
    }
    _log->appendCode(text);
    QTimer::singleShot(0, _log, [log = _log]()
    {
        const QScrollArea* scrollArea = log->findChild<QScrollArea*>();
        if (scrollArea != nullptr)
        {
            scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->maximum());
        }
    });
}
