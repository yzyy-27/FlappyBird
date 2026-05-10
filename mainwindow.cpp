#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMouseEvent>
#include <QMessageBox>
#include <QFont>
#include <ctime>
#include <QGraphicsLineItem>
#include <QPen>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    srand(time(nullptr));

    scene = new QGraphicsScene(this);
    scene->setSceneRect(0, 0, 400, 600);
    view = new QGraphicsView(scene, this);
    view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setCentralWidget(view);
    setFixedSize(400, 600);
    setWindowTitle("Flappy Bird");
    view->installEventFilter(this);

    bgPix.load(":/pic/bg.png");
    birdPix.load(":/pic/0.png");
    pipeUpPix.load(":/pic/pipeUp.png");
    pipeDownPix.load(":/pic/pipeDown.png");
    menuBgPix.load(":/pic/start.png");
    btnNormalPix.load(":/pic/normal.png");
    btnHardPix.load(":/pic/hard.png");

    gravity = 0.9;
    jumpPower = -10.0;

    mainTimer = new QTimer(this);
    mainTimer->setInterval(20);
    connect(mainTimer, &QTimer::timeout, this, &MainWindow::gameLoop);

    weatherTimer = new QTimer(this);
    weatherTimer->setInterval(100);
    connect(weatherTimer, &QTimer::timeout, this, &MainWindow::weatherLoop);

    rainTimer = new QTimer(this);
    rainTimer->setInterval(40);
    connect(rainTimer, &QTimer::timeout, this, &MainWindow::rainLoop);

    blinkTimer = new QTimer(this);
    blinkTimer->setInterval(100);
    connect(blinkTimer, &QTimer::timeout, this, &MainWindow::blinkInvincible);

    weatherLabel = nullptr;
    livesLabel = nullptr;
    warningLabel = nullptr;
    invincibleLabel = nullptr;
    fogOverlay = nullptr;
    viewOriginalPos = view->pos();
    magneticSpacePressed = false;
    invincible = false;
    invincibleCounter = 0;
    showInvincibleText = false;

    createMenu();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createMenu()
{
    mainTimer->stop();
    weatherTimer->stop();
    rainTimer->stop();
    blinkTimer->stop();
    scene->clear();
    gameOver = true;

    scene->addPixmap(bgPix);
    scene->addPixmap(menuBgPix)->setPos(0, 0);

    QGraphicsPixmapItem *btnNormal = scene->addPixmap(btnNormalPix);
    btnNormal->setPos(100, 360);
    btnNormal->setData(0, NormalMode);

    QGraphicsPixmapItem *btnHard = scene->addPixmap(btnHardPix);
    btnHard->setPos(100, 450);
    btnHard->setData(0, HardMode);

    raindrops.clear();
    pipes.clear();
    currentWeather = Sunny;
    isWarningActive = false;
    view->move(viewOriginalPos);
    magneticSpacePressed = false;
    invincible = false;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == view && event->type() == QEvent::MouseButtonPress && gameOver)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QGraphicsItem *item = scene->itemAt(view->mapToScene(mouseEvent->pos()), QTransform());
        if (item && item->data(0).isValid())
        {
            GameMode selectedMode = static_cast<GameMode>(item->data(0).toInt());
            startGame(selectedMode);
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::startGame(GameMode selectedMode)
{
    scene->clear();
    mode = selectedMode;
    gameOver = false;
    gameStarted = false;
    score = 0;
    yVelocity = 0;
    invincible = false;
    invincibleCounter = 0;
    showInvincibleText = false;
    blinkTimer->stop();

    scene->addPixmap(bgPix);
    birdItem = scene->addPixmap(birdPix);
    birdItem->setPos(100, 200);
    birdItem->setZValue(50);

    pipes.clear();
    pipeSpawnCounter = 0;
    pipeSpeed = (mode == NormalMode) ? 3.5 : 3.0;
    pipeSpawnInterval = 100;
    view->move(viewOriginalPos);
    magneticSpacePressed = false;

    if (mode == NormalMode)
    {
        weatherTimer->stop();
        rainTimer->stop();
        currentWeather = Sunny;
    }
    else // HardMode
    {
        lives = 3;
        currentWeather = Sunny;
        weatherDurationCounter = 0;
        isWarningActive = false;
        warningCounter = 0;

        QFont font;
        font.setPointSize(14);
        font.setBold(true);
        weatherLabel = scene->addText("晴天");
        weatherLabel->setFont(font);
        weatherLabel->setDefaultTextColor(Qt::white);
        weatherLabel->setPos(15, 10);
        weatherLabel->setZValue(1000);

        livesLabel = scene->addText(QString("❤️ %1").arg(lives));
        livesLabel->setFont(font);
        livesLabel->setDefaultTextColor(Qt::white);
        livesLabel->setPos(330, 10);
        livesLabel->setZValue(1000);

        QFont warnFont;
        warnFont.setPointSize(12);
        warnFont.setBold(true);
        warningLabel = scene->addText("");
        warningLabel->setFont(warnFont);
        warningLabel->setDefaultTextColor(QColor(255, 200, 100));
        warningLabel->setPos(15, 32);
        warningLabel->setZValue(1000);
        warningLabel->setVisible(false);

        // 无敌文字：放在生命值右侧，小号金色，加粗
        invincibleLabel = scene->addText("");
        QFont invFont;
        invFont.setPointSize(11);
        invFont.setBold(true);
        invincibleLabel->setFont(invFont);
        invincibleLabel->setDefaultTextColor(QColor(255, 215, 0));
        invincibleLabel->setPos(270, 12);
        invincibleLabel->setZValue(1000);
        invincibleLabel->setVisible(false);

        fogOverlay = scene->addPixmap(QPixmap(400, 600));
        QPixmap fogPix(400, 600);
        fogPix.fill(QColor(255, 255, 255, 180));
        fogOverlay->setPixmap(fogPix);
        fogOverlay->setZValue(20);
        fogOverlay->setVisible(false);

        weatherTimer->start();
    }

    mainTimer->start();
}

void MainWindow::gameLoop()
{
    if (gameOver) return;
    if (!gameStarted) return;

    if (invincible)
    {
        invincibleCounter--;
        if (invincibleCounter <= 0)
        {
            invincible = false;
            blinkTimer->stop();
            if (birdItem) birdItem->setOpacity(1.0);
            if (invincibleLabel) invincibleLabel->setVisible(false);
        }
        else
        {
            if (showInvincibleText && invincibleLabel)
            {
                int secLeft = (invincibleCounter + 24) / 25;
                invincibleLabel->setPlainText(QString("无敌 %1 秒").arg(secLeft));
                invincibleLabel->setVisible(true);
            }
            else if (invincibleLabel)
            {
                invincibleLabel->setVisible(false);
            }
        }
    }

    // 速度更新
    if (mode == HardMode && currentWeather == Magnetic)
    {
        const double UP_ACC = -0.4;
        const double DOWN_ACC = 0.8;
        const double MAX_UP = -6.0;
        const double MAX_DOWN = 8.0;
        const double DAMP = 0.98;

        if (magneticSpacePressed)
            yVelocity += DOWN_ACC;
        else
            yVelocity += UP_ACC;

        yVelocity = qBound(MAX_UP, yVelocity, MAX_DOWN);
        yVelocity *= DAMP;
    }
    else
    {
        double currentGravity = gravity;
        if (mode == HardMode && currentWeather == Rain)
            currentGravity = 1.05;
        yVelocity += currentGravity;
    }

    birdItem->setPos(100, birdItem->y() + yVelocity);

    if (birdItem->y() > 550 || birdItem->y() < 0)
    {
        killPlayer();
        return;
    }

    pipeSpawnCounter++;
    if (pipeSpawnCounter >= pipeSpawnInterval)
    {
        spawnPipe();
        pipeSpawnCounter = 0;
    }

    for (int i = 0; i < pipes.size(); ++i)
    {
        QGraphicsPixmapItem *pipe = pipes[i];
        pipe->setPos(pipe->x() - pipeSpeed, pipe->y());

        if (mode == HardMode && currentWeather == Fog)
        {
            double opacity = 1.0;
            if (pipe->x() < 300)
                opacity = qMax(0.0, (pipe->x() - 50) / 250.0);
            pipe->setOpacity(opacity);
            if (fogOverlay) fogOverlay->setVisible(true);
        }
        else if (fogOverlay)
        {
            pipe->setOpacity(1.0);
            fogOverlay->setVisible(false);
        }

        if (pipe->x() < 100 && pipe->data(1).toInt() == 0)
        {
            pipe->setData(1, 1);
            score++;
        }

        if (pipe->x() < -100)
        {
            scene->removeItem(pipe);
            delete pipe;
            pipes.removeAt(i);
            i--;
        }
    }

    if (!invincible)
    {
        for (auto pipe : pipes)
        {
            if (birdItem->collidesWithItem(pipe))
            {
                killPlayer();
                return;
            }
        }
    }

    if (view->pos() != viewOriginalPos)
        view->move(viewOriginalPos);

    if (mode == HardMode)
    {
        if (weatherLabel)
        {
            QString wname;
            switch (currentWeather)
            {
            case Sunny: wname = "晴天"; break;
            case Rain:  wname = "暴雨"; break;
            case Fog:   wname = "大雾"; break;
            case Magnetic: wname = "磁场 (按住空格下降)"; break;
            }
            weatherLabel->setPlainText(wname);
        }
        if (livesLabel)
            livesLabel->setPlainText(QString("❤️ %1").arg(lives));
    }
}

void MainWindow::spawnPipe()
{
    const int PIPE_HEIGHT = 320;
    int gap = 145;
    if (mode == HardMode)
    {
        if (currentWeather == Rain) gap = 120;
        if (currentWeather == Magnetic) gap = 130 + rand() % 20;
    }
    int gapY = rand() % 120 + 120;

    QGraphicsPixmapItem *up = scene->addPixmap(pipeUpPix);
    up->setData(1, 0);
    up->setPos(400, gapY - PIPE_HEIGHT);
    up->setZValue(10);
    pipes.append(up);

    QGraphicsPixmapItem *down = scene->addPixmap(pipeDownPix);
    down->setData(1, 0);
    down->setPos(400, gapY + gap);
    down->setZValue(10);
    pipes.append(down);
}

void MainWindow::killPlayer()
{
    if (mode == NormalMode)
    {
        gameOver = true;
        mainTimer->stop();
        weatherTimer->stop();
        rainTimer->stop();
        blinkTimer->stop();
        QMessageBox::information(this, "游戏结束", QString("得分：%1").arg(score));
        createMenu();
    }
    else
    {
        if (lives > 1)
        {
            lives--;
            resetAfterDeath();
        }
        else
        {
            gameOver = true;
            mainTimer->stop();
            weatherTimer->stop();
            rainTimer->stop();
            blinkTimer->stop();
            QMessageBox::information(this, "游戏结束", QString("最终得分：%1").arg(score));
            createMenu();
        }
    }
}

void MainWindow::resetAfterDeath()
{
    if (birdItem)
        birdItem->setPos(100, 200);
    yVelocity = 0;
    gameStarted = true;
    magneticSpacePressed = false;
    // 掉血后无敌0.5秒，不显示文字
    setInvincible(25, false);
}

void MainWindow::setInvincible(int frames, bool showText)
{
    invincible = true;
    invincibleCounter = frames;
    showInvincibleText = showText;
    if (invincibleLabel)
    {
        if (showText)
        {
            int secLeft = (frames + 24) / 25;
            invincibleLabel->setPlainText(QString("无敌 %1 秒").arg(secLeft));
            invincibleLabel->setVisible(true);
        }
        else
        {
            invincibleLabel->setVisible(false);
        }
    }
    blinkTimer->start();
    if (birdItem) birdItem->setOpacity(1.0);
}

void MainWindow::blinkInvincible()
{
    if (!invincible || !birdItem)
    {
        blinkTimer->stop();
        return;
    }
    double op = birdItem->opacity();
    birdItem->setOpacity(op == 1.0 ? 0.4 : 1.0);
}

void MainWindow::weatherLoop()
{
    if (gameOver || mode != HardMode) return;

    if (isWarningActive)
    {
        warningCounter--;
        if (warningLabel && warningCounter >= 0)
        {
            int sec = (warningCounter + 9) / 10;
            warningLabel->setPlainText(QString("⚠️ %1秒后: %2")
                                           .arg(sec).arg(weatherDescription(pendingWeather)));
        }
        if (warningCounter <= 0)
        {
            isWarningActive = false;
            changeWeather(pendingWeather);
            if (warningLabel) warningLabel->setVisible(false);
        }
        return;
    }

    weatherDurationCounter++;
    if (weatherDurationCounter < WEATHER_DURATION_FRAMES) return;

    int rnd = rand() % 3;
    WeatherType newWeather;
    switch (rnd)
    {
    case 0: newWeather = Rain; break;
    case 1: newWeather = Fog; break;
    default: newWeather = Magnetic; break;
    }
    if (newWeather == currentWeather)
        newWeather = (currentWeather == Rain) ? Fog : (currentWeather == Fog) ? Magnetic : Rain;

    startWeatherWarning(newWeather);
}

void MainWindow::startWeatherWarning(WeatherType weather)
{
    if (warningLabel)
    {
        warningLabel->setPlainText(QString("⚠️ 即将切换: %1")
                                       .arg(weatherDescription(weather)));
        warningLabel->setVisible(true);
    }
    isWarningActive = true;
    warningCounter = WARNING_FRAMES;
    pendingWeather = weather;
}

void MainWindow::changeWeather(WeatherType newWeather)
{
    currentWeather = newWeather;
    weatherDurationCounter = 0;
    applyWeatherEffects();
    if (newWeather == Magnetic)
        yVelocity = -2;
    // 切换天气后无敌3秒，显示文字
    setInvincible(INVINCIBLE_FRAMES, true);
}

QString MainWindow::weatherDescription(WeatherType weather) const
{
    switch (weather)
    {
    case Rain:     return "暴雨 🌧️ 重力↑ 跳跃↓";
    case Fog:      return "大雾 🌫️ 管道渐隐";
    case Magnetic: return "磁场 🧲 按住空格下降";
    default:       return "";
    }
}

void MainWindow::applyWeatherEffects()
{
    if (currentWeather == Rain)
        rainTimer->start();
    else
    {
        rainTimer->stop();
        for (auto drop : raindrops)
        {
            scene->removeItem(drop);
            delete drop;
        }
        raindrops.clear();
    }
}

void MainWindow::rainLoop()
{
    if (mode != HardMode || currentWeather != Rain) return;

    int dropCount = (rand() % 4) + 2;
    for (int i = 0; i < dropCount; ++i)
    {
        if (rand() % 100 < 40)
        {
            QGraphicsLineItem *drop = new QGraphicsLineItem();
            drop->setLine(0, 0, 15, 20);
            QPen pen(QColor(180, 220, 255, 200));
            pen.setWidth(2);
            drop->setPen(pen);
            drop->setRotation(30);
            drop->setPos(rand() % 400, 0);
            drop->setZValue(15);
            scene->addItem(drop);
            raindrops.append(drop);
        }
    }

    for (int i = 0; i < raindrops.size(); ++i)
    {
        QGraphicsLineItem *drop = raindrops[i];
        drop->setPos(drop->x(), drop->y() + 12);
        if (drop->y() > 600)
        {
            scene->removeItem(drop);
            delete drop;
            raindrops.removeAt(i);
            i--;
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (gameOver) return;

    if (event->key() == Qt::Key_Space)
    {
        if (mode == HardMode && currentWeather == Magnetic)
        {
            magneticSpacePressed = true;
            if (!gameStarted)
            {
                gameStarted = true;
                yVelocity = -2;
            }
        }
        else
        {
            double jump = jumpPower;
            if (mode == HardMode && currentWeather == Rain)
                jump = -9.0;

            if (!gameStarted)
            {
                gameStarted = true;
                yVelocity = jump;
            }
            else
            {
                yVelocity = jump;
            }
        }
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space)
    {
        if (mode == HardMode && currentWeather == Magnetic)
            magneticSpacePressed = false;
    }
    QMainWindow::keyReleaseEvent(event);
}