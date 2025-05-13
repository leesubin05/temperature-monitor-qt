#ifndef FULLSCREENCHARTVIEW_H
#define FULLSCREENCHARTVIEW_H

#include <QtCharts/QChartView>

QT_CHARTS_USE_NAMESPACE

class FullScreenChartView : public QChartView
{
    Q_OBJECT
public:
    explicit FullScreenChartView(QChart *chart, QWidget *parent = nullptr)
        : QChartView(chart, parent) {
        this->setRenderHint(QPainter::Antialiasing);
        this->setWindowFlags(Qt::Window); // 새 창으로
        this->resize(1200, 800);          // 원하는 크기로 설정
    }
};

#endif // FULLSCREENCHARTVIEW_H

