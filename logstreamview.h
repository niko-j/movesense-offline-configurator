#ifndef LOGSTREAMVIEW_H
#define LOGSTREAMVIEW_H

#include <QDialog>
#include "sensor.h"

namespace Ui {
class LogStreamView;
}

class LogStreamView : public QDialog
{
    Q_OBJECT

public:
    explicit LogStreamView(QWidget *parent = nullptr);
    ~LogStreamView();

    void setSensorDevice(QSharedPointer<Sensor> sensor);
    void onMessage(const DebugMessagePacket& packet);

private:
    Ui::LogStreamView *ui;

    QSharedPointer<Sensor> sensor;
};

#endif // LOGSTREAMVIEW_H
