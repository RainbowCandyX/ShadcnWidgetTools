#include "HomePage.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QGridLayout>
#include <QGuiApplication>
#include <QLocale>
#include <QScreen>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QSysInfo>
#include <QUrl>
#include <QWidget>
#include <QtGlobal>

#include <functional>

#ifndef QT_NO_OPENGL
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#endif

#include "ExampleI18n.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadItem.h"

#if defined(Q_OS_WIN)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(Q_OS_MACOS)
#include <sys/sysctl.h>
#include <sys/types.h>
#elif defined(Q_OS_UNIX)
#include <unistd.h>
#endif

namespace
{
    // 每行列数
    constexpr int kGridColumns = 2;
    constexpr int kGridSpacing = 12;

    const QString kRepositoryUrl = QStringLiteral("https://github.com/RainbowCandyX/ShadcnWidgetTools");

    QGridLayout* createGridLayout(QWidget* grid)
    {
        QGridLayout* layout = new QGridLayout(grid);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(kGridSpacing);
        for (int column = 0; column < kGridColumns; ++column)
        {
            layout->setColumnStretch(column, 1);
        }
        return layout;
    }

    void addToGrid(QGridLayout* layout, QWidget* widget)
    {
        const int index = layout->count();
        layout->addWidget(widget, index / kGridColumns, index % kGridColumns);
    }

    ShadItem* createFeatureItem(ShadIconType::IconName icon, ExampleI18n::TextFunction title, ExampleI18n::TextFunction description, QWidget* parent)
    {
        ShadItem* item = new ShadItem(parent);
        ExampleI18n::bind(item, [item, title, description]()
        {
            item->setTitle(title());
            item->setDescription(description());
        });
        item->setVariant(ShadItemType::Outline);
        item->setMediaIcon(icon);
        return item;
    }

    // 无法获取时返回 0
    qint64 physicalMemoryBytes()
    {
#if defined(Q_OS_WIN)
        MEMORYSTATUSEX status{};
        status.dwLength = sizeof(status);
        return ::GlobalMemoryStatusEx(&status) ? static_cast<qint64>(status.ullTotalPhys) : 0;
#elif defined(Q_OS_MACOS)
        quint64 bytes = 0;
        size_t size = sizeof(bytes);
        return ::sysctlbyname("hw.memsize", &bytes, &size, nullptr, 0) == 0 ? static_cast<qint64>(bytes) : 0;
#elif defined(Q_OS_UNIX)
        const long pages = ::sysconf(_SC_PHYS_PAGES);
        const long pageSize = ::sysconf(_SC_PAGE_SIZE);
        return pages > 0 && pageSize > 0 ? static_cast<qint64>(pages) * pageSize : 0;
#else
        return 0;
#endif
    }

    QString memoryText()
    {
        const qint64 bytes = physicalMemoryBytes();
        if (bytes <= 0)
        {
            return QCoreApplication::translate("HomePage", "未知");
        }
        return QLocale::c().formattedDataSize(bytes, 0, QLocale::DataSizeTraditionalFormat);
    }

    // 渲染器名称，无法获取时为空；创建 OpenGL 上下文较慢，只在构造时查询一次
    QString gpuRendererName()
    {
#ifndef QT_NO_OPENGL
        QOffscreenSurface surface;
        surface.create();
        QOpenGLContext context;
        if (!context.create() || !context.makeCurrent(&surface))
        {
            return QString();
        }
        const GLubyte* renderer = context.functions()->glGetString(GL_RENDERER);
        const QString name = renderer != nullptr ? QString::fromLatin1(reinterpret_cast<const char*>(renderer)) : QString();
        context.doneCurrent();
        return name;
#else
        return QString();
#endif
    }

    QString gpuText(const QString& rendererName)
    {
        return rendererName.isEmpty() ? QCoreApplication::translate("HomePage", "未知") : rendererName;
    }

    QString screenText()
    {
        const QScreen* screen = QGuiApplication::primaryScreen();
        if (screen == nullptr)
        {
            return QCoreApplication::translate("HomePage", "未知");
        }
        const QSize size = screen->size();
        return QCoreApplication::translate("HomePage", "%1 × %2，缩放 %3").arg(size.width()).arg(size.height()).arg(screen->devicePixelRatio());
    }
} // namespace

HomePage::HomePage(QWidget* parent)
    : ExampleBasePage([] { return tr("首页"); }, [] { return tr("shadcn/ui 风格的 Qt 6 Widgets 组件库。"); }, parent)
{
    ShadButton* repositoryButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    repositoryButton->setButtonSize(ShadButtonType::IconSmall);
    // Lucide 字体无品牌图标，Github 没有字形
    repositoryButton->setShadIcon(ShadIconType::ExternalLink);
    ExampleI18n::bind(repositoryButton, [repositoryButton]() { repositoryButton->setToolTip(tr("GitHub：%1").arg(kRepositoryUrl)); });
    connect(repositoryButton, &ShadButton::clicked, this, []() { QDesktopServices::openUrl(QUrl(kRepositoryUrl)); });
    addDescriptionWidget(repositoryButton);

    ShadCard* featureCard = addSection([] { return tr("特性"); }, [] { return tr("对齐 shadcn/ui v4"); });
    QWidget* featureGrid = new QWidget(this);
    QGridLayout* featureLayout = createGridLayout(featureGrid);
    addToGrid(featureLayout, createFeatureItem(ShadIconType::Palette, [] { return tr("主题"); }, [] { return tr("五种基色，亮暗模式即时切换"); }, featureGrid));
    addToGrid(featureLayout, createFeatureItem(ShadIconType::Blocks, [] { return tr("图标"); }, [] { return tr("内置 Lucide 图标字体"); }, featureGrid));
    addToGrid(featureLayout, createFeatureItem(ShadIconType::Languages, [] { return QStringLiteral("RTL"); }, [] { return tr("跟随 layoutDirection 整体镜像"); }, featureGrid));
    addToGrid(featureLayout, createFeatureItem(ShadIconType::AppWindow, [] { return tr("自绘窗口"); }, [] { return tr("ShadWindow 与 ShadDialog 自绘标题栏"); }, featureGrid));
    addStretchItem(featureCard, featureGrid);

    ShadCard* systemCard = addSection([] { return tr("系统信息"); }, [] { return tr("当前运行环境"); });
    QWidget* systemGrid = new QWidget(this);
    QGridLayout* systemLayout = createGridLayout(systemGrid);
    const QString rendererName = gpuRendererName();
    addToGrid(systemLayout, createFeatureItem(ShadIconType::Monitor, [] { return tr("操作系统"); }, [] { return QSysInfo::prettyProductName(); }, systemGrid));
    addToGrid(systemLayout, createFeatureItem(ShadIconType::Terminal, [] { return tr("内核"); }, [] { return QStringLiteral("%1 %2").arg(QSysInfo::kernelType(), QSysInfo::kernelVersion()); }, systemGrid));
    addToGrid(systemLayout, createFeatureItem(ShadIconType::Cpu, [] { return tr("处理器架构"); }, [] { return QSysInfo::currentCpuArchitecture(); }, systemGrid));
    addToGrid(systemLayout, createFeatureItem(ShadIconType::MemoryStick, [] { return tr("内存"); }, [] { return memoryText(); }, systemGrid));
    addToGrid(systemLayout, createFeatureItem(ShadIconType::Gpu, [] { return QStringLiteral("GPU"); }, [rendererName] { return gpuText(rendererName); }, systemGrid));
    addToGrid(systemLayout, createFeatureItem(ShadIconType::Layers, [] { return tr("Qt 版本"); }, [] { return QString::fromLatin1(qVersion()); }, systemGrid));
    addToGrid(systemLayout, createFeatureItem(ShadIconType::Laptop, [] { return tr("主屏幕"); }, [] { return screenText(); }, systemGrid));
    addToGrid(systemLayout, createFeatureItem(ShadIconType::Globe, [] { return tr("系统语言"); }, [] { return QLocale::system().name(); }, systemGrid));
    addStretchItem(systemCard, systemGrid);

    ShadCard* categoryCard = addSection([] { return tr("组件"); }, [] { return tr("点击打开第一个演示页"); });
    _categoryGrid = new QWidget(this);
    _categoryLayout = createGridLayout(_categoryGrid);
    addStretchItem(categoryCard, _categoryGrid);
}

HomePage::~HomePage()
{
}

ShadItem* HomePage::addCategoryItem(ShadIconType::IconName icon, ExampleI18n::TextFunction name, std::function<QStringList()> pageTitles)
{
    ShadItem* item = new ShadItem(_categoryGrid);
    ExampleI18n::bind(item, [item, name, pageTitles]()
    {
        const QStringList titles = pageTitles();
        item->setTitle(name());
        item->setDescription(titles.join(tr("、", "列表分隔符")));
        item->setToolTip(tr("共 %1 个演示页").arg(titles.size()));
    });
    item->setVariant(ShadItemType::Outline);
    item->setMediaIcon(icon);
    item->setClickable(true);
    addToGrid(_categoryLayout, item);
    return item;
}
