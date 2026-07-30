#include "MainWindow.h"
#include "OcctWidget.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QcomboBox>
#include <QHBoxLayout>
#include "MaterialDB.h"
#include <QLocale>

MainWindow::MainWindow() {
    setWindowTitle("CNC / MCT 가공 견적 프로그램");
    resize(1280, 720); // 16:9 기본 해상도 설정

    // 1. 수직 레이아웃이 적용도리 메인 컹테이너 위젯 생성
    QWidget* centralContainer = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralContainer);
    mainLayout->setContentsMargins(10, 10, 10, 10); // 테두리 바깥 여백
    mainLayout->setSpacing(5);                      // 구성 요소 사이 여백

    // 2. 상단 치수 정보를 띄워줄 QLabel을 스타일링하여 생성
    myInfoLabel = new QLabel(" [안내] 분석을 진행할 STEP 파일을 상단 메뉴에서 열어주세요.", this);

    // 2-1. 재질 선택과 견적 결과를 나란히 보여줄 가로 레이아웃 생성
    QBoxLayout* costLayout = new QHBoxLayout();

    // 드롭다운 메뉴 생성 및 재질 DB 연동
    materialCombo = new QComboBox(this);
    QList<MaterialData> materials = getMaterialDatabase();
    for (const MaterialData& mat : materials) {
        // 화면에 보여줄 글자와 숨겨둘 고유 데이터를 함께 콤보박스에 넣음
        materialCombo->addItem(mat.name);
    }

    // 콤보박스 디자인 입히기
    materialCombo->setStyleSheet("font-size: 14px; padding: 5px");

    // 계산된 원가를 띄워줄 텍스트 생성
    costLabel = new QLabel("예상 재료비: (STEP 파일을 열고 재질을 선택하세요)", this);
    costLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #FFA500; padding: 5px");

    costLayout->addWidget(new QLabel("가공 재질 선택: ", this));
    costLayout->addWidget(materialCombo);
    costLayout->addWidget(costLabel);
    costLayout->addStretch();

    myInfoLabel->setStyleSheet(
        "font-size: 15px;"
        "font-weight: bold;"
        "color: #00FF00;"            // 녹색 텍스트
        "background-color: #222222;" // 어두운 회색 배경
        "padding: 10px;"
        "border-radius: 5px;"
    );
    mainLayout->addWidget(myInfoLabel); // 레이아웃에 글자 추가 (0번째)

    // 순서 2: 재질 선택과 견적 결과를 담은 가로 레이아웃 추가 (1번째)
    mainLayout->addLayout(costLayout);

    // 3. 하단 3D CAD 위젯 추가 및 영역 자동 늘리기 설정 (2번째)
    occtWidget = new OcctWidget(this);
    mainLayout->addWidget(occtWidget);
    mainLayout->setStretch(2, 1); // 2번째(3D 위젯)가 나머지 빈 공간을 꽉 채우도록 설정

    setCentralWidget(centralContainer); // 수직 정렬이 완료된 컨테이너를 메인 화면으로 설정

    // 상단 메뉴바 구성
    QMenu* fileMenu = menuBar()->addMenu("파일(&F)");
    QAction* openAction = fileMenu->addAction("STEP 파일 열기(&O)");

    // '파일 열기' 버튼을 눌렀을 때 작동할 이벤트 연결
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenFile);

    // 콤보박스 재질을 변경하면 즉시 calculateCost 함수가 실행되도록 연결
    connect(materialCombo, &QComboBox::currentIndexChanged, this, &MainWindow::calculateCost);
}

void MainWindow::onOpenFile() {
    // 파일 탐색기를 띄워 .step 또는 .stp 파일 선택 유도
    QString filePath = QFileDialog::getOpenFileName(
        this, "CAD 파일 선택", "", "STEP Files (*.step *.stp);;All Files (*.*)"
    );

    if (!filePath.isEmpty()) {
        bool success = occtWidget->loadStepFile(filePath);
        if (success) {
            QString infoText = QString("[분석 성공] 파일명: %1 | 모델 치수(X x Y x Z): %2 x %3 x %4 mm")
                .arg(QFileInfo(filePath).fileName())
                .arg(QString::number(occtWidget->getWidthX(), 'f', 2))
                .arg(QString::number(occtWidget->getLengthY(), 'f', 2))
                .arg(QString::number(occtWidget->getHeightZ(), 'f', 2));

            myInfoLabel->setText(infoText);
            
            // 파일 분석이 끝났으니 최초 1회 비용 계산 실행
            calculateCost();
        }
        else {
            QMessageBox::critical(this, "오류", "CAD 파일을 분석하는 데 실패했습니다.");
        }
    }
}

void MainWindow::calculateCost() {
    // 1. 모델에서 가로, 세로, 높이 가져오기 (단위: mm)
    double widthX = occtWidget->getWidthX();
    double lengthY = occtWidget->getLengthY();
    double heightZ = occtWidget->getHeightZ();

    // 모델을 열지 않은 상태(치수가 모두 0)이면 계산하지 않음
    if (widthX == 0.0 && lengthY == 0.0 && heightZ == 0.0) {
        costLabel->setText("예상 재료비: (STEP 파일을 열어주세요");
        return;
    }

    // 2. 부피 계산: mm^3 -> cm^3 변환을 위해 1000으로 나눔
    double volumeCm3 = (widthX * lengthY * heightZ) / 1000.0;

    // 3. 사용자가 현재 선택한 재질 정보 가져오기
    int selectedIndex = materialCombo->currentIndex();
    MaterialData selectedMat = getMaterialDatabase()[selectedIndex];

    // 4. 중량 계산
    double weightKg = (volumeCm3 * selectedMat.specificGravity) / 1000.0;

    // 5. 가공 난이도 가져오기
    double difficultyFactor = occtWidget->getDifficultyFactor();
    QString difficultyString = occtWidget->getDifficultyString();

    // 6. 최종 견적 금액 계산: 중량 * kg당 단가 * 가공 난이도 가중치
    double finalCost = weightKg * selectedMat.costPerKg * difficultyFactor;

    // 7. 계산 결과를 문자열로 예쁘게 포맷팅하여 라벨 갱신
    QString resultText = QString("재질: %1  |  중량: %2kg  |  난이도: %3 (x%4)  |  최종 견적: %5 원")
        .arg(selectedMat.name)
        .arg(QString::number(weightKg, 'f', 2))
        .arg(difficultyString)
        .arg(QString::number(difficultyFactor, 'f', 1))
        .arg(QLocale(QLocale::Korean).toString(static_cast<long long>(finalCost)));

    costLabel->setText(resultText);
}