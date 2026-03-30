#include "stdSize.h"
#include <iostream>

double stdWidth(double b)
{
    const std::vector<int> speWidth = { 150,180,200,220,250 };
    if (b <= 250) {
        int res = 150;
        int mdif = 1e3;
        for (int w : speWidth) {
            if (abs((int)b - w) < mdif) {
                res = w;
                mdif = abs((int)b - w);
            }
        }
        return res;
    }
    return ceil((b - 250) / 50.0) * 50 + 250;
}

double stdHeight(double h)
{
    if (h <= 200)return 200;
    else if (h <= 800)return ceil((h - 200) / 50.0) * 50 + 200;
    else return ceil((h - 800) / 100.0) * 100 + 800;
}
