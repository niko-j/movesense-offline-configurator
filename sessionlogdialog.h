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

    Ui::SessionLogDialog *ui;
    QSharedPointer<Sensor> sensor;
};

#endif // SESSIONLOGDIALOG_H
