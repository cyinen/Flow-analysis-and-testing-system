#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPushButton>
#include <dataaquire.h>
#include <fft.h>
#include <QList>
#include <QObject>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    myserial = new QSerialPort();
    serial_flag = true;
    start_flag = true;
    init();
    connect(ui->pushButton_Exit,&QPushButton::clicked,this,&MainWindow::close);


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::init()
{
    //init
    setWindowState(Qt::WindowMaximized);
    setWindowTitle("CYW-01型管道流量分析检测系统");
    setWindowIcon(QIcon(":/images/Luffy.png"));


    setupPlot();

    ui->groupBox_2->setAutoFillBackground(true);
    ui->groupBox_2->setPalette(QPalette(Qt::gray));
    ui->lineEdit_SetFFTNum->setText("1024");
    ui->lineEdit_SetTheta->setText("50");
    ui->lineEdit_SetFrequency->setText("10");
    ui->lineEdit_SetTemper_T->setText("60");
    ui->lineEdit_WorkFreq->setText("24.056");
    ui->lineEdit_RealSensorT->setText("30.0");
    da = new DataAquire();
    da->setFs(Fs);
    // 2. 子线程类
    pthread = new QThread(this);
    // 3. 移动业务对象到子线程
    da->moveToThread(pthread);
    connect(this, &MainWindow::sigWorking, da, &DataAquire::doMyWork);

    void (DataAquire::*dataREADY)(float * ,int,float *) = &DataAquire::dataREADY;
    connect(da,dataREADY,this,&MainWindow::showWaves);

    fft = new FFT();
    fftthread = new QThread(this);
    fft->moveToThread(fftthread);
    connect(da, dataREADY, fft, &FFT::doMyWork);
    void (FFT::*sigDATA)(float [],int) = &FFT::sigDATA;
    connect(fft,sigDATA,this,&MainWindow::showSpectrum);

    //定时器 每30ms刷新下图表
    timer = new  QTimer(this);

    connect(timer,&QTimer::timeout,this,&MainWindow::refresh);

    //保存数据
    dataI = new QVector<float>();
    dataQ = new QVector<float>();
    //读取温度
    timerT = new  QTimer(this);
    //Tthread = new QThread();
    //timerT->moveToThread(tthread);


}

void MainWindow::showWaves(float * dataArry1,int N,float * dataArry2)
{

    static double m=0;
    int n=0;
    int index=2;float temp_data=dataArry1[2];
    while(n<N)
    {
        if(dataArry1[n] > temp_data+1 ) {
            index=n;
            temp_data = dataArry1[n];
        }
        //ui->widget_plotwaves->xAxis->setRange(0,m);
        ui->widget_plotwaves->graph(0)->addData(m,dataArry1[n]);//添加数据1到曲线1
        ui->widget_plotwaves->graph(1)->addData(m,dataArry2[n]);//添加数据1到曲线2
        dataI->append(dataArry1[n]);
        dataQ->append(dataArry2[n]);
        if(m>N)
        {
            ui->widget_plotwaves->xAxis->setRange((double)(m-N),m);//设定x轴的范围
        }
        else
        ui->widget_plotwaves->xAxis->setRange(0,N);//设定x轴的范围
        //ui->widget_plotwaves->yAxis->setRange(-1,dataArry1[index]+1);//设定y轴范围
        n+=1;m++;

    }

}

void MainWindow::showSpectrum(float* dataArry,int N)
{

    int n=1;
    int index=2;float temp_data=dataArry[2];
    ui->widget_plotSpect->graph(0)->removeData(0,Fs/2);
    ui->widget_plotSpect->xAxis->setRange(0,200);
    ui->widget_plotSpect->xAxis->setTickStep(Fs/N);//设置刻度间距5
    ui->widget_plotwaves->item();
    while(n < N/2)
    {
        if(dataArry[n] > temp_data+1 ) {
            index=n;
            temp_data = dataArry[n];
        }

        //if(dataArry[n]>100) index=n;
        ui->widget_plotSpect->graph(0)->addData(n*Fs/N,dataArry[n]);//添加数据1到曲线1
        n++;
    }
    ui->widget_plotSpect->yAxis->setRange(-1,dataArry[index]+1);
    QString str = QString("Max@f=%1Hz    Peak found！").arg(index*Fs/N);
    double v=index*Fs/N;
    qDebug()<<"频率："<<v;
    v=(0.3*v)/(2*24.056*0.64278761);
    ui->lineEdit_Velocity->setText(QString::number(v));
    textLabel->setText(str);

}

void MainWindow::on_btn_search_port_clicked()
{
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())//读取串口信息
        {
            myserial->setPort(info);//这里相当于自动识别串口号之后添加到了cmb，如果要手动选择可以用下面列表的方式添加进去
            if(myserial->open(QIODevice::ReadWrite))
             {
              ui->comboBox->addItem(myserial->portName());//将串口号添加到cmb
              myserial->close();//关闭串口等待人为(打开串口按钮)打开
             }
         }

}

void MainWindow::on_btn_open_port_clicked()
{
    if(!timerT->isActive())
    {

        timerT->start(300);
    }

    if(serial_flag)
        {
            ui->comboBox->setDisabled(true); //禁止修改串口
            myserial->setPortName(ui->comboBox->currentText()); //设置串口号
            myserial->setBaudRate(QSerialPort::Baud9600); //设置波特
            myserial->setDataBits(QSerialPort::Data8); //设置数据位数
            myserial->setParity(QSerialPort::NoParity);//设置奇偶校验
            myserial->setStopBits(QSerialPort::OneStop);//设置停止位
            myserial->setFlowControl(QSerialPort::NoFlowControl);//非流控制
            if(myserial->open(QIODevice::ReadWrite))
            {
                //qDebug()<<"串口打开成功";

               // Sleep(1000);
                AnalyzeData();
                //
               // mystarttime = QDateTime::currentDateTime();//图像横坐标初始值参考点，读取初始时间
                 //connect(myserial,&QSerialPort::readyRead,this,&MainWindow::AnalyzeData);
                connect(timerT,&QTimer::timeout,this,&MainWindow::AnalyzeData);
                qDebug()<<"串口打开成功";
            }
            else
            {
                qDebug()<<"串口打开失败";
                //QMessageBox::warning(this,tr("waring"),tr("串口打开失败"),QMessageBox::Close);
            }
            ui->btn_open_port->setText("关闭串口");
            serial_flag = false;//串口标志位置失效
        }
        else
        {
            ui->comboBox->setEnabled(true);//串口号下拉按钮使能工作
            myserial->close();
            ui->btn_open_port->setText("打开串口");//按钮显示“打开串口”
            serial_flag = true;//串口标志位置工作
        }
}

void MainWindow::AnalyzeData()
{
    QString m_strSendData ="01 03 00 00 00 01 84 0A";

            if (m_strSendData.contains(" "))
            {
                m_strSendData.replace(QString(" "),QString(""));    //把空格去掉
            }

            QByteArray sendBuf;

           StringToHex(m_strSendData, sendBuf);             //把QString 转换 为 hex
           myserial->write(sendBuf);


        QByteArray mytemp = myserial->readAll();//定义mytemp为串口读取的所有数据
       // qDebug()<<"mytemp:"<<mytemp;
        if(!mytemp.isEmpty())
        {

            //初代代码
//            QString StrI1=tr(mytemp.mid(mytemp.indexOf("T")+1,mytemp.indexOf("P")-mytemp.indexOf("T")-1));//自定义了简单协议，通过前面字母读取需要的数据
//            QString StrI2=tr(mytemp.mid(mytemp.indexOf("H")+1,mytemp.indexOf("I")-mytemp.indexOf("H")-1));
//            ui->line_Temp->setText(StrI1);//显示读取温度值
//            ui->line_Humi->setText(StrI2);//显示读取湿度值
//            float dataI1=StrI1.toFloat();//将字符串转换成float类型进行数据处理
//            float dataI2=StrI2.toFloat();//将字符串转换成float类型进行数据处理

            QDataStream out(&mytemp,QIODevice::ReadWrite);    //将字节数组读入
            int m=0;QString str1;QString str2;
            double data1; bool ok;

                while(!out.atEnd())
                {
                    m++;
                    qint8 outChar = 0;
                    out>>outChar;   //每字节填充一次，直到结束
                    //qDebug()<<outChar;
                    if(m<4 || m >5)
                        continue;
                    //十六进制的转换

                    //ui->RecvTextEdit->insertPlainText(str.toUpper());//大写


                    //qDebug()<<dec;
                    if(m==4)
                    {
                      str1 = QString("%1").arg(outChar&0xFF,2,16,QLatin1Char('0'));
                     }
                     if(m==5){
                           str2 = QString("%1").arg(outChar&0xFF,2,16,QLatin1Char('0'));
                       }
                       //ui->RecvTextEdit->insertPlainText(" ");//每发送两个字符后添加一个空格
                       //ui->RecvTextEdit->moveCursor(QTextCursor::End);
                }
                data1= (str1+str2).toInt(&ok,16)/10.0;
                 //qDebug()<<data1;
              QString str = QString("%1").arg(data1);
                ui->lineEdit_Realtime_T->setText(str);

        }
}


void MainWindow::setupPlot()
{
    //设置曲线一
        ui->widget_plotwaves->addGraph();//添加一条曲线
        QPen pen;
        pen.setWidth(1);//设置画笔线条宽度
        pen.setColor(Qt::blue);
        ui->widget_plotwaves->graph(0)->setPen(pen);//设置画笔颜色
        ui->widget_plotwaves->graph(0)->setBrush(QBrush(QColor(0, 0, 255, 20))); //设置曲线画刷背景
        ui->widget_plotwaves->graph(0)->setName("CH I");
        ui->widget_plotwaves->graph(0)->setAntialiasedFill(false);
        ui->widget_plotwaves->graph(0)->setLineStyle(QCPGraph::LineStyle::lsNone);//曲线画笔
        ui->widget_plotwaves->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPlus,5));//曲线形状

    //设置曲线二
        ui->widget_plotwaves->addGraph();//添加一条曲线
        pen.setColor(Qt::red);
        ui->widget_plotwaves->graph(1)->setPen(pen);//设置画笔颜色
        ui->widget_plotwaves->graph(1)->setBrush(QBrush(QColor(0, 0, 255, 20))); //设置曲线画刷背景
        ui->widget_plotwaves->graph(1)->setName("CH Q");
        ui->widget_plotwaves->graph(1)->setAntialiasedFill(false);
        ui->widget_plotwaves->graph(1)->setLineStyle(QCPGraph::LineStyle::lsNone);//曲线画笔
        ui->widget_plotwaves->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssStar,5));//曲线形状
    //设置频谱
        ui->widget_plotSpect->addGraph();
        pen.setColor(Qt::red);
        ui->widget_plotSpect->graph(0)->setPen(pen);//设置画笔颜色
        ui->widget_plotSpect->graph(0)->setBrush(QBrush(QColor(0, 0, 255, 20))); //设置曲线画刷背景
        ui->widget_plotSpect->graph(0)->setName("频谱");
        ui->widget_plotSpect->graph(0)->setAntialiasedFill(false);
        ui->widget_plotSpect->graph(0)->setLineStyle((QCPGraph::LineStyle::lsImpulse));//曲线画笔
        ui->widget_plotSpect->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone,5));//曲线形状
    //设置图表1
        ui->widget_plotwaves->xAxis->setLabel(QStringLiteral("时间/s"));//设置x坐标轴名称
        ui->widget_plotwaves->xAxis->setLabelColor(QColor(20,20,20));//设置x坐标轴名称颜色
        ui->widget_plotwaves->xAxis->setAutoTickStep(false);//设置是否自动分配刻度间距
        ui->widget_plotwaves->xAxis->setTickStep(100);//设置刻度间距5
        ui->widget_plotwaves->xAxis->setRange(0,100);//设定x轴的范围

        ui->widget_plotwaves->yAxis->setLabel(QStringLiteral("幅度"));//设置y坐标轴名称
        ui->widget_plotwaves->yAxis->setLabelColor(QColor(20,20,20));//设置y坐标轴名称颜色
        ui->widget_plotwaves->yAxis->setAutoTickStep(false);//设置是否自动分配刻度间距
        ui->widget_plotwaves->yAxis->setTickStep(0.5);//设置刻度间距1
        ui->widget_plotwaves->yAxis->setRange(0,15);//设定y轴范围

        ui->widget_plotwaves->axisRect()->setupFullAxesBox(true);//设置缩放，拖拽，设置图表的分类图标显示位置
        ui->widget_plotwaves->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom| QCP::iSelectAxes);
        ui->widget_plotwaves->axisRect()->insetLayout()->setInsetAlignment(0,Qt::AlignTop | Qt::AlignRight);//图例显示位置右上
        ui->widget_plotwaves->legend->setVisible(true);//显示图例

        ui->widget_plotwaves->replot();
        //设置图表2
        ui->widget_plotSpect->xAxis->setLabel(QStringLiteral("频率/Hz"));//设置x坐标轴名称
        ui->widget_plotSpect->xAxis->setLabelColor(QColor(20,20,20));//设置x坐标轴名称颜色
        ui->widget_plotSpect->xAxis->setAutoTickStep(true);//设置是否自动分配刻度间距
        ui->widget_plotSpect->xAxis->setTickStep(1);//设置刻度间距5
        ui->widget_plotSpect->xAxis->setRange(0,1024);//设定x轴的范围

        ui->widget_plotSpect->yAxis->setLabel(QStringLiteral("幅度"));//设置y坐标轴名称
        ui->widget_plotSpect->yAxis->setLabelColor(QColor(20,20,20));//设置y坐标轴名称颜色
        ui->widget_plotSpect->yAxis->setAutoTickStep(true);//设置是否自动分配刻度间距
        ui->widget_plotSpect->yAxis->setTickStep(1);//设置刻度间距1
        ui->widget_plotSpect->yAxis->setRange(0,20);//设定y轴范围

        ui->widget_plotSpect->axisRect()->setupFullAxesBox(true);//设置缩放，拖拽，设置图表的分类图标显示位置
        ui->widget_plotSpect->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom| QCP::iSelectAxes);
        ui->widget_plotSpect->axisRect()->insetLayout()->setInsetAlignment(0,Qt::AlignTop | Qt::AlignRight);//图例显示位置右上
        ui->widget_plotSpect->legend->setVisible(true);//显示图例

        //添加文本框显示最大频率

        textLabel = new QCPItemText(ui->widget_plotSpect);//在QCustomplot中新建文字框
        textLabel->setPositionAlignment(Qt::AlignTop|Qt::AlignLeft);//文字布局：顶、左对齐
        textLabel->position->setType(QCPItemPosition::ptAxisRectRatio);//位置类型（当前轴范围的比例为单位/实际坐标为单位）
        textLabel->position->setCoords(0.05,0.05); //把文字框放在X轴的中间，Y轴的最顶部
        textLabel->setText("Max@f=0Hz    Peak found！");
        textLabel->setFont(QFont(font().family(), 12)); //字体大小
        textLabel->setPen(QPen(Qt::white)); //字体颜色
        textLabel->setPadding(QMargins(2,2,2,2));//文字距离边框几个像素
        ui->widget_plotSpect->addItem(textLabel);
        ui->widget_plotSpect->replot();
}

void MainWindow::saveData()
{
    QString date ;
    QString filename;
    QDateTime time;
    QDir dir;
    time = QDateTime::currentDateTime();
    date    = time.toString("yyyy-MM-dd");
    filename = time.toString("hh:mm:ss");
    filename = "C://data//" + date +"//"+ filename.split(":").at(0)+"-"+filename.split(":").at(1)+"-"+filename.split(":").at(2);
    if(!dir.exists("C://data"))
    {
        dir.mkdir("C://data");
        date = "C://data//" + date;
       if(!dir.exists(date))
           dir.mkdir(date);
    }

    QString filenameI=filename+"-I-data.txt";
    QString filenameQ=filename+"-Q-data.txt";
    QFile fileI(filenameI);
    QFile fileQ(filenameQ);
    qDebug()<<filenameI;
    qDebug()<<filenameQ;
    if(!fileI.open(QIODevice::WriteOnly))
    {
        qDebug()<<"文件I打开失败";
        return;
    }
    if(!fileQ.open(QIODevice::WriteOnly))
    {
        qDebug()<<"文件Q打开失败";
        return;
    }
    int size = dataI->size();
    for (int i=0;i<size;i++)
    {
        QString str =  QString::number(dataI->at(i))+"\n";
        fileI.write(str.toUtf8());
        str =  QString::number(dataQ->at(i))+"\n";
        fileQ.write(str.toUtf8());
    }
    QMessageBox::warning(this, "提示!", "数据默认保存为"+filename+"路径下");
    fileI.close();
    fileQ.close();




}

void MainWindow::setFreq()
{
    bool ok;
    QString freq = ui->lineEdit_SetFrequency->text();


    int da = freq.toInt(&ok,10);
    da=da*200;
    int d1;int temp;int d2;
    d1=da/4096;da-=d1*4096;
    temp=da/256;
    d1=d1*16+temp;
    da-=temp*256;

    d2=da/16;da-=d2*16;
    d2=d2*16+da;

    unsigned char data[8] = { 0x02,0x06,0x10,0x00,27,10 };
    data[4]=d1;data[5]=d2;
    CrcCheck(data, 6);


    QString m_strSendData;
    m_strSendData.sprintf("0%x,0%x,%x,0%x,%x,%x,%x,%x",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);
    QStringList strlist = m_strSendData.split(",");
    m_strSendData="";
    for (int i=0;i<8;++i) {
        if(strlist[i].length()!=2)
            m_strSendData=m_strSendData+" 0"+strlist[i];
        else
            m_strSendData=m_strSendData+" "+strlist[i];
    }
    qDebug()<<m_strSendData;


    if (m_strSendData.contains(" "))
    {
        m_strSendData.replace(QString(" "),QString(""));    //把空格去掉
    }

    QByteArray sendBuf;

   StringToHex(m_strSendData, sendBuf);             //把QString 转换 为 hex
   qDebug()<<sendBuf;
   int n=0;
   n=myserial->write(sendBuf);
   Sleep(500);
    qDebug()<<n;

}


void MainWindow::StringToHex(QString str, QByteArray &senddata)
{
    int hexdata,lowhexdata;
    int hexdatalen = 0;
    int len = str.length();
    senddata.resize(len/2);
    char lstr,hstr;
    for(int i=0; i<len; )
    {
        //char lstr,
        hstr=str[i].toLatin1();//获取一个字节
        if(hstr == ' ')//如果第一个是空的，就直接放弃了，结束本次循环跳入下次循环
        {
            i++;
            continue;
        }
        i++;
        if(i >= len)
            break;
        lstr = str[i].toLatin1();//低以为，先装高，后装低
        hexdata = ConvertHexChar(hstr);
        lowhexdata = ConvertHexChar(lstr);
        if((hexdata == 16) || (lowhexdata == 16))
            break;
        else
            hexdata = hexdata*16+lowhexdata;
        i++;
        senddata[hexdatalen] = (char)hexdata;
        hexdatalen++;
    }
    senddata.resize(hexdatalen);//senddata为我所要
}

char MainWindow::ConvertHexChar(char ch)
{
    if((ch >= '0') && (ch <= '9'))
        return ch-0x30;
    else if((ch >= 'A') && (ch <= 'F'))
        return ch-'A'+10;
    else if((ch >= 'a') && (ch <= 'f'))
        return ch-'a'+10;
    else return ch-ch;//不在0-f范围内的会发送成0
}

void MainWindow::on_pushButton_Stop_clicked()
{
    unsigned char data[8]={0x02,0x06,0x20,0x00,00,06};
    writeCommand(data);
    if(!pthread->isRunning())
    {
     da->setFlage(false);
    }
    da->setFlage(true);
    qDebug()<<"停止";
    pthread->quit();
    pthread->wait();

    fftthread->quit();
    fftthread->wait();


    timer->stop();

    saveData();
}

void MainWindow::on_pushButton_Playing_clicked()
{
    unsigned char data[8]={0x02,0x06,0x20,0x00,00,01};
    ///writeCommand(data);
    Sleep(2000);
    if(pthread->isRunning() )
    {
        return;
    }

    da->setFlage(false);
    qDebug()<<"开始";
    pthread->start();
    // 发信号, 让子线程工作
    emit sigWorking();

    QString str = ui->lineEdit_SetFFTNum->text();
    fft->setN(str.toInt());
    fftthread->start();

    mystarttime = QDateTime::currentDateTime();//图像横坐标初始值参考点，读取初始时间

    if(!timer->isActive())
    {

        timer->start(10);
    }
}

void MainWindow::refresh()
{
    ui->widget_plotwaves->replot();//每次画完曲线一定要更新显示
    ui->widget_plotSpect->replot();//每次画完曲线一定要更新显示
     AnalyzeData();
    //qDebug()<<"刷新数据";
}

void MainWindow::on_btn_clear_clicked()
{
    ui->widget_plotwaves->graph(0)->data()->clear();
    ui->widget_plotwaves->graph(1)->data()->clear();
    dataI->clear();
    dataQ->clear();
    ui->widget_plotwaves->replot();
}

void MainWindow::on_btn_save_clicked()
{
    saveData();
}

void MainWindow::on_pushButton_replay_clicked()
{
   ui->widget_plotwaves->graph(0)->data()->clear();
   dataI->clear();
   dataQ->clear();
   QString fileName = QFileDialog::getOpenFileName(
           this,
           tr("选择需要回放的文件."),
           QDir::currentPath(),
           tr("*.txt;;All files(*.*)"));
   if (fileName.isEmpty()) {
       QMessageBox::warning(this, "Warning!", "文件打开失败");
       return;
   }
   QFile file(fileName);
   if(!file.open(QIODevice::ReadOnly))
   {
       qDebug()<<"文件打开失败";
   }

   float da;char data[1024];
   int m=0;

   while(file.readLine(data,sizeof(data))!=-1)
   {
        da = QString::fromUtf8(data).simplified().toFloat();
        ui->widget_plotwaves->graph(0)->addData(m,da);
        if(m%200==0)
        {

            ui->widget_plotwaves->replot();
        }
        if(m>200)
        {
            ui->widget_plotwaves->xAxis->setRange((double)(m-200),m);//设定x轴的范围
        }
        else ui->widget_plotwaves->xAxis->setRange(0,200);//设定x轴的范围
        m++;
   }

   file.close();
}

void MainWindow::on_pushButton_Stop_2_clicked()
{

    if(timerT->isActive())
    {

        timerT->stop();
    }

    setFreq();

    Sleep(300);

    timerT->start(300);



}

void MainWindow::CrcCheck(unsigned char *buf, unsigned char len)
{
    unsigned short crc = 0xFFFF;
    unsigned char i, j = 0;
    while (j < len)
    {
        crc ^= buf[j];
        for (i = 0; i < 8; i++)
        {
            if (crc & 0x01) {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
                crc >>= 1;
        }
        j++;
    }
    buf[j] = crc % 0x100;
    buf[j + 1] = crc / 0x100;
}



void MainWindow::writeCommand(unsigned char data[8])
{
    //unsigned char data[8] = { 0x02,0x06,0x10,0x00,27,10 };
    //data[4]=d1;data[5]=d2;
    CrcCheck(data, 6);

    QString m_strSendData;
    m_strSendData.sprintf("0%x,0%x,%x,0%x,%x,%x,%x,%x",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);
    qDebug()<<m_strSendData;
    QStringList strlist = m_strSendData.split(",");
    m_strSendData="";
    for (int i=0;i<8;++i) {
        if(strlist[i].length()!=2)
            m_strSendData=m_strSendData+" 0"+strlist[i];
        else
            m_strSendData=m_strSendData+" "+strlist[i];
    }
    qDebug()<<m_strSendData;


    if (m_strSendData.contains(" "))
    {
        m_strSendData.replace(QString(" "),QString(""));    //把空格去掉
    }
    qDebug()<<m_strSendData;
   QByteArray sendBuf;

   StringToHex(m_strSendData, sendBuf);             //把QString 转换 为 hex

   qDebug()<<sendBuf;

   int n=0;
   n=myserial->write(sendBuf);
   Sleep(500);
   qDebug()<<n;
}
