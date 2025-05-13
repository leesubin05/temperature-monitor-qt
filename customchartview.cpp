#include "CustomChartView.h"
#include <QtCharts/QLineSeries>
#include <QDateTime>
#include <QToolTip>
#include <QtCharts/QValueAxis>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QLabel>

CustomChartView::CustomChartView(QChart *chart, QWidget *parent)
    : QChartView(chart, parent)
{
    setRenderHint(QPainter::Antialiasing);

    // 드래그로 이동 가능하게 설정
    setDragMode(QGraphicsView::ScrollHandDrag);

    // 확대 영역 선택 금지
    setRubberBand(QChartView::NoRubberBand);

    setFocusPolicy(Qt::StrongFocus);

    setMouseTracking(true);

    tooltipLabel = new QLabel(this);
    tooltipLabel->setStyleSheet("QLabel { background-color: rgba(255, 255, 255, 230); "
                                "border: 1px solid gray; padding: 3px; }");
    tooltipLabel->setVisible(false);
}

void CustomChartView::setSeriesList(const QList<QLineSeries*>& seriesList)
{
    m_seriesList = seriesList;
}

// 더블클릭 이벤트 (전체화면용 등)
void CustomChartView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit doubleClicked();
    }
    QChartView::mouseDoubleClickEvent(event);
}

// 마우스로 클릭했을 때 해당 위치 온도 보여주기
void CustomChartView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QPointF chartPos = this->chart()->mapToValue(mapToScene(event->pos()));

        QString tooltip;

        QList<QAbstractSeries*> seriesList;
        if (m_seriesList.isEmpty()) {
            seriesList = chart()->series();
        } else {
            for (QLineSeries* s : m_seriesList)
                seriesList.append(s);
        }

        for (int i = 0; i < seriesList.size(); ++i) {
            QLineSeries *s = qobject_cast<QLineSeries *>(seriesList.at(i));
            if (!s) continue;

            for (const QPointF &point : s->points()) {
                if (qAbs(point.x() - chartPos.x()) < 500 && qAbs(point.y() - chartPos.y()) < 3.0) {
                    QDateTime time = QDateTime::fromMSecsSinceEpoch(point.x());
                    tooltip += QString("CH%1 [%2] = %3°C\n")
                                   .arg(i + 1)
                                   .arg(time.toString("HH:mm:ss"))
                                   .arg(point.y(), 0, 'f', 2);
                }
            }
        }

        if (!tooltip.isEmpty()) {
            tooltipLabel->setText(tooltip);
            tooltipLabel->adjustSize();
            tooltipLabel->move(event->pos() + QPoint(15, 15));
            tooltipLabel->show();
        } else {
            tooltipLabel->hide();
        }
    }

    QChartView::mousePressEvent(event);
}

// 마우스 커서만 갖다 대도 툴팁 보이게
void CustomChartView::mouseMoveEvent(QMouseEvent *event)
{
    QPointF chartPos = this->chart()->mapToValue(mapToScene(event->pos()));

    QValueAxis *xAxis = qobject_cast<QValueAxis *>(this->chart()->axisX());
    if (xAxis) {
        if (chartPos.x() < xAxis->min() || chartPos.x() > xAxis->max()) {
            tooltipLabel->hide();
            return;
        }
    }

    QString tooltip;

    QList<QAbstractSeries*> seriesList;
    if (m_seriesList.isEmpty()) {
        seriesList = chart()->series();
    } else {
        for (QLineSeries* s : m_seriesList)
            seriesList.append(s);
    }

    for (int i = 0; i < seriesList.size(); ++i) {
        QLineSeries *s = qobject_cast<QLineSeries *>(seriesList.at(i));
        if (!s) continue;

        for (const QPointF &point : s->points()) {
            if (qAbs(point.x() - chartPos.x()) < 10.0) {
                QDateTime time = QDateTime::fromMSecsSinceEpoch(point.x());
                tooltip += QString("CH%1 [%2] = %3°C\n")
                               .arg(i + 1)
                               .arg(time.toString("HH:mm:ss"))
                               .arg(point.y(), 0, 'f', 2);
                break;
            }
        }
    }

    if (!tooltip.isEmpty()) {
        tooltipLabel->setText(tooltip);
        tooltipLabel->adjustSize();
        tooltipLabel->move(event->pos() + QPoint(15, 15));
        tooltipLabel->show();
    } else {
        tooltipLabel->hide();
    }

    QChartView::mouseMoveEvent(event);
}

// 마우스 휠로 확대/축소
void CustomChartView::wheelEvent(QWheelEvent *event)
{
    QChart *c = chart();
    if (!c) {
        event->ignore();
        return;
    }

    qreal zoomFactor = (event->angleDelta().y() > 0) ? 0.8 : 1.25;

    QValueAxis *xAxis = qobject_cast<QValueAxis *>(c->axisX());
    QValueAxis *yAxis = qobject_cast<QValueAxis *>(c->axisY());

    if (!xAxis || !yAxis) {
        event->ignore();
        return;
    }

    // Qt 5: event->pos() 사용
    QPointF mousePos = mapToScene(event->pos());
    QPointF chartPos = c->mapToValue(mousePos);

    qreal xCenter = chartPos.x();
    qreal xRange = (xAxis->max() - xAxis->min()) * zoomFactor;

    xAxis->setRange(xCenter - xRange / 2.0, xCenter + xRange / 2.0);

    // Y축 확대도 필요하면 아래 주석 해제
    /*
    qreal yCenter = chartPos.y();
    qreal yRange = (yAxis->max() - yAxis->min()) * zoomFactor;
    yAxis->setRange(yCenter - yRange / 2.0, yCenter + yRange / 2.0);
    */

    event->accept();
}




// 마우스가 위젯에서 벗어났을 때 툴팁 숨기기
void CustomChartView::leaveEvent(QEvent *event)
{
    tooltipLabel->hide();
    QChartView::leaveEvent(event);
}
