#include "rebar.h"
#include <iostream>

Rebar calLayout(double b, double As,double h)
{
    Rebar res;
    res.fitable = false;
    const std::vector<int> dia = { 10,12,14,16,18,20,22,24,26,28,30,32 };
    double dif = 1e9;//偏差值，得到总面积最接近所需面积的钢筋
    for (int d : dia) {
        double s = M_PI * d * d / 4.0;//一根钢筋的面积
        int n = ceil(As / s);//钢筋数量
        if (b >= 100)n = std::max(n, 2);
        double space = std::max(25.0, (double)d);//钢筋间距
        int lay = calLayer(n, space, b, d, h);
        if (lay > 0) {
            double ndif = abs(As - n * s);
            if (ndif < dif) {
                dif = ndif;
                res.layer = lay;
                res.d = d;
                res.n = n;
                res.fitable = true;
                res.space = space;
            }
        }
    }
    if (!res.fitable) {
        std::cout << "警告：未找到可行配筋！As=" << As << " b=" << b << " h=" << h << '\n';
    }
    return res;
}

int calLayer(int n, double space, double b, int d, double h)
{
    int res = 1;//层数
    while (res <= n) {
        int t = ceil(n / (double)res);//最下面一层的钢筋数
        double bnew = (t - 1) * space + 2 * c + t * d;
        double hnew = res * d + 2 * c + (res - 1) * space;
        if (bnew <= b && hnew <= h)return res;
        res++;
    }
    return 0;
}
