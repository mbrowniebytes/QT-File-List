#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushButtonScan_clicked();

    void on_textEditScanDirectory_textChanged();

    void on_pushButtonExport_clicked();

    void on_pushButtonScanBrowse_clicked();

private:
    Ui::MainWindow *ui;

    void setupTreeWidgetScanResults();

    void setupProgressBarStatus();
    void progressWorking();
    void progressDone();
    void progressFilesStyle();
    void progressResetStyle();
};

#endif // MAINWINDOW_H
