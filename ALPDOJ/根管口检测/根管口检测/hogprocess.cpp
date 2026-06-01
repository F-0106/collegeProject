#include "hogprocess.h"

static Mat rgb;
static Mat gradient;
static Mat theta;
static std::vector<float> hogFeature;

void getRgbRoi(const Mat& img)
{
    Mat rgbImg;
    toothKmeans(img);
    findRoi(img);
    Rect roi = drawRoi(img);
    Rect imageRect(0, 0, img.cols, img.rows);
    Rect safePulpRect = roi & imageRect;
    cvtColor(img, rgbImg, COLOR_BGR2RGB);
    rgb = rgbImg(safePulpRect).clone();
}

void calGradient()
{
    Mat rgbFloat;
    rgb.convertTo(rgbFloat, CV_32F);

    std::vector<Mat> channels;
    split(rgbFloat, channels); // channels[0]=R, channels[1]=G, channels[2]=B

    gradient = Mat::zeros(rgb.rows, rgb.cols, CV_32F);
    theta = Mat::zeros(rgb.rows, rgb.cols, CV_32F);

    for (int i = 1; i < rgb.rows - 1; i++)
    {
        for (int j = 1; j < rgb.cols - 1; j++)
        {
            float bestMag = -1.0f;
            float bestAngle = 0.0f;

            for (int c = 0; c < 3; c++)
            {
                float Gx = channels[c].at<float>(i, j + 1) - channels[c].at<float>(i, j - 1);
                float Gy = channels[c].at<float>(i + 1, j) - channels[c].at<float>(i - 1, j);

                float mag = sqrt(Gx * Gx + Gy * Gy);

                float angle = atan2(Gy, Gx) * 180.0f / CV_PI;

                if (angle < 0) {
                    angle += 180.0f;
                }

                if (angle >= 180.0f) {
                    angle -= 180.0f;
                }

                if (mag > bestMag)
                {
                    bestMag = mag;
                    bestAngle = angle;
                }
            }

            gradient.at<float>(i, j) = bestMag;
            theta.at<float>(i, j) = bestAngle;
        }
    }

}

void calFeature()
{
    int cellSize = 8;
    int numBins = 9;
    int blockCellSize = 2; // block = 2×2 cells

    int cellsX = gradient.cols / cellSize;
    int cellsY = gradient.rows / cellSize;

    if (cellsX < 2 || cellsY < 2) {
        std::cout << "ROI 太小，无法组成 16x16 block" << std::endl;
        return;
    }

    std::vector<std::vector<float>> cellHists(
        cellsX * cellsY,
        std::vector<float>(numBins, 0.0f)
    );

    // 计算每个 cell 的 9 维方向直方图
    for (int cy = 0; cy < cellsY; cy++) {
        for (int cx = 0; cx < cellsX; cx++) {

            int cellX = cx * cellSize;
            int cellY = cy * cellSize;

            std::vector<float> hist(numBins, 0.0f);

            for (int y = cellY; y < cellY + cellSize; y++) {
                for (int x = cellX; x < cellX + cellSize; x++) {

                    float mag = gradient.at<float>(y, x);
                    float angle = theta.at<float>(y, x);

                    int bin = int(angle / 20.0f);
                    if (bin >= numBins) {
                        bin = numBins - 1;
                    }

                    hist[bin] += mag;
                }
            }

            int index = cy * cellsX + cx;
            cellHists[index] = hist;
        }
    }

    // 每 2×2 个 cell 组成一个 block，并做归一化
    hogFeature.clear();

    for (int by = 0; by <= cellsY - blockCellSize; by++) {
        for (int bx = 0; bx <= cellsX - blockCellSize; bx++) {

            std::vector<float> blockFeature;

            for (int dy = 0; dy < blockCellSize; dy++) {
                for (int dx = 0; dx < blockCellSize; dx++) {

                    int cy = by + dy;
                    int cx = bx + dx;

                    int cellIndex = cy * cellsX + cx;

                    for (int k = 0; k < numBins; k++) {
                        blockFeature.push_back(cellHists[cellIndex][k]);
                    }
                }
            }

            // L2 归一化
            float sumSq = 0.0f;

            for (float v : blockFeature) {
                sumSq += v * v;
            }

            float norm = sqrt(sumSq + 1e-6f);

            for (float& v : blockFeature) {
                v /= norm;
            }

            // 加入 HOG 特征
            hogFeature.insert(
                hogFeature.end(),
                blockFeature.begin(),
                blockFeature.end()
            );
        }
    }

    std::cout << "cellsX = " << cellsX << std::endl;
    std::cout << "cellsY = " << cellsY << std::endl;
    std::cout << "HOG feature size = " << hogFeature.size() << std::endl;
}

void drawHog()
{
    int cellSize = 8;
    int numBins = 9;

    int cellsX = gradient.cols / cellSize;
    int cellsY = gradient.rows / cellSize;

    Mat canvas;
    cvtColor(rgb, canvas, COLOR_RGB2BGR);

    for (int cy = 0; cy < cellsY; cy++) {
        for (int cx = 0; cx < cellsX; cx++) {

            int cellX = cx * cellSize;
            int cellY = cy * cellSize;

            float hist[9] = { 0 };

            for (int y = cellY; y < cellY + cellSize; y++) {
                for (int x = cellX; x < cellX + cellSize; x++) {
                    float mag = gradient.at<float>(y, x);
                    float angle = theta.at<float>(y, x);

                    int bin = int(angle / 20.0f);
                    if (bin >= numBins) {
                        bin = numBins - 1;
                    }

                    hist[bin] += mag;
                }
            }
            float sumHist = 0.0f;
            for (int k = 0; k < numBins; k++) {
                sumHist += hist[k];
            }
            if (sumHist < 50.0f) {
                continue;
            }
            int maxBin = 0;
            float maxValue = hist[0];

            for (int k = 1; k < numBins; k++) {
                if (hist[k] > maxValue) {
                    maxValue = hist[k];
                    maxBin = k;
                }
            }

            float angle = maxBin * 20.0f + 10.0f;
            float rad = angle * CV_PI / 180.0f;

            Point center(
                cellX + cellSize / 2,
                cellY + cellSize / 2
            );

            float strength = maxValue / (sumHist + 1e-6f);
            int length = int((cellSize / 2) * strength);

            if (length < 2) {
                continue;
            }

            Point p1(
                center.x - int(length * cos(rad)),
                center.y - int(length * sin(rad))
            );

            Point p2(
                center.x + int(length * cos(rad)),
                center.y + int(length * sin(rad))
            );

            line(canvas, p1, p2, Scalar(0, 0, 255), 1);
        }
    }

    imwrite("HOG特征图.png", canvas);
}

void visualizePixelGradient()
{
    Mat magShow;
    normalize(gradient, magShow, 0, 255, NORM_MINMAX, CV_8UC1);

    Mat hsv = Mat::zeros(gradient.rows, gradient.cols, CV_8UC3);

    for (int y = 0; y < gradient.rows; y++) {
        for (int x = 0; x < gradient.cols; x++) {
            float angle = theta.at<float>(y, x);   // [0, 180)
            uchar mag = magShow.at<uchar>(y, x);   // [0, 255]

            // OpenCV HSV 的 H 范围是 [0,179]
            uchar hue = saturate_cast<uchar>(angle / 180.0f * 179.0f);

            hsv.at<Vec3b>(y, x) = Vec3b(hue, 255, mag);
        }
    }

    Mat bgrVis;
    cvtColor(hsv, bgrVis, COLOR_HSV2BGR);
    imwrite("梯度大小图.png", bgrVis);
}