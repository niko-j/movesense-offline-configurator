#include "sessionlogdialog.h"
#include "ui_sessionlogdialog.h"

SessionLogDialog::SessionLogDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SessionLogDialog)
{
    ui->setupUi(this);

    connect(ui->closeButton, &QPushButton::clicked, this, &QDialog::close);

    connect(ui->eraseLogsButton, &QPushButton::clicked, this, &SessionLogDialog::onEraseLogs);
    connect(ui->refreshListButton, &QPushButton::clicked, this, &SessionLogDialog::onFetchSessions);
    connect(ui->downloadAllButton, &QPushButton::clicked, this, &SessionLogDialog::onDownloadAll);
    connect(ui->downloadSelectedButton, &QPushButton::clicked, this, &SessionLogDialog::onDownloadSelected);

    ui->downloadAllButton->setEnabled(false);
    ui->downloadSelectedButton->setEnabled(false);
    ui->refreshListButton->setEnabled(false);
    ui->eraseLogsButton->setEnabled(false);
}

SessionLogDialog::~SessionLogDialog()
{
    delete ui;
}

void SessionLogDialog::setSensorDevice(QSharedPointer<Sensor> sensor)
{
    if(this->sensor)
    {
        // TODO: Disconnect signals
    }

    this->sensor = sensor;

    if(this->sensor)
    {
        // TODO: Connect signals


    }
}


void SessionLogDialog::onEraseLogs()
{

}

void SessionLogDialog::onFetchSessions()
{

}

void SessionLogDialog::onDownloadAll()
{

}

void SessionLogDialog::onDownloadSelected()
{

}
