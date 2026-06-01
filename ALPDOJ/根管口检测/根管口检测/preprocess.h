#pragma once
#include <opencv2\opencv.hpp>
#include <opencv2/ml.hpp>
#include <iostream>
#include <vector>

using namespace cv;

void toothKmeans(const Mat& img);	//通过k-means算法生成牙齿和髓室mask
void findRoi(const Mat& img);		//找到牙齿ROI从而找到髓室ROI
Rect drawRoi(const Mat& img);		//返回髓室ROI在原图坐标中的位置矩形