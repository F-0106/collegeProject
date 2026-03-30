#pragma once
#include <QWidget>
#include <QPixmap>
#include <QPainter>
#include "beamGA.h"

class DrawWidget : public QWidget
{
	Q_OBJECT
public:
	explicit DrawWidget(QWidget* parent = nullptr) :QWidget(parent) {
		setAutoFillBackground(true);
		QPalette pal = palette();
		pal.setColor(QPalette::Window, Qt::white);
		setPalette(pal);
	}
	void setData(const elements& data);
	void clear();
	QPixmap expoMain() { return renderPix(0); }
	QPixmap expoSect() { return renderPix(1); }
protected:
	void paintEvent(QPaintEvent* event) override;
private:
	elements mdata;
	QPixmap renderPix(int vm);
	void drawMain(QPainter* painter);
	void drawSect(QPainter* painter);
};

