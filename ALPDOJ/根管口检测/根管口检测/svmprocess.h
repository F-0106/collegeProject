#pragma once
#include "preprocess.h"

using namespace cv::ml;

extern Mat gray;            //灰度图roi
extern Rect grayRoiRect;    //用于将检测坐标返回到原图

struct CandidateBox {
    Rect box;               //候选框，坐标相对于grayroi
    float score;            //SVM分数越小越可能是目标
};

Ptr<SVM> loadSVMModel(const std::string& modelPath);    //加载SVM模型，失败则返回nullptr
bool getGrayRoi(const Mat& img);                        //提取SVM需要的灰度ROI，成功则返回true
std::vector<CandidateBox> detectCandidate(Ptr<SVM> svm);//计算所有的滑动窗口候选框和分数
float calcIoU(const Rect& a, const Rect& b);            //计算两个框的交并比
std::vector<CandidateBox> applyNMS(std::vector<CandidateBox> candidates, float iouThresh);      //进行非极大值抑制
std::vector<CandidateBox> selectTopKCandidates(std::vector<CandidateBox> candidates, int topK); //选择排序后前K个候选框
Mat drawResult(const Mat& img, const std::vector<CandidateBox>& finalBoxes);                    //将最终检测图映射回原图像
