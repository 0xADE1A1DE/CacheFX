/****************************************************************************
** Meta object code from reading C++ file 'ConfigWindowPresenter.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "ConfigWindowPresenter.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ConfigWindowPresenter.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ConfigWindowPresenter_t {
    QByteArrayData data[18];
    char stringdata0[276];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ConfigWindowPresenter_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ConfigWindowPresenter_t qt_meta_stringdata_ConfigWindowPresenter = {
    {
QT_MOC_LITERAL(0, 0, 21), // "ConfigWindowPresenter"
QT_MOC_LITERAL(1, 22, 15), // "upButtonClicked"
QT_MOC_LITERAL(2, 38, 0), // ""
QT_MOC_LITERAL(3, 39, 17), // "downButtonClicked"
QT_MOC_LITERAL(4, 57, 19), // "removeButtonClicked"
QT_MOC_LITERAL(5, 77, 16), // "newButtonClicked"
QT_MOC_LITERAL(6, 94, 25), // "cacheListSelectionChanged"
QT_MOC_LITERAL(7, 120, 11), // "QModelIndex"
QT_MOC_LITERAL(8, 132, 8), // "indexNew"
QT_MOC_LITERAL(9, 141, 8), // "indexOld"
QT_MOC_LITERAL(10, 150, 25), // "cacheTypeSelectionChanged"
QT_MOC_LITERAL(11, 176, 5), // "index"
QT_MOC_LITERAL(12, 182, 20), // "cachePropertyChanged"
QT_MOC_LITERAL(13, 203, 14), // "QStandardItem*"
QT_MOC_LITERAL(14, 218, 4), // "item"
QT_MOC_LITERAL(15, 223, 25), // "algorithmSelectionChanged"
QT_MOC_LITERAL(16, 249, 4), // "algo"
QT_MOC_LITERAL(17, 254, 21) // "saveConfigurationFile"

    },
    "ConfigWindowPresenter\0upButtonClicked\0"
    "\0downButtonClicked\0removeButtonClicked\0"
    "newButtonClicked\0cacheListSelectionChanged\0"
    "QModelIndex\0indexNew\0indexOld\0"
    "cacheTypeSelectionChanged\0index\0"
    "cachePropertyChanged\0QStandardItem*\0"
    "item\0algorithmSelectionChanged\0algo\0"
    "saveConfigurationFile"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ConfigWindowPresenter[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   59,    2, 0x0a /* Public */,
       3,    0,   60,    2, 0x0a /* Public */,
       4,    0,   61,    2, 0x0a /* Public */,
       5,    0,   62,    2, 0x0a /* Public */,
       6,    2,   63,    2, 0x0a /* Public */,
      10,    1,   68,    2, 0x0a /* Public */,
      12,    1,   71,    2, 0x0a /* Public */,
      15,    1,   74,    2, 0x0a /* Public */,
      17,    0,   77,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 7, 0x80000000 | 7,    8,    9,
    QMetaType::Void, QMetaType::Int,   11,
    QMetaType::Void, 0x80000000 | 13,   14,
    QMetaType::Void, QMetaType::QString,   16,
    QMetaType::Void,

       0        // eod
};

void ConfigWindowPresenter::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ConfigWindowPresenter *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->upButtonClicked(); break;
        case 1: _t->downButtonClicked(); break;
        case 2: _t->removeButtonClicked(); break;
        case 3: _t->newButtonClicked(); break;
        case 4: _t->cacheListSelectionChanged((*reinterpret_cast< QModelIndex(*)>(_a[1])),(*reinterpret_cast< QModelIndex(*)>(_a[2]))); break;
        case 5: _t->cacheTypeSelectionChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->cachePropertyChanged((*reinterpret_cast< QStandardItem*(*)>(_a[1]))); break;
        case 7: _t->algorithmSelectionChanged((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 8: _t->saveConfigurationFile(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ConfigWindowPresenter::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ConfigWindowPresenter.data,
    qt_meta_data_ConfigWindowPresenter,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ConfigWindowPresenter::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ConfigWindowPresenter::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ConfigWindowPresenter.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ConfigWindowPresenter::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 9;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
