#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTimer>
#include <QPixmap>
#include <QList>
#include <QGraphicsPixmapItem>
#include <QKeyEvent>
#include <QEvent>

// 游戏模式枚举
enum GameMode
{
    NormalMode,
    HardMode
};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::MainWindow *ui;

    QGraphicsScene *scene;
    QGraphicsView *view;
    QTimer *timer;
    double speed;

    QPixmap bg;
    QPixmap bird;
    QGraphicsPixmapItem* birdItem;
    QPixmap pipeUp;
    QPixmap pipeDown;
    QPixmap menuBg;
    QPixmap btnNormal;
    QPixmap btnHard;

    QList<QGraphicsPixmapItem*> pipeList;
    int pipeTimerCount;
    bool isGameOver;
    GameMode currentMode;

    int score;
    int pipeSpawnInterval;
    double pipeMoveSpeed;
    bool gameStart;

    void createMenu();
    void startGame(GameMode mode);
    void backToMenu();
    void spawnPipe();
};

#endif // MAINWINDOW_H