#ifndef SESSIONLOGDIALOG_H
#define SESSIONLOGDIALOG_H

#include <QDialog>
#include "sensor.h"

namespace Ui {
class SessionLogDialog;
}

class SessionLogDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SessionLogDialog(QWidget *parent = nullptr);
    ~SessionLogDialog();

    void setSensorDevice(QSharedPointer<Sensor> sensor);

private:
    void onEraseLogs();
    void onFetchSessions();
    void onDownloadAll();
    void onDownloadSelected();

    void onLogSelected();
    void onClearList();

    void onReceiveLogList(uint8_t ref, const QList<OfflineLogPacket::LogItem>& items, bool complete);
    void onReceiveData(uint8_t ref, const QByteArray& data);
    void onReceiveDataProgress(uint8_t ref, uint32_t recvBytes, uint32_t totalBytes);
    void onReceiveStatusResponse(uint8_t ref, uint16_t status);

    void startRequest(uint8_t ref);
    void completeRequest(uint8_t ref);

    Ui::SessionLogDialog *ui;
    QSharedPointer<Sensor> sensor;
    uint8_t pendingRequestRef = OfflinePacket::INVALID_REF;
};

#endif // SESSIONLOGDIALOG_H
