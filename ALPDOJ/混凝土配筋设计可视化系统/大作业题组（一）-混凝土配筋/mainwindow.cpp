#include "mainwindow.h"
#include <iostream>

void MainWindow::passParams() {
    std::cout << 0;
    double span = spanEdit->text().toDouble();
    double load = loadEdit->text().toDouble();
    double spacing = spacingEdit->text().toDouble();


    curData.L = span;
    curData.P = load * 1e3;
    curData.d = spacing * 1e3;
    curData.hasData = false; // 还未计算，标记为无效

    spanEdit->clear();
    loadEdit->clear();
    spacingEdit->clear();
    drawWidget->clear();
}

void MainWindow::showResult()
{
    std::cout << "L:" << curData.L << " P:" << curData.P;
    if (curData.L <= 0 || curData.P <= 0) {
        std::cout << 5;
        return;
    }
    beamGA beam(curData);
    Individual res = beam.run();
    curData = beam.getData();
    
    std::cout << 4;
    drawWidget->setData(curData);
}

void MainWindow::generateRepo()
{
    std::cout << 3;
    // 1. 检查是否有计算结果
    if (!curData.hasData) {
        std::cout << "3!data";
        return;
    }
    QString htmlPath = QFileDialog::getSaveFileName(this, "report", "report.html", "HTML文件 (*.html)");
    if (htmlPath.isEmpty()) return;


    // 2. 创建临时目录存储图片
    QFileInfo htmlFileInfo(htmlPath);
    QString reportDir = htmlFileInfo.absolutePath() + "/report";
    QDir().mkpath(reportDir); // 创建目录

    QString mainImgPath = reportDir + "/主视图.png";
    QString sectionImgPath = reportDir + "/截面图.png";

    bool mainImgok=drawWidget->expoMain().save(mainImgPath);
    bool sectImgok=drawWidget->expoSect().save(sectionImgPath);
    if (!mainImgok)std::cout << "主失败";
    if (!sectImgok)std::cout << "侧失败";
    QString mainImgRelPath = QDir(htmlFileInfo.absolutePath()).relativeFilePath(mainImgPath);
    QString sectImgRelPath = QDir(htmlFileInfo.absolutePath()).relativeFilePath(sectionImgPath);
    QString htmlContent = R"(
    <html>
    <head>
        <meta charset="UTF-8">
        <title>混凝土配筋计算报告</title>
        <style>
            body { font-family: "微软雅黑", sans-serif; padding: 50px; background: #f5f5f5; }
            .container { max-width: 1000px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
            h1 { color: #333; text-align: center; border-bottom: 2px solid #0078d7; padding-bottom: 15px; }
            .info { margin: 30px 0; }
            table { width: 100%; border-collapse: collapse; margin: 20px 0; }
            th, td { border: 1px solid #ddd; padding: 12px; text-align: center; }
            th { background: #f2f2f2; color: #333; }
            .img-group { text-align: center; margin: 40px 0; }
            .img-group img { max-width: 80%; margin: 20px 0; border: 1px solid #ddd; }
            .caption { font-weight: bold; color: #666; margin-top: 10px; }
        </style>
    </head>
    <body>
        <div class="container">
            <h1>混凝土配筋计算报告</h1>
            <div class="info">
                <p>生成时间：)" + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") + R"(</p>
            </div>

            <h2>一、输入参数</h2>
            <table>
                <tr><th>参数名称</th><th>数值</th><th>单位</th></tr>
                <tr><td>桥梁跨度</td><td>)" + QString::number(curData.L) + R"(</td><td>mm</td></tr>
                <tr><td>集中荷载</td><td>)" + QString::number(curData.P / 1e3) + R"(</td><td>kN</td></tr>
                <tr><td>车轮间距</td><td>)" + QString::number(curData.d / 1e3) + R"(</td><td>m</td></tr>
            </table>

            <h2>二、计算结果</h2>
            <table>
                <tr><th>参数名称</th><th>数值</th><th>单位</th></tr>
                <tr><td>梁宽</td><td>)" + QString::number(curData.b) + R"(</td><td>mm</td></tr>
                <tr><td>梁高</td><td>)" + QString::number(curData.h) + R"(</td><td>mm</td></tr>
                <tr><td>钢筋数量</td><td>)" + QString::number(curData.n) + R"(</td><td>根</td></tr>
                <tr><td>钢筋直径</td><td>)" + QString::number(curData.dia) + R"(</td><td>mm</td></tr>
                <tr><td>最大弯矩</td><td>)" + QString::number(curData.Mmax) + R"(</td><td>N·mm</td></tr>
                <tr><td>最大承载力</td><td>)" + QString::number(curData.Mu) + R"(</td><td>N·mm</td></tr>
                <tr><td>总报价</td><td>)" + QString::number(curData.value) + R"(</td><td>元</td></tr>
            </table>

            <h2>三、多视角图形</h2>
            <div class="img-group">
                <img src=")" + mainImgRelPath + R"(" alt="主视图">
                <div class="caption">图1：桥梁配筋主视图</div>
            </div>
            <div class="img-group">
                <img src=")" + sectImgRelPath + R"(" alt="截面图">
                <div class="caption">图3：梁截面配筋图</div>
            </div>
        </div>
    </body>
    </html>
    )";

   
    QFile file(htmlPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << htmlContent;
        file.close();

        // 自动打开报告
        QDesktopServices::openUrl(QUrl::fromLocalFile(htmlPath));
    }
}

void MainWindow::initUI()
{
    setWindowTitle("混凝土配筋系统");
    resize(1000, 600);
    // 中心部件+主布局（左右分栏）
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);

    // 左侧：输入区+按钮区（垂直布局）
    QVBoxLayout* leftLayout = new QVBoxLayout;

    // 1. 几何参数输入组
    QGroupBox* geoGroup = new QGroupBox("几何参数输入");
    QVBoxLayout* geoLayout = new QVBoxLayout(geoGroup);
    // 跨度输入
    QHBoxLayout* spanLayout = new QHBoxLayout;
    spanLayout->addWidget(new QLabel("跨度 (mm)："));
    spanEdit = new QLineEdit;
    spanLayout->addWidget(spanEdit);
    geoLayout->addLayout(spanLayout);

    // 2. 荷载参数输入组
    QGroupBox* loadGroup = new QGroupBox("荷载与边界输入");
    QVBoxLayout* loadLayout = new QVBoxLayout(loadGroup);
    // 集中荷载输入
    QHBoxLayout* loadLayoutRow = new QHBoxLayout;
    loadLayoutRow->addWidget(new QLabel("集中荷载 (kN)："));
    loadEdit = new QLineEdit;
    loadLayoutRow->addWidget(loadEdit);
    loadLayout->addLayout(loadLayoutRow);
    // 车轮间距输入
    QHBoxLayout* spacingLayout = new QHBoxLayout;
    spacingLayout->addWidget(new QLabel("车轮间距 (m)："));
    spacingEdit = new QLineEdit;
    spacingLayout->addWidget(spacingEdit);
    loadLayout->addLayout(spacingLayout);

    // 3. 按钮组（水平布局）
    QHBoxLayout* btnLayout = new QHBoxLayout;
    QPushButton* passBtn = new QPushButton("传递参数");
    QPushButton* resultBtn = new QPushButton("显示结果");
    QPushButton* reportBtn = new QPushButton("report");
    btnLayout->addWidget(passBtn);
    btnLayout->addWidget(resultBtn);
    btnLayout->addWidget(reportBtn);

    // 左侧布局组装
    leftLayout->addWidget(geoGroup);
    leftLayout->addWidget(loadGroup);
    leftLayout->addLayout(btnLayout);
    leftLayout->setStretch(0, 1);
    leftLayout->setStretch(1, 1);
    leftLayout->setStretch(2, 0);

    // 右侧：绘图控件
    drawWidget = new DrawWidget;

    // 主布局组装（左侧占30%，右侧占70%）
    mainLayout->addLayout(leftLayout, 3);
    mainLayout->addWidget(drawWidget, 7);
}

void MainWindow::initConnect()
{
    QList<QPushButton*> btns = findChildren<QPushButton*>();
    connect(btns[0], &QPushButton::clicked, this, &MainWindow::passParams);
    connect(btns[1], &QPushButton::clicked, this, &MainWindow::showResult);
    connect(btns[2], &QPushButton::clicked, this, &MainWindow::generateRepo);
}
