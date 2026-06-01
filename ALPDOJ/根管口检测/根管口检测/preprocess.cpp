#include "preprocess.h"


Mat toothMask, pulpMask;	//牙齿区域mask和髓室mask
Mat show, stats;			//每个区域的外接框、面积等信息
Rect safeToothRect;			//牙齿ROI在原图中的矩形区域
Rect unionRect;				//合并多个髓室连通后的ROI
bool hasCandidate = false;	//是否成功找到髓室
int bestLabel = -1;			//牙齿连通编号
int K = 3;					//k-means聚类类别数

void toothKmeans(const Mat& img)
{
	Mat lab;
	Mat labels;
	Mat centers;
	cvtColor(img, lab, COLOR_BGR2Lab);
	Mat samples(lab.rows * lab.cols, 3, CV_32F);

	for (int y = 0; y < lab.rows; y++) {
		for (int x = 0; x < lab.cols; x++) {
			Vec3b color = lab.at<Vec3b>(y, x);

			int index = y * lab.cols + x;

			samples.at<float>(index, 0) = color[0];
			samples.at<float>(index, 1) = color[1];
			samples.at<float>(index, 2) = color[2];
		}
	}

	TermCriteria criteria(TermCriteria::EPS + TermCriteria::MAX_ITER, 10, 1.0);
	kmeans(samples, K, labels, criteria, 3, KMEANS_PP_CENTERS, centers);
	for (int k = 0; k < K; k++) {
		Mat mask = Mat::zeros(img.rows, img.cols, CV_8UC1);

		for (int y = 0; y < img.rows; y++) {
			for (int x = 0; x < img.cols; x++) {
				int index = y * img.cols + x;
				int label = labels.at<int>(index, 0);

				if (label == k) {
					mask.at<uchar>(y, x) = 255;
				}
			}
		}

		if (k == 0) {
			toothMask = mask.clone();
		}
		if (k == 2) {
			pulpMask = mask.clone();
		}
	}

	Mat kerMask = getStructuringElement(MORPH_ELLIPSE, Size(15, 15));
	morphologyEx(toothMask, toothMask, MORPH_CLOSE, kerMask);
	morphologyEx(toothMask, toothMask, MORPH_OPEN, kerMask);
}

void findRoi(const Mat& img)
{
	Mat labelsCC, centroids;
	int numLabels = connectedComponentsWithStats(toothMask, labelsCC, stats, centroids, 8, CV_32S);

	Point2d imageCenter(img.cols / 2.0, img.rows / 2.0);

	double bestScore = 1e18;
	for (int i = 1; i < numLabels; i++) {
		int x = stats.at<int>(i, CC_STAT_LEFT);
		int y = stats.at<int>(i, CC_STAT_TOP);
		int w = stats.at<int>(i, CC_STAT_WIDTH);
		int h = stats.at<int>(i, CC_STAT_HEIGHT);
		int area = stats.at<int>(i, CC_STAT_AREA);

		if (area < 50000) {
			continue;
		}

		Point2d center(centroids.at<double>(i, 0), centroids.at<double>(i, 1));

		double dx = center.x - imageCenter.x;
		double dy = center.y - imageCenter.y;
		double distance = dx * dx + dy * dy;

		if (distance < bestScore) {
			bestScore = distance;
			bestLabel = i;
		}
	}

	int x = stats.at<int>(bestLabel, CC_STAT_LEFT);
	int y = stats.at<int>(bestLabel, CC_STAT_TOP);
	int w = stats.at<int>(bestLabel, CC_STAT_WIDTH);
	int h = stats.at<int>(bestLabel, CC_STAT_HEIGHT);

	bestScore = 1e18;
	Rect toothRect(x, y, w, h);
	rectangle(show, toothRect, Scalar(0, 0, 255), 3);

	Rect imageRect(0, 0, img.cols, img.rows);
	Rect safeToothRect = toothRect & imageRect;
	Mat pulpROI = pulpMask(safeToothRect).clone();

	Mat kernelPulp = getStructuringElement(MORPH_ELLIPSE, Size(7, 7));

	morphologyEx(pulpMask, pulpMask, MORPH_CLOSE, kernelPulp);
	morphologyEx(pulpMask, pulpMask, MORPH_OPEN, kernelPulp);

	Mat labelsPulp, statsPulp, centroidsPulp;
	int numPulpLabels = connectedComponentsWithStats(pulpMask, labelsPulp, statsPulp, centroidsPulp, 8, CV_32S);

	Point2d roiCenter(pulpMask.cols / 2.0, pulpMask.rows / 2.0);

	for (int i = 1; i < numPulpLabels; i++) {
		int x = statsPulp.at<int>(i, CC_STAT_LEFT);
		int y = statsPulp.at<int>(i, CC_STAT_TOP);
		int w = statsPulp.at<int>(i, CC_STAT_WIDTH);
		int h = statsPulp.at<int>(i, CC_STAT_HEIGHT);
		int area = statsPulp.at<int>(i, CC_STAT_AREA);

		Point2d center(
			centroidsPulp.at<double>(i, 0),
			centroidsPulp.at<double>(i, 1)
		);

		bool touchBorder =
			x <= 5 ||
			y <= 5 ||
			x + w >= pulpMask.cols - 5 ||
			y + h >= pulpMask.rows - 5;

		if (touchBorder) {
			continue;
		}

		if (area < 200) {
			continue;
		}

		// 限制在 toothRect 的中间区域附近
		bool nearCenterX =
			center.x > pulpMask.cols * 0.25 &&
			center.x < pulpMask.cols * 0.75;

		bool nearCenterY =
			center.y > pulpMask.rows * 0.15 &&
			center.y < pulpMask.rows * 0.85;

		if (!nearCenterX || !nearCenterY) {
			continue;
		}

		Rect currentRect(x, y, w, h);

		if (!hasCandidate) {
			unionRect = currentRect;
			hasCandidate = true;
		}
		else {
			unionRect = unionRect | currentRect;
		}
	}
	if (hasCandidate) {
		int padX = 30;
		int padY = 40;

		unionRect.x -= padX;
		unionRect.y -= padY;
		unionRect.width += 2 * padX;
		unionRect.height += 2 * padY;

		Rect roiBoundary(0, 0, pulpMask.cols, pulpMask.rows);
		unionRect = unionRect & roiBoundary;
	}
}

Rect drawRoi(const Mat& img)
{
	show = img.clone();
	if (bestLabel != -1) {
		int x = stats.at<int>(bestLabel, CC_STAT_LEFT);
		int y = stats.at<int>(bestLabel, CC_STAT_TOP);
		int w = stats.at<int>(bestLabel, CC_STAT_WIDTH);
		int h = stats.at<int>(bestLabel, CC_STAT_HEIGHT);

		Rect toothRect(x, y, w, h);
		rectangle(show, toothRect, Scalar(0, 0, 255), 3);

	}
	if (hasCandidate) {
		Rect pulpRect(
			unionRect.x + safeToothRect.x,
			unionRect.y + safeToothRect.y,
			unionRect.width,
			unionRect.height
		);

		rectangle(show, pulpRect, Scalar(255, 0, 0), 3);
		return pulpRect;
	}
	else {
		std::cout << "没有找到合适的髓室候选区域" << std::endl;
	}
}
