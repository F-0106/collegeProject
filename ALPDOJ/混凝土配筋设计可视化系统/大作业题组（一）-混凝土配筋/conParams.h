#pragma once
#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include <vector>
const double c = 30;				//保护层厚度		
const double ksib = 0.518;			//最大受压区高度
const double pmin = 0.0029;			//最小配筋率
const double a1 = 1;
const double gamma = 25 * 1e-6;		//混凝土容重
const double fy = 400;				//钢筋屈服强度
const double fc = 32.4;				//混凝土抗压强度
const double ft = 2.65;				//混凝土抗拉强度
const double hmax = 10.0;			//动态调整h，b的取值范围
const double hmin = 40.0;
const double bmax = 1.5;
const double bmin = 3;