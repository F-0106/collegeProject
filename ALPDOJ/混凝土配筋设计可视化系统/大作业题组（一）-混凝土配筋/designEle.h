#pragma once

struct elements
{
	//接收传入的数据
	double L; //跨度（mm)
	double P; //荷载(kN)
	double d; //车轮间距(m)

	//绘图用数据
	int h;
	int b;
	double Mu;
	double Mmax;
	double n;
	double space;
	double dia;
	double layer;
	double value;
	bool hasData = false;
};