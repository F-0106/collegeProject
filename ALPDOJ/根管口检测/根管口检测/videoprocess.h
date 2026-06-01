#pragma once
#include "svmprocess.h"

Mat processFrame(Mat& img, Ptr<SVM> svm);
void makeVideo(const std::string& inputPath, const std::string& outputPath, Ptr<SVM> svm);