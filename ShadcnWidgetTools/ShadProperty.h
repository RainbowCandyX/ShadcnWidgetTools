#ifndef SHADPROPERTY_H
#define SHADPROPERTY_H

#include <QScopedPointer>
#include <QtCore/qglobal.h>

// 导出宏：静态库为空，编译动态库时导出，使用动态库时导入
#if defined(SHADCN_STATIC)
#define SHADCN_EXPORT
#elif defined(SHADCN_WIDGETTOOLS_LIBRARY)
#define SHADCN_EXPORT Q_DECL_EXPORT
#else
#define SHADCN_EXPORT Q_DECL_IMPORT
#endif

// 公有类声明 d 指针，配合 Q_D 使用；公有类的析构函数必须在 .cpp 中定义
#define SHAD_Q_CREATE(CLASS)                \
protected:                                  \
    QScopedPointer<CLASS##Private> d_ptr;   \
                                            \
private:                                    \
    Q_DISABLE_COPY(CLASS)                   \
    Q_DECLARE_PRIVATE(CLASS)

// 私有类声明 q 指针，配合 Q_Q 使用
#define SHAD_D_CREATE(CLASS)                \
public:                                     \
    CLASS* q_ptr{nullptr};                  \
                                            \
private:                                    \
    Q_DECLARE_PUBLIC(CLASS)

#endif // SHADPROPERTY_H
