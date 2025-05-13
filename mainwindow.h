#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>
#include <QTextStream>
#include <QtCharts/QDateTimeAxis>
#include <QMouseEvent>
#include <QPointF>
#include <QToolTip>
#include <QDateTime>
#include <QTextEdit>
QT_CHARTS_USE_NAMESPACE

namespace Ui {
class MainWindow;
}

//  stringToHex는 전역 함수이므로 클래스 외부에서 선언합니다.
QByteArray stringToHex(const QString &input);

class MainWindow : public QMainWindow
{
    Q_OBJECT



public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    //float getTemperatureValue(const QString &data, int channelIndex);
    void startMeasurement(int interval, int count);
    void addTimeMarker(const QString &label);


private slots:
    void on_pushButton_connect_clicked();
    void on_pushButton_disconnect_clicked();
    void on_pushButton_send_clicked();
    void readSerialData();
    void updateGraph();
    void on_clearButton_clicked();
    void on_savebutton_clicked();
    void sendPollCommand();
    void updatePorts();
    void on_timeConnectClicked();
    void on_pushButton_connect_2_clicked();
    void on_saveGraphButton_clicked();
    void onMeasurementFinished();
    void on_loadDataButton_clicked();
    void on_pauseButton_clicked();
    void onToggleStopClicked();
    void onToggleMaxClicked();
    void onToggleT1Clicked();
    void onToggleCClicked();
    void onToggleRECClicked();
    void onToggleCancelClicked();

    //이것도 마찬가지.
    void sendInquiryCommand();



private:
    Ui::MainWindow *ui;
    QSerialPort *serial;
    QChart *chart;
    QLineSeries *series[4];
    QDateTimeAxis *axisX;
    QValueAxis *axisY;
    QChartView *chartView;
    QTimer *updateTimer;
    qint64 startTime;
    qint64 endTime;
    int sendCounter;
    int receiveCounter;
    QTimer *commandTimer;
    int currentMeasureCount = 0;
    int totalMeasureCount = 0;
    QTimer *measureTimer;
    int receivedCount;
    int totalCount;
    int measurementInterval = 1;  // 측정 간격 (초)
    int measurementCount = 0;
    qint64 totalMeasurementTime = 0;
    int totalMeasureTime = 0;
    QString lastReceivedData;
    bool isMeasuring = false;
    //bool isMeasuring;// 이것을 false 로 해버리면 하루종일 뭔 값을 넣든 false 로 나오는 느낌이라서 false 없애버림.
    QDateTime currentTime;
    void adjustXAxis();
    bool isLoadingFromFile = false;
    QVector<double> temperatureData[4];
    void updateTemperatureStatistics();
    void on_addMarkerButton_clicked();
    bool isPaused = false;
    QList<QLineSeries*> markerLines;
    QList<QGraphicsSimpleTextItem*> markerLabels;
    QTextEdit *markerLog;

    //안되면 그냥 빼고 ui 버튼 클릭만 사용.
    QTimer *inquiryTimer;

    //float getTemperatureValue(const QString &data, int channelIndex);
};

#endif // MAINWINDOW_H
