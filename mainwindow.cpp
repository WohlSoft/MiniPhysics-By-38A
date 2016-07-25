#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDesktopServices>
#include <QMenu>
#include <QAction>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QSurfaceFormat fmt;
    fmt.setRenderableType(QSurfaceFormat::OpenGL);
    fmt.setDepthBufferSize(32);
    fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    fmt.setSwapInterval(1);
    ui->centralWidget->setFormat(fmt);
    ui->centralWidget->grabKeyboard();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_MainWindow_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);
    QAction *getSrc = menu.addAction("Get source code of this tool");
    QAction *reply  = menu.exec( mapToGlobal(pos) );

    if( reply == getSrc )
        QDesktopServices::openUrl(QUrl("https://github.com/Wohlhabend-Networks/MiniPhysics-By-38A"));
}
