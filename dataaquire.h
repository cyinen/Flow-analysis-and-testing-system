#ifndef DATAAQUIRE_H
#define DATAAQUIRE_H

#include <QThread>
#include <synchapi.h>

#include "stdafx.h"
#include "conio.h"
#include "usb3202.h"
#include <vector>
using namespace std;
class DataAquire : public QObject
{
    Q_OBJECT
public:
    explicit DataAquire(QObject *parent = nullptr);
    void doMyWork();
    void setFlage(bool bl);
protected:
    U16 nBinArray[32768];
    USB3202_AI_PARAM AIParam;
    U32 nReadSampsPerChan = 1024, nSampsPerChanRead = 0, nReadableSamps = 0;
    F64 fTimeout = 10.0; // 10秒钟超时
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    float dataArry[1024];
signals:
    void dataREADY(float *);
public slots:
private:
    volatile bool isStop;
};

#endif // DATAAQUIRE_H
