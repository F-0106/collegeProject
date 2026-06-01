#include "videoprocess.h"

Mat processFrame(Mat& img, Ptr<SVM> svm)
{
    gray = Mat();
    grayRoiRect = Rect();
    resize(img, img, Size(1272, 1080));
    if (!getGrayRoi(img)) {
        return img;
    }

    std::vector<CandidateBox> candidates = detectCandidate(svm);
    std::vector<CandidateBox> topCandidates = selectTopKCandidates(candidates, 10);
    std::vector<CandidateBox> finalBoxes = applyNMS(topCandidates, 0.3f);
    return drawResult(img, finalBoxes);
}

void makeVideo(const std::string& inputPath, const std::string& outputPath, Ptr<SVM> svm)
{
    VideoCapture cap(inputPath);

    if (!cap.isOpened()) {
        std::cout << "视频打开失败" << std::endl;
        return;
    }

    double fps = cap.get(CAP_PROP_FPS);

    Size outputSize(1272, 1080);

    VideoWriter writer(
        outputPath,
        VideoWriter::fourcc('m', 'p', '4', 'v'),
        fps,
        outputSize
    );

    if (!writer.isOpened()) {
        std::cout << "输出视频创建失败" << std::endl;
        return;
    }

    Mat frame;
    Mat lastResult;
    int frameIndex = 0;

    while (cap.read(frame)) {
        Mat result;
        if (frameIndex % 10 == 0) {
            result = processFrame(frame, svm);
            lastResult = result.clone();
        }
        else {
            Mat img = frame.clone();
            resize(img, img, Size(1272, 1080));
            result = img;
        }

        writer.write(result);

        frameIndex++;
        std::cout << "processed frame: " << frameIndex << std::endl;
    }

    cap.release();
    writer.release();

    std::cout << "视频检测完成" << std::endl;
}
