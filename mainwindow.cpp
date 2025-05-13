#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "measurementdialog.h"
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QtSerialPort/QSerialPortInfo>
#include <QDebug>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>
#include <QScrollBar>
#include "customchartview.h"
#include "fullscreenchartview.h"

QT_CHARTS_USE_NAMESPACE

// 문자열을 HEX로 변환하는 함수 (공백, 특수문자 제거 포함)
QByteArray stringToHex(const QString &input) {
    QByteArray hexData;
    QString cleanedInput = input;
    cleanedInput.remove(QRegExp("[^0-9A-Fa-f]")); // 16진수 문자만 남김
    if (cleanedInput.size() % 2 != 0)
        cleanedInput.prepend("0"); // 홀수 길이면 앞에 0 추가

    for (int i = 0; i < cleanedInput.length(); i += 2) {
        QString byteString = cleanedInput.mid(i, 2);
        bool ok;
        char byte = static_cast<char>(byteString.toUInt(&ok, 16));
        if (ok)
            hexData.append(byte);
    }
    return hexData;
}

// 생성자: UI 초기화 및 시리얼포트/차트/타이머 연결
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), serial(new QSerialPort(this)),
      sendCounter(0), receiveCounter(0), receivedCount(0), totalCount(0) {

    ui->setupUi(this);
    ui->receivedData->setWordWrapMode(QTextOption::NoWrap);
    updatePorts(); // 포트 목록 갱신

    // 기존 시그널-슬롯 연결 제거 ( 모든 버튼 끊고 연결해서 중복연결 절대 안생기게 방지 )
    disconnect(ui->pushButton_connect, nullptr, nullptr, nullptr);
    disconnect(ui->pushButton_send, nullptr, nullptr, nullptr);
    disconnect(ui->pushButton_Disconnect, nullptr, nullptr, nullptr);
    disconnect(ui->clearButton, nullptr, nullptr, nullptr);
    disconnect(serial, nullptr, nullptr, nullptr);
    disconnect(ui->savebutton, nullptr, nullptr, nullptr);
    disconnect(ui->saveGraphButton, nullptr, nullptr, nullptr);
    disconnect(ui->loadButton, nullptr, nullptr, nullptr);
    disconnect(ui->addMarkerButton_2, nullptr, nullptr, nullptr);
    disconnect(ui->btn_toggleUnit_STOP, nullptr, nullptr, nullptr);
    disconnect(ui->btn_toggleUnit_MAX, nullptr, nullptr, nullptr);
    disconnect(ui->btn_toggleUnit_T1, nullptr, nullptr, nullptr);
    disconnect(ui->btn_toggleUnit_C, nullptr, nullptr, nullptr);
    disconnect(ui->btn_toggleUnit_REC, nullptr, nullptr, nullptr);
    disconnect(ui->btn_toggleUnit_CancelALL, nullptr, nullptr, nullptr);


    // 시그널-슬롯 연결
    connect(ui->pushButton_connect, &QPushButton::clicked, this, &MainWindow::on_pushButton_connect_clicked);
    connect(ui->pushButton_send, &QPushButton::clicked, this, &MainWindow::on_pushButton_send_clicked);
    connect(ui->pushButton_Disconnect, &QPushButton::clicked, this, &MainWindow::on_pushButton_disconnect_clicked);
    connect(ui->clearButton, &QPushButton::clicked, this, &MainWindow::on_clearButton_clicked);
    connect(serial, &QSerialPort::readyRead, this, &MainWindow::readSerialData);
    connect(ui->savebutton, &QPushButton::clicked, this, &MainWindow::on_savebutton_clicked);
    connect(ui->TimeConnect, &QPushButton::clicked, this, &MainWindow::on_timeConnectClicked);
    connect(ui->saveGraphButton, &QPushButton::clicked, this, &MainWindow::on_saveGraphButton_clicked);
    connect(ui->loadButton, &QPushButton::clicked, this, &MainWindow::on_loadDataButton_clicked);
    connect(ui->addMarkerButton_2, &QPushButton::clicked, this, &MainWindow::on_addMarkerButton_clicked);
    connect(ui->btn_toggleUnit_STOP, &QPushButton::clicked, this, &MainWindow::onToggleStopClicked);
    connect(ui->btn_toggleUnit_MAX, &QPushButton::clicked, this, &MainWindow::onToggleMaxClicked);
    connect(ui->btn_toggleUnit_T1, &QPushButton::clicked, this, &MainWindow::onToggleT1Clicked);
    connect(ui->btn_toggleUnit_C, &QPushButton::clicked, this, &MainWindow::onToggleCClicked);
    connect(ui->btn_toggleUnit_REC, &QPushButton::clicked, this, &MainWindow::onToggleRECClicked);
    connect(ui->btn_toggleUnit_CancelALL, &QPushButton::clicked, this, &MainWindow::onToggleCancelClicked);




    //체크박스와 그래프 연결 - series 0, 1, 2, 3 로 체크박스마다 다르게해서 각각 따로 따로 설정할 수 있게 놔둠
    connect(ui->checkBox_ch1, &QCheckBox::toggled, this, [=](bool checked) {
        series[0]->setVisible(checked);
    });
    connect(ui->checkBox_ch2, &QCheckBox::toggled, this, [=](bool checked) {
        series[1]->setVisible(checked);
    });
    connect(ui->checkBox_ch3, &QCheckBox::toggled, this, [=](bool checked) {
        series[2]->setVisible(checked);
    });
    connect(ui->checkBox_ch4, &QCheckBox::toggled, this, [=](bool checked) {
        series[3]->setVisible(checked);
    });


    // 차트 초기화
    chart = new QChart();
    chart->legend()->setVisible(true);

    for (int i = 0; i < 4; ++i) {
        series[i] = new QLineSeries();
        series[i]->setName(QString("CH%1").arg(i + 1));
        switch (i) {
            case 0: series[i]->setColor(Qt::red); break;
            case 1: series[i]->setColor(Qt::green); break;
            case 2: series[i]->setColor(Qt::blue); break;
        case 3: series[i]->setColor(Qt::magenta); break;
        }
        chart->addSeries(series[i]);
    }

    axisX = new QDateTimeAxis;
    axisX->setFormat("HH:mm:ss");
    axisX->setTitleText("TIME");
    QDateTime now = QDateTime::currentDateTime();
    axisX->setRange(now.addSecs(-60), now);
    chart->addAxis(axisX, Qt::AlignBottom);

    for (int i = 0; i < 4; ++i) {
        series[i]->attachAxis(axisX);
    }

    //그래프 y축의 온도는 -20도에서 100도 까지.
    axisY = new QValueAxis;
    axisY->setRange(-20, 100);
    axisY->setTickInterval(10);
    axisY->setTickCount(13);
    axisY->setLabelFormat("%.0f°C");
    axisY->setTitleText("Temperature (°C)");
    chart->addAxis(axisY, Qt::AlignLeft);

    for (int i = 0; i < 4; ++i) {
        series[i]->attachAxis(axisY);
    }

    // CustomChartView 사용
    chartView = new CustomChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    // 👇 사이즈 명시적으로 지정
    chartView->setMinimumSize(800, 400);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    ui->verticalLayout->addWidget(chartView);

    // 더블클릭 시 새 창에 복제 그래프 띄우기 (원본은 그대로 유지)
    CustomChartView *customView = qobject_cast<CustomChartView*>(chartView);
    if (customView) {
        connect(customView, &CustomChartView::doubleClicked, this, [=]() {
            QChart *popupChart = new QChart();
            popupChart->setTitle(chart->title());
            popupChart->legend()->setVisible(chart->legend()->isVisible());

            // 시리즈 복제
            for (int i = 0; i < 4; ++i) {
                QLineSeries *newSeries = new QLineSeries();
                *newSeries << series[i]->points(); // 데이터 복사
                newSeries->setName(series[i]->name());
                newSeries->setColor(series[i]->color());
                popupChart->addSeries(newSeries);
            }

            // 축 복제
            QDateTimeAxis *popupAxisX = new QDateTimeAxis;
            popupAxisX->setFormat("HH:mm:ss");
            popupAxisX->setTitleText("TIME");
            popupAxisX->setRange(axisX->min(), axisX->max());
            popupChart->addAxis(popupAxisX, Qt::AlignBottom);

            QValueAxis *popupAxisY = new QValueAxis;
            popupAxisY->setRange(axisY->min(), axisY->max());
            popupAxisY->setTickInterval(axisY->tickInterval());
            popupAxisY->setTickCount(axisY->tickCount());
            popupAxisY->setLabelFormat(axisY->labelFormat());
            popupAxisY->setTitleText(axisY->titleText());
            popupChart->addAxis(popupAxisY, Qt::AlignLeft);

            // 축 연결
            foreach (QAbstractSeries *s, popupChart->series()) {
                s->attachAxis(popupAxisX);
                s->attachAxis(popupAxisY);
            }

            // 팝업 차트 창 띄우기(확대되고 실시간으로 되는게 아니라 그냥 더블클릭하면 그때까지 측정한 그래프 만큼 나옴.
            FullScreenChartView *popup = new FullScreenChartView(popupChart);
            popup->setAttribute(Qt::WA_DeleteOnClose);
            popup->show();
        });
    }
    //ui 온도계 활성화 시키기 위한 설정
    inquiryTimer = new QTimer(this);
    connect(inquiryTimer, &QTimer::timeout, this, &MainWindow::sendInquiryCommand);
    inquiryTimer->start(1000); // 1초마다 "A" 명령 보냄


    // 그래프 갱신 및 명령 송신 타이머 설정
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::updateGraph);

    commandTimer = new QTimer(this);
    connect(commandTimer, &QTimer::timeout, this, &MainWindow::sendPollCommand);


    // 시작 시간 설정 및 그래프 타이머 시작
    startTime = QDateTime::currentMSecsSinceEpoch();
    updateTimer->start(1000);

    qDebug() << "chartView parent:" << chartView->parent();
    qDebug() << "chartView size:" << chartView->size();
    qDebug() << "layout count:" << ui->verticalLayout->count();

    //로그 찍기위해 알맞은 범위 설정
    markerLog = new QTextEdit(this);
    markerLog->setReadOnly(true);
    markerLog->setFixedSize(250,120);
    markerLog->move(10, 10);
    markerLog->setStyleSheet("background-color: rgba(255, 255, 255, 200); boder: 1px solid gray;");
    markerLog->setPlaceholderText("마커 로그");

    // 이 코드는 반드시 chartView가 레이아웃에서 완전히 배치된 이후에 실행되어야 함
    int margin = 10;
    int logWidth = 250;
    int logHeight = 120;

    markerLog->setFixedSize(logWidth, logHeight);

    // 오른쪽 위 위치 계산
    int x = ui->chartContainer->width() - logWidth - margin;
    int y = margin;  // 위쪽에서 margin 만큼 떨어짐

    // chartView 기준 좌표 → MainWindow 좌표로 변환
    QPoint globalPos = ui->chartContainer->mapToParent(QPoint(x, y));
    markerLog->move(globalPos);
}






// 소멸자: UI 메모리 해제
MainWindow::~MainWindow() {
    delete ui;
}

// 사용 가능한 시리얼 포트 목록 갱신
void MainWindow::updatePorts() {
    ui->comboBox->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports)
        ui->comboBox->addItem(port.portName());
}


// 연결 버튼 클릭 시 동작(맞지 않은 값이 들어가면 연결 실패 뜨게함)
void MainWindow::on_pushButton_connect_clicked()
{

    //포트는 지금 컴퓨터에 연결되어있는 포트만 나타나게 표현
    QString portName = ui->comboBox->currentText();
    serial->setPortName(portName);
    serial->setBaudRate(ui->comboBox_2->currentText().toInt());

    // DataBits 설정 기본값은 8
    QString dataBitsStr = ui->comboBox_3->currentText();
    if (dataBitsStr == "5") serial->setDataBits(QSerialPort::Data5);
    else if (dataBitsStr == "6") serial->setDataBits(QSerialPort::Data6);
    else if (dataBitsStr == "7") serial->setDataBits(QSerialPort::Data7);
    else serial->setDataBits(QSerialPort::Data8); // default

    // Parity 기본값은 None
    QString parityStr = ui->comboBox_4->currentText();
    if (parityStr == "None") serial->setParity(QSerialPort::NoParity);
    else if (parityStr == "Even") serial->setParity(QSerialPort::EvenParity);
    else if (parityStr == "Odd") serial->setParity(QSerialPort::OddParity);
    else if (parityStr == "Mark") serial->setParity(QSerialPort::MarkParity);
    else if (parityStr == "Space") serial->setParity(QSerialPort::SpaceParity);

    // StopBits 기본값은 1
    QString stopBitsStr = ui->comboBox_5->currentText();
    if (stopBitsStr == "1") serial->setStopBits(QSerialPort::OneStop);
    else if (stopBitsStr == "1.5") serial->setStopBits(QSerialPort::OneAndHalfStop);
    else if (stopBitsStr == "2") serial->setStopBits(QSerialPort::TwoStop);

    // FlowControl 기본값은 None
    QString flowStr = ui->comboBox_6->currentText();
    if (flowStr == "None") serial->setFlowControl(QSerialPort::NoFlowControl);
    else if (flowStr == "RTS/CTS") serial->setFlowControl(QSerialPort::HardwareControl);
    else if (flowStr == "XON/XOFF") serial->setFlowControl(QSerialPort::SoftwareControl);

    //알맞은 갓을 넣어서 알맞은 값 나오면 A=
    if (serial->open(QIODevice::ReadWrite)) {
        ui->label_status->setText(portName + " Connected to the port.");
        ui->label_status->setStyleSheet("color: blue;");
    } else {
        ui->label_status->setText("Failed to connect to port..");
        ui->label_status->setStyleSheet("color: red;");
    }
}

//그냥 connect 가 아니라 connect2는 간격 횟수 설정 하는 버튼클릭 함수
void MainWindow::on_pushButton_connect_2_clicked() {
    MeasurementDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        int interval = dialog.samplingInterval();  // 측정 간격
        int count = dialog.measureCount();         // 측정 횟수

        // 전체 측정 시간 계산 및 표시
        int totalMeasurementTime = interval * count;
        int minutes = totalMeasurementTime / 60;
        int seconds = totalMeasurementTime % 60;
        ui->label_totalMeasurementTime->setText(QString("Total Time: %1:%2").arg(minutes, 2, 10, QLatin1Char('0')).arg(seconds, 2, 10, QLatin1Char('0')));

        // 측정 시작 함수 호출
        startMeasurement(interval, count);
    }
}

// x 축을 조절해주는 함수
void MainWindow::adjustXAxis()
{
    if (isLoadingFromFile) return; // 불러오는 중일 땐 X축 조절 금지

    if (!series[0]->points().isEmpty()) {
        QDateTime firstTime = QDateTime::fromMSecsSinceEpoch(series[0]->points().first().x());
        QDateTime lastTime = QDateTime::fromMSecsSinceEpoch(series[0]->points().last().x());

        axisX->setRange(firstTime, lastTime.addSecs(5));
    }
}



void MainWindow::startMeasurement(int interval, int count)
{
    isMeasuring = true;

    inquiryTimer->stop();
    // 실시간 조회 타이머가 있으면 중지
    if (inquiryTimer && inquiryTimer->isActive()) {
        inquiryTimer->stop();

    }

     //여기서 트루 시키고 finish 함수에서 false 로 바꿔주기

    QDateTime now = QDateTime::currentDateTime();

    qint64 startTimeMs = now.toMSecsSinceEpoch(); //  이름 변경
    qint64 endTimeMs = startTimeMs + static_cast<qint64>(interval) * count * 1000;

    QDateTime startDateTime = QDateTime::fromMSecsSinceEpoch(startTimeMs);
    QDateTime endDateTime = QDateTime::fromMSecsSinceEpoch(endTimeMs);

    axisX->setRange(startDateTime, endDateTime);
    startTime = startTimeMs;
    endTime = endTimeMs;

    updateGraph();  // 초기 그래프 범위 갱신

    currentMeasureCount = 0;
    totalMeasureCount = count;

    commandTimer->stop();
    commandTimer->setInterval(interval * 1000);

    disconnect(commandTimer, nullptr, nullptr, nullptr);

    connect(commandTimer, &QTimer::timeout, this, [=]() {
        if (currentMeasureCount >= totalMeasureCount) {
            onMeasurementFinished();
            return;
        }
        sendPollCommand();
        currentMeasureCount++;
        qDebug() << "Measurement command sent, count =" << currentMeasureCount;
    });

    commandTimer->start();

    qDebug() << "Measurement started: Interval =" << interval << "seconds, count =" << count;
}



void MainWindow::onMeasurementFinished() {


    // 측정이 끝나면 타이머 멈추기
    updateTimer->stop();   // 그래프 갱신 타이머 멈춤
    commandTimer->stop();  // 명령 송신 타이머 멈춤

    isMeasuring = false; //측정이 끝나면 false 로 끊어준 다음에 ui 온도계는 다시 작동 시작.

    // 마지막 시간 갱신 후 멈춤
    currentTime = QDateTime::currentDateTime();

    //  실시간 조회 타이머 재개
    if (inquiryTimer && !inquiryTimer->isActive()) {
        inquiryTimer->start();

    }
}



void MainWindow::on_timeConnectClicked() {
    if (!serial->isOpen()) {
        QMessageBox::warning(this, "Port connection error", "Serial port is not connected.");
        return;
    }

    MeasurementDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        int interval = dialog.samplingInterval();
        int count = dialog.measureCount();

        currentMeasureCount = 0;
        totalMeasureCount = count;

        commandTimer->stop();

        commandTimer->setInterval(interval);

        endTime = startTime + interval * totalCount;

        // 시그널 중복 연결 방지 (기존 연결 해제)
        disconnect(commandTimer, nullptr, nullptr, nullptr);

        connect(commandTimer, &QTimer::timeout, this, [=]() {
            if (currentMeasureCount >= totalMeasureCount) {
                commandTimer->stop();
                isMeasuring = false;
                QMessageBox::information(this, "Measurement completed", "The specified number of measurements have been completed.");
                return;
            }
            sendPollCommand();
            currentMeasureCount++;
        });

        isMeasuring = true;

        commandTimer->start();
    }
}

// 그래프 X축 시간 범위 갱신
void MainWindow::updateGraph() {
    if (startTime != 0 && endTime != 0) {
            QDateTime start = QDateTime::fromMSecsSinceEpoch(startTime);
            QDateTime end = QDateTime::fromMSecsSinceEpoch(endTime);
            axisX->setRange(start, end);
        }
}

// 시리얼 데이터 수신 처리
void MainWindow::readSerialData() {
    if (isPaused) return;
    qDebug() << "[DEBUG] isMeasuring = " << isMeasuring;
    static QByteArray buffer;
    buffer.append(serial->readAll());

    while (true) {
        int startIdx = buffer.indexOf(0x02);
        int endIdx = buffer.indexOf(0x03, startIdx + 1);
        if (startIdx == -1 || endIdx == -1) break;

        QByteArray packet = buffer.mid(startIdx + 1, endIdx - startIdx - 1);
        buffer.remove(0, endIdx + 1);

        const int offset = 6;
        if (packet.size() < offset + 8) continue;

        QDateTime now = QDateTime::currentDateTime();
        if (startTime == 0) startTime = now.toMSecsSinceEpoch();

        QStringList hexParts, tempStrings;

        for (int i = 0; i < 4; ++i) {
            int index = offset + i * 2;
            if (packet.size() >= index + 2) {
                qint16 rawValue = static_cast<qint16>((static_cast<quint8>(packet[index]) << 8) |
                                                       static_cast<quint8>(packet[index + 1]));
                double temperature = rawValue / 10.0;
                temperatureData[i].append(temperature);  // 항상 저장

                //  조회 모드든 측정 모드든 라벨만은 항상 갱신
                QLabel* tempLabel = nullptr;
                switch (i) {
                    case 0: tempLabel = ui->label_temp1; break;
                    case 1: tempLabel = ui->label_temp2; break;
                    case 2: tempLabel = ui->label_temp3; break;
                    case 3: tempLabel = ui->label_temp4; break;
                }
                if (tempLabel) {
                    tempLabel->setText(QString("T%1: %2°C").arg(i + 1).arg(temperature, 0, 'f', 1));
                }

                //  측정 중일 때만 그래프 및 로그 업데이트
                if (isMeasuring) {
                    qint64 timestamp = now.toMSecsSinceEpoch();
                    series[i]->append(timestamp, temperature);

                    if (ui->checkBox_showTemp->isChecked()) {
                        tempStrings << QString("CH%1=%2°C").arg(i + 1).arg(temperature, 0, 'f', 2);
                        qDebug() << QString("CH%1 = %2°C").arg(i + 1).arg(temperature);
                    }
                }
            }
        }

        if (isMeasuring) {
            if (ui->checkBox_showHex->isChecked()) {
                for (int i = 0; i < packet.size(); ++i) {
                    hexParts << QString("%1").arg(static_cast<quint8>(packet[i]), 2, 16, QLatin1Char('0')).toUpper();
                }
            }

            QString displayLine;
            QString timeStamp = "[" + now.toString("yyyy-MM-dd, HH:mm:ss") + "]";

            if (ui->checkBox_showHex->isChecked()) {
                displayLine += "HEX: " + hexParts.join(" ");
            }

            if (ui->checkBox_showTemp->isChecked()) {
                if (!displayLine.isEmpty()) displayLine += " | ";
                displayLine += tempStrings.join(", ");
            }

            if (!displayLine.isEmpty()) {
                ui->receivedData->append(timeStamp + displayLine);
                ui->receivedData->verticalScrollBar()->setValue(ui->receivedData->verticalScrollBar()->maximum());
                receiveCounter++;
                ui->label_receiveCounter->setText(QString::number(receiveCounter));
            }

            updateTemperatureStatistics();
            adjustXAxis();
        }
    }

    //  그래프 미사용 시 라벨 초기화 (이미 위에서 갱신하므로 생략 가능하지만 남겨둠)
    for (int i = 0; i < 4; ++i) {
        if (temperatureData[i].isEmpty()) {
            QLabel* tempLabel = nullptr;
            switch (i) {
                case 0: tempLabel = ui->label_temp1; break;
                case 1: tempLabel = ui->label_temp2; break;
                case 2: tempLabel = ui->label_temp3; break;
                case 3: tempLabel = ui->label_temp4; break;
            }
            if (tempLabel) tempLabel->clear();
        }
    }
}


// 전송 버튼 클릭 시 동작
void MainWindow::on_pushButton_send_clicked() {
    if (!serial->isOpen()) return;

    // lineEdit에서 입력된 데이터를 가져옵니다
    QString data = ui->lineEdit->text();
    QByteArray sendData;

    // HEX 변환을 체크한 경우
    if (ui->checkBox_2->isChecked()) {
        sendData = stringToHex(data); // HEX 전송
    } else {
        sendData = data.toUtf8();     // 일반 텍스트
    }

    // 체크박스에 따라 \r과 \n 추가
    if (ui->checkBox->isChecked()) sendData.append('\r');
    if (ui->checkBox_3->isChecked()) sendData.append('\n');

    // 시리얼 포트로 데이터 전송
    serial->write(sendData);

    // 송수신 카운트 증가
    sendCounter++;
    ui->label_sendCounter->setText(QString::number(sendCounter));

    // 송신된 데이터를 receivedData에 추가 (송신 후 표시)
    QString timeStamp = "[" + QDateTime::currentDateTime().toString("yyyy-MM-dd, HH:mm:ss") + "]";
    ui->receivedData->append(timeStamp + " SENT: " + data);
    ui->receivedData->verticalScrollBar()->setValue(ui->receivedData->verticalScrollBar()->maximum());

    // 송신 버튼 색상 변화 (초록색으로 표시)
    ui->pushButton_send->setStyleSheet("background-color: green;");

    // 송신 후 일정 시간(예: 2초) 뒤에 버튼 색상 원상복구
    QTimer::singleShot(2000, this, [this]() {
        ui->pushButton_send->setStyleSheet("");  // 원래 색으로 복구
    });
}

// 연결 해제 버튼
void MainWindow::on_pushButton_disconnect_clicked() {
    if (serial->isOpen()) {
        serial->close();
        commandTimer->stop();
        ui->label_status->setText("Current: Disconnected");
        ui->label_status->setStyleSheet("color: red;");
    }
}

// 화면 초기화 버튼
void MainWindow::on_clearButton_clicked() {
    ui->receivedData->clear();
    sendCounter = 0;
    receiveCounter = 0;
    ui->label_sendCounter->setText("0");
    ui->label_receiveCounter->setText("0");

    for (int i = 0; i < 4; ++i) {
        series[i]->clear();
        temperatureData[i].clear();  // 측정 데이터 초기화

        // 평균/최대/최소 라벨 초기화
        QLabel *avgLabel = this->findChild<QLabel*>(QString("label_avg_CH%1").arg(i + 1));
        QLabel *maxLabel = this->findChild<QLabel*>(QString("label_max_CH%1").arg(i + 1));
        QLabel *minLabel = this->findChild<QLabel*>(QString("label_min_CH%1").arg(i + 1));

        if (avgLabel) avgLabel->setText("Avg: --°C");
        if (maxLabel) maxLabel->setText("Max: --°C");
        if (minLabel) minLabel->setText("Min: --°C");
    }

    //QList<QGraphicsItem*> items = chart->scene()->items();
    // 저장된 마커들만 제거
    for (auto line : markerLines) {
        chart->removeSeries(line);
        delete line;
    }
    markerLines.clear();

    for (auto label : markerLabels) {
        chart->scene()->removeItem(label);
        delete label;
    }
    markerLabels.clear();


    startTime = QDateTime::currentMSecsSinceEpoch();
    if (markerLog) markerLog->clear();

}


// 1초마다 명령 송신 ("A" 또는 "41")
void MainWindow::sendPollCommand() {
    if (!serial->isOpen()) return;

    QByteArray command;
    if (ui->checkBox_2->isChecked()) {
        command = QByteArray::fromHex("41");
    } else {
        command = "A";
    }

    serial->write(command);
    serial->flush();
}


void MainWindow::on_saveGraphButton_clicked() {
    // QChartView에서 그래프를 캡처
    QPixmap pixmap = chartView->grab();  // 차트의 현재 상태를 QPixmap으로 캡처

    // 저장할 파일 경로 선택
    QString fileName = QFileDialog::getSaveFileName(this, "Save Graph", "", "PNG Files (*.png);;JPEG Files (*.jpg)");
    if (!fileName.isEmpty()) {
        // 선택한 경로로 이미지를 저장
        if (pixmap.save(fileName)) {
            QMessageBox::information(this, "Graph Saved", "Graph has been saved successfully.");
        } else {
            QMessageBox::warning(this, "Save Error", "Failed to save the graph.");
        }
    }
}

// 수신 데이터 저장
void MainWindow::on_savebutton_clicked() {
    QString fileName = QFileDialog::getSaveFileName(this, "Save Data", "", "Text Files (*.txt)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << ui->receivedData->toPlainText();
            file.close();
        } else {
            QMessageBox::warning(this, "Save Error", "Failed to save the file.");
        }
    }
}


//이전에 저장한 온도 텍스트 파일 불러오면 그래프에 나오게 하는 함수
void MainWindow::on_loadDataButton_clicked() {
    QString fileName = QFileDialog::getOpenFileName(this, "Load Data", "", "Text Files (*.txt)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            isLoadingFromFile = true;

            QTextStream in(&file);

            for (int i = 0; i < 4; ++i)
                series[i]->clear();

            QVector<double> sum(4, 0.0);
            QVector<int> count(4, 0);
            QVector<double> max(4, std::numeric_limits<double>::lowest());
            QVector<double> min(4, std::numeric_limits<double>::max());

            QRegularExpression regex(R"(\[(.*?)\]CH1=(\-?\d+\.\d+)°C, CH2=(\-?\d+\.\d+)°C, CH3=(\-?\d+\.\d+)°C, CH4=(\-?\d+\.\d+)°C)");

            QDateTime firstTime, lastTime;
            bool firstLine = true;

            while (!in.atEnd()) {
                QString line = in.readLine();
                QRegularExpressionMatch match = regex.match(line);
                if (match.hasMatch()) {
                    QDateTime time = QDateTime::fromString(match.captured(1), "yyyy-MM-dd, HH:mm:ss");
                    if (!time.isValid()) continue;

                    qint64 msec = time.toMSecsSinceEpoch();
                    for (int i = 0; i < 4; ++i) {
                        double temp = match.captured(i + 2).toDouble();
                        series[i]->append(msec, temp);

                        temperatureData[i].append(temp);

                        sum[i] += temp;
                        count[i]++;
                        if (temp > max[i]) max[i] = temp;
                        if (temp < min[i]) min[i] = temp;
                    }

                    if (firstLine) {
                        firstTime = time;
                        firstLine = false;
                    }
                    lastTime = time;

                    ui->receivedData->append(line);
                }
            }

            file.close();

            if (firstTime.isValid() && lastTime.isValid())
                axisX->setRange(firstTime, lastTime.addSecs(5));

            // 평균, 최댓값, 최솟값 라벨 업데이트

            updateTemperatureStatistics();

            isLoadingFromFile = false;
            ui->receivedData->append("Load completed!");

        } else {
            QMessageBox::warning(this, "Load Error", "Failed to load the file.");
        }
    }
}

//평균값, 최댓값, 최솟값을 나타내기위해 사용되는 함수
void MainWindow::updateTemperatureStatistics() {
    for (int i = 0; i < 4; ++i) {
        if (temperatureData[i].isEmpty()) continue;

        double sum = std::accumulate(temperatureData[i].begin(), temperatureData[i].end(), 0.0);
        double avg = sum / temperatureData[i].size();
        double max = *std::max_element(temperatureData[i].begin(), temperatureData[i].end());
        double min = *std::min_element(temperatureData[i].begin(), temperatureData[i].end());

        QLabel *avgLabel = this->findChild<QLabel*>(QString("label_avg_CH%1").arg(i + 1));
        QLabel *maxLabel = this->findChild<QLabel*>(QString("label_max_CH%1").arg(i + 1));
        QLabel *minLabel = this->findChild<QLabel*>(QString("label_min_CH%1").arg(i + 1));

        if (avgLabel) avgLabel->setText(QString("Avg: %1°C").arg(avg, 0, 'f', 2));
        if (maxLabel) maxLabel->setText(QString("Max: %1°C").arg(max, 0, 'f', 2));
        if (minLabel) minLabel->setText(QString("Min: %1°C").arg(min, 0, 'f', 2));
    }
}


//마커 기능 함수
void MainWindow::addTimeMarker(const QString& label) {
    qint64 markerTime = QDateTime::currentDateTime().toMSecsSinceEpoch();

    if (!series[0]->points().isEmpty()) {
        markerTime = series[0]->points().last().x();
    }

    QLineSeries *marker = new QLineSeries();
    marker->append(markerTime, axisY->min());
    marker->append(markerTime, axisY->max());

    chart->addSeries(marker);
    marker->attachAxis(axisX);
    marker->attachAxis(axisY);

    //마커를 일자 선이 아닌 점선으로 해서 알아보기 쉽게 구현
    QPen pen(Qt::red);
    pen.setWidth(2);
    pen.setStyle(Qt::DashLine);
    marker->setPen(pen);

    markerLines.append(marker);  // <- 이거 꼭 있어야 clear()에서 제거됨

    // 텍스트 라벨 추가
    QGraphicsSimpleTextItem *textItem = new QGraphicsSimpleTextItem(label);
    textItem->setBrush(Qt::red);
    textItem->setPos(chart->mapToPosition(QPointF(markerTime, axisY->max())).x() + 5, 10);
    chart->scene()->addItem(textItem);
    markerLabels.append(textItem);  // <- 이것도 꼭 있어야 나중에 삭제 가능

    if (markerTime > axisX->max().toMSecsSinceEpoch()) {
        axisX->setMax(QDateTime::fromMSecsSinceEpoch(markerTime + 2000));
    }

    chart->update();

    // 로그 기록
    QString timeStr = QDateTime::fromMSecsSinceEpoch(markerTime).toString("yyyy-MM-dd HH:mm:ss");
    if (markerLog) {
        markerLog->append(timeStr + " - " + label);
        for (int i = 0; i < 4; ++i) {
            if (!temperatureData[i].isEmpty()) {
                double temp = temperatureData[i].last();
                markerLog->append(QString("CH%1: %2°C").arg(i + 1).arg(temp, 0, 'f', 1));
            } else {
                markerLog->append(QString("CH%1: --°C").arg(i + 1));
            }
        }
        markerLog->append("");
    }
}




void MainWindow::on_addMarkerButton_clicked() {
    qDebug() << "addMarkerButton clicked!";
    addTimeMarker("마킹");  // 날짜는 addTimeMarker 안에서 처리
}


//일시정지 버튼 생성

void MainWindow::on_pauseButton_clicked() {
    isPaused = !isPaused;

    if (isPaused) {
        commandTimer->stop(); // 명령 전송 중단
        ui->pauseButton->setText("Resume");
    } else {
        commandTimer->start(); // 명령 재시작
        ui->pauseButton->setText("Pause");
    }
}


//실제 온도계 버튼

// 일시정지 Hold 버튼
void MainWindow::onToggleStopClicked() {
    QByteArray cmd = "H";
    serial->write(cmd);
}

// 최댓값, 최솟값 만 표시 가능하게 하는 버튼
void MainWindow::onToggleMaxClicked() {
    QByteArray cmd = "M"; //ASC 4DH
    serial->write(cmd);
}

// T1, T2 값의 온도 차이를 나타내주는 버튼
void MainWindow::onToggleT1Clicked() {
    QByteArray cmd = "T"; //ASC 54H
    serial->write(cmd);
}

// 섭시/화씨 바꿔주는 버튼
void MainWindow::onToggleCClicked() {
    QByteArray cmd = "C"; //ASC43H
    serial->write(cmd);
}

// 데이터를 기록할 수 있게 해주는 버튼
void MainWindow::onToggleRECClicked() {
    QByteArray cmd = "E"; //ASC 45H
    serial->write(cmd);
}

// 이 버튼의 존재 유무에 따라 프로그램이 잘 실행되고 안되고가 나뉨 - 아마도 누르면 그냥 default 값 으로 바꿔주는거같음.
void MainWindow::onToggleCancelClicked() {
    QByteArray cmd = "N";
    serial->write(cmd);
}

//이것도 작동 안되면 그냥 없애기.
void MainWindow::sendInquiryCommand() {
    if (serial && serial->isOpen()) {
        serial->write("A");  // 조회 명령
    }
}



















