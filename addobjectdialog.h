#ifndef ADDOBJECTDIALOG_H
#define ADDOBJECTDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QLabel>

class AddObjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddObjectDialog(QWidget *parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("Add Object");
        QVBoxLayout* layout = new QVBoxLayout(this);

        layout->addWidget(new QLabel("Type:"));
        typeCombo = new QComboBox;
        typeCombo->addItem("Coil");
        layout->addWidget(typeCombo);

        // Optional: Name field if we want custom names later
        // layout->addWidget(new QLabel("Name:"));
        // nameEdit = new QLineEdit("New Object");
        // layout->addWidget(nameEdit);

        QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttonBox);
    }

    QString getSelectedType() const { return typeCombo->currentText(); }

private:
    QComboBox* typeCombo;
};

#endif // ADDOBJECTDIALOG_H
