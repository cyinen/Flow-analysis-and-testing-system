#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>//用于在控制台输出调试信息
#include <QTime>//定时器
#include <QPainter>//坐标系绘图
/*------------------------用户代码头文件---------------------------*/
#include <QtSerialPort/QSerialPort>//串口
#include <QtSerialPort/QSerialPortInfo>//串口
#include <dataaquire.h>
#include <fft.h>
#include <vector>
#include <qcustomplot.h>
using namespace std;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void init();
    void showWaves(float * dataArry,int N);
    void showSpectrum(float * dataArry,int N);
    void setupPlot();//初始化
    void saveData();

private slots:
    void on_btn_search_port_clicked();

    void on_btn_open_port_clicked();

    void AnalyzeData();//数据读取

    void StringToHex(QString str, QByteArray &senddata);

    char ConvertHexChar(char ch);

    void on_pushButton_Stop_clicked();

    void on_pushButton_Playing_clicked();

    void refresh();

    void on_btn_clear_clicked();

    void on_btn_save_clicked();

    void on_pushButton_replay_clicked();

private:
    Ui::MainWindow *ui;
    QSerialPort *myserial;//声明串口类，myserial是QSerialPort的实例
    bool serial_flag,start_flag;//定义两个标志位
    QByteArray alldata;//接收串口数据
    //绘图函数
    QDateTime mycurrenttime;//系统当前时间
    QDateTime mystarttime;//系统开始时间

    DataAquire *da;
    QThread *pthread;

    FFT *fft;
    QThread *fftthread;
    QThread *tthread;
    QTimer *timer;
    QCPItemText *textLabel;

    QVector<float> *data;

signals:
    void sigWorking();
    void sigDdtaReady(float *,int);
};

#endif // MAINWINDOW_H
