// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QCommandLineParser>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QKeyEvent>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QSlider>
#include <QStatusBar>
#include <QSurfaceFormat>
#include <QTimer>

#include <memory>

#ifndef QT_NO_OPENGL
#include "mainwidget.h"
#endif

#ifndef QT_NO_OPENGL
// cube-final: settings dialog, every control is applied immediately
static QDialog *createSettingsDialog(QMainWindow *window, MainWidget *widget,
                                     QAction *dieViewAction, QAction *lightAction)
{
    QDialog *dialog = new QDialog(window);
    dialog->setWindowTitle("설정");
    QFormLayout *form = new QFormLayout(dialog);

    // Shape: index = GeometryEngine::Shape
    QComboBox *shapeBox = new QComboBox(dialog);
    shapeBox->addItems({"정육면체 주사위 (D6)", "정사면체 (D4)", "정팔면체 (D8)", "정이십면체 (D20)"});
    QObject::connect(shapeBox, &QComboBox::currentIndexChanged, widget, &MainWidget::setShape);
    form->addRow("도형", shapeBox);

    // Field of view, kept in sync with the Z / X keys and Ctrl + wheel
    QSlider *fovSlider = new QSlider(Qt::Horizontal, dialog);
    fovSlider->setRange(15, 100);
    fovSlider->setValue(int(widget->currentFieldOfView()));
    QLabel *fovLabel = new QLabel(QString("%1°").arg(fovSlider->value()), dialog);
    fovLabel->setMinimumWidth(40);
    QObject::connect(fovSlider, &QSlider::valueChanged, widget, [widget, fovLabel](int value) {
        fovLabel->setText(QString("%1°").arg(value));
        widget->setFieldOfView(float(value));
    });
    QObject::connect(widget, &MainWidget::fieldOfViewChanged, fovSlider, [fovSlider](float degrees) {
        fovSlider->setValue(qRound(degrees));
    });
    QHBoxLayout *fovRow = new QHBoxLayout;
    fovRow->addWidget(fovSlider);
    fovRow->addWidget(fovLabel);
    form->addRow("시야각 (광각 ↔ 망원)", fovRow);

    // Check boxes mirror the menu actions both ways
    QCheckBox *dieViewBox = new QCheckBox("주사위 눈으로 카메라 보기", dialog);
    dieViewBox->setChecked(dieViewAction->isChecked());
    QObject::connect(dieViewBox, &QCheckBox::toggled, dieViewAction, &QAction::setChecked);
    QObject::connect(dieViewAction, &QAction::toggled, dieViewBox, &QCheckBox::setChecked);
    form->addRow("시점", dieViewBox);

    QCheckBox *lightBox = new QCheckBox("Phong 조명", dialog);
    lightBox->setChecked(lightAction->isChecked());
    QObject::connect(lightBox, &QCheckBox::toggled, lightAction, &QAction::setChecked);
    QObject::connect(lightAction, &QAction::toggled, lightBox, &QCheckBox::setChecked);
    form->addRow("조명", lightBox);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
    QObject::connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::hide);
    form->addRow(buttons);

    return dialog;
}

// cube-final: menu bar wired to the slots of MainWidget
static void createMenus(QMainWindow *window, MainWidget *widget)
{
    // 파일
    QMenu *fileMenu = window->menuBar()->addMenu("파일(&F)");
    QAction *quitAction = fileMenu->addAction("종료(&X)");
    quitAction->setShortcut(QKeySequence::Quit);
    QObject::connect(quitAction, &QAction::triggered, window, &QMainWindow::close);

    // 보기
    QMenu *viewMenu = window->menuBar()->addMenu("보기(&V)");
    QAction *resetAction = viewMenu->addAction("초기 상태로(&R)");
    resetAction->setShortcut(Qt::Key_R);
    QObject::connect(resetAction, &QAction::triggered, widget, &MainWidget::resetScene);

    QAction *perspectiveAction = viewMenu->addAction("원근 투영(&P)");
    perspectiveAction->setCheckable(true);
    perspectiveAction->setChecked(true);
    perspectiveAction->setShortcut(Qt::Key_P);
    QObject::connect(perspectiveAction, &QAction::toggled, widget, &MainWidget::setPerspectiveProjection);

    QAction *followAction = viewMenu->addAction("카메라 이동 모드 (주사위·바닥 고정)(&C)");
    followAction->setCheckable(true);
    followAction->setShortcut(Qt::Key_C);
    QObject::connect(followAction, &QAction::toggled, widget, &MainWidget::setCameraFollowsObject);

    QAction *dieViewAction = viewMenu->addAction("주사위 시점 (주사위 눈으로 카메라 보기)(&V)");
    dieViewAction->setCheckable(true);
    dieViewAction->setShortcut(Qt::Key_V);
    QObject::connect(dieViewAction, &QAction::toggled, widget, &MainWidget::setDieView);
    QObject::connect(widget, &MainWidget::dieViewChanged, dieViewAction, &QAction::setChecked);

    QMenu *fovMenu = viewMenu->addMenu("시야각");
    const struct { const char *name; float degrees; } fovPresets[] = {
        {"광각 90°", 90.0f},
        {"표준 45°", 45.0f},
        {"망원 20°", 20.0f},
    };
    for (const auto &f : fovPresets) {
        QAction *fovAction = fovMenu->addAction(f.name);
        const float degrees = f.degrees;
        QObject::connect(fovAction, &QAction::triggered, widget, [widget, degrees] { widget->setFieldOfView(degrees); });
    }

    // Rebuilt every time it opens, so it lists the faces of the current shape (4 ~ 20)
    QMenu *faceMenu = viewMenu->addMenu("주사위 면 보기");
    QObject::connect(faceMenu, &QMenu::aboutToShow, widget, [faceMenu, widget] {
        faceMenu->clear();
        for (int n = 1; n <= widget->faceCount(); ++n) {
            QAction *faceAction = faceMenu->addAction(QString("%1 면\t%1").arg(n));
            QObject::connect(faceAction, &QAction::triggered, widget, [widget, n] { widget->showDiceFace(n); });
        }
    });

    // 조명
    QMenu *lightMenu = window->menuBar()->addMenu("조명(&L)");
    QAction *lightAction = lightMenu->addAction("조명 켜기(&L)");
    lightAction->setCheckable(true);
    lightAction->setChecked(true);
    lightAction->setShortcut(Qt::Key_L);
    QObject::connect(lightAction, &QAction::toggled, widget, &MainWidget::setLightingEnabled);

    QMenu *colorMenu = lightMenu->addMenu("광원 색");
    QActionGroup *colorGroup = new QActionGroup(window);
    const struct { const char *name; QVector3D color; } colors[] = {
        {"흰색",   QVector3D(1.0f, 1.0f, 1.0f)},
        {"노을색", QVector3D(1.0f, 0.75f, 0.5f)},
        {"푸른색", QVector3D(0.55f, 0.7f, 1.0f)},
    };
    for (int i = 0; i < 3; ++i) {
        QAction *colorAction = colorMenu->addAction(colors[i].name);
        colorAction->setObjectName(QString("lightColor%1").arg(i));   // found by --script color=N
        colorAction->setCheckable(true);
        colorGroup->addAction(colorAction);
        const QVector3D color = colors[i].color;
        QObject::connect(colorAction, &QAction::triggered, widget, [widget, color] { widget->setLightColor(color); });
    }
    colorGroup->actions().first()->setChecked(true);

    // 설정 창: shape, field of view, die view, lighting in one non-modal dialog
    QAction *settingsAction = viewMenu->addAction("설정...(&S)");
    settingsAction->setShortcuts({QKeySequence(Qt::Key_F2), QKeySequence("Ctrl+,")});
    viewMenu->insertAction(resetAction, settingsAction);
    viewMenu->insertSeparator(resetAction);
    QDialog *settings = createSettingsDialog(window, widget, dieViewAction, lightAction);
    QObject::connect(settingsAction, &QAction::triggered, settings, [settings] {
        settings->show();
        settings->raise();
        settings->activateWindow();
    });

    // 도움말
    QMenu *helpMenu = window->menuBar()->addMenu("도움말(&H)");
    QAction *keysAction = helpMenu->addAction("조작법");
    QObject::connect(keysAction, &QAction::triggered, window, [window] {
        QMessageBox::information(window, "조작법",
            "마우스 왼쪽 드래그 : 주사위 회전 (C 모드: 카메라가 주사위 주위를 돎)\n"
            "마우스 휠 : 카메라 확대 / 축소\n"
            "방향키 : 드래그와 같은 동작을 5도씩\n"
            "A / D : 카메라를 y축 기준으로 회전\n"
            "Q / E : 카메라를 x축 기준으로 회전 (위 / 아래)\n"
            "W / S : 카메라 가까이 / 멀리\n"
            "Z / X 또는 Ctrl + 휠 : 시야각 넓게(광각) / 좁게(망원)\n"
            "J / K : 광원을 y축 기준으로 회전\n"
            "C : 주사위 회전 모드 / 카메라 이동 모드 전환\n"
            "V : 주사위 시점 (주사위 1번 면의 눈으로 보기, 드래그로 둘러보기)\n"
            "숫자 키 : 해당 번호의 면을 카메라 쪽으로 (두 자리는 빠르게 이어서, 예: 1 → 2 = 12)\n"
            "L : 조명 켜기/끄기,  P : 원근/직교 투영,  R : 초기화\n"
            "F2 또는 Ctrl + , : 설정 창 (도형 바꾸기: D4 / D6 / D8 / D20)");
    });
}

// cube-final: one step of --script. A step is a key ("D", "Left", "L", ...), "wait",
// "shape=N" or "color=N". Keys with a menu shortcut trigger that action, the others go to the widget.
static void runScriptStep(QMainWindow *window, MainWidget *widget, const QString &step)
{
    if (step == "wait")
        return;
    if (step.startsWith("shape=")) {
        widget->setShape(step.mid(6).toInt());
        return;
    }
    if (step.startsWith("color=")) {
        if (QAction *colorAction = window->findChild<QAction *>("lightColor" + step.mid(6)))
            colorAction->trigger();
        return;
    }

    const QKeySequence key(step);
    if (key.isEmpty())
        return;
    for (QAction *action : window->findChildren<QAction *>()) {
        if (action->shortcuts().contains(key)) {
            action->trigger();
            return;
        }
    }
    QKeyEvent press(QEvent::KeyPress, key[0].key(), Qt::NoModifier);
    QCoreApplication::sendEvent(widget, &press);
}
#endif

int main(int argc, char *argv[])
{
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);

    app.setApplicationName("cube-final");
    app.setApplicationVersion("0.1");
#ifndef QT_NO_OPENGL
    QMainWindow window;
    MainWidget *widget = new MainWidget(&window);
    window.setCentralWidget(widget);
    window.setWindowTitle("cube-final");

    createMenus(&window, widget);
    QObject::connect(widget, &MainWidget::statusMessage, window.statusBar(),
                     [&window](const QString &message) { window.statusBar()->showMessage(message, 3000); });
    window.statusBar()->showMessage("도움말 > 조작법에서 키 목록을 볼 수 있습니다");

    // Optional start-up state, handy for report / slide screenshots and videos:
    //   cube-final --shape 3 --die-view --face 1 --grab shot.png
    //   cube-final --size 960x720 --script "C,Right*72" --record frames
    QCommandLineParser parser;
    QCommandLineOption shapeOption("shape", "0 = D6, 1 = D4, 2 = D8, 3 = D20", "index");
    QCommandLineOption dieViewOption("die-view", "Start in the die view");
    QCommandLineOption faceOption("face", "Turn this face towards the camera", "number");
    QCommandLineOption grabOption("grab", "Save a screenshot to this file and quit", "file");
    QCommandLineOption sizeOption("size", "Window size, e.g. 960x720", "WxH");
    QCommandLineOption scriptOption("script",
        "Comma separated steps, one per frame (25 fps): a key such as D, Left or L, "
        "wait, shape=N or color=N, each with an optional *count", "steps");
    QCommandLineOption recordOption("record", "Save every --script frame to this folder and quit", "folder");
    parser.addOptions({shapeOption, dieViewOption, faceOption, grabOption, sizeOption, scriptOption, recordOption});
    parser.process(app);

    if (parser.isSet(sizeOption)) {
        const QStringList wh = parser.value(sizeOption).split('x');
        if (wh.size() == 2)
            window.resize(wh[0].toInt(), wh[1].toInt());
    }

    window.show();
    widget->setFocus();

    // "Right*3,wait" -> Right, Right, Right, wait
    QStringList steps;
    for (const QString &token : parser.value(scriptOption).split(',', Qt::SkipEmptyParts)) {
        const QStringList parts = token.trimmed().split('*');
        const int count = parts.size() > 1 ? parts[1].toInt() : 1;
        for (int i = 0; i < count; ++i)
            steps << parts[0];
    }
    const QString recordDir = parser.value(recordOption);

    // After the first frame, so that the GL resources exist
    QTimer::singleShot(200, widget, [&] {
        if (parser.isSet(shapeOption))
            widget->setShape(parser.value(shapeOption).toInt());
        if (parser.isSet(dieViewOption))
            widget->setDieView(true);
        if (parser.isSet(faceOption))
            widget->showDiceFace(parser.value(faceOption).toInt());
        if (!parser.isSet(grabOption) && !parser.isSet(scriptOption))
            return;

        // After the face animation (0.3 s), run the script one step per 40 ms frame
        QTimer *scriptTimer = new QTimer(widget);
        scriptTimer->setInterval(40);
        auto frame = std::make_shared<int>(0);
        QObject::connect(scriptTimer, &QTimer::timeout, widget, [&, scriptTimer, frame] {
            if (*frame < steps.size()) {
                runScriptStep(&window, widget, steps[*frame]);
                if (!recordDir.isEmpty()) {
                    QDir().mkpath(recordDir);
                    widget->grabFramebuffer().save(QString("%1/frame_%2.png").arg(recordDir).arg(*frame, 4, 10, QChar('0')));
                }
                ++*frame;
                return;
            }
            scriptTimer->stop();
            if (parser.isSet(grabOption))
                widget->grabFramebuffer().save(parser.value(grabOption));
            if (parser.isSet(grabOption) || !recordDir.isEmpty())
                app.quit();
        });
        QTimer::singleShot(800, scriptTimer, qOverload<>(&QTimer::start));
    });
#else
    QLabel note("OpenGL Support required");
    note.show();
#endif
    return app.exec();
}
