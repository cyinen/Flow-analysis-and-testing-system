#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPushButton>
#include <dataaquire.h>
#include <fft.h>
#include <QList>
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
    ui->groupBox->setAutoFillBackground(true);
    ui->groupBox->setPalette(QPalette(Qt::gray));
    ui->groupBox_2->setAutoFillBackground(true);
    ui->groupBox_2->setPalette(QPalette(Qt::gray));
    ui->lineEdit_SetFFTNum->setText("1024");
    da = new DataAquire();
    // 2. 子线程类
    pthread = new QThread(this);
    // 3. 移动业务对象到子线程
    da->moveToThread(pthread);
    connect(this, &MainWindow::sigWorking, da, &DataAquire::doMyWork);

    void (DataAquire::*dataREADY)(float * ) = &DataAquire::dataREADY;
    connect(da,dataREADY,this,&MainWindow::showWaves);

    fft = new FFT();
    fftthread = new QThread(this);
    fft->moveToThread(fftthread);
    connect(this, &MainWindow::sigDdtaReady, fft, &FFT::doMyWork);
    void (FFT::*sigDATA)(float []) = &FFT::sigDATA;
    connect(fft,sigDATA,this,&MainWindow::showSpectrum);


}

void MainWindow::showWaves(float * dataArry)
{
    emit sigDdtaReady(dataArry,64);
    qDebug()<<"开始处理";

    int n=0;
    while(n<64)
    {
        mycurrenttime = QDateTime::currentDateTime();//获取系统时间
        double xzb = mystarttime.msecsTo(mycurrenttime)/100;//获取横坐标，相对时间就是从0开始
        ui->widget_plotwaves->graph(0)->addData(xzb,dataArry[n]);//添加数据1到曲线1
        if(xzb>30)
        {
            ui->widget_plotwaves->xAxis->setRange((double)qRound(xzb-30),xzb);//设定x轴的范围
        }
        else ui->widget_plotwaves->xAxis->setRange(0,30);//设定x轴的范围
        ui->widget_plotwaves->replot();//每次画完曲线一定要更新显示
        n+=1;
        //qDebug()<<"主:"<<n;
    }
     qDebug()<<"结束";
}

void MainWindow::showSpectrum(float* dataArry)
{

    int n=0;

    while(n<1024)
    {
        mycurrenttime = QDateTime::currentDateTime();//获取系统时间
        double xzb = mystarttime.msecsTo(mycurrenttime)/100;//获取横坐标，相对时间就是从0开始
        ui->widget_plotSpect->graph(0)->addData(xzb,dataArry[n]);//添加数据1到曲线1
        if(xzb>30)
        {
            ui->widget_plotSpect->xAxis->setRange((double)qRound(xzb-30),xzb);//设定x轴的范围
        }
        else ui->widget_plotSpect->xAxis->setRange(0,30);//设定x轴的范围
        ui->widget_plotSpect->replot();//每次画完曲线一定要更新显示
        n++;

    }

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

                //Sleep(1000);
                AnalyzeData();
                connect(myserial,&QSerialPort::readyRead,this,&MainWindow::AnalyzeData);
                mystarttime = QDateTime::currentDateTime();//图像横坐标初始值参考点，读取初始时间
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
        qDebug()<<"mytemp:"<<mytemp;
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
            int m=0;
            float data1;
                while(!out.atEnd())
                {
                    m++;
                       qint8 outChar = 0;
                       out>>outChar;   //每字节填充一次，直到结束
                       //qDebug()<<outChar;
                       //十六进制的转换
                       QString str = QString("%1").arg(outChar&0xFF,2,16,QLatin1Char('0'));
                       //ui->RecvTextEdit->insertPlainText(str.toUpper());//大写

                       bool ok;

                       QString hex = str;

                       float dec = hex.toInt(&ok, 16);


                       //qDebug()<<dec;
                       if(m==5){
                           dec=dec/10;
                           data1=dec;
                           ui->lineEdit_Realtime_T->setText(QString::number(dec));
                       }

                       //ui->RecvTextEdit->insertPlainText(" ");//每发送两个字符后添加一个空格
                       //ui->RecvTextEdit->moveCursor(QTextCursor::End);
                }
              QString str = QString("%1").arg(data1);

              ui->lineEdit_RealSensorT->setText(str);
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
        ui->widget_plotwaves->graph(1)->setLineStyle((QCPGraph::LineStyle)1);//曲线画笔
        ui->widget_plotwaves->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone,5));//曲线形状
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
        ui->widget_plotwaves->xAxis->setTickStep(2);//设置刻度间距5
        ui->widget_plotwaves->xAxis->setRange(0,30);//设定x轴的范围

        ui->widget_plotwaves->yAxis->setLabel(QStringLiteral("幅度"));//设置y坐标轴名称
        ui->widget_plotwaves->yAxis->setLabelColor(QColor(20,20,20));//设置y坐标轴名称颜色
        ui->widget_plotwaves->yAxis->setAutoTickStep(false);//设置是否自动分配刻度间距
        ui->widget_plotwaves->yAxis->setTickStep(10);//设置刻度间距1
        ui->widget_plotwaves->yAxis->setRange(-50,50);//设定y轴范围

        ui->widget_plotwaves->axisRect()->setupFullAxesBox(true);//设置缩放，拖拽，设置图表的分类图标显示位置
        ui->widget_plotwaves->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom| QCP::iSelectAxes);
        ui->widget_plotwaves->axisRect()->insetLayout()->setInsetAlignment(0,Qt::AlignTop | Qt::AlignRight);//图例显示位置右上
        ui->widget_plotwaves->legend->setVisible(true);//显示图例

        ui->widget_plotwaves->replot();
        //设置图表2
        ui->widget_plotSpect->xAxis->setLabel(QStringLiteral("频率/Hz"));//设置x坐标轴名称
        ui->widget_plotSpect->xAxis->setLabelColor(QColor(20,20,20));//设置x坐标轴名称颜色
        ui->widget_plotSpect->xAxis->setAutoTickStep(true);//设置是否自动分配刻度间距
        ui->widget_plotSpect->xAxis->setTickStep(2);//设置刻度间距5
        ui->widget_plotSpect->xAxis->setRange(0,30);//设定x轴的范围

        ui->widget_plotSpect->yAxis->setLabel(QStringLiteral("幅度"));//设置y坐标轴名称
        ui->widget_plotSpect->yAxis->setLabelColor(QColor(20,20,20));//设置y坐标轴名称颜色
        ui->widget_plotSpect->yAxis->setAutoTickStep(true);//设置是否自动分配刻度间距
        ui->widget_plotSpect->yAxis->setTickStep(10);//设置刻度间距1
        ui->widget_plotSpect->yAxis->setRange(0,1000);//设定y轴范围

        ui->widget_plotSpect->axisRect()->setupFullAxesBox(true);//设置缩放，拖拽，设置图表的分类图标显示位置
        ui->widget_plotSpect->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom| QCP::iSelectAxes);
        ui->widget_plotSpect->axisRect()->insetLayout()->setInsetAlignment(0,Qt::AlignTop | Qt::AlignRight);//图例显示位置右上
        ui->widget_plotSpect->legend->setVisible(true);//显示图例

        ui->widget_plotSpect->replot();
}


void MainWindow::StringToHex(QString str, QByteArray &senddata) //字符串转换为十六进制数据0-F
{
    int hexdata,lowhexdata;
    int hexdatalen = 0;
    int len = str.length();
    senddata.resize(len/2);
    char lstr,hstr;

    for(int i=0; i<len; )
    {
        //char lstr,
        hstr=str[i].toLatin1();
        if(hstr == ' ')
        {
            i++;
            continue;
        }
        i++;
        if(i >= len)
            break;
        lstr = str[i].toLatin1();
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
    senddata.resize(hexdatalen);
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
}

void MainWindow::on_pushButton_Playing_clicked()
{
    if(pthread->isRunning())
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
}
