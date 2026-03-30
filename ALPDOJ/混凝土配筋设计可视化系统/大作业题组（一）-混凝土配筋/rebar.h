#pragma once
#include "conParams.h"

struct Rebar {
	int n;			//钢筋数量
	int d;			//钢筋直径
	bool fitable;	//钢筋是否放得下
	int layer;		//钢筋层数
	double space;	//钢筋间距，多排时计算h0
	Rebar() : n(0), d(0), fitable(false), layer(0), space(0) {};
};

Rebar calLayout(double b, double As,double h);
int calLayer(int n, double space, double b,int d,double h);