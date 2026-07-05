// CNC_quote_program.cpp : 애플리케이션의 진입점을 정의합니다.
//

#include "CNC_quote_program.h"

using namespace std;

class OcctWidget : public QWidget {
public:
	OcctWidget(QWidget* parent = nullptr) : QWidget(parent) {
		// Qt가 이 위젯의 배경을 직접 그리지 않고, Open CASCADE(OpenGL)가 그리도록 설정
		setAttribute(Qt::WA_PaintOnScreen);
		setAttribute(Qt::WA_NoSystemBackground);

		// 마우스 트래킹 활성화
		setMouseTracking(true);

		// 그래픽 그라이버 초기화
		Handle(Aspect_DisplayConnection) DisplayConnection = new Aspect_DisplayConnection();
		Handle(OpenGl_GraphicDriver) graphicDriver = new OpenGl_GraphicDriver(DisplayConnection);

		// 뷰어(Viewer) 및 뷰(View) 생성
		myViewer = new V3d_Viewer(graphicDriver);
		myView = myViewer->CreateView();

		// 3D 공간 안의 물체들을 관리하고 마우스 선택 등을 처리하는 컨텍스트 생성
		myContext = new AIS_InteractiveContext(myViewer);

		// 기본 조명 설정 및 배경색(어두운 회색) 지정
		myViewer->SetDefaultLights();
		myViewer->SetLightOn();
		Quantity_Color bgColor(Quantity_NOC_GRAY30);
		myView->SetBackgroundColor(bgColor);
	}

	// STEP 파일을 읽어서 3D 화면에 띄우는 핵심 함수
	bool loadStepFile(const QString& filePath) {
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

		// 3D 화면에 표시할 수 있는 대화형 인터랙티브 객체(AIS_Shape)로 포장
		myContext->RemoveAll(Standard_True); // 기존에 떠있던 모델 지우기
		Handle(AIS_Shape) aisShape = new AIS_Shape(shape);

		// 화면에 오브젝트를 표시하고, 보기 좋은 각도로 카메라 자동 정렬 (FitAll)
		myContext->Display(aisShape, Standard_True);
		myView->FitAll();
		myView->Redraw();

		return true;
	}

protected:
	// 마우스 버튼을 누르는 순간 좌표 저장 및 시점 조작 시작
	virtual void mousePressEvent(QMouseEvent* event) override {
		myPanStartPoint = event->pos(); // 드래그 시작점 저장

		if (event->button() == Qt::LeftButton) {
			// 왼쪽 버튼: 회전 시작 위치 지정
			myView->StartRotation(event->x(), event->y());
		}
	}

	// 마우스를 누른 채 움직일 때(드래그) 실제 시점 연산 처리
	virtual void mouseMoveEvent(QMouseEvent* event) override {
		if (event->buttons() & Qt::LeftButton) {
			// 왼쪽 드래그: 3D 회전
			myView->Rotation(event->x(), event->y());
		}
		else if (event->buttons() & Qt::RightButton) {
			// 오른쪽 드래그: 시점 평면 이동
			myView->Pan(event->x() - myPanStartPoint.x(), myPanStartPoint.y() - event->y());
			myPanStartPoint = event->pos(); // 다음 연산을 위해 시작점 갱신
		}
	}

	// 마우스 휠 조작 시 확대/축소 처리
	virtual void wheelEvent(QWheelEvent* event) override {
		// 휠 회전 방향 및 수치 계산
		double delta = event->angleDelta().y();
		if (delta > 0) {
			myView->ZoomAtPoint(event->position().x(), event->position().y(), event->position().x() + 10, event->position().y() + 10);
		}
		else {
			myView->ZoomAtPoint(event->position().x(), event->position().y(), event->position().x() - 10, event->position().y() - 10);
		}
	}

	// 위젯이 화면에 처음 나타나거나 크기가 변경도리 때 Windows 창 핸들(HWND)을 연결하는 부분
	virtual void showEvent(QShowEvent* event) override {
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

	// 창 크기가 조절될 때 3D 그래픽 화면의 해상도도 함께 맞춰주는 함수
	virtual void resizeEvent(QResizeEvent* event) override {
		QWidget::resizeEvent(event);
		if (!myView.IsNull()) {
			myView->MustBeResized();
		}
	}

	// 화면을 새로 그려야 할 때 호출
	virtual QPaintEngine* paintEngine() const override {
		return nullptr; // Qt의 2D 페인트 엔진을 무력화하고 OpenGL이 제어하게 함
	}

private:
	Handle(V3d_Viewer) myViewer;
	Handle(V3d_View) myView;
	Handle(AIS_InteractiveContext) myContext;
	Handle(WNT_Window) myWindow;

	QPoint myPanStartPoint;
};

// 2. 메인 윈도우 클래스 (상단 메뉴바 및 레이아웃 관리)
class MainWindow : public QMainWindow {
public:
	MainWindow() {
		setWindowTitle("CNC / MCT AI 가공 견적 프로그램");
		resize(1280, 720); // 16:9 기본 해상도 설정

		// 중앙에 3D 그래픽 위젯 배치
		occtWidget = new OcctWidget(this);
		setCentralWidget(occtWidget);

		// 상단 메뉴바 구성
		QMenu* fileMenu = menuBar()->addMenu("파일(&F)");
		QAction* openAction = fileMenu->addAction("STEP 파일 열기(&0)");

		// '파일 열기' 버튼을 눌렀을 때 작동할 이벤트 연결 (람다식 사용)
		connect(openAction, &QAction::triggered, this, &MainWindow::onOpenFile);
	}

private:
	void onOpenFile() {
		// 파일 탐색기를 띄워 .step 또는 .stp 파일 선택 유도
		QString filePath = QFileDialog::getOpenFileName(
			this, "CAD 파일 선택", "", "STEP Files (*.step *.stp);;All Files (*.*)"
		);

		if (!filePath.isEmpty()) {
			bool success = occtWidget->loadStepFile(filePath);
			if (!success) {
				QMessageBox::critical(this, "오류", "CAD 파일을 분석하는 데 실패했습니다.");
			}
		}
	}

	OcctWidget* occtWidget;
};

int main(int argc, char* argv[]){
	QApplication app(argc, argv);

	MainWindow window;
	window.show();

	return app.exec(); // 이벤트 루프 시작 (창이 닫힐 때까지 대기)
}
