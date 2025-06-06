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
        uint8_t ref = this->sensor->sendCommand(CommandPacket::CmdClearLogs, {});
        startRequest(ref);
    }
}

void SessionLogDialog::onFetchSessions()
{
    onClearList();
    if(this->sensor)
    {
        uint8_t ref = this->sensor->sendCommand(CommandPacket::CmdListLogs, {});
        startRequest(ref);
    }
}

void SessionLogDialog::onDownloadSelected()
{
    auto item = ui->listWidget->currentItem();
    if(this->sensor)
    {
        auto itemData = item->data(Qt::UserRole);

        CommandPacket::Params params;
        params.readLog.logIndex = (uint16_t) (itemData.toUInt());

        uint8_t ref = this->sensor->sendCommand(CommandPacket::CmdReadLog, params);
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

void SessionLogDialog::onReceiveLogList(uint8_t ref, const QList<LogListPacket::LogItem>& items, bool complete)
{
    for(const auto& item : items)
    {
        qInfo("REF %u - Item: %u Size: %u Modified: %llu", ref, item.id, item.size, item.modified);

        QString label = QString::asprintf("LOG# %u - Modified: %llu - Size: %u", item.id, item.modified, item.size);

        QListWidgetItem* listItem = new QListWidgetItem(label);
        listItem->setData(Qt::UserRole, item.id);
        ui->listWidget->addItem(listItem);
    }

    if(complete)
        completeRequest(ref);
}

void SessionLogDialog::onReceiveData(uint8_t ref, const QByteArray& data)
{
    qInfo("Receiving data (ref: %u)", ref);
    ui->progressBar->setValue(100);

    auto path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QString filename = QFileDialog::getSaveFileName(this, "Save log", path, "SBEM File (*.sbem)");
    if(!filename.isEmpty())
    {
        std::fstream file(filename.toStdString(), std::ios::trunc | std::ios::binary | std::ios::out);
        if(file.is_open())
        {
            file.write(data.data(), data.size());
            file.close();
        }
    }

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

    pendingRequestRef = Packet::INVALID_REF;

    onLogSelected();
    ui->refreshListButton->setEnabled(true);
    ui->eraseLogsButton->setEnabled(true);
}
