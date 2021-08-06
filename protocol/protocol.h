#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QByteArray>
#include <QList>


class Frame
{
public:
    Frame():Frame(0, 0){};
    Frame(quint8 type, const QByteArray &data = QByteArray()):Frame(0, type, data){};
    Frame(quint8 addr, quint8 type, const QByteArray &data = QByteArray());
    Frame(const QByteArray &raw);//原始有效数据（即addr+type+len+data+crc）
    ~Frame(){};

    quint16 length(){return ((((_data[5]) << 8)&0xff00) | (_data[6]&0xff));};
    quint16 size(){return length();};

    quint8 addr(){return ((_data[1]-'0')*10 + _data[2]-'0');};
    quint8 type(){return ((_data[3]-'0')*10 + _data[4]-'0');};

    QByteArray& getData(){return frame_data;};
    QByteArray& getFrame(){return _data;};

    //修改帧数据

    void setFrame(quint8 addr, quint8 type, const QByteArray &data, bool gencrc = false);
    void setFrame(const QByteArray &data, bool gencrc = false);
//    void setFrameAddr(quint8 addr, bool gencrc = false);
//    void setFrameType(quint8 type, bool gencrc = false);


    static quint16 genCrc(const QByteArray &data);

    static const quint8 frame_head = 0x02;//帧头
    static const quint8 frame_tail = 0x03;//帧尾
private:
    QByteArray frame_data;//数据（纯粹的数据，不包含任何东西）
    QByteArray _data;//帧数据（全部数据，包括帧头帧尾）


};





class Protocol
{
public:
    Protocol();

    bool isEmpty(){return frameList.isEmpty();};
    Frame getFrame(){return frameList.takeFirst();};//返回list第一个数据，并从list中删除 注意：如果没有会产生异常，为了避免该现象，应该先查看是否有

    bool process(quint8 ch);
    bool process(const QByteArray &data){
        bool res = false;
        for(quint8 ch : data)
        {
            if(process(ch))
                res = true;
        }
        return res;
    };


private:
    QList<Frame> frameList;
};

#endif // PROTOCOL_H
