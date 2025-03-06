#include "logstreamview.h"
#include "ui_logstreamview.h"

#include <QDateTime>

LogStreamView::LogStreamView(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LogStreamView)
{
    ui->setupUi(this);
}

LogStreamView::~LogStreamView()
{
    delete ui;
}

void LogStreamView::setSensorDevice(QSharedPointer<Sensor> sensor)
{
    ui->messages->clear();

    if(this->sensor)
    {
        this->sensor->stopStreamingLogMessages();
        disconnect(this->sensor.get(), &Sensor::onReceiveLogStream, this, &LogStreamView::onMessage);
    }

    this->sensor = sensor;

    if(this->sensor)
    {
        this->sensor->startStreamingLogMessages();
        connect(this->sensor.get(), &Sensor::onReceiveLogStream, this, &LogStreamView::onMessage);
    }
}

void LogStreamView::onMessage(const DebugMessagePacket& packet)
{
    static const QString levelLabels[] = { "[FATAL]", "[ERROR]", "[WARNING]", "[INFO]", "[VERBOSE]" };

    uint32_t ms = packet.timestamp % 1000;
    uint32_t epoch = packet.timestamp / 1000;

    QString line = QString::asprintf("%d.%03d ", epoch, ms);
    if(packet.level >= 0 && packet.level <= 4)
        line += levelLabels[packet.level] + " ";

    size_t len = packet.message.get_read_size();
    auto data = packet.message.get_read_ptr();
    QByteArray messageBytes((const char*) data, len);

    line += messageBytes.toStdString();
    ui->messages->addItem(line);
    ui->messages->scrollToBottom();
}
