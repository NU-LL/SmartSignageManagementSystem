#ifndef SIGNOBJECT_H
#define SIGNOBJECT_H

#include <QObject>


//接口类
class SignObject
{
public:
    SignObject(){};
    virtual ~SignObject(){};

    //序列化
    virtual const QString serialization() const = 0;
    //反序列化
    virtual void deserialization(const QString& str) = 0;

    //初始化列表
    //需要一开始就调用，获得全局唯一的表
    virtual void Init();
    //保存表
    virtual bool save();
};

#endif // SIGNOBJECT_H
