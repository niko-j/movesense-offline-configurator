#include "sessionlogdialog.h"
#include "ui_sessionlogdialog.h"

#include <QFileDialog>
#include <QStandardPaths>
#include <fstream>

SessionLogDialog::SessionLogDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SessionLogDialog)
{
    ui->setupUi(this);

    connect(ui->closeButton, &QPushButton::clicked, this, &QDialog::close);
    connect(ui->eraseLogsButton, &QPushButton::clicked, this, &SessionLogDialog::onEraseLogs);
    connect(ui->refreshListButton, &QPushButton::clicked, this, &SessionLogDialog::onFetchSessions);
    connect(ui->downloadSelectedButton, &QPushButton::clicked, this, &SessionLogDialog::onDownloadSelected);
    connect(ui->listWidget, &QListWidget::itemSelectionChanged, this, &SessionLogDialog::onLogSelected);

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
    ui->listWidget->clear();
    ui->progressBar->setValue(0);

    if(this->sensor)
    {
        disconnect(this->sensor.get(), &Sensor::onLogListReceived, this, &SessionLogDialog::onReceiveLogList);
        disconnect(this->sensor.get(), &Sensor::onStatusResponse, this, &SessionLogDialog::onReceiveStatusResponse);
        disconnect(this->sensor.get(), &Sensor::onDataTransmissionCompleted, this, &SessionLogDialog::onReceiveData);
        disconnect(this->sensor.get(), &Sensor::onDataTransmissionProgressUpdate, this, &SessionLogDialog::onReceiveDataProgress);

        ui->downloadSelectedButton->setEnabled(false);
        ui->refreshListButton->setEnabled(false);
        ui->eraseLogsButton->setEnabled(false);
    }

    this->sensor = sensor;

    if(this->sensor)
    {
        connect(this->sensor.get(), &Sensor::onLogListReceived, this, &SessionLogDialog::onReceiveLogList);
        connect(this->sensor.get(), &Sensor::onStatusResponse, this, &SessionLogDialog::onReceiveStatusResponse);
        connect(this->sensor.get(), &Sensor::onDataTransmissionCompleted, this, &SessionLogDialog::onReceiveData);
        connect(this->sensor.get(), &Sensor::onDataTransmissionProgressUpdate, this, &SessionLogDialog::onReceiveDataProgress);

        ui->refreshListButton->setEnabled(true);
        ui->eraseLogsButton->setEnabled(true);
    }

    onFetchSessions();
}


void SessionLogDialog::onEraseLogs()
{
    onClearList();
    if(this->sensor)
    {
        uint8_t ref = this->sensor->sendCommand(OfflineCommandPacket::CmdClearLogs, {});
        startRequest(ref);
    }
}

void SessionLogDialog::onFetchSessions()
{
    onClearList();
    if(this->sensor)
    {
        uint8_t ref = this->sensor->sendCommand(OfflineCommandPacket::CmdListLogs, {});
        startRequest(ref);
    }
}

void SessionLogDialog::onDownloadSelected()
{
    auto index = ui->listWidget->currentIndex();
    if(this->sensor && index.isValid())
    {
        OfflineCommandPacket::CommandParams params;
        params.ReadLogParams.logIndex = (uint16_t) (index.row() + 1);

        uint8_t ref = this->sensor->sendCommand(OfflineCommandPacket::CmdReadLog, params);
        startRequest(ref);
    }
}

void SessionLogDialog::onLogSelected()
{
    auto index = ui->listWidget->currentIndex();
    ui->downloadSelectedButton->setEnabled(index.isValid());
}

void SessionLogDialog::onClearList()
{
    ui->downloadSelectedButton->setEnabled(false);
    ui->listWidget->clear();
}

void SessionLogDialog::onReceiveLogList(uint8_t ref, const QList<OfflineLogPacket::LogItem>& items, bool complete)
{
    for(const auto& item : items)
    {
        qInfo("REF %u - Item: %u Size: %u Modified: %llu", ref, item.id, item.size, item.modified);

        QString label = QString::asprintf("LOG# %u - Modified: %llu - Size: %u", item.id, item.modified, item.size);
        ui->listWidget->addItem(label);
    }

    if(complete)
        completeRequest(ref);
}

void SessionLogDialog::onReceiveData(uint8_t ref, const QByteArray& data)
{
    qInfo("Receiving data (ref: %u)", ref);

    auto path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QString filename = QFileDialog::getSaveFileName(this, "Save log", path, "SBEM File (*.sbem)");
    if(filename.isEmpty())
        return;

    std::fstream file(filename.toStdString(), std::ios::trunc | std::ios::binary | std::ios::out);
    if(!file.is_open())
        return;

    file.write(data.data(), data.size());
    file.close();

    completeRequest(ref);
}

void SessionLogDialog::onReceiveDataProgress(uint8_t ref, uint32_t recvBytes, uint32_t totalBytes)
{
    if(pendingRequestRef == ref)
    {
        int progress = (int)(100.0 * ((float) recvBytes / totalBytes));
        ui->progressBar->setValue(progress);
        qInfo("Data download progress: %d%%", progress);
    }
}

void SessionLogDialog::onReceiveStatusResponse(uint8_t ref, uint16_t status)
{
    qInfo("Status response (ref %u): %u", ref, status);
    completeRequest(ref);
}

void SessionLogDialog::startRequest(uint8_t ref)
{
    pendingRequestRef = ref;
    ui->progressBar->setValue(0);
    ui->downloadSelectedButton->setEnabled(false);
    ui->refreshListButton->setEnabled(false);
    ui->eraseLogsButton->setEnabled(false);
}

void SessionLogDialog::completeRequest(uint8_t ref)
{
    if(pendingRequestRef != ref)
        return;

    pendingRequestRef = OfflinePacket::INVALID_REF;

    ui->progressBar->setValue(100);

    onLogSelected();
    ui->refreshListButton->setEnabled(true);
    ui->eraseLogsButton->setEnabled(true);
}
