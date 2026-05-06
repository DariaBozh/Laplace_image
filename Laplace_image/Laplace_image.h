#pragma once
#include <iostream>
#include <QtWidgets/QMainWindow>
#include <QtWidgets>
#include "ui_Laplace_image.h"

#include <random>
#include <Eigen/Sparse>
#include <Eigen/SparseLU>
#include <Eigen/IterativeLinearSolvers>
#include <Eigen/Dense>

enum ProcessMode { Gray, RGB };

class Laplace_image : public QMainWindow
{
    Q_OBJECT

public:
    Laplace_image(QWidget *parent = nullptr);
    ~Laplace_image();

    void load(double* u, std::string filename, int* height, int* width, ProcessMode processMode);

    void randomlyRemove(int* Mask, double* u, int range, int p);
    void restore(int* Mask, double* u, int height, int width, ProcessMode processMode);
    void smooth(ProcessMode processMode);

    void refreshDisplay();

private:
    Ui::Laplace_imageClass ui;

    QGraphicsScene* scene;
    QGraphicsPixmapItem* pixmapItem; //rendering

    QImage originalImage;
    QImage currentImage; //calculations (bytes)
    
    int height = 0;
    int width = 0;
    int range = 0; // height*width

    std::vector<double> u; //black-white
    std::vector<double> u_R, u_G, u_B;

    std::vector<int> Mask;

    ProcessMode mode;

public slots:
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_tbRemove_clicked();
    void on_tbRestore_clicked();
    void on_tbSmooth_clicked();
};