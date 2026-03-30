#include "drawWidget.h"

void DrawWidget::setData(const elements& data)
{
    mdata = data;
    update();
}

void DrawWidget::clear() {
    mdata.hasData = false;
    update();
}

void DrawWidget::paintEvent(QPaintEvent* event)
{
	
		Q_UNUSED(event);
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing); 
		if (!mdata.hasData) {
			return;
		}
		drawMain(&painter);
	
}

QPixmap DrawWidget::renderPix(int vm)
{
    QPixmap pixmap(size());
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    if (mdata.hasData) {
        switch (vm) {
        case 0: drawMain(&painter); break;  
        case 1: drawSect(&painter); break;
        }
    }
    return pixmap;
}

void DrawWidget::drawMain(QPainter* painter)
{
    double scale = qMin(width() / (mdata.b * 1.2), height() / (mdata.h * 3.0));
    scale = qMin(scale, 1.0); // 最大1:1缩放

    // 移动绘图原点到左下角
    painter->save();
    painter->translate(width() / 2.0, height() / 2.0);
    painter->scale(scale, -scale); // Y轴翻转（让向上为正）
    painter->translate(-mdata.b / 2.0, -mdata.h / 2.0);

    double centerX = width() / 2.0;
    double centerY = height() / 2.0;

    QRectF beamRect(0, 0, mdata.b, mdata.h);
    painter->setBrush(Qt::blue);
    painter->setPen(Qt::black);
    painter->drawRect(beamRect);

    double rebarRadius = mdata.dia / 2;
    painter->setBrush(Qt::red);
    painter->setPen(Qt::NoPen);

    // 钢筋均匀分布在梁底部
    double t = ceil(mdata.n / mdata.layer);
    for (int i = 0; i < mdata.layer; ++i) {
        double y = i * (mdata.dia + mdata.space) + c + rebarRadius;
        if (i == mdata.layer - 1)t = mdata.n - i * t;
        for (int k = 0; k < t; ++k) {
            double space = 0.0;
            if (t != 1)space = std::max(mdata.space, (mdata.b - 2 * c - t * mdata.dia) / (t - 1));
            else if (t == 1)space = mdata.b * 1.0 / 2;
            double x = c + (k + 0.5) * mdata.dia + k * space;
            if (t == 1)x = space;
            painter->drawEllipse(QPointF(x, y), rebarRadius, rebarRadius);
        }
    }
    double Left = centerX - (mdata.b * scale) / 2;
    double Right = centerX + (mdata.b * scale) / 2;
    double Top = centerY - (mdata.h * scale) / 2;
    double Bottom = centerY + (mdata.h * scale) / 2;

    painter->restore();
    painter->setPen(Qt::black);
    painter->drawText(Left + mdata.b * scale / 2 - 20, Bottom + 30,
        QString("%1mm").arg(mdata.b));
    painter->drawText(Right + 20, Top + mdata.h * scale / 2 + 5,
        QString("%1mm").arg(mdata.h));
    painter->drawText(Right + 20, Top + mdata.h * scale / 2 - 20,
        QString("%1Φ%2").arg(mdata.n).arg(mdata.dia));
}

void DrawWidget::drawSect(QPainter* painter)
{
    const double marginRatio = 0.8;
    double scaleW = (width() * marginRatio) / mdata.L;  
    double scaleH = (height() * marginRatio) / mdata.h; 
    double scale = qMin(scaleW, scaleH);         
    scale = qMin(scale, 1.0);

    double rectW = mdata.L * scale; 
    double rectH = mdata.h * scale; 
    double winCenterX = width() / 2.0;
    double winCenterY = height() / 2.0;

    QRectF sectionRect(
        winCenterX - rectW / 2.0,
        winCenterY - rectH / 2.0,
        rectW,
        rectH
    );

    painter->save();
    painter->setBrush(Qt::blue);
    painter->setPen(Qt::black);
    painter->drawRect(sectionRect);

    // 钢筋均匀分布在截面底部
    painter->setPen(QPen(Qt::red, 2));
    for (int i = 0; i < mdata.layer; ++i) {
        double barY = sectionRect.bottom() - (2 * c + mdata.dia / 2 + i * (mdata.dia + mdata.space)) * scale;
        painter->drawLine(sectionRect.left(), barY, sectionRect.right(), barY);
    }
    painter->restore();

    painter->setPen(Qt::black);
    painter->drawText(sectionRect.center().x() - 30, sectionRect.bottom() + 25,
        QString("%1mm").arg(mdata.L));
    painter->drawText(sectionRect.left() - 70, sectionRect.center().y() + 5,
        QString("%1mm").arg(mdata.h));
}

