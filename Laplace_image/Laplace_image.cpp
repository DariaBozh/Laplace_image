#include "Laplace_image.h"

Laplace_image::Laplace_image(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    scene = new QGraphicsScene(this);
    ui.graphicsView->setScene(scene);
    pixmapItem = scene->addPixmap(QPixmap());

    ui.tbRemove->setEnabled(false);
    ui.tbRestore->setEnabled(false);
}

Laplace_image::~Laplace_image()
{}

static std::mt19937 rng(std::random_device{}());

void Laplace_image::load(double* u, std::string filename, int* height, int* width, ProcessMode processMode)
{
    this->mode = processMode;

    if (processMode == Gray)
        currentImage = currentImage.convertToFormat(QImage::Format_Grayscale8);
    else
        currentImage = currentImage.convertToFormat(QImage::Format_RGB32);

    int range = (*width) * (*height);
    
    for (int j = 0; j < *height; j++) { // row
        for (int i = 0; i < *width; i++) {  // column
            int k = j * (*width) + i; // index
            int qt_y = ((*height) - 1) - j; //lower left to upper left (flipping)
            int qt_x = i;
            int pixel;
            if (processMode == Gray) {
                pixel = qGray(currentImage.pixel(qt_x, qt_y)); //get grayscale intensity
                u[k] = pixel;
            }
            else {
                int r = qRed(currentImage.pixel(qt_x, qt_y));
                int g = qGreen(currentImage.pixel(qt_x, qt_y));
                int b = qBlue(currentImage.pixel(qt_x, qt_y));
                u_R[k] = r;
                u_G[k] = g;
                u_B[k] = b;
            }
        }
    }

    pixmapItem->setPixmap(QPixmap::fromImage(currentImage));
    ui.graphicsView->fitInView(pixmapItem, Qt::KeepAspectRatio); //formating
}

void Laplace_image::randomlyRemove(int* Mask, double* u, int range, int p)
{
    int n = range * p / 100; //number of pixels to DAMAGE
    std::uniform_int_distribution<int> dist(0, range - 1); //generate a random number in this range

    int s = 0;

    //If p > 50, we kipping heigth*width-n pixels
    if (p >= 50) {
        while (s < range - n) {
            int keep = dist(rng);

            //Selection for the first time
            if (Mask[keep] == 3) {
                Mask[keep] = 1;
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
        while (s < n) {
            int keep = dist(rng);

            if (Mask[keep] == 3) {
                Mask[keep] = 0;
                u[keep] = 0.;
                s += 1;
            }
        }
        for (int k = 0; k < range; k++) {
            if (Mask[k] == 3) {
                Mask[k] = 1;
            }
        }
    }

	qDebug() << "Pixels to damage: " << n << ", pixels to save: "<< s << "\nOut of " << range;
    
}
void Laplace_image::restore(int* Mask, double* u, int height, int width, ProcessMode processMode)
{
    Eigen::SparseMatrix<double> M(height * width, height * width); //column major as the default
    Eigen::VectorXd b = Eigen::VectorXd::Zero(height * width);
    Eigen::VectorXd xs(height * width);

    Eigen::VectorXd b_R = Eigen::VectorXd::Zero(height * width);
    Eigen::VectorXd b_G = Eigen::VectorXd::Zero(height * width);
    Eigen::VectorXd b_B = Eigen::VectorXd::Zero(height * width);
    Eigen::VectorXd xs_R(height * width);
    Eigen::VectorXd xs_G(height * width);
    Eigen::VectorXd xs_B(height * width);

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
            M.insert(k, k) = 4.;

            //corner (0,0) bottom left
            if (i == 0 && j == 0) {
                M.insert(k, k + width) = -2.;
                M.insert(k, k + 1) = -2.;
            }
            //corner (0,w-1) bottom right
            else if (i == 0 && j == width - 1) {
                M.insert(k, k + width) = -2.;
                M.insert(k, k - 1) = -2.;
            }
            //corner (h-1,0) top left
            else if (i == height - 1 && j == 0) {
                M.insert(k, k - width) = -2.;
                M.insert(k, k + 1) = -2.;
            }
            //corner (h-1, w-1) top right
            else if (i == height - 1 && j == width - 1) {
                M.insert(k, k - width) = -2.;
                M.insert(k, k - 1) = -2.;
            }
            //top edge
            else if (i == height - 1 && j >= 1 && j <= width - 2) {
                M.insert(k, k - width) = -2.;
                M.insert(k, k + 1) = -1.;
                M.insert(k, k - 1) = -1.;
            }
            //right edge
            else if (i >= 1 && i <= height - 2 && j == width - 1) {
                M.insert(k, k - width) = -1.;
                M.insert(k, k + width) = -1.;
                M.insert(k, k - 1) = -2.;
            }
            //left edge
            else if (i >= 1 && i <= height - 2 && j == 0) {
                M.insert(k, k - width) = -1.;
                M.insert(k, k + width) = -1.;
                M.insert(k, k + 1) = -2.;
            }
            //bottom edge
            else if (i == 0 && j >= 1 && j <= width - 2) {
                M.insert(k, k + width) = -2.;
                M.insert(k, k + 1) = -1.;
                M.insert(k, k - 1) = -1.;
            }
            //inside the domain
            else {
                M.insert(k, k - width) = -1.;
                M.insert(k, k + width) = -1.;
                M.insert(k, k - 1) = -1.;
                M.insert(k, k + 1) = -1.;
            }
        }
        else
        {
            M.insert(k, k) = 1.;
            if (processMode == RGB) {
                b_R(k) = u_R[k];
                b_G(k) = u_G[k];
                b_B(k) = u_B[k];
            }
            else b(k) = u[k];
        }
    }

    //Solve the system
    M.makeCompressed();
    Eigen::BiCGSTAB<Eigen::SparseMatrix<double>, Eigen::IncompleteLUT<double>> solver;
    solver.setTolerance(1e-8); //standart
    solver.setMaxIterations(height * width);
    solver.compute(M);
    //BiCGSTAB handles non-symmetric matrices fine
    //The IncompleteLUT preconditioner is what makes it converge fast for Poisson-style problems Ч without a preconditioner it would crawl

    if (solver.info() != Eigen::Success) {
        std::cout << "Error in solver setup" << std::endl;
        return;
    }
    if (processMode == RGB) {
        xs_R = solver.solve(b_R);
        xs_G = solver.solve(b_G);
        xs_B = solver.solve(b_B);
        std::cout << "Iterations: " << solver.iterations() << " error: " << solver.error() << std::endl;
        for (int i = 0; i < height * width; i++) {
            u_R[i] = xs_R[i];
            u_G[i] = xs_G[i];
            u_B[i] = xs_B[i];
        }
    }
    else {
        xs = solver.solve(b);
        std::cout << "Iterations: " << solver.iterations() << " error: " << solver.error() << std::endl;
        for (int i = 0; i < height * width; i++) {
            u[i] = xs[i];
        }
    }
    
}

void Laplace_image::smooth(ProcessMode processMode)
{
    std::cout << "Hi from smooth!"  << std::endl;

    double lambda = ui.dsbSmoothLambda->value(); //менша-б≥льше розмаже

    Eigen::SparseMatrix<double> M(range, range); //column major as the default
    Eigen::VectorXd b = Eigen::VectorXd::Zero(height * width);
    Eigen::VectorXd xs(range);

    Eigen::VectorXd b_R = Eigen::VectorXd::Zero(range);
    Eigen::VectorXd b_G = Eigen::VectorXd::Zero(range);
    Eigen::VectorXd b_B = Eigen::VectorXd::Zero(range);
    Eigen::VectorXd xs_R(range);
    Eigen::VectorXd xs_G(range);
    Eigen::VectorXd xs_B(range);
    M.reserve(Eigen::VectorXi::Constant(range, 5));

    //Construct the matrix
    for (int k = 0; k < height * width; k++)
    {
        int i = k / width;
        int j = k % width;

        //Fidality term: diagonal value + lambda
        M.insert(k, k) = 4 + lambda;

        if (processMode == RGB) {
            b_R(k) = lambda * u_R[k];
            b_G(k) = lambda * u_G[k];
            b_B(k) = lambda * u_B[k];
        }
        else b(k) = lambda * u[k]; //u0 - restored image

        //Smoothing term and mirroring
        //corner (0,0) bottom left
        if (i == 0 && j == 0) {
            M.insert(k, k + width) = -2.;
            M.insert(k, k + 1) = -2.;
        }
        //corner (0,w-1) bottom right
        else if (i == 0 && j == width - 1) {
            M.insert(k, k + width) = -2.;
            M.insert(k, k - 1) = -2.;
        }
        //corner (h-1,0) top left
        else if (i == height - 1 && j == 0) {
            M.insert(k, k - width) = -2.;
            M.insert(k, k + 1) = -2.;
        }
        //corner (h-1, w-1) top right
        else if (i == height - 1 && j == width - 1) {
            M.insert(k, k - width) = -2.;
            M.insert(k, k - 1) = -2.;
        }
        //top edge
        else if (i == height - 1 && j >= 1 && j <= width - 2) {
            M.insert(k, k - width) = -2.;
            M.insert(k, k + 1) = -1.;
            M.insert(k, k - 1) = -1.;
        }
        //right edge
        else if (i >= 1 && i <= height - 2 && j == width - 1) {
            M.insert(k, k - width) = -1.;
            M.insert(k, k + width) = -1.;
            M.insert(k, k - 1) = -2.;
        }
        //left edge
        else if (i >= 1 && i <= height - 2 && j == 0) {
            M.insert(k, k - width) = -1.;
            M.insert(k, k + width) = -1.;
            M.insert(k, k + 1) = -2.;
        }
        //bottom edge
        else if (i == 0 && j >= 1 && j <= width - 2) {
            M.insert(k, k + width) = -2.;
            M.insert(k, k + 1) = -1.;
            M.insert(k, k - 1) = -1.;
        }
        //inside the domain
        else {
            M.insert(k, k - width) = -1.;
            M.insert(k, k + width) = -1.;
            M.insert(k, k - 1) = -1.;
            M.insert(k, k + 1) = -1.;
        }
    }

    //Solve the system
    M.makeCompressed();
    Eigen::BiCGSTAB<Eigen::SparseMatrix<double>, Eigen::IncompleteLUT<double>> solver;
    solver.setTolerance(1e-8); //standart
    solver.setMaxIterations(height * width);
    solver.compute(M);
    //BiCGSTAB handles non-symmetric matrices fine
    //The IncompleteLUT preconditioner is what makes it converge fast for Poisson-style problems Ч without a preconditioner it would crawl

    if (solver.info() != Eigen::Success) {
        std::cout << "Error in solver setup" << std::endl;
        return;
    }
    if (processMode == RGB) {
        xs_R = solver.solve(b_R);
        xs_G = solver.solve(b_G);
        xs_B = solver.solve(b_B);
        std::cout << "Iterations: " << solver.iterations() << " error: " << solver.error() << std::endl;
        for (int i = 0; i < height * width; i++) {
            u_R[i] = xs_R[i];
            u_G[i] = xs_G[i];
            u_B[i] = xs_B[i];
        }
    }
    else {
        xs = solver.solve(b);
        std::cout << "Iterations: " << solver.iterations() << " error: " << solver.error() << std::endl;
        for (int i = 0; i < height * width; i++) {
            u[i] = xs[i];
        }
    }
    QMessageBox::information(this, "Success", "The image was smoothed");

}

void Laplace_image::refreshDisplay()
{
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            int k = j * width + i;

            int qt_x = i;
            int qt_y = (height - 1) - j;

            if (mode == RGB) {
                int r = std::clamp(int(std::round(u_R[k])), 0, 255);
                int g = std::clamp(int(std::round(u_G[k])), 0, 255);
                int b = std::clamp(int(std::round(u_B[k])), 0, 255);
                currentImage.setPixel(qt_x, qt_y, qRgb(r, g, b));
            }
            else {
                int v = std::clamp(static_cast<int>(std::round(u[k])), 0, 255);
                currentImage.setPixel(qt_x, qt_y, qRgb(v, v, v));
            }
        }
    }
    pixmapItem->setPixmap(QPixmap::fromImage(currentImage));
}

void Laplace_image::on_actionOpen_triggered()
{
    QMessageBox msgBox;
    msgBox.setText("Select processing mode: ");
    QPushButton* rgbButton = msgBox.addButton("Color (RGB)", QMessageBox::ActionRole);
    QPushButton* bwButton = msgBox.addButton("Black and white", QMessageBox::ActionRole);
    msgBox.exec();

    if (msgBox.clickedButton() == rgbButton) mode = RGB;
    else mode = Gray;

    QString fileFilter = "Image data (*.jpg *.jpeg *.png *.pgm)";
    QString fileName = QFileDialog::getOpenFileName(this, "Load image", "", fileFilter);
    if (fileName.isEmpty()) { 
        QMessageBox::information(this, "Action Canceled", "No image was selected.");
        return; 
    }

    if (!currentImage.load(fileName)) {
        QMessageBox::critical(this, "Error", "Failed to load the image. The file might be corrupted or unsupported.");
        return;
    }
    originalImage = currentImage;

    width = currentImage.width();
    height = currentImage.height();
    range = width * height;

    u.resize(range);
    u_R.resize(range);
    u_G.resize(range);
    u_B.resize(range);

    Mask.resize(range);
    std::fill(Mask.begin(), Mask.end(), 3); // initialize

    load(u.data(), fileName.toStdString(), &height, &width, mode);

    QMessageBox::information(this, "Success", "Image successfully loaded and processed!");
    ui.tbRemove->setEnabled(true);
    ui.tbRestore->setEnabled(false);
}
void Laplace_image::on_actionSave_triggered()
{
    if (currentImage.isNull()) {
        QMessageBox::warning(this, "Action Canceled", "No image was selected.");
        return;
    }

    QString fileFilter = "PNG Image (*.png);;JPG Image (*.jpg);;BMP Image (*.bmp);;All Files (*)";
    QString fileName = QFileDialog::getSaveFileName(this, "Save Image", "", fileFilter);

    if (fileName.isEmpty()) {
        return;
    }

    if (currentImage.save(fileName)) {
        QMessageBox::information(this, "Success", "Image successfully saved and processed!");
    }
    else {
        QMessageBox::critical(this, "Error", "Cannot save file");
    }
}
void Laplace_image::on_tbRemove_clicked()
{
    ui.tbRestore->setEnabled(true);
    currentImage = originalImage;

    // Reload original pixels and reset the mask before damaging again
    load(u.data(), "", &height, &width, mode);
    std::fill(Mask.begin(), Mask.end(), 3);

    int percent = ui.sbPercent->value();
    randomlyRemove(Mask.data(), u.data(), range, percent);

    if (mode == RGB) {
        for (int k = 0; k < range; ++k) {
            if (Mask[k] == 0) {
                u_R[k] = 0.;
                u_G[k] = 0.;
                u_B[k] = 0.;
            }
        }
    }

    refreshDisplay();
}
void Laplace_image::on_tbRestore_clicked()
{
    ui.tbRemove->setEnabled(true);

    restore(Mask.data(), u.data(), height, width, mode);
    refreshDisplay();
}

void Laplace_image::on_tbSmooth_clicked()
{
    std::cout << "Hi from public slot!" << std::endl;

    if (range == 0) return; 

    smooth(this->mode);
    refreshDisplay();
}

