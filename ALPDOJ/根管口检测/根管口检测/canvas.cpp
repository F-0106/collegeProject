#include "canvas.h"

#include <cerrno>
#include <direct.h>
#include <iostream>
#include <limits>
#include <string>

namespace {

    const std::string kResultDir = "result";
    const std::string kImageResultDir = "result\\image";
    const std::string kVideoResultDir = "result\\video";

    bool ensureDirectory(const std::string& path)
    {
        if (_mkdir(path.c_str()) == 0) {
            return true;
        }

        return errno == EEXIST;
    }

    bool ensureResultDirectories()
    {
        return ensureDirectory(kResultDir) &&
            ensureDirectory(kImageResultDir) &&
            ensureDirectory(kVideoResultDir);
    }

    std::string fileStem(const std::string& path)
    {
        size_t slash = path.find_last_of("\\/");
        size_t begin = slash == std::string::npos ? 0 : slash + 1;
        size_t dot = path.find_last_of('.');

        if (dot == std::string::npos || dot < begin) {
            dot = path.size();
        }

        std::string stem = path.substr(begin, dot - begin);
        if (stem.empty()) {
            stem = "output";
        }

        for (char& ch : stem) {
            bool valid =
                (ch >= '0' && ch <= '9') ||
                (ch >= 'A' && ch <= 'Z') ||
                (ch >= 'a' && ch <= 'z') ||
                ch == '_' ||
                ch == '-' ||
                ch == '.';

            if (!valid) {
                ch = '_';
            }
        }

        return stem;
    }

    bool changeDirectory(const std::string& path)
    {
        return _chdir(path.c_str()) == 0;
    }

    std::string currentDirectory()
    {
        char buffer[FILENAME_MAX];
        if (_getcwd(buffer, sizeof(buffer)) == nullptr) {
            return std::string();
        }

        return std::string(buffer);
    }

    void generateHogImages(const Mat& img, const std::string& outputDir)
    {
        std::string oldDir = currentDirectory();
        if (oldDir.empty()) {
            std::cout << "cannot get current directory" << std::endl;
            return;
        }

        if (!changeDirectory(outputDir)) {
            std::cout << "cannot enter output directory: " << outputDir << std::endl;
            return;
        }

        Mat hogInput = img.clone();
        resize(hogInput, hogInput, Size(1272, 1080));
        getRgbRoi(hogInput);
        calGradient();
        calFeature();
        visualizePixelGradient();
        drawHog();

        changeDirectory(oldDir);
    }

    void runImageMode(Ptr<SVM> svm)
    {
        std::string imagePath;
        std::cout << "Input image relative path: ";
        std::getline(std::cin, imagePath);

        Mat img = imread(imagePath);
        if (img.empty()) {
            std::cout << "image read failed: " << imagePath << std::endl;
            return;
        }

        if (!ensureResultDirectories()) {
            std::cout << "result directory create failed" << std::endl;
            return;
        }

        std::string outputDir = kImageResultDir + "\\" + fileStem(imagePath);
        if (!ensureDirectory(outputDir)) {
            std::cout << "image output directory create failed" << std::endl;
            return;
        }

        generateHogImages(img, outputDir);

        Mat detectInput = img.clone();
        Mat result = processFrame(detectInput, svm);
        std::string resultPath = outputDir + "\\detected.png";
        imwrite(resultPath, result);

        std::cout << "image results saved to: " << outputDir << std::endl;
        imshow("Detected Image", result);
        waitKey(0);
    }

    void runVideoMode(Ptr<SVM> svm)
    {
        std::string videoPath;
        std::cout << "Input video relative path: ";
        std::getline(std::cin, videoPath);

        if (!ensureResultDirectories()) {
            std::cout << "result directory create failed" << std::endl;
            return;
        }

        std::string outputPath = kVideoResultDir + "\\" + fileStem(videoPath) + "_result.mp4";
        makeVideo(videoPath, outputPath, svm);
        std::cout << "video result saved to: " << outputPath << std::endl;
    }

    void printMenu()
    {
        std::cout << std::endl;
        std::cout << "============================" << std::endl;
        std::cout << " Root Canal Detection Menu" << std::endl;
        std::cout << "============================" << std::endl;
        std::cout << "1. Image mode" << std::endl;
        std::cout << "2. Video mode" << std::endl;
        std::cout << "3. Exit" << std::endl;
        std::cout << "Select mode: ";
    }

} // namespace

void runCanvas()
{
    Ptr<SVM> svm = loadSVMModel("64_svm_model.xml");
    if (svm == nullptr) {
        return;
    }

    while (true) {
        printMenu();

        int mode = 0;
        if (!(std::cin >> mode)) {
            std::cout << "invalid input" << std::endl;
            return;
        }

        std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

        if (mode == 1) {
            runImageMode(svm);
        }
        else if (mode == 2) {
            runVideoMode(svm);
        }
        else if (mode == 3) {
            std::cout << "exit" << std::endl;
            break;
        }
        else {
            std::cout << "unknown mode" << std::endl;
        }
    }
}
