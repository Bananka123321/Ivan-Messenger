/****************************************************************************
** Meta object code from reading C++ file 'Handler.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../App/Handler.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Handler.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_Handler_t {
    uint offsetsAndSizes[32];
    char stringdata0[8];
    char stringdata1[15];
    char stringdata2[1];
    char stringdata3[12];
    char stringdata4[6];
    char stringdata5[8];
    char stringdata6[14];
    char stringdata7[7];
    char stringdata8[18];
    char stringdata9[17];
    char stringdata10[10];
    char stringdata11[7];
    char stringdata12[5];
    char stringdata13[11];
    char stringdata14[25];
    char stringdata15[6];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_Handler_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_Handler_t qt_meta_stringdata_Handler = {
    {
        QT_MOC_LITERAL(0, 7),  // "Handler"
        QT_MOC_LITERAL(8, 14),  // "S_loginSuccess"
        QT_MOC_LITERAL(23, 0),  // ""
        QT_MOC_LITERAL(24, 11),  // "std::string"
        QT_MOC_LITERAL(36, 5),  // "login"
        QT_MOC_LITERAL(42, 7),  // "user_id"
        QT_MOC_LITERAL(50, 13),  // "S_loginFailed"
        QT_MOC_LITERAL(64, 6),  // "reason"
        QT_MOC_LITERAL(71, 17),  // "S_registerSuccess"
        QT_MOC_LITERAL(89, 16),  // "S_registerFailed"
        QT_MOC_LITERAL(106, 9),  // "S_Message"
        QT_MOC_LITERAL(116, 6),  // "sender"
        QT_MOC_LITERAL(123, 4),  // "text"
        QT_MOC_LITERAL(128, 10),  // "S_userList"
        QT_MOC_LITERAL(139, 24),  // "std::vector<std::string>"
        QT_MOC_LITERAL(164, 5)   // "users"
    },
    "Handler",
    "S_loginSuccess",
    "",
    "std::string",
    "login",
    "user_id",
    "S_loginFailed",
    "reason",
    "S_registerSuccess",
    "S_registerFailed",
    "S_Message",
    "sender",
    "text",
    "S_userList",
    "std::vector<std::string>",
    "users"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_Handler[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,   50,    2, 0x06,    1 /* Public */,
       6,    1,   55,    2, 0x06,    4 /* Public */,
       8,    2,   58,    2, 0x06,    6 /* Public */,
       9,    1,   63,    2, 0x06,    9 /* Public */,
      10,    2,   66,    2, 0x06,   11 /* Public */,
      13,    1,   71,    2, 0x06,   14 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, QMetaType::Int,    4,    5,
    QMetaType::Void, 0x80000000 | 3,    7,
    QMetaType::Void, 0x80000000 | 3, QMetaType::Int,    4,    5,
    QMetaType::Void, 0x80000000 | 3,    7,
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 3,   11,   12,
    QMetaType::Void, 0x80000000 | 14,   15,

       0        // eod
};

Q_CONSTINIT const QMetaObject Handler::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_Handler.offsetsAndSizes,
    qt_meta_data_Handler,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_Handler_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<Handler, std::true_type>,
        // method 'S_loginSuccess'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const std::string &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const int &, std::false_type>,
        // method 'S_loginFailed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const std::string &, std::false_type>,
        // method 'S_registerSuccess'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const std::string &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const int &, std::false_type>,
        // method 'S_registerFailed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const std::string &, std::false_type>,
        // method 'S_Message'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const std::string &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const std::string &, std::false_type>,
        // method 'S_userList'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const std::vector<std::string> &, std::false_type>
    >,
    nullptr
} };

void Handler::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Handler *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->S_loginSuccess((*reinterpret_cast< std::add_pointer_t<std::string>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 1: _t->S_loginFailed((*reinterpret_cast< std::add_pointer_t<std::string>>(_a[1]))); break;
        case 2: _t->S_registerSuccess((*reinterpret_cast< std::add_pointer_t<std::string>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 3: _t->S_registerFailed((*reinterpret_cast< std::add_pointer_t<std::string>>(_a[1]))); break;
        case 4: _t->S_Message((*reinterpret_cast< std::add_pointer_t<std::string>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<std::string>>(_a[2]))); break;
        case 5: _t->S_userList((*reinterpret_cast< std::add_pointer_t<std::vector<std::string>>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (Handler::*)(const std::string & , const int & );
            if (_t _q_method = &Handler::S_loginSuccess; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (Handler::*)(const std::string & );
            if (_t _q_method = &Handler::S_loginFailed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (Handler::*)(const std::string & , const int & );
            if (_t _q_method = &Handler::S_registerSuccess; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (Handler::*)(const std::string & );
            if (_t _q_method = &Handler::S_registerFailed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (Handler::*)(const std::string & , const std::string & );
            if (_t _q_method = &Handler::S_Message; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (Handler::*)(const std::vector<std::string> & );
            if (_t _q_method = &Handler::S_userList; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
    }
}

const QMetaObject *Handler::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Handler::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Handler.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Handler::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void Handler::S_loginSuccess(const std::string & _t1, const int & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Handler::S_loginFailed(const std::string & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void Handler::S_registerSuccess(const std::string & _t1, const int & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void Handler::S_registerFailed(const std::string & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void Handler::S_Message(const std::string & _t1, const std::string & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void Handler::S_userList(const std::vector<std::string> & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
