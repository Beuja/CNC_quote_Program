#pragma once

#include <QWidget>
#include <QMouseEvent>
#include <QShowEvent>
#include <QResizeEvent>
#include <QWheelEvent>

#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <AIS_InteractiveContext.hxx>
#include <WNT_Window.hxx>

#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <QDebug>

class OcctWidget : public QWidget {
    Q_OBJECT
public:
    explicit OcctWidget(QWidget* parent = nullptr);
    bool loadStepFile(const QString& filePath);
    double getWidthX() const;
    double getLengthY() const;
    double getHeightZ() const;
    double getDifficultyFactor() const;
    QString getDifficultyString() const;

protected:
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void showEvent(QShowEvent* event) override;
    virtual void resizeEvent(QResizeEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void wheelEvent(QWheelEvent* event) override;
    virtual QPaintEngine* paintEngine() const override;

private:
    Handle(V3d_Viewer) myViewer;
    Handle(V3d_View) myView;
    Handle(AIS_InteractiveContext) myContext;
    Handle(WNT_Window) myWindow;
    QPoint myPanStartPoint;
    double myWidthX;
    double myLengthY;
    double myHeightZ;
    double myDifficultyFactor;
    QString myDifficultyString;
};
