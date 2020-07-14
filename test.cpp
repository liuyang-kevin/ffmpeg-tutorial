#include <math.h>
#include <float.h>
#include <iostream>
using namespace std;
int main(int argc, char const *argv[])
{
    // 在空气中1帕斯卡等于94分贝声压级
    int air_dB = 94;
    int max_dB = 20 * log10(65536);
    cout << max_dB << endl;

    int dB256 = 20 * log10(256);
    cout << dB256 << endl;

    int dBFLTMAX = 20 * log10(FLT_MAX);
    cout << dBFLTMAX << endl;

    int A1 = pow(10, 2);
    cout << A1 << endl;

    for (int i = -5; i < 96; i++)
    {
        float multiplier = pow(10, i / 20.0);
        cout << multiplier << endl;
    }

    return 0;
}
