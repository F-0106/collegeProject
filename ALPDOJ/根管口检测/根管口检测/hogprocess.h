#pragma once
#include "preprocess.h"

void getRgbRoi(const Mat& img);	//提取出RGB的ROI
void calGradient();				//分别计算三个通道的梯度并选择幅值最大的作为结果
void calFeature();				//计算特征向量
void drawHog();					//绘制可视化图像，根据方向直方图选择每个celll的主方向
void visualizePixelGradient();	//色相表示方向，亮度表示梯度幅值