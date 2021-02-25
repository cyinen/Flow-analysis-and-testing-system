#include "fft.h"
#include "math.h"
#include "complex"
#include "time.h"
#include "vector"
#include<QDebug>
const float PI=3.141592653589;
FFT::FFT(QObject *parent) : QObject(parent)
{
   // Wn_table = new float[1024][1024];
    for (int i = 0; i < N; ++i)
        {
            for (int j = 0; j < N; ++j)
            {
                Wn_table[i][j] = Wn(i, j, N);
            }
        }
}

void FFT::doMyWork(float * data,int M)
{
    complex<float>* DATA = new complex<float>[1024];
    for(int i=0;i<N;i++)
    {
        if (i < M)
        {
            DATA[i].imag(0);
            DATA[i].real(data[i]);

        }
        else
        DATA[i] = 0;
    }
    fft(DATA, N);
    emit sigDATA(dataSpec);
}

void FFT::ReverseOrder(complex<float> *&A, int N)
{
    int J = 0;
        int K = 0;
        for (int I = 0; I < N - 1; I++)
        {
            if (I < J)
            {
                complex<float> T;
                T = A[I];
                A[I] = A[J];
                A[J] = T;
            }
            K = N >> 1;
            while (K <= J)
            {
                J -= K;
                K >>= 1;
            }
            J += K;
        }
}

void FFT::fft(complex<float> *&A, int N)
{
    int M = log2(N);
    ReverseOrder(A, N);
    for (int L = 1; L <= M; ++L)
    {
        int B = pow(2, L - 1);
        //cout<<"B:"<<B<<endl;
        for (int j = 0; j <= B - 1; ++j)
        {
            int p = j * pow(2, M - L);
            for (int k = j; k <= N - 1; k += pow(2, L))
            {
                complex<float> T = A[k] + A[k + B] * Wn_table[1][p];
                // cout<<A[k+B]<<"*"<<Wn_table[p]<<"="<<A[k+B]*Wn_table[p]<<endl;
                A[k + B] =A[k] - A[k + B] * Wn_table[1][p];
                A[k] = T;
                dataSpec[k+B] = abs(A[k + B]);dataSpec[k]=abs(A[k]);
            }
        }
    }
}

complex<float> FFT::Wn(int k, int n, int N)
{
    complex<float> z2{0, -2 * PI * k * n / N};
    return exp(z2);
}

bool FFT::setN(int N)
{
    this->N=N;
    return true;
}


