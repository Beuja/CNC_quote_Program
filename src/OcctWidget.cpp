#include "OcctWidget.h"
#include <Aspect_DisplayConnection.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <STEPControl_Reader.hxx>
#include <TopoDS_Shape.hxx>
#include <AIS_Shape.hxx>
#include <Bnd_Box.hxx>      // 바운딩 박스 데이터 구조
#include <BRepBndLib.hxx>   // 형상에서 바운딩 박스를 계산하는 라이브러리
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <QDebug>

OcctWidget::OcctWidget(QWidget* parent) : QWidget(parent), myWidthX(0.0), myLengthY(0.0), myHeightZ(0.0), myDifficultyFactor(1.0), myDifficultyString("분석 전") {
    // Qt가 이 위젯의 배경을 직접 그리지 않고, Open CASCADE(OpenGL)가 그리도록 설정
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);

    // 마우스 트래킹 활성화
    setMouseTracking(true);

    // 그래픽 드라이버 초기화
    Handle(Aspect_DisplayConnection) displayConnection = new Aspect_DisplayConnection();
    Handle(OpenGl_GraphicDriver) graphicDriver = new OpenGl_GraphicDriver(displayConnection);

    // 뷰어 및 뷰 생성
    myViewer = new V3d_Viewer(graphicDriver);
    myView = myViewer->CreateView();

    // 3D 공간 안의 물체들을 관리하고 마우스 선택 등을 처리하는 컨텍스트 생성
    myContext = new AIS_InteractiveContext(myViewer);

    // 기본 조명 설정 및 배경색(어두운 회색) 지정
    myViewer->SetDefaultLights();
    myViewer->SetLightOn();
    myView->SetBackgroundColor(Quantity_NOC_GRAY30);
}

bool OcctWidget::loadStepFile(const QString& filePath) {
    if (filePath.isEmpty()) return false;

    STEPControl_Reader reader;
    // QString을 표준 string 및 C 스타일 문자열로 변환하여 파일 로드
    IFSelect_ReturnStatus status = reader.ReadFile(filePath.toLocal8Bit().constData());
    if (status != IFSelect_RetDone) {
        return false; // 파일 읽기 실패
    }

    // STEP 파일의 기하학 데이터를 Open CASCADE의 형상(Shape) 데이터로 변환
    reader.TransferRoots();
    TopoDS_Shape shape = reader.OneShape();

    Bnd_Box boundingBox;
    BRepBndLib::Add(shape, boundingBox); // 모델 데이터를 박스에 매핑

    Standard_Real Xmin, Ymin, Zmin, Xmax, Ymax, Zmax;
    boundingBox.Get(Xmin, Ymin, Zmin, Xmax, Ymax, Zmax); // 최소/최대 좌표 추출

    myWidthX = Xmax - Xmin;
    myLengthY = Ymax - Ymin;
    myHeightZ = Zmax - Zmin;

    // 1. 실제 부품의 부피(Volume) 및 표면적(Area) 연산
    GProp_GProps systemProps;
    BRepGProp::VolumeProperties(shape, systemProps);
    double actualVolume = systemProps.Mass(); // mm³ 단위 실제 부품 부피

    BRepGProp::SurfaceProperties(shape, systemProps);
    double surfaceArea = systemProps.Mass();  // mm² 단위 표면적

    // 2. 바운딩 박스(원소재) 부피 계산
    double stockVolume = myWidthX * myLengthY * myHeightZ;

    // 3. 제거 체적율(%) 계산 (얼마나 많이 깎아내야 하는가?)
    double removalRatio = ((stockVolume - actualVolume) / stockVolume) * 100.0;

    // 4. 면(Face)의 개수 카운트 (형상이 얼마나 복잡한가?)
    int faceCount = 0;
    for (TopExp_Explorer expl(shape, TopAbs_FACE); expl.More(); expl.Next()) {
        faceCount++;
    }

    // -------------------------------------------------------------
    // 💡 [난이도 판정 로직 예시]
    // -------------------------------------------------------------
    myDifficultyString = "쉬움 (보통 가공)";
    myDifficultyFactor = 1.0; // 공임 가중치

    if (removalRatio > 70.0 || faceCount > 50) {
        myDifficultyString = "어려움 (시간 소요/고난이도)";
        myDifficultyFactor = 1.5; // 공임 50% 할증
    } else if (removalRatio > 50.0 || faceCount > 25) {
        myDifficultyString = "보통";
        myDifficultyFactor = 1.2; // 공임 20% 할증
    }

    qDebug() << "실제 부피:" << actualVolume << "mm³";
    qDebug() << "제거 체적율:" << removalRatio << "%";
    qDebug() << "면의 개수:" << faceCount << "개";
    qDebug() << "가공 난이도:" << myDifficultyString;

    // 3D 화면에 표시할 수 있는 대화형 인터랙티브 객체(AIS_Shape)로 포장
    myContext->RemoveAll(Standard_True); // 기존에 떠있던 모델 지우기
    Handle(AIS_Shape) aisShape = new AIS_Shape(shape);

    // 화면에 오브젝트를 표시하고, 보기 좋은 각도로 카메라 자동 정렬 (FitAll)
    myContext->Display(aisShape, Standard_True);
    myView->FitAll();
    myView->Redraw();

    return true;
}

void OcctWidget::mousePressEvent(QMouseEvent* event) {
    myPanStartPoint = event->pos();
    if (event->button() == Qt::LeftButton) {
        myView->StartRotation(event->x(), event->y());
    }
}

// 마우스 드래그 시 회전(좌클릭) 및 평면 이동(우클릭)
void OcctWidget::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        myView->Rotation(event->x(), event->y());   // 회전 카메라 조작
    }
    else if (event->buttons() & Qt::RightButton) {
        // 평면 이동(Pan) 조작
        myView->Pan(event->x() - myPanStartPoint.x(), myPanStartPoint.y() - event->y());
        myPanStartPoint = event->pos();
    }
}

// 마우스 휠 스크롤 시 휠을 굴린 위치를 기준으로 확대/축소
void OcctWidget::wheelEvent(QWheelEvent* event) {
    double delta = event->angleDelta().y();
    if (delta > 0) {
        // 줌 인
        myView->ZoomAtPoint(event->position().x(), event->position().y(),
            event->position().x() + 10, event->position().y() + 10);
    }
    else {
        // 줌 아웃
        myView->ZoomAtPoint(event->position().x(), event->position().y(),
            event->position().x() - 10, event->position().y() - 10);
    }
}

void OcctWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (myWindow.IsNull()) {
        // Qt 위젯의 고유 윈도우 핸들 ID를 추출하여 Open CASCADE 윈도우 래퍼에 주입
        myWindow = new WNT_Window((Aspect_Handle)winId());
        myView->SetWindow(myWindow);
        if (!myWindow->IsMapped()) {
            myWindow->Map();
        }
    }
}

void OcctWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (!myView.IsNull()) {
        myView->MustBeResized();
    }
}

QPaintEngine* OcctWidget::paintEngine() const {
    return nullptr; // Qt의 2D 페인트 엔진을 무력화하고 OpenGL이 제어하게 함
}

double OcctWidget::getWidthX() const { return myWidthX; }
double OcctWidget::getLengthY() const { return myLengthY; }
double OcctWidget::getHeightZ() const { return myHeightZ; }
double OcctWidget::getDifficultyFactor() const { return myDifficultyFactor; }
QString OcctWidget::getDifficultyString() const { return myDifficultyString; }