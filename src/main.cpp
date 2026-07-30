#include <QApplication>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec(); // 이벤트 루프 시작 (창이 닫힐 때까지 대기)
}
