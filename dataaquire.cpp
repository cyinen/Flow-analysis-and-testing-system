#include "dataaquire.h"
#include<QDebug>
DataAquire::DataAquire(QObject *parent) : QObject(parent)
{
    isStop = false;

}

void DataAquire::doMyWork()
{
    // 第一步 创建设备对象

    hDevice = USB3202_DEV_Create(0, 0);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        printf("DEV_Create Error\n");
        getch();
        //return 0;
    }

    memset(&AIParam, 0, sizeof(USB3202_AI_PARAM));

    // 通道参数
    AIParam.nSampChanCount = 4;
    for (int nChannel = 0; nChannel < USB3202_AI_MAX_CHANNELS; nChannel++)
    {
        AIParam.CHParam[nChannel].nChannel = nChannel;
        AIParam.CHParam[nChannel].nSampleGain = USB3202_AI_SAMPGAIN_1MULT;
        AIParam.CHParam[nChannel].nRefGround = USB3202_AI_REFGND_RSE;
        AIParam.CHParam[nChannel].nReserved0 = 0;
        AIParam.CHParam[nChannel].nReserved1 = 0;
        AIParam.CHParam[nChannel].nReserved2 = 0;
    }
    AIParam.nSampleSignal = USB3202_AI_SAMPSIGNAL_AI;
    AIParam.nSampleRange = USB3202_AI_SAMPRANGE_N10_P10V;
    AIParam.nReserved0 = 0;

    // 时钟参数
    AIParam.nSampleMode = USB3202_AI_SAMPMODE_CONTINUOUS;
    AIParam.nSampsPerChan = 1024;
    AIParam.fSampleRate = 100000.0;
    AIParam.nClockSource = USB3202_AI_CLKSRC_LOCAL;
    AIParam.bClockOutput = FALSE;
    AIParam.nReserved1 = 0;
    AIParam.nReserved2 = 0;

    // 触发参数
    AIParam.bDTriggerEn = TRUE;
    AIParam.nDTriggerDir = USB3202_AI_TRIGDIR_FALLING;
    AIParam.bATriggerEn = TRUE;
    AIParam.nATriggerDir = USB3202_AI_TRIGDIR_FALLING;
    AIParam.fTriggerLevel = 0.0;
    AIParam.nTriggerSens = 5;
    AIParam.nDelaySamps = 0;

    // 其他参数
    AIParam.nReserved3 = 0;
    AIParam.nReserved4 = 0;
    AIParam.nReserved5 = 0;
    AIParam.nReserved6 = 0;

    if (!USB3202_AI_VerifyParam(hDevice, &AIParam))
    {
        printf("参数有错，已被调整为合法值，请注意查看日志文件USB3202.log, 按任意键继续...\n");
        getch();
    }

    // 第二步 初始化AI采样任务
    if (!USB3202_AI_InitTask(hDevice, &AIParam, NULL))
    {
        printf("AI_InitTask Error\n");
        getch();
    }

    // 第三步 启动AI采样任务
    if (!USB3202_AI_StartTask(hDevice))
    {
        printf("AI_StartTask Error\n");
        getch();
    }

    // 第四步 发送软件触发事件(硬件外触发时不需要)
    if (!USB3202_AI_SendSoftTrig(hDevice))
    {
        printf("AI_SendSoftTrig Error\n");
        getch();
    }
    int num=0;
    qDebug()<<"running";
    while (!isStop)
    {

        float dataI1;
        //printf("Wait...\n");

        if (!USB3202_AI_ReadBinary(hDevice, nBinArray, nReadSampsPerChan, &nSampsPerChanRead, &nReadableSamps, fTimeout))
        {
            //printf("Timeout nSampsPerChanRead=%d\n", nSampsPerChanRead);
            getch();
        }
        for (U32 nIndex = 0; nIndex < 64; nIndex++)
        {

            for (int nChannel = 0; nChannel < 4; nChannel++)

            {
               double n = (double(20) / 65536)*nBinArray[nChannel + nIndex * 4]-10.0;
               //qDebug()<< n << "  ";
                //printf("AI%02d=%04d(Bin)  ", nChannel, nBinArray[nChannel + nIndex * 4]);

               if(nChannel==1)
               {
                   dataI1=n;
                   //ui->line_V->setText(QString::number(dataI1));
               }


           }
            if(num == 1024)
            {
                emit dataREADY(dataArry,1024);
                num=0;
            }

            dataArry[num]=dataI1;
            num++;
            //qDebug()<<"子："<<num;

            //printf("\n");
        }
        //QThread::sleep(2);
        if(isStop)
            break;
    }
    // 第六步 停止AI采样
    qDebug()<<"第六步 停止AI采样";

    if (!USB3202_AI_StopTask(hDevice))
    {
        printf("AI_StopTask Error\n");
        getch();
    }

     qDebug()<<"第七步 释放AI采集任务";
    if (!USB3202_AI_ReleaseTask(hDevice))
    {
        printf("AI_ReleaseTask Error\n");
        getch();
    }

    qDebug()<<"第八步 释放设备对象";
    if (!USB3202_DEV_Release(hDevice))
    {
        printf("DEV_Release Error\n");
        getch();
    }
    qDebug()<<"finish!";
}

void DataAquire::setFlage(bool bl)
{
    isStop = bl;
}

