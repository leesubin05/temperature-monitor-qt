#ifndef CUSTOMCHARTVIEW_H
#define CUSTOMCHARTVIEW_H

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QLabel>

QT_CHARTS_USE_NAMESPACE

class CustomChartView : public QChartView
{
    Q_OBJECT
public:
    explicit CustomChartView(QChart *chart, QWidget *parent = nullptr);

    void setSeriesList(const QList<QLineSeries*>& seriesList); // 외부에서 시리즈 지정 가능

signals:
    void doubleClicked();

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QList<QLineSeries*> m_seriesList; // 그래프 시리즈 저장
    QLabel *tooltipLabel; // 마우스 툴팁 표시용
};

#endif // CUSTOMCHARTVIEW_H
