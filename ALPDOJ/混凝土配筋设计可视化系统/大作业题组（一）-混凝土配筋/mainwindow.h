#pragma once
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QFileDialog>
#include <QDir>
#include <QPushButton>
#include <QDesktopServices>
#include "drawWidget.h"
#include "beamGA.h"


class MainWindow :public QMainWindow {
	Q_OBJECT
public:
	MainWindow(QWidget* parent = nullptr) :QMainWindow(parent) {
		initUI();
		initConnect();
	}
private slots:
	void passParams();
	void showResult();
	void generateRepo();
private:
	QLineEdit* spanEdit;    // ¿ç¶ÈÊäÈë¿ò
	QLineEdit* loadEdit;    // ºÉÔØÊäÈë¿ò
	QLineEdit* spacingEdit; // ¼ä¾àÊäÈë¿ò
	QPushButton* passBtn;
	QPushButton* resultBtn;
	QPushButton* reportBtn;

	DrawWidget* drawWidget; // »æÍ¼¿Ø¼ş
	elements curData;

	void initUI();
	void initConnect();
};