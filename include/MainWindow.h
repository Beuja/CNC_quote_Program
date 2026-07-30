#pragma once

#include <QMainWindow>

class OcctWidget;
class QLabel;
class QComboBox;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow();

private slots:
    void onOpenFile();
    void calculateCost();

private:
    OcctWidget* occtWidget;
    QLabel* myInfoLabel;

    QComboBox* materialCombo;
    QLabel* costLabel;
};
