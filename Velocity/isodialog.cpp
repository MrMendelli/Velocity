#include "isodialog.h"
#include "ui_isodialog.h"

ISODialog::ISODialog(ISO *iso, QStatusBar *statusBar, QWidget *parent) :
    QDialog(parent), ui(new Ui::ISODialog), iso(iso), statusBar(statusBar)
{
    ui->setupUi(this);

    LoadISO();

    ui->treeWidget->setColumnWidth(0, 300);

    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
}

ISODialog::~ISODialog()
{
    delete ui;
    delete iso;
}

void ISODialog::LoadISO()
{
    iso->GetFileListing();
    LoadDirectory(ui->treeWidget, iso->root, true);

    ui->lblXGDVersion->setText(QString::fromStdString(iso->GetXGDVersion()));
}

void ISODialog::LoadDirectory(QObject *parent, std::vector<GdfxFileEntry> directoryContents, bool root)
{
    // create a tree widget for all the items, and recursively call for directories
    for (size_t i = 0; i < directoryContents.size(); i++)
    {
        GdfxFileEntry curEntry = directoryContents.at(i);

        // it's necessary to differentiate the parent type
        QTreeWidgetItem *item;
        if (root)
            item = new QTreeWidgetItem((QTreeWidget*)parent);
        else
            item = new QTreeWidgetItem((QTreeWidgetItem*)parent);

        UINT64 fileAddress = iso->SectorToAddress(curEntry.sector);

        item->setText(0, QString::fromStdString(curEntry.name));
        item->setText(1, QtHelpers::ToHexString(fileAddress));
        item->setText(2, QtHelpers::ToHexString(curEntry.sector));
        item->setText(3, QString::fromStdString(ByteSizeToString(curEntry.size)));

        // get the path of the file in the ISO
        QString pathInISO = QString::fromStdString(curEntry.filePath + curEntry.name);
        item->setData(IsoTreeWidgetItemDataPathInISO, Qt::UserRole, pathInISO);

        DWORD magic = 0;
        if (curEntry.attributes & GdfxDirectory)
        {
            LoadDirectory((QObject*)item, curEntry.files);
            item->setIcon(0, QIcon(QPixmap(":/Images/FolderFileIcon")));
        }
        else
        {
            // if it isn't a directory then read the file magic
            IsoIO *curIO = iso->GetIO(pathInISO.toStdString());
            curIO->SetEndian(BigEndian);

            magic = curIO->ReadDword();
            delete curIO;

            QIcon icon;
            QtHelpers::GetFileIcon(magic, QString::fromStdString(curEntry.name), icon, *item, FileSystemSTFS);
            item->setIcon(0, icon);
        }

        item->setData(IsoTreeWidgetItemDataMagic, Qt::UserRole, (quint32)magic);
    }
}

void ISODialog::on_pushButton_clicked()
{
    QString outDirectory = QFileDialog::getExistingDirectory(this, "Choose a location to extract the files to", QtHelpers::DesktopLocation());
    if (outDirectory.isEmpty())
        return;

    SingleProgressDialog *dialog;
    try
    {
        dialog = new SingleProgressDialog(FileSystemISO, iso, OpExtractAll, "", outDirectory, NULL, this);
        dialog->setModal(true);
        dialog->show();
        dialog->start();
    }
    catch (std::string error)
    {
        QMessageBox::critical(this, "Extract All Error", "An error occurred while extracting all the files.\n\n" + QString::fromStdString(error));
        dialog->close();
    }
}

void ISODialog::showContextMenu(QPoint point)
{
    QPoint globalPos = ui->treeWidget->mapToGlobal(point);
    QMenu contextMenu;

    if (ui->treeWidget->selectedItems().size() != 0)
        contextMenu.addAction(QPixmap(":/Images/extract.png"), "Extract");

    QAction *selectedItem = contextMenu.exec(globalPos);
    if (selectedItem == NULL)
        return;

    if (selectedItem->text() == "Extract")
    {
        QTreeWidgetItem *selectedItem = ui->treeWidget->selectedItems().at(0);
        QString pathInISO = selectedItem->data(IsoTreeWidgetItemDataPathInISO, Qt::UserRole).toString();

        QString outDirectory = QFileDialog::getExistingDirectory(this, "Choose a place to extract the file to", QtHelpers::DesktopLocation());
        iso->ExtractFile(outDirectory.toStdString(), pathInISO.toStdString());
    }
}

void ISODialog::on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    try
    {
        DWORD magic = item->data(IsoTreeWidgetItemDataMagic, Qt::UserRole).toUInt();
        std::string pathInISO = item->data(IsoTreeWidgetItemDataPathInISO, Qt::UserRole).toString().toStdString();
        IsoIO *io = iso->GetIO(pathInISO);

        switch (magic)
        {
            case CON:
            case LIVE:
            case PIRS:
                StfsPackage *package = new StfsPackage(io);
                PackageViewer viewer(statusBar, package);
                viewer.exec();
                break;
        }
    }
    catch (std::string error)
    {
        QMessageBox::critical(this, "Error Opening File", "Unable to open the file.\n\n" +
                              QString::fromStdString(error)) ;
    }
}

void ISODialog::on_btnSectorTool_clicked()
{
    IsoSectorDialog sectorDialog(iso, this);
    sectorDialog.exec();
}
