#include "svmprocess.h"

Mat gray;
Rect grayRoiRect;

Ptr<SVM> loadSVMModel(const std::string& modelPath)
{
    Ptr<SVM> svm = SVM::load(modelPath);

    if (svm.empty()) {
        std::cout << "SVM 模型加载失败，请检查路径：" << modelPath << std::endl;
        return nullptr;
    }

    std::cout << "SVM 模型加载成功！" << std::endl;
    std::cout << "SVM var count = " << svm->getVarCount() << std::endl;

    return svm;
}

bool getGrayRoi(const Mat& img)
{
    Mat grayImg;
    toothKmeans(img);
    findRoi(img);
    Rect roi = drawRoi(img);
    Rect imageRect(0, 0, img.cols, img.rows);
    grayRoiRect = roi & imageRect;
    if (grayRoiRect.area() <= 0) {
        std::cout << "ROI 提取失败" << std::endl;
        return false;
    }
    cvtColor(img, grayImg, COLOR_BGR2GRAY);
    gray = grayImg(grayRoiRect).clone();
    return true;
}

std::vector<CandidateBox> detectCandidate(Ptr<SVM> svm)
{
    std::vector<CandidateBox> candidates;

    HOGDescriptor hog(
        Size(128, 128),
        Size(16, 16),
        Size(8, 8),
        Size(8, 8),
        9
    );

    int winW = 128;
    int winH = 128;
    int stride = 16;

    for (int y = 0; y <= gray.rows - winH; y += stride) {
        for (int x = 0; x <= gray.cols - winW; x += stride) {

            Rect winRect(x, y, winW, winH);
            Mat window = gray(winRect).clone();

            std::vector<float> descriptors;
            hog.compute(window, descriptors);

            Mat sample(1, descriptors.size(), CV_32F);

            for (int i = 0; i < descriptors.size(); i++) {
                sample.at<float>(0, i) = descriptors[i];
            }

            float raw = svm->predict(sample, noArray(), StatModel::RAW_OUTPUT);

            CandidateBox cand;
            cand.box = winRect;
            cand.score = raw;

            candidates.push_back(cand);
        }
    }

    std::cout << "all windows = " << candidates.size() << std::endl;

    return candidates;
}


float calcIoU(const Rect& a, const Rect& b)
{
    int x1 = std::max(a.x, b.x);
    int y1 = std::max(a.y, b.y);
    int x2 = std::min(a.x + a.width, b.x + b.width);
    int y2 = std::min(a.y + a.height, b.y + b.height);

    int interW = std::max(0, x2 - x1);
    int interH = std::max(0, y2 - y1);
    int interArea = interW * interH;

    int unionArea = a.area() + b.area() - interArea;

    if (unionArea == 0) return 0.0f;

    return (float)interArea / unionArea;
}

std::vector<CandidateBox> applyNMS(std::vector<CandidateBox> candidates, float iouThresh)
{
    std::vector<CandidateBox> result;

    sort(candidates.begin(), candidates.end(),
        [](const CandidateBox& a, const CandidateBox& b) {
            return a.score < b.score;   // 分数越小越好
        });

    std::vector<bool> removed(candidates.size(), false);

    for (int i = 0; i < candidates.size(); i++) {
        if (removed[i]) continue;

        result.push_back(candidates[i]);

        for (int j = i + 1; j < candidates.size(); j++) {
            if (removed[j]) continue;

            float iou = calcIoU(candidates[i].box, candidates[j].box);
            if (iou > iouThresh) {
                removed[j] = true;
            }
        }
    }

    return result;
}

std::vector<CandidateBox> selectTopKCandidates(std::vector<CandidateBox> candidates, int topK)
{
    std::sort(
        candidates.begin(),
        candidates.end(),
        [](const CandidateBox& a, const CandidateBox& b) {
            return a.score < b.score;   // 当前实验结果：score 越小越好
        }
    );

    if (topK > candidates.size()) {
        topK = candidates.size();
    }

    std::vector<CandidateBox> topCandidates(
        candidates.begin(),
        candidates.begin() + topK
    );

    return topCandidates;
}

Mat drawResult(const Mat& img, const std::vector<CandidateBox>& finalBoxes)
{
    Mat show = img.clone();

    for (int i = 0; i < finalBoxes.size(); i++) {
        Rect localBox = finalBoxes[i].box;

        Rect globalBox(
            localBox.x + grayRoiRect.x,
            localBox.y + grayRoiRect.y,
            localBox.width,
            localBox.height
        );

        rectangle(show, globalBox, Scalar(0, 255, 0), 2);

        std::cout << "final box " << i
            << " local = " << localBox
            << " global = " << globalBox
            << " score = " << finalBoxes[i].score
            << std::endl;
    }

    return show;
}
