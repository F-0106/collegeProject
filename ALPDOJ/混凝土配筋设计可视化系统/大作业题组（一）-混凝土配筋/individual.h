#pragma once
#include "rebar.h"

//个体
struct Individual {
	double bRaw;	//原生的b
	double hRaw;
	double b;		//规范后的b
	double h;
	double As;		//配筋面积
	Rebar rebar;	//钢筋
	double value;	//造价,即适应度
	bool feasible;	//是否满足所有条件
	double Mmax;	//荷载弯矩值
	double Mu;		//抗弯承载力
	Individual() : bRaw(0), hRaw(0), b(0), h(0), As(0), rebar(), value(1e18), Mmax(0), Mu(0), feasible(false) {};
};