#include "mainwindow.h"

#include "axisgizmo.h"

#include <QAction>
#include <QActionGroup>
#include <QDockWidget>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("cube-v3");

    viewport = new MainWidget(this);
    setCentralWidget(viewport);

    createActions();
    createSidePanel();
    createMenus();
    createToolBar();
    createStatusBar();
}

// One QAction is shared by the menu and the tool bar,
// so both always show the same checked state
void MainWindow::createActions()
{
    // An exclusive group keeps exactly one render mode checked
    renderModeGroup = new QActionGroup(this);

    addRenderModeAction("&Wireframe", Qt::Key_1, "Show triangle edges only", RenderMode::Wireframe);
    addRenderModeAction("&Solid", Qt::Key_2, "Show a grey shaded surface", RenderMode::Solid);
    QAction *textured = addRenderModeAction("&Textured", Qt::Key_3, "Show the texture", RenderMode::Textured);
    textured->setChecked(true);

    resetViewAction = new QAction("&Reset View", this);
    resetViewAction->setShortcut(Qt::Key_Home);
    resetViewAction->setStatusTip("Restore the initial object and camera poses");
    connect(resetViewAction, &QAction::triggered, viewport, &MainWidget::resetView);

    rotateHandlesAction = new QAction("&Rotate Handles", this);
    rotateHandlesAction->setCheckable(true);
    rotateHandlesAction->setShortcut(Qt::Key_R);
    rotateHandlesAction->setStatusTip("Switch the selected object's handles between moving and rotating");
    connect(rotateHandlesAction, &QAction::toggled, viewport, &MainWidget::setRotateHandles);

    quitAction = new QAction("&Quit", this);
    quitAction->setShortcut(QKeySequence("Ctrl+Q"));
    connect(quitAction, &QAction::triggered, this, &QWidget::close);
}

QAction *MainWindow::addRenderModeAction(const QString &text, const QKeySequence &shortcut,
                                         const QString &tip, RenderMode mode)
{
    QAction *action = renderModeGroup->addAction(text);
    action->setCheckable(true);
    action->setShortcut(shortcut);
    action->setStatusTip(tip);
    connect(action, &QAction::triggered, viewport, [this, mode] { viewport->setRenderMode(mode); });
    return action;
}

void MainWindow::createSidePanel()
{
    AxisGizmo *gizmo = new AxisGizmo;

    // The two widgets only know each other's signals and slots
    connect(viewport, &MainWidget::cameraChanged, gizmo, &AxisGizmo::setCameraAxes);
    connect(gizmo, &AxisGizmo::axisClicked, viewport, &MainWidget::viewAlongAxis);

    QLabel *hint = new QLabel("Click an axis to look along it.\nClick it again for the opposite side.");
    hint->setWordWrap(true);

    QWidget *content = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(content);
    layout->addWidget(gizmo, 0, Qt::AlignHCenter);
    layout->addWidget(hint);
    layout->addStretch();

    QDockWidget *panel = new QDockWidget("Navigation", this);
    panel->setWidget(content);
    addDockWidget(Qt::RightDockWidgetArea, panel);

    // The dock widget provides a checkable action that shows and hides it
    sidePanelAction = panel->toggleViewAction();
    sidePanelAction->setShortcut(Qt::Key_N);
    sidePanelAction->setStatusTip("Show or hide the side panel");
}

void MainWindow::createMenus()
{
    QMenu *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(quitAction);

    QMenu *shadingMenu = menuBar()->addMenu("&Shading");
    shadingMenu->addActions(renderModeGroup->actions());

    QMenu *objectMenu = menuBar()->addMenu("&Object");
    objectMenu->addAction(rotateHandlesAction);

    viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction(resetViewAction);
    viewMenu->addSeparator();
    viewMenu->addAction(sidePanelAction);
}

void MainWindow::createToolBar()
{
    QToolBar *toolBar = addToolBar("Shading");
    toolBar->addActions(renderModeGroup->actions());
    toolBar->addSeparator();
    toolBar->addAction(rotateHandlesAction);
    toolBar->addSeparator();
    toolBar->addAction(resetViewAction);
    toolBar->addAction(sidePanelAction);

    // Show or hide the tool bar from the View menu
    viewMenu->addAction(toolBar->toggleViewAction());
}

void MainWindow::createStatusBar()
{
    // A permanent widget stays visible while status tips replace the normal message
    statusBar()->addPermanentWidget(new QLabel("Click cube: select  |  R: rotate handles  |  Drag: orbit  |  Shift + Drag: pan  |  Wheel: zoom"));
}
