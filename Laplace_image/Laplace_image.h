#pragma once
#include <iostream>
#include <QtWidgets/QMainWindow>
#include <QtWidgets>
#include "ui_Laplace_image.h"

#include <random>
#include <Eigen/Sparse>
#include <Eigen/SparseLU>
#include <Eigen/Dense>

class Laplace_image : public QMainWindow
{
    Q_OBJECT

public:
    Laplace_image(QWidget *parent = nullptr);
    ~Laplace_image();

    void load(double* u, std::string filename, int* height, int* width);
    void RandomlyRemove(int* Mask, double* u, int range, int p);

    void restoration(int* Mask, double* u, int height, int width);

    void on_actionOpen_triggered();
    void on_actionSave_triggered();

private:
    Ui::Laplace_imageClass ui;

    QGraphicsScene* scene;
    QImage currentImage;

    static std::mt19937 rng;

    double* u[]; 
};

