#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "util.h"
#include "structs.h"

#include <QDebug>
#include <QMessageBox>

#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>
#include <QJsonArray>

#include <QFileDialog>
#include <QStandardPaths>
#include <QFileIconProvider>
#include <QHeaderView>
#include <QFont>

#include <QThread>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // setup widget treeWidgetScanResults
    this->setupTreeWidgetScanResults();

    // setup widget progressBarStatus
    this->setupProgressBarStatus();

    ui->radioButtonExportJson->setChecked(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * @brief setup widget progressBarStatus
 */
void MainWindow::setupProgressBarStatus()
{
    ui->progressBarStatus->setMinimum(0);
    ui->progressBarStatus->setMaximum(100);
    ui->progressBarStatus->setValue(0);
    ui->progressBarStatus->setTextVisible(false);
}

/**
 * @brief progressBarStatus scanning
 */
void MainWindow::progressWorking()
{
    ui->progressBarStatus->setMinimum(0);
    ui->progressBarStatus->setMaximum(0);
    ui->progressBarStatus->setValue(0);

    ui->pushButtonScan->setEnabled(false);
    ui->pushButtonScanBrowse->setEnabled(false);
    ui->pushButtonExport->setEnabled(false);
    ui->textEditScanDirectory->setEnabled(false);

    ui->radioButtonExportJson->setEnabled(false);
    ui->radioButtonExportCsv->setEnabled(false);
}

/**
 * @brief progressBarStatus done
 */
void MainWindow::progressDone()
{
    ui->progressBarStatus->setMinimum(0);
    ui->progressBarStatus->setMaximum(100);
    ui->progressBarStatus->setValue(0);

    ui->pushButtonScan->setEnabled(true);
    ui->pushButtonScanBrowse->setEnabled(true);
    ui->pushButtonExport->setEnabled(true);
    ui->textEditScanDirectory->setEnabled(true);

    ui->radioButtonExportJson->setEnabled(true);
    ui->radioButtonExportCsv->setEnabled(true);
}

void MainWindow::progressFilesStyle()
{
    QString scanDirCSS = "QProgressBar::chunk { background-color: rgba(200, 200, 0, 80); width: 20px; }";
    ui->progressBarStatus->setStyleSheet(scanDirCSS);
}

void MainWindow::progressResetStyle()
{
    ui->progressBarStatus->setStyleSheet("");
}

/**
 * @brief setup widget treeWidgetScanResults
 */
void MainWindow::setupTreeWidgetScanResults()
{
    // remove any gui set parms
    ui->treeWidgetScanResults->reset();

    // flatten tree
    ui->treeWidgetScanResults->setRootIsDecorated(false);

    // columns
    ui->treeWidgetScanResults->setColumnCount(3);

    int tableWidth = ui->treeWidgetScanResults->width();
    int columnIconWidth = 25;
    int columnSizeWidth = 100;
    ui->treeWidgetScanResults->setColumnWidth(0, columnIconWidth);
    ui->treeWidgetScanResults->setColumnWidth(1, tableWidth - columnSizeWidth - columnIconWidth - 25);
    ui->treeWidgetScanResults->setColumnWidth(2, columnSizeWidth);

    ui->treeWidgetScanResults->setIconSize(QSize(16,16));

    QStringList tableHeader = QStringList()<<" "<<"Path"<<"Size";
    ui->treeWidgetScanResults->setHeaderLabels(tableHeader);

    // only expand path column
    ui->treeWidgetScanResults->header()->setStretchLastSection(false);
    ui->treeWidgetScanResults->header()->setSectionResizeMode(1, QHeaderView::Stretch);
}

/**
 * @brief scan directory
 */
void MainWindow::on_pushButtonScan_clicked()
{
    QMessageBox messageBox;

    QString scanDir = ui->textEditScanDirectory->toPlainText();

    // for quick dev
    if (scanDir.isEmpty()) {
        // scanDir = "C:\\Transfer\\Scratch";
        // ui->textEditScanDirectory->setText(scanDir);
    }

    // indicate working
    this->progressWorking();
    ui->pushButtonScan->setText("Scanning..");
    ui->treeWidgetScanResults->clear();

    this->progressFilesStyle();

    QVector <fileScanInfo> fileScanInfos = util::scanDirectoryJob(scanDir);
    long nbrFileScanInfos = fileScanInfos.length();

    this->progressResetStyle();

    for (long index = 0; index < nbrFileScanInfos; index++) {
        fileScanInfo scanInfo = fileScanInfos.at(index);

        QTreeWidgetItem *treeItem = new QTreeWidgetItem();
        treeItem->setIcon(0, scanInfo.iconImage);
        // treeItem->setText(0, scanInfo.iconName); // dev
        // treeItem->setText(0, scanInfo.suffix); // dev

        treeItem->setText(1, scanInfo.relativeFilePath);
        // treeItem->setText(1, scanInfo.absoluteFilePath); // for dev
        QFont itemFont = QFont();
        if (scanInfo.dirType == "base") {
            itemFont.setBold(true);
        }
        treeItem->setFont(1, itemFont);

        treeItem->setText(2, scanInfo.sizeFormatted);
        treeItem->setTextAlignment(2, Qt::AlignLeft);

        // style path column
        QLinearGradient gradient1(0, 0, 50, 50);
        if (scanInfo.depth > 5) {
            gradient1.setColorAt(0, QColor(100, 10, 10, 100));
        } else {
            gradient1.setColorAt(0, QColor(50, 50, 50, 50));
        }
        gradient1.setColorAt(1, QColor(0, 0, 0, 0));
        QBrush brush1(gradient1);
        treeItem->setBackground(1, brush1);

        // style size column based on size
        QLinearGradient gradient2(0, 0, 50, 50);
        if (scanInfo.suffix == "dir") { // dir
            gradient2.setColorAt(0, QColor(200, 200, 0, 80));
            gradient2.setColorAt(1, QColor(0, 0, 0, 0));
        } else if (scanInfo.size > 1073741824) { // 1gb
            gradient2.setColorAt(0, QColor(50, 150, 50, 120));
            gradient2.setColorAt(1, QColor(0, 0, 0, 0));
        } else if (scanInfo.size > 536870912) { // 512mb
            gradient2.setColorAt(0, QColor(50, 150, 50, 100));
            gradient2.setColorAt(1, QColor(0, 0, 0, 0));
        } else if (scanInfo.size > 134217728) { // 128mb
            gradient2.setColorAt(0, QColor(50, 150, 50, 80));
            gradient2.setColorAt(1, QColor(0, 0, 0, 0));
        } else if (scanInfo.size > 1048576) { // 1mb
            gradient2.setColorAt(0, QColor(50, 50, 150, 60));
            gradient2.setColorAt(1, QColor(0, 0, 0, 0));
        } else if (scanInfo.size == 0) { // 0mb
            gradient2.setColorAt(0, QColor(150, 150, 150, 80));
            gradient2.setColorAt(1, QColor(0, 0, 0, 0));
        } else { // else
            gradient2.setColorAt(0, QColor(0, 0, 0, 0));
            gradient2.setColorAt(1, QColor(0, 0, 0, 0));
        }
        QBrush brush2(gradient2);
        treeItem->setBackground(2, brush2);

        ui->treeWidgetScanResults->addTopLevelItem(treeItem);

        if (index % 10 == 0) {
            // allow gui to be somewhat responsive
            QCoreApplication::processEvents(QEventLoop::AllEvents, 4000);
        }

        if (index > 10000) {
            ui->statusBar->showMessage("Only showing 10,000 files/dirs");
            QThread::sleep(2);
            ui->statusBar->showMessage("");
            break;
        }
    }

    this->progressDone();
    ui->pushButtonScan->setText("Scan");

    ui->statusBar->showMessage("");
}

void MainWindow::on_textEditScanDirectory_textChanged()
{

}

/**
 * @brief export scanned results
 */
void MainWindow::on_pushButtonExport_clicked()
{
    QMessageBox messageBox;

    QString scanDir = ui->textEditScanDirectory->toPlainText();

    // for quick dev
    if (scanDir.isEmpty()) {
        // scanDir = "C:\\Transfer\\Scratch";
        // ui->textEditScanDirectory->setText(scanDir);
    }

    QString selectedFilter;
    QString defaultFileName = "FileListExport";
    if (ui->radioButtonExportJson->isChecked()) {
        selectedFilter = tr("json (*.json)");
        defaultFileName += ".json";
    } else if (ui->radioButtonExportCsv->isChecked()) {
        selectedFilter = tr("csv (*.csv)");
        defaultFileName += ".csv";
    } else {
        selectedFilter = "";
        defaultFileName += ".txt";
    }

    // select file to save to
    QString downloadsFolder = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    downloadsFolder += "/" + defaultFileName;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                               downloadsFolder,
                               selectedFilter);

    // verify can write to file
    QFile f( fileName );
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        messageBox.setText("Cannot save " + fileName);
        messageBox.exec();
        this->progressDone();
        ui->pushButtonExport->setText("Export");
        return;
    }
    f.close();


    // indicate working
    this->progressWorking();
    ui->pushButtonExport->setText("Exporting..");


    QJsonArray scanResults;

    this->progressFilesStyle();

    // scan dir again
    // TODO: maybe better to read from treeview? but would require putting all data in treeview .. somehow
    QVector <fileScanInfo> fileScanInfos = util::scanDirectoryJob(scanDir);
    int nbrFileScanInfos = fileScanInfos.length();

    this->progressResetStyle();

    // validate results
    if (nbrFileScanInfos <= 0) {
        this->progressDone();
        ui->pushButtonExport->setText("Export");
        return;
    }


    if (ui->radioButtonExportJson->isChecked()) {
        // iterate over results and build json result
        for (long index = 0; index < nbrFileScanInfos; index++) {
            fileScanInfo scanInfo = fileScanInfos.at(index);

            // build up json result
            QJsonObject scanResult;
            scanResult["absoluteFilePath"] = scanInfo.absoluteFilePath;
            scanResult["relativeFilePath"] = scanInfo.relativeFilePath;
            scanResult["relativeDir"] = scanInfo.relativeDir;
            scanResult["dirType"] = scanInfo.dirType;
            scanResult["fileName"] = scanInfo.fileName;
            scanResult["suffix"] = scanInfo.suffix;
            scanResult["fileType"] = scanInfo.iconName;
            scanResult["lastModified"] = scanInfo.lastModified;
            scanResult["size"] = scanInfo.size;
            scanResult["sizeFormatted"] = scanInfo.sizeFormatted;

            scanResults.append(scanResult);

            if (index % 10 == 0) {
                // allow gui to be somewhat responsive
                QCoreApplication::processEvents(QEventLoop::AllEvents, 4000);
            }
        }

        // save export
        QJsonDocument doc(scanResults);

        QFile f( fileName );
        f.open(QIODevice::WriteOnly | QIODevice::Text);
        f.write(doc.toJson(QJsonDocument::Indented));
        f.close();

    } else if (ui->radioButtonExportCsv->isChecked()) {
        QFile f( fileName );
        f.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream textStream(&f);

        QStringList stringList;
        stringList << "absoluteFilePath";
        stringList << "relativeFilePath";
        stringList << "relativeDir";
        stringList << "fileName";
        stringList << "suffix";
        stringList << "dirType";
        stringList << "depth";
        stringList << "iconName";
        stringList << "lastModified";
        stringList << "size";
        stringList << "sizeFormatted";

        textStream << stringList.join(",")+"\n";

        // iterate over results and stream csv result
        for (long index = 0; index < nbrFileScanInfos; index++) {
            fileScanInfo scanInfo = fileScanInfos.at(index);

            QStringList stringList;
            stringList << util::simpleEscapeCsv(scanInfo.absoluteFilePath);
            stringList << util::simpleEscapeCsv(scanInfo.relativeFilePath);
            stringList << util::simpleEscapeCsv(scanInfo.relativeDir);
            stringList << util::simpleEscapeCsv(scanInfo.fileName);
            stringList << util::simpleEscapeCsv(scanInfo.suffix);
            stringList << util::simpleEscapeCsv(scanInfo.dirType);
            stringList << util::simpleEscapeCsv(QString::number(scanInfo.depth));
            stringList << util::simpleEscapeCsv(scanInfo.iconName);
            stringList << util::simpleEscapeCsv(scanInfo.lastModified);
            stringList << util::simpleEscapeCsv(QString::number(scanInfo.size));
            stringList << util::simpleEscapeCsv(scanInfo.sizeFormatted);

            textStream << stringList.join(",")+"\n";

            if (index % 10 == 0) {
                // allow gui to be somewhat responsive
                QCoreApplication::processEvents(QEventLoop::AllEvents, 4000);
            }
        }
        f.close();
    } else {
        ui->statusBar->showMessage("Unknown export type");
    }

    this->progressDone();
    ui->pushButtonExport->setText("Export");
}


/**
 * @brief show dir selection and set textEditScanDirectory
 */
void MainWindow::on_pushButtonScanBrowse_clicked()
{
    QString scanDir = ui->textEditScanDirectory->toPlainText();
    QDir dir;

    QString defaultDir;
    if (scanDir.isEmpty()) {
        defaultDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    } else {
        defaultDir = scanDir;
    }
    QString selectedScanDir = QFileDialog::getExistingDirectory(this, tr("Select a directory to scan"),
                                                 defaultDir,
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);

    if (!selectedScanDir.isEmpty() && dir.exists(selectedScanDir)) {
        ui->textEditScanDirectory->setText(selectedScanDir);
    }
}
