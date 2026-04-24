#include "Laplace_image.h"

Laplace_image::Laplace_image(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    scene = new QGraphicsScene(this);
}

Laplace_image::~Laplace_image()
{}

static std::mt19937 rng(std::random_device{}());

void Laplace_image::load(double* u, std::string filename, int* height, int* width)
{
    currentImage = currentImage.convertToFormat(QImage::Format_Grayscale8);

    //int range = width * height;
}

void Laplace_image::RandomlyRemove(int* Mask, double* u, int range, int p)
{
    int n = range * p / 100;

    //If p > 50, we kipping heigth*width-n pixels

    if (p >= 50) {
        int s = 0;
        while (s, (range - n)) {
            //Generate a random number in the range [0, heigth*width-1]
            std::uniform_int_distribution<int> dist(0, range - 1);
            int keep = dist(rng);

            //Selection for the first time
            if (Mask[keep] == 3) {
                Mask[keep] == 1;
                s += 1;
            }
        }
        for (int k = 0; k < range; k++) {
            if (Mask[k] == 3) {
                Mask[k] = 0;
                u[k] = 0.;
            }
        }
    }
    else if (p < 50) {
        int s = 0;
        while (s, n) {
            std::uniform_int_distribution<int> dist(0, range - 1);
            int keep = dist(rng);

            if (Mask[keep] == 3) {
                Mask[keep] == 0;
                s += 1;
            }
        }
        for (int k = 0; k < range; k++) {
            if (Mask[k] == 3) {
                Mask[k] = 1;
                u[k] = 1.; 
            }
        }
    }


    
}
void Laplace_image::restoration(int* Mask, double* u, int height, int width)
{
    Eigen::SparseMatrix<double, Eigen::RowMajor> M(height * width, height * width);
    Eigen::VectorXd b = Eigen::VectorXd::Zero(height * width);
    Eigen::VectorXd xs(height * width);

    //Expect 5 non-zeros values for every row
    M.reserve(Eigen::VectorXi::Constant(height * width, 5));

    //To acces matrix:
    //for the first time -  M.insert(k, k);
    //modify a value you have alredy inserted - M.coeffRef(k, k);

    //Construct the matrix
    for (int k = 0; k < height * width; k++)
    {
        int i = k / width;
        int j = k % width;

        if (Mask[k] == 0) {
            //corner (0,0) bottom left
            if (i == 0 && j == 0) {
                M.insert(k, k) = 4.;
                M.insert(k, k + width) = -2.;
                M.insert(k, k + 1) = -2.;
            }
            //corner (0,w-1) bottom right
            else if (i == 0 && j == width - 1) {
                M.insert(k, k) = 4.;
                M.insert(k, k + width) = -2.;
                M.insert(k, k - 1) = -2.;
            }
            //corner (h-1,0) top left
            else if (i == height - 1 && j == 0) {
                M.insert(k, k) = 4.;
                M.insert(k, k - width) = -2.;
                M.insert(k, k + 1) = -2.;
            }
            //corner (h-1, w-1) top right
            else if (i == height - 1 && j == width - 1) {
                M.insert(k, k) = 4.;
                M.insert(k, k - width) = -2.;
                M.insert(k, k - 1) = -2.;
            }
            //top edge
            else if (i == height - 1 && j >= 1 && j <= width - 2) {
                M.insert(k, k) = 4.;
                M.insert(k, k - width) = -2.;
                M.insert(k, k + 1) = -1.;
                M.insert(k, k - 1) = -1.;
            }
            //right edge
            else if (i >= 1 && i <= height - 2 && j == width - 1) {
                M.insert(k, k) = 4.;
                M.insert(k, k - width) = -1.;
                M.insert(k, k + width) = -1.;
                M.insert(k, k - 1) = -2.;
            }
            //left edge
            else if (i >= 1 && i <= height - 2 && j == 0) {
                M.insert(k, k) = 4.;
                M.insert(k, k - width) = -1.;
                M.insert(k, k + width) = -1.;
                M.insert(k, k + 1) = -2.;
            }
            //bottom edge
            else if (i == 0 && j >= 1 && j <= width - 2) {
                M.insert(k, k) = 4.;
                M.insert(k, k - width) = -1.;
                M.insert(k, k + width) = -1.;
                M.insert(k, k + 1) = -2.;
            }
            //inside the domain
            else {
                M.insert(k, k) = 4.;
                M.insert(k, k - width) = -1.;
                M.insert(k, k + width) = -1.;
                M.insert(k, k - 1) = -1.;
                M.insert(k, k - 1) = -1.;
            }
        }
        else
        {
            M.insert(k, k) = 1.;
            b(k) = u[k];
        }
    }

    //Solve the system
    M.makeCompressed();
    Eigen::SparseLU<Eigen::SparseMatrix<double>> solver;
    solver.analyzePattern(M);
    solver.factorize(M);

    if (solver.info() != Eigen::Success) {
        std::cout << "Error in factorization of matrix" << std::endl;
        return;
    }

    xs = solver.solve(b);

    if (solver.info() != Eigen::Success) {
        std::cout << "Error in solver" << std::endl;
    }

    for (int i = 0; i < height * width; i++) {
        u[i] = xs[i];
    }
}

void Laplace_image::on_actionOpen_triggered()
{
    QString fileFilter = "Image data (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm);;All files (*)";
    QString fileName = QFileDialog::getOpenFileName(this, "Load image", fileFilter);
    if (fileName.isEmpty()) { return; }

    //load();

    // ...
}
void Laplace_image::on_actionSave_triggered()
{
    QString fileFilter = "Image data (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm);;All files (*)";
    QString fileName = QFileDialog::getSaveFileName(this, "Save image", fileFilter);
    // ...
}

