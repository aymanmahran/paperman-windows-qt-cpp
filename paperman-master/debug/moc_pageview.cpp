/****************************************************************************
** Meta object code from reading C++ file 'pageview.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../pageview.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'pageview.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_Pageview_t {
    QByteArrayData data[12];
    char stringdata0[127];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Pageview_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Pageview_t qt_meta_stringdata_Pageview = {
    {
QT_MOC_LITERAL(0, 0, 8), // "Pageview"
QT_MOC_LITERAL(1, 9, 14), // "signalNewScale"
QT_MOC_LITERAL(2, 24, 0), // ""
QT_MOC_LITERAL(3, 25, 9), // "new_scale"
QT_MOC_LITERAL(4, 35, 8), // "showInfo"
QT_MOC_LITERAL(5, 44, 11), // "QModelIndex"
QT_MOC_LITERAL(6, 56, 5), // "index"
QT_MOC_LITERAL(7, 62, 19), // "slotPagePartChanged"
QT_MOC_LITERAL(8, 82, 5), // "image"
QT_MOC_LITERAL(9, 88, 14), // "scaled_linenum"
QT_MOC_LITERAL(10, 103, 14), // "currentChanged"
QT_MOC_LITERAL(11, 118, 8) // "previous"

    },
    "Pageview\0signalNewScale\0\0new_scale\0"
    "showInfo\0QModelIndex\0index\0"
    "slotPagePartChanged\0image\0scaled_linenum\0"
    "currentChanged\0previous"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Pageview[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   34,    2, 0x06 /* Public */,
       4,    1,   37,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       7,    3,   40,    2, 0x0a /* Public */,
      10,    2,   47,    2, 0x09 /* Protected */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, 0x80000000 | 5,    6,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 5, QMetaType::QImage, QMetaType::Int,    6,    8,    9,
    QMetaType::Void, 0x80000000 | 5, 0x80000000 | 5,    6,   11,

       0        // eod
};

void Pageview::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Pageview *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->signalNewScale((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->showInfo((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 2: _t->slotPagePartChanged((*reinterpret_cast< const QModelIndex(*)>(_a[1])),(*reinterpret_cast< const QImage(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 3: _t->currentChanged((*reinterpret_cast< const QModelIndex(*)>(_a[1])),(*reinterpret_cast< const QModelIndex(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (Pageview::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Pageview::signalNewScale)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (Pageview::*)(const QModelIndex & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Pageview::showInfo)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject Pageview::staticMetaObject = { {
    QMetaObject::SuperData::link<QListView::staticMetaObject>(),
    qt_meta_stringdata_Pageview.data,
    qt_meta_data_Pageview,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *Pageview::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Pageview::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Pageview.stringdata0))
        return static_cast<void*>(this);
    return QListView::qt_metacast(_clname);
}

int Pageview::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QListView::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void Pageview::signalNewScale(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Pageview::showInfo(const QModelIndex & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
