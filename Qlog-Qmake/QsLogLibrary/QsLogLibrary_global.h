// QsLogLibrary_global.h - 这是你需要新建的一个头文件，内容是正确的
#ifndef QSLOGLIBRARY_GLOBAL_H
#define QSLOGLIBRARY_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QSLOGLIBRARY_LIBRARY)
#  define QSLOGLIBRARY_EXPORT Q_DECL_EXPORT
#else
#  define QSLOGLIBRARY_EXPORT Q_DECL_IMPORT
#endif

#endif // QSLOGLIBRARY_GLOBAL_H
