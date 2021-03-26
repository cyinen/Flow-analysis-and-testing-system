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
    void setFs(int Fs=2000);
protected:
    U16 nBinArray[32768];
    USB3202_AI_PARAM AIParam;
    U32 nReadSampsPerChan = 1024, nSampsPerChanRead = 0, nReadableSamps = 0;
    F64 fTimeout = 10.0; // 10秒钟超时
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    float dataArry1[1024];
    float dataArry2[1024];
    void movmedian(float *DATA,int length,int n);
signals:
    void dataREADY(float *,int,float*);
public slots:
private:
    volatile bool isStop;
    int Fs=2000;
};

#endif // DATAAQUIRE_H
