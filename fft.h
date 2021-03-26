#ifndef FFT_H
#define FFT_H

#include "math.h"
#include <complex>
#include "time.h"
#include "vector"
#include <QObject>
#include <QList>
using namespace std;
class FFT : public QObject
{
    Q_OBJECT
public:
    explicit FFT(QObject *parent = nullptr);
    void doMyWork(float* data, int M);
    void setFlage(bool bl);

    void ReverseOrder(std::complex<float> *&A, int N);
    void fft(std::complex<float> *&A, int N);
    std::complex<float> Wn(int k, int n, int N);
    bool setN(int N);
signals:
    void sigDATA(float *,int );
public slots:
    //void slotSetData(float* arry);
private:
    int N=1024*2;
    float dataSpec[1024*2];
    std::complex<float> Wn_table[2048][2048];
};

#endif // FFT_H
