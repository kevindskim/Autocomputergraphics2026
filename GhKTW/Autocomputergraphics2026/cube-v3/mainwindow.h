#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "mainwidget.h"

#include <QKeySequence>
#include <QMainWindow>

class QAction;
class QActionGroup;
class QMenu;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void createActions();
    void createSidePanel();
    void createMenus();
    void createToolBar();
    void createStatusBar();

    QAction *addRenderModeAction(const QString &text, const QKeySequence &shortcut,
                                 const QString &tip, RenderMode mode);

    MainWidget *viewport = nullptr;
    QMenu *viewMenu = nullptr;

    QActionGroup *renderModeGroup = nullptr;
    QAction *resetViewAction = nullptr;
    QAction *quitAction = nullptr;
    QAction *sidePanelAction = nullptr;
    QAction *rotateHandlesAction = nullptr;
};

#endif // MAINWINDOW_H
