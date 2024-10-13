#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QtLogging>
#include <QMessageBox>
#include <QLayout>
#include <QComboBox>
#include <QCheckBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , scanner(this)
    , ui(new Ui::MainWindow)
    , config({})
    , sessionDialog(new SessionLogDialog(this))
{
    ui->setupUi(this);

    // Connect button actions
    connect(ui->startScanButton, &QPushButton::clicked, &scanner, &Scanner::start);
    connect(ui->stopScanButton, &QPushButton::clicked, &scanner, &Scanner::stop);

    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::onConnect);
    connect(ui->disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnect);
    connect(ui->applyButton, &QPushButton::clicked, this, &MainWindow::onApplySettings);
    connect(ui->resetButton, &QPushButton::clicked, this, &MainWindow::onResetSettings);
    connect(ui->sessionLogsButton, &QPushButton::clicked, this, &MainWindow::onOpenSessionLogs);

    connect(sessionDialog, &QDialog::finished, this, &MainWindow::onCloseSessionLogs);

    // Connect device list actions
    connect(ui->deviceList, &QListWidget::itemSelectionChanged, this, &MainWindow::onSelectDevice);

    // Set widget states
    ui->stopScanButton->hide();
    ui->disconnectButton->hide();
    ui->connectButton->setEnabled(false);

    ui->resetButton->setEnabled(false);
    ui->applyButton->setEnabled(false);
    ui->sessionLogsButton->setEnabled(false);

    connect(&scanner, &Scanner::deviceListUpdated, this, &MainWindow::onUpdateDeviceList);
    connect(&scanner, &Scanner::stateChanged, this, &MainWindow::onScannerStateChanged);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onConnect()
{
    scanner.stop();

    auto devices = scanner.listDevices();
    auto index = ui->deviceList->currentIndex();
    if(!index.isValid())
        return;
    auto& device = devices.at(index.row());

    ui->deviceList->setEnabled(false);
    ui->connectButton->hide();
    ui->disconnectButton->show();

    sensor = QSharedPointer<Sensor>(new Sensor(this, device));
    connect(sensor.get(), &Sensor::onStateChanged, this, &MainWindow::onSensorStateChanged);
    connect(sensor.get(), &Sensor::onError, this, &MainWindow::onSensorError);
    connect(sensor.get(), &Sensor::onConfigUpdated, this, &MainWindow::onSensorConfigChanged);

    sensor->connectDevice();
}

void MainWindow::onDisconnect()
{
    sensor->disconnectDevice();
}

void MainWindow::onApplySettings()
{
    if(sensor)
    {
        sensor->sendConfig(config);
        ui->applyButton->setEnabled(false);
    }
}

void MainWindow::onResetSettings()
{
    if(sensor)
    {
        config = {};
        config.wakeup_behavior = SensorWakeUpConnector;
        config.sleep_delay = 30 * 60;
        onSensorConfigChanged(config);
        onApplySettings();
    }
}

void MainWindow::onDownloadData()
{
    qWarning("Not implemented!");
}

void MainWindow::onSelectDevice()
{
    auto index = ui->deviceList->currentIndex();
    ui->connectButton->setEnabled(index.isValid());
}

void MainWindow::onSettingsEdited()
{
    ui->applyButton->setEnabled(true);
}

void MainWindow::onOpenSessionLogs()
{
    this->hide();
    sessionDialog->show();
    sessionDialog->setSensorDevice(sensor);
}

void MainWindow::onCloseSessionLogs()
{
    sessionDialog->setSensorDevice(nullptr);
    sessionDialog->hide();
    this->show();
}

void MainWindow::onSensorStateChanged(Sensor::State state)
{
    switch(state)
    {
        case Sensor::Disconnected:
        {
            qInfo("Sensor disconnected!");
            sensor.reset();

            ui->resetButton->setEnabled(false);
            ui->applyButton->setEnabled(false);
            ui->sessionLogsButton->setEnabled(false);

            ui->disconnectButton->hide();
            ui->connectButton->show();

            ui->deviceList->setEnabled(true);

            auto* settingsWidget = ui->settingsScrollArea->takeWidget();
            if(settingsWidget)
                delete settingsWidget;

            break;
        }
        case Sensor::Connecting:
        {
            qInfo("Sensor connecting...");
            break;
        }
        case Sensor::DiscoveringServices:
        {
            qInfo("Discovering sensor services...");
            break;
        }
        case Sensor::Connected:
        {
            qInfo("Sensor connected!");
            ui->sessionLogsButton->setEnabled(true);
            break;
        }
    }
}

void MainWindow::onSensorError(Sensor::Error error)
{
    QString msg = QString::asprintf("Sensor reported an error: %u", error);
    QMessageBox::warning(this, "Sensor error", msg);
}

void MainWindow::onSensorConfigChanged(const SensorConfig& config)
{
    this->config = config;

    ui->resetButton->setEnabled(true);

    QWidget* settings = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(settings);

    QWidget* ecg = createMeasurementSettingsItem(
        "ECG",
        SENSOR_SAMPLERATES_ECG,
        config.sample_rates.by_sensor.ECG,
        [this](uint16_t val) {
            this->config.sample_rates.by_sensor.ECG = val;
        });
    layout->addWidget(ecg);

    QWidget* hr = createMeasurementSettingsItem(
        "HR",
        SENSOR_SAMPLERATES_ONOFF,
        config.sample_rates.by_sensor.HeartRate,
        [this](uint16_t val) {
            this->config.sample_rates.by_sensor.HeartRate = val;
        });
    layout->addWidget(hr);

    QWidget* accel = createMeasurementSettingsItem(
        "Acceleration",
        SENSOR_SAMPLERATES_IMU,
        config.sample_rates.by_sensor.Acceleration,
        [this](uint16_t val) {
            this->config.sample_rates.by_sensor.Acceleration = val;
        });
    layout->addWidget(accel);

    QWidget* gyro = createMeasurementSettingsItem(
        "Gyro",
        SENSOR_SAMPLERATES_IMU,
        config.sample_rates.by_sensor.Gyro,
        [this](uint16_t val) {
            this->config.sample_rates.by_sensor.Gyro = val;
        });
    layout->addWidget(gyro);

    QWidget* magn = createMeasurementSettingsItem(
        "Magnetometer",
        SENSOR_SAMPLERATES_IMU,
        config.sample_rates.by_sensor.Magnetometer,
        [this](uint16_t val) {
            this->config.sample_rates.by_sensor.Magnetometer = val;
        });
    layout->addWidget(magn);

    QWidget* temp = createMeasurementSettingsItem(
        "Temp",
        SENSOR_SAMPLERATES_ONOFF,
        config.sample_rates.by_sensor.Temperature,
        [this](uint16_t val) {
            this->config.sample_rates.by_sensor.Temperature = val;
        });
    layout->addWidget(temp);

    QWidget* dev = createDeviceSettingsItem();
    layout->addWidget(dev);

    ui->settingsScrollArea->setWidget(settings);
}

void MainWindow::onUpdateDeviceList(const QList<QBluetoothDeviceInfo>& devices)
{
    ui->deviceList->clear();
    for(const auto& dev : devices)
    {
        if(dev.name().isEmpty())
            ui->deviceList->addItem(dev.deviceUuid().toString());
        else
            ui->deviceList->addItem(dev.name());
    }
}

void MainWindow::onScannerStateChanged(Scanner::State state)
{
    switch(state)
    {
        case Scanner::Stopped:
        {
            ui->startScanButton->show();
            ui->stopScanButton->hide();
            break;
        }
        case Scanner::Scanning:
        {
            ui->startScanButton->hide();
            ui->stopScanButton->show();
            break;
        }
    }
}

QWidget* MainWindow::createMeasurementSettingsItem(
    const QString& name,
    const QList<uint16_t>& sampleRates,
    uint16_t current,
    std::function<void(uint16_t)> onValueChanged)
{
    QWidget* item = new QWidget();
    QGridLayout* layout = new QGridLayout(item);
    {
        QLabel* label = new QLabel(name);
        layout->addWidget(label, 0, 0);

        QComboBox* dropdown = new QComboBox();
        int select = 0;
        for(auto i = 0; i < sampleRates.length(); i++)
        {
            int value = sampleRates[i];

            if(value == SENSOR_OFF)
                dropdown->addItem("Off", SENSOR_OFF);
            else if (value == SENSOR_ON)
                dropdown->addItem("On", SENSOR_ON);
            else
                dropdown->addItem(QString::asprintf("%d Hz", value), value);

            if(value == current)
                select = i;
        }
        dropdown->setCurrentIndex(select);

        connect(dropdown, &QComboBox::currentIndexChanged, this, [this, onValueChanged, dropdown](int index) {
            int value = dropdown->itemData(index).toInt();
            onValueChanged(value);
            this->onSettingsEdited();
        });

        layout->addWidget(dropdown, 0, 1);
    }
    return item;
}

QWidget* MainWindow::createDeviceSettingsItem()
{
    QWidget* item = new QWidget();
    QGridLayout* layout = new QGridLayout(item);
    {
        QLabel* wakeupLabel = new QLabel("Wake up device when");
        layout->addWidget(wakeupLabel, 0, 0);

        QList<QPair<QString, SensorWakeUp>> wakeupOptionItems = {
            { "Always on", SensorWakeUpAlwaysOn },
            { "Connected", SensorWakeUpConnector },
            { "Movement", SensorWakeUpMovement },
            { "Single tap", SensorWakeUpSingleTapOn },
            { "Double tap", SensorWakeUpDoubleTapOn },
        };

        QList<QPair<QString, uint16_t>> sleepDelayItems = {
            { "Never (double tap to sleep)", 0 },
            { "30 seconds", 30 },
            { "1 minute", 60 },
            { "5 minutes", 5 * 60 },
            { "15 minutes", 15 * 60 },
            { "30 minutes", 30 * 60 },
            { "1 hour", 60 * 60 },
            { "2 hours", 2 * 60 * 60 },
            { "3 hours", 3 * 60 * 60 },
            { "6 hours", 6 * 60 * 60 },
            { "12 hours", 12 * 60 * 60 }, // Still fits unsigned 16-bit int
        };

        QComboBox* wakeupOptions = new QComboBox();
        {
            int i = 0;
            int select = 0;
            for(auto& item : wakeupOptionItems)
            {
                wakeupOptions->addItem(item.first, item.second);
                if(item.second == this->config.wakeup_behavior)
                    select = i;
                i++;
            }
            wakeupOptions->setCurrentIndex(select);
        }
        layout->addWidget(wakeupOptions, 0, 1);

        QLabel* sleepDelayLabel = new QLabel("Automatic sleep after");
        layout->addWidget(sleepDelayLabel, 1, 0);

        QComboBox* sleepDelayOptions = new QComboBox();
        {
            int i = 0;
            int select = 0;
            for(auto& item : sleepDelayItems)
            {
                sleepDelayOptions->addItem(item.first, item.second);
                if(item.second == this->config.sleep_delay)
                    select = i;
                i++;
            }
            sleepDelayOptions->setCurrentIndex(select);
        }
        layout->addWidget(sleepDelayOptions, 1, 1);

        connect(wakeupOptions, &QComboBox::currentIndexChanged, this, [this, wakeupOptions](int index) {
            uint8_t value = wakeupOptions->itemData(index).toInt();
            this->config.wakeup_behavior = value;
            this->onSettingsEdited();
        });

        connect(sleepDelayOptions, &QComboBox::currentIndexChanged, this, [this, sleepDelayOptions](int index) {
            uint16_t value = sleepDelayOptions->itemData(index).toInt();
            this->config.sleep_delay = value;
            this->onSettingsEdited();
        });
    }
    return item;
}
