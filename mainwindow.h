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

private:
    enum GameMode { NormalMode, HardMode };
    enum WeatherType { Sunny, Rain, Fog, Magnetic };

    Ui::MainWindow *ui;

    QGraphicsScene *scene;
    QGraphicsView *view;
    QGraphicsPixmapItem *birdItem;
    QList<QGraphicsPixmapItem*> pipes;
    QList<QGraphicsLineItem*> raindrops;
    QGraphicsTextItem *weatherLabel;
    QGraphicsTextItem *livesLabel;
    QGraphicsTextItem *warningLabel;
    QGraphicsTextItem *invincibleLabel;   // 无敌提示（仅切换天气时显示）
    QGraphicsPixmapItem *fogOverlay;

    QPixmap bgPix, birdPix, pipeUpPix, pipeDownPix, menuBgPix, btnNormalPix, btnHardPix;

    QTimer *mainTimer;
    QTimer *weatherTimer;
    QTimer *rainTimer;
    QTimer *blinkTimer;

    GameMode mode;
    bool gameOver;
    bool gameStarted;
    int score;
    int lives;

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
    bool showInvincibleText;   // 掉血无敌不显示文字

    QPoint viewOriginalPos;

    static constexpr int WEATHER_DURATION_FRAMES = 70;
    static constexpr int WARNING_FRAMES = 30;
    static constexpr int INVINCIBLE_FRAMES = 150;

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
};

#endif // MAINWINDOW_H