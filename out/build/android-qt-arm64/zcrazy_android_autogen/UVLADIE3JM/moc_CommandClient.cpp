/****************************************************************************
** Meta object code from reading C++ file 'CommandClient.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../src/CommandClient.h"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CommandClient.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.5.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {

#ifdef QT_MOC_HAS_STRINGDATA
struct qt_meta_stringdata_CLASSCommandClientENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSCommandClientENDCLASS = QtMocHelpers::stringData(
    "CommandClient",
    "ipPrefixChanged",
    "",
    "updateCommandParams",
    "robotID",
    "velX",
    "velY",
    "velR",
    "ctrl",
    "mode",
    "shoot",
    "power",
    "useImu",
    "angle",
    "controlAll",
    "controlAllWhichTeam",
    "sendCommand",
    "subscribeWithIp",
    "ip",
    "robotId",
    "setNeedChangeTeam",
    "v",
    "ipPrefix"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSCommandClientENDCLASS_t {
    uint offsetsAndSizes[46];
    char stringdata0[14];
    char stringdata1[16];
    char stringdata2[1];
    char stringdata3[20];
    char stringdata4[8];
    char stringdata5[5];
    char stringdata6[5];
    char stringdata7[5];
    char stringdata8[5];
    char stringdata9[5];
    char stringdata10[6];
    char stringdata11[6];
    char stringdata12[7];
    char stringdata13[6];
    char stringdata14[11];
    char stringdata15[20];
    char stringdata16[12];
    char stringdata17[16];
    char stringdata18[3];
    char stringdata19[8];
    char stringdata20[18];
    char stringdata21[2];
    char stringdata22[9];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSCommandClientENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSCommandClientENDCLASS_t qt_meta_stringdata_CLASSCommandClientENDCLASS = {
    {
        QT_MOC_LITERAL(0, 13),  // "CommandClient"
        QT_MOC_LITERAL(14, 15),  // "ipPrefixChanged"
        QT_MOC_LITERAL(30, 0),  // ""
        QT_MOC_LITERAL(31, 19),  // "updateCommandParams"
        QT_MOC_LITERAL(51, 7),  // "robotID"
        QT_MOC_LITERAL(59, 4),  // "velX"
        QT_MOC_LITERAL(64, 4),  // "velY"
        QT_MOC_LITERAL(69, 4),  // "velR"
        QT_MOC_LITERAL(74, 4),  // "ctrl"
        QT_MOC_LITERAL(79, 4),  // "mode"
        QT_MOC_LITERAL(84, 5),  // "shoot"
        QT_MOC_LITERAL(90, 5),  // "power"
        QT_MOC_LITERAL(96, 6),  // "useImu"
        QT_MOC_LITERAL(103, 5),  // "angle"
        QT_MOC_LITERAL(109, 10),  // "controlAll"
        QT_MOC_LITERAL(120, 19),  // "controlAllWhichTeam"
        QT_MOC_LITERAL(140, 11),  // "sendCommand"
        QT_MOC_LITERAL(152, 15),  // "subscribeWithIp"
        QT_MOC_LITERAL(168, 2),  // "ip"
        QT_MOC_LITERAL(171, 7),  // "robotId"
        QT_MOC_LITERAL(179, 17),  // "setNeedChangeTeam"
        QT_MOC_LITERAL(197, 1),  // "v"
        QT_MOC_LITERAL(199, 8)   // "ipPrefix"
    },
    "CommandClient",
    "ipPrefixChanged",
    "",
    "updateCommandParams",
    "robotID",
    "velX",
    "velY",
    "velR",
    "ctrl",
    "mode",
    "shoot",
    "power",
    "useImu",
    "angle",
    "controlAll",
    "controlAllWhichTeam",
    "sendCommand",
    "subscribeWithIp",
    "ip",
    "robotId",
    "setNeedChangeTeam",
    "v",
    "ipPrefix"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSCommandClientENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       1,   79, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   44,    2, 0x06,    2 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       3,   12,   45,    2, 0x02,    3 /* Public */,
      16,    0,   70,    2, 0x02,   16 /* Public */,
      17,    2,   71,    2, 0x02,   17 /* Public */,
      20,    1,   76,    2, 0x02,   20 /* Public */,

 // signals: parameters
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void, QMetaType::Int, QMetaType::Double, QMetaType::Double, QMetaType::Double, QMetaType::Double, QMetaType::Bool, QMetaType::Bool, QMetaType::Double, QMetaType::Bool, QMetaType::Double, QMetaType::Bool, QMetaType::Bool,    4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14,   15,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,   18,   19,
    QMetaType::Void, QMetaType::Bool,   21,

 // properties: name, type, flags
      22, QMetaType::QString, 0x00015103, uint(0), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject CommandClient::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSCommandClientENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSCommandClientENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSCommandClientENDCLASS_t,
        // property 'ipPrefix'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<CommandClient, std::true_type>,
        // method 'ipPrefixChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateCommandParams'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'sendCommand'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'subscribeWithIp'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'setNeedChangeTeam'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>
    >,
    nullptr
} };

void CommandClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CommandClient *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->ipPrefixChanged(); break;
        case 1: _t->updateCommandParams((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[5])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[6])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[7])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[8])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[9])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[10])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[11])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[12]))); break;
        case 2: _t->sendCommand(); break;
        case 3: _t->subscribeWithIp((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 4: _t->setNeedChangeTeam((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CommandClient::*)();
            if (_t _q_method = &CommandClient::ipPrefixChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<CommandClient *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->ipPrefix(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<CommandClient *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setIpPrefix(*reinterpret_cast< QString*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
}

const QMetaObject *CommandClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CommandClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSCommandClientENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int CommandClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void CommandClient::ipPrefixChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
