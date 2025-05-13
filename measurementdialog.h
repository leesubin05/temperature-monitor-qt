#ifndef MEASUREMENTDIALOG_H
#define MEASUREMENTDIALOG_H

#include <QDialog>
#include <QSpinBox>
#include <QLabel>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QTime>

class MeasurementDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MeasurementDialog(QWidget *parent = nullptr);

    int samplingInterval() const;
    int measureCount() const;

private slots:
    void updateTotalTime();

private:
    QSpinBox *intervalSpin;
    QSpinBox *countSpin;
    QLabel *totalTimeLabel;
};

#endif // MEASUREMENTDIALOG_H
