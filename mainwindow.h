#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTimer>
#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <QGraphicsTextItem>
#include <QGraphicsLineItem>

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
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void gameLoop();
    void weatherLoop();
    void rainLoop();
    void blinkInvincible();
    void shakeView();   // 震动函数（替代黄框）

private:
    enum GameMode { NormalMode, HardMode };
    enum WeatherType { Sunny, Rain, Fog, Magnetic, Rainbow };

    Ui::MainWindow *ui;

    QGraphicsScene *scene;
    QGraphicsView *view;
    QGraphicsPixmapItem *birdItem;
    QList<QGraphicsPixmapItem*> pipes;
    QList<QGraphicsLineItem*> raindrops;
    QGraphicsTextItem *weatherLabel;
    QGraphicsTextItem *livesLabel;
    QGraphicsTextItem *warningLabel;
    QGraphicsTextItem *invincibleLabel;
    QGraphicsTextItem *nextLifeLabel;
    QGraphicsPixmapItem *fogOverlay;

    QPixmap bgPix, birdPix, pipeUpPix, pipeDownPix, menuBgPix, btnNormalPix, btnHardPix;

    QTimer *mainTimer;
    QTimer *weatherTimer;
    QTimer *rainTimer;
    QTimer *blinkTimer;
    QTimer *shakeTimer;      // 震动定时器

    GameMode mode;
    bool gameOver;
    bool gameStarted;
    int score;
    int lives;
    int nextLifeThreshold;
    bool scoreDoubled;

    double yVelocity;
    double gravity;
    double jumpPower;

    double pipeSpeed;
    int pipeSpawnCounter;
    int pipeSpawnInterval;

    WeatherType currentWeather;
    int weatherDurationCounter;
    bool isWarningActive;
    int warningCounter;
    WeatherType pendingWeather;

    bool magneticSpacePressed;

    bool invincible;
    int invincibleCounter;
    bool showInvincibleText;

    QPoint viewOriginalPos;
    int shakeCounter;        // 震动剩余次数

    static constexpr int WEATHER_DURATION_FRAMES = 70;   // 7秒
    static constexpr int WARNING_FRAMES = 30;            // 3秒
    static constexpr int INVINCIBLE_FRAMES = 150;        // 3秒无敌

    void createMenu();
    void startGame(GameMode mode);
    void spawnPipe();
    void killPlayer();
    void resetAfterDeath();
    void changeWeather(WeatherType newWeather);
    void startWeatherWarning(WeatherType weather);
    QString weatherDescription(WeatherType weather) const;
    void applyWeatherEffects();
    void setInvincible(int frames, bool showText);
    void updateNextLifeDisplay();
    void addScore(int points);
};

#endif // MAINWINDOW_H