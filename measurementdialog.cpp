#include "measurementdialog.h"

MeasurementDialog::MeasurementDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Measurement Settings");

    intervalSpin = new QSpinBox(this);
    intervalSpin->setRange(100, 10000); // 100ms ~ 10s
    intervalSpin->setSingleStep(100);
    intervalSpin->setSuffix(" ms");
    intervalSpin->setValue(1000); // Default 1s

    countSpin = new QSpinBox(this);
    countSpin->setRange(1, 10000);
    countSpin->setValue(10);

    totalTimeLabel = new QLabel("total measurement time: ", this);

    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow("sampling interval:", intervalSpin);
    formLayout->addRow("Number of measurements:", countSpin);
    formLayout->addRow(totalTimeLabel);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttonBox);

    // Connect spinbox changes to time update
    connect(intervalSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MeasurementDialog::updateTotalTime);
    connect(countSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MeasurementDialog::updateTotalTime);

    updateTotalTime(); // 초기 표시
}

void MeasurementDialog::updateTotalTime()
{
    int interval = intervalSpin->value();
    int count = countSpin->value();
    int totalMillis = interval * count;

    QTime totalTime(0, 0);
    totalTime = totalTime.addMSecs(totalMillis);

    totalTimeLabel->setText("total measurement time: " + totalTime.toString("hh:mm:ss"));
}

int MeasurementDialog::samplingInterval() const
{
    return intervalSpin->value();
}

int MeasurementDialog::measureCount() const
{
    return countSpin->value();
}
