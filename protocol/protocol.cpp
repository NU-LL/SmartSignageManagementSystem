#include "protocol/protocol.h"
#include "config.h"

#include <qdebug.h>

Frame::Frame(quint8 addr, quint8 type, const QByteArray &data)
{
    if(addr > 99 || type > 99)
        throw "帧地址或帧类型范围错误";

    frame_data = data;

    _data += frame_head;//帧头
    _data += QString("%1").arg(addr, 2, 10, QLatin1Char('0')).toLocal8Bit();//地址
    _data += QString("%1").arg(type, 2, 10, QLatin1Char('0')).toLocal8Bit();//类型
    _data += (data.length() >> 8) & 0xff;//长度
    _data += data.length() & 0xff;
    _data += data;//数据
    quint16 crc = genCrc(_data.right(_data.size()-1));//不需要计算帧头
    _data += (crc >> 8) & 0xff;//校验位
    _data += crc & 0xff;
    _data += frame_tail;//帧尾
}

//原始有效数据（即addr+type+len+data+crc）
Frame::Frame(const QByteArray &raw)
{
    for(quint8 idx = 6; idx < raw.length()-2; idx++)
        frame_data += raw[idx];
    _data += frame_head;//帧头
    _data += raw;
    _data += frame_tail;//帧尾

    if(addr() > 99 || type() > 99)
        throw "帧地址或帧类型范围错误";
}

//修改帧数据
void Frame::setFrame(quint8 addr, quint8 type, const QByteArray &data, bool gencrc)
{
    if(addr > 99 || type > 99)
        throw "修改时，帧地址或帧类型范围错误";

    QByteArray temp;
    temp += QString("%1").arg(addr, 2, 10, QLatin1Char('0')).toLocal8Bit();//地址
    temp += QString("%1").arg(type, 2, 10, QLatin1Char('0')).toLocal8Bit();//类型
    temp += (data.length() >> 8) & 0xff;//长度
    temp += data.length() & 0xff;
    temp += data;//数据

    if(gencrc)
    {
//        temp.push_front(Frame::frame_head);//加上帧头 计算crc
        quint16 crc = genCrc(temp);
//        temp.remove(0, 1);//移除帧头
        temp += (crc >> 8) & 0xff;//校验位
        temp += crc & 0xff;
        //_data替换起始偏移 替换长度 替换内容
        _data.replace(1, 2+2+2+length()+2, temp);
    }else
    {
        //_data替换起始偏移 替换长度 替换内容
        _data.replace(1, 2+2+2+length(), temp);
    }
}

//只修改数据
void Frame::setFrame(const QByteArray &data, bool gencrc)
{
    QByteArray temp;
    temp += (data.length() >> 8) & 0xff;//长度
    temp += data.length() & 0xff;
    temp += data;//数据

    if(gencrc)
    {
        temp += _data[1];temp += _data[2];//地址
        temp += _data[3];temp += _data[4];//类型
//        temp.push_front(Frame::frame_head);//加上帧头 计算crc
        quint16 crc = genCrc(temp);
//        temp.remove(0, 1);//移除帧头
        temp += (crc >> 8) & 0xff;//校验位
        temp += crc & 0xff;
        //_data替换起始偏移 替换长度 替换内容
        _data.replace(5, 2+length()+2, temp.left(temp.length()-4));
    }else
    {
        //_data替换起始偏移 替换长度 替换内容
        _data.replace(5, 2+length(), temp);
    }
}


//head+addr+type+len+data
quint16 Frame::genCrc(const QByteArray &data)
{
    quint16 wcrc = 0;
    for(quint8 ch : data)
    {
        for(quint8 pos = 0; pos < 8; pos++)
        {
            quint8 treat = ch & 0x80;
            ch <<= 1;
            quint8 bcrc = (wcrc >> 8) & 0x80;
            wcrc <<= 1;
            if (treat != bcrc)
                wcrc ^= 0x1021;
        }
    }
    return wcrc;
}


Protocol::Protocol()
{

}


typedef enum
{
    PROTOCOL_START = 0,
    PROTOCOL_ADDR,
    PROTOCOL_TYPE,
    PROTOCOL_LEN,
    PROTOCOL_DATA,
    PROTOCOL_CRC,
    PROTOCOL_END,
    PROTOCOL_ERROR
}protocol_status;

//协议解析
bool Protocol::process(quint8 ch)
{
    static protocol_status status = PROTOCOL_START;

//    static quint8 addr = 0;
//    static quint8 type = 0;
//    static quint16 len = 0;
//    static QByteArray data;//帧数据
//    static quint16 crc = 0;
    static QByteArray raw;//原始有效数据（即addr+type+len+data+crc）

    static quint8 times = 0;

    bool res = false;

    switch(static_cast<quint8>(status))
    {
        case(PROTOCOL_ERROR):
        case(PROTOCOL_START):
        {
//            addr = 0;
//            type = 0;
//            len = 0;
//            data.clear();
//            crc = 0;
            raw.clear();

            times = 0;

//            if(lastFrame == nullptr)
//                lastFrame = new Frame();

            if(ch == Frame::frame_head)
                status = PROTOCOL_ADDR;
            break;
        }
        case(PROTOCOL_ADDR):
        {
//            addr = addr*10 + ch-'0';
            raw += ch;
            if(times++ >= 1)
            {
                status = PROTOCOL_TYPE;
                times = 0;
            }
            break;
        }
        case(PROTOCOL_TYPE):
        {
//            type = type*10 + ch-'0';
            raw += ch;
            if(times++ >= 1)
            {
                status = PROTOCOL_LEN;
                times = 0;
            }
            break;
        }
        case(PROTOCOL_LEN):
        {
//            len = (len<<8) | ch;
            raw += ch;
            if(times++ >= 1)
            {
                quint16 len = (raw[4] << 8) | raw[5];
                if(len == 0)
                    status = PROTOCOL_CRC;
                else
                    status = PROTOCOL_DATA;
                times = 0;
            }
            break;
        }
        case(PROTOCOL_DATA):
        {
//            data += ch;
            raw += ch;
            quint16 len = (raw[4] << 8) | raw[5];
            if(times++ >= len-1)
            {
                status = PROTOCOL_CRC;
                times = 0;
            }
            break;
        }
        case(PROTOCOL_CRC):
        {
//            crc = (crc<<8) | ch;
            raw += ch;
            if(times++ >= 1)
            {
//                raw.push_front(Frame::frame_head);//加上帧头 计算crc
                quint16 gencrc = Frame::genCrc(raw.left(raw.length()-2));
//                raw.remove(0, 1);//移除帧头
//                qDebug() << QString("0x%1").arg((((raw[raw.length()-2]) << 8)&0xff00), 4, 16, QLatin1Char('0'));
//                qDebug() << QString("0x%1").arg(raw[raw.length()-1], 4, 16, QLatin1Char('0'));

                quint16 crc = ((raw[raw.length()-2] << 8)&0xff00) | (raw[raw.length()-1]&0xff);
                if(crc == gencrc)
                    status = PROTOCOL_END;
                else
                {
                    qDebug() << QString("校验位计算错误，接收校验位：0x%1 计算校验位：0x%2").arg(crc, 4, 16, QLatin1Char('0')).arg(gencrc, 4, 16, QLatin1Char('0'));
                    status = PROTOCOL_ERROR;
                }
                times = 0;
            }
            break;
        }
        case(PROTOCOL_END):
        {
            if(ch == Frame::frame_tail)
            {
                //帧解析成功
                status = PROTOCOL_START;
                frameList.append(Frame(raw));
                res = true;
            }else
            {
                qDebug() << "帧结束位接收错误";
                status = PROTOCOL_ERROR;
            }
            break;
        }
    }
    return res;
}



