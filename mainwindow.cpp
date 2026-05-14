#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMouseEvent>
#include <QMessageBox>
#include <QFont>
#include <ctime>
#include <QGraphicsLineItem>
#include <QPen>

//游戏初始化
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    srand(time(nullptr)); //随机种子

    //图形场景和视图
    scene = new QGraphicsScene(this);
    scene->setSceneRect(0, 0, 400, 600);
    view = new QGraphicsView(scene, this);
    view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setCentralWidget(view);
    setFixedSize(400, 600);
    setWindowTitle("Flappy Bird");
    view->installEventFilter(this);// 处理菜单点击

    //图片资源
    bgPix.load(":/pic/bg.png");
    birdPix.load(":/pic/0.png");
    pipeUpPix.load(":/pic/pipeUp.png");
    pipeDownPix.load(":/pic/pipeDown.png");
    menuBgPix.load(":/pic/start.png");
    btnNormalPix.load(":/pic/normal.png");
    btnHardPix.load(":/pic/hard.png");

    //物理引擎
    gravity = 0.9;
    jumpPower = -10.0;

    //主要定时器
    mainTimer = new QTimer(this);
    mainTimer->setInterval(20);
    connect(mainTimer, &QTimer::timeout, this, &MainWindow::gameLoop);

    //天气定时器
    weatherTimer = new QTimer(this);
    weatherTimer->setInterval(100);
    connect(weatherTimer, &QTimer::timeout, this, &MainWindow::weatherLoop);

    //雨滴定时器
    rainTimer = new QTimer(this);
    rainTimer->setInterval(40);
    connect(rainTimer, &QTimer::timeout, this, &MainWindow::rainLoop);

    //无敌定时器
    blinkTimer = new QTimer(this);
    blinkTimer->setInterval(100);
    connect(blinkTimer, &QTimer::timeout, this, &MainWindow::blinkInvincible);

    //预警震动定时器
    shakeTimer = new QTimer(this);
    shakeTimer->setInterval(30);   // 每30ms震动一次
    connect(shakeTimer, &QTimer::timeout, this, &MainWindow::shakeView);

    weatherLabel = nullptr;
    livesLabel = nullptr;
    warningLabel = nullptr;
    invincibleLabel = nullptr;
    nextLifeLabel = nullptr;
    fogOverlay = nullptr;
    viewOriginalPos = view->pos();
    magneticSpacePressed = false;
    invincible = false;
    invincibleCounter = 0;
    showInvincibleText = false;
    nextLifeThreshold = 50;
    scoreDoubled = false;
    shakeCounter = 0;

    createMenu();// 显示开始菜单
}

// 创建开始菜单
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
    shakeTimer->stop();
    scene->clear();//清空
    gameOver = true;
    view->move(viewOriginalPos);  //复位

    scene->addPixmap(bgPix);
    scene->addPixmap(menuBgPix)->setPos(0, 0);

    //按钮
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
    magneticSpacePressed = false;
    invincible = false;
}

// 处理鼠标点击按钮，开始游戏
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

// 开始新游戏
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
    shakeTimer->stop();
    nextLifeThreshold = 50;
    scoreDoubled = false;

    //鸟
    scene->addPixmap(bgPix);
    birdItem = scene->addPixmap(birdPix);
    birdItem->setPos(100, 200);
    birdItem->setZValue(50);
    birdItem->setTransformOriginPoint(birdItem->boundingRect().center());
    birdItem->setRotation(0);

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
        weatherLabel->setPos(15, 12);
        weatherLabel->setZValue(1000);

        livesLabel = scene->addText(QString("❤️ %1").arg(lives));
        livesLabel->setFont(font);
        livesLabel->setDefaultTextColor(Qt::white);
        livesLabel->setPos(330, 12);
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

        invincibleLabel = scene->addText("");
        QFont invFont;
        invFont.setPointSize(11);
        invFont.setBold(true);
        invincibleLabel->setFont(invFont);
        invincibleLabel->setDefaultTextColor(QColor(255, 215, 0));
        invincibleLabel->setPos(270, 12);
        invincibleLabel->setZValue(1000);
        invincibleLabel->setVisible(false);

        nextLifeLabel = scene->addText(QString("下一命: %1分").arg(nextLifeThreshold - score));
        nextLifeLabel->setFont(invFont);
        nextLifeLabel->setDefaultTextColor(Qt::white);
        nextLifeLabel->setPos(15, 55);
        nextLifeLabel->setZValue(1000);

        //大雾遮挡（默认不可见）
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

// 增加分数，得分翻倍，奖励生命
void MainWindow::addScore(int points)
{
    if (scoreDoubled) points *= 2;
    score += points;
    while (score >= nextLifeThreshold)
    {
        lives++;
        nextLifeThreshold += 50;
        if (livesLabel) livesLabel->setPlainText(QString("❤️ %1").arg(lives));
    }
    updateNextLifeDisplay();
}

//下一命提示文字
void MainWindow::updateNextLifeDisplay()
{
    if (nextLifeLabel)
    {
        int need = nextLifeThreshold - score;
        if (need < 0) need = 0;
        nextLifeLabel->setPlainText(QString("下一命: %1分").arg(need));
    }
}

//震动效果：预警期间每30ms随机偏移视图
void MainWindow::shakeView()
{
    if (!isWarningActive)
    {
        shakeTimer->stop();
        view->move(viewOriginalPos);
        return;
    }
    //随机偏移 -2~2 像素
    int dx = (rand() % 5) - 2;
    int dy = (rand() % 5) - 2;
    view->move(viewOriginalPos.x() + dx, viewOriginalPos.y() + dy);
}

//游戏主循环
void MainWindow::gameLoop()
{
    if (gameOver) return;
    if (!gameStarted) return;

    //无敌倒计时
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

    //小鸟旋转
    if (birdItem)
    {
        double angle = yVelocity * 2.5;
        angle = qBound(-30.0, angle, 30.0);
        birdItem->setRotation(angle);
    }

    //速度更新
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
        if (mode == HardMode && currentWeather == Rain) currentGravity = 1.05;
        yVelocity += currentGravity;
    }

    birdItem->setPos(100, birdItem->y() + yVelocity);

    if (birdItem->y() > 550 || birdItem->y() < 0)
    {
        killPlayer();
        return;
    }

    //生成管道
    pipeSpawnCounter++;
    if (pipeSpawnCounter >= pipeSpawnInterval)
    {
        spawnPipe();
        pipeSpawnCounter = 0;
    }

    //管道移动，计分，删除
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


        int isUpPipe = pipe->data(2).toInt();
        if (pipe->x() < 100 && pipe->data(1).toInt() == 0 && isUpPipe == 1)
        {
            pipe->setData(1, 1);  // 已计分
            addScore(1);          // 一组加1分
        }

        if (pipe->x() < -100)
        {
            scene->removeItem(pipe);
            delete pipe;
            pipes.removeAt(i);
            i--;
        }
    }

    //碰撞检测
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

    //视图复位
    if (view->pos() != viewOriginalPos && !isWarningActive)
        view->move(viewOriginalPos);

    if (mode == HardMode)
    {
        if (weatherLabel)
        {
            QString wname;
            switch (currentWeather)
            {
            case Sunny: wname = "☀️晴天"; break;
            case Rain:  wname = "🌧️ 暴雨"; break;
            case Fog:   wname = "🌫️大雾"; break;
            case Magnetic: wname = " 🧲磁场紊乱 (按住空格下降)"; break;
            case Rainbow: wname = "彩虹 🌈 得分翻倍"; break;
            }
            weatherLabel->setPlainText(wname);
        }
        if (livesLabel)
            livesLabel->setPlainText(QString("❤️ %1").arg(lives));
    }
}

//管道生成
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

    //上
    QGraphicsPixmapItem *up = scene->addPixmap(pipeUpPix);
    up->setData(1, 0);   // 计分标记
    up->setData(2, 1);   // 标记为上管道
    up->setPos(400, gapY - PIPE_HEIGHT);
    up->setZValue(10);
    pipes.append(up);

    //下
    QGraphicsPixmapItem *down = scene->addPixmap(pipeDownPix);
    down->setData(1, 0);
    down->setData(2, 0);   // 下管道，不计分
    down->setPos(400, gapY + gap);
    down->setZValue(10);
    pipes.append(down);
}

//死亡处理
void MainWindow::killPlayer()
{
    if (mode == NormalMode)
    {
        gameOver = true;
        mainTimer->stop();
        weatherTimer->stop();
        rainTimer->stop();
        blinkTimer->stop();
        shakeTimer->stop();
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
            shakeTimer->stop();
            QMessageBox::information(this, "游戏结束", QString("最终得分：%1").arg(score));
            createMenu();
        }
    }
}

//复活
void MainWindow::resetAfterDeath()
{
    if (birdItem)
        birdItem->setPos(100, 200);
    yVelocity = 0;
    gameStarted = true;
    magneticSpacePressed = false;
    setInvincible(25, false);
}

//设置无敌状态
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

//闪烁
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

//天气循环
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
            shakeTimer->stop();
            view->move(viewOriginalPos); // 复位视图
        }
        return;
    }

    weatherDurationCounter++;
    if (weatherDurationCounter < WEATHER_DURATION_FRAMES) return;

    int rnd = rand() % 100;
    WeatherType newWeather;
    if (rnd < 2)
        newWeather = Rainbow;
    else
    {
        int type = rand() % 3;
        switch (type)
        {
        case 0: newWeather = Rain; break;
        case 1: newWeather = Fog; break;
        default: newWeather = Magnetic; break;
        }
    }
    if (newWeather == currentWeather)
    {
        int rnd2 = rand() % 3;
        switch (rnd2)
        {
        case 0: newWeather = Rain; break;
        case 1: newWeather = Fog; break;
        default: newWeather = Magnetic; break;
        }
    }

    startWeatherWarning(newWeather);
}

//天气预警
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
    // 启动震动（每30ms一次）
    shakeTimer->start();
}

//切换天气
void MainWindow::changeWeather(WeatherType newWeather)
{
    currentWeather = newWeather;
    weatherDurationCounter = 0;
    scoreDoubled = (newWeather == Rainbow);
    applyWeatherEffects();
    if (newWeather == Magnetic)
        yVelocity = -2;
    setInvincible(INVINCIBLE_FRAMES, true);
}

QString MainWindow::weatherDescription(WeatherType weather) const
{
    switch (weather)
    {
    case Rain:     return "暴雨 🌧️ 重力↑ 跳跃↓";
    case Fog:      return "大雾 🌫️ 管道渐隐";
    case Magnetic: return "磁场 🧲 按住空格下降";
    case Rainbow:  return "彩虹 🌈 得分翻倍 无负面";
    default:       return "";
    }
}

//天气特效
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
    if (fogOverlay)
        fogOverlay->setVisible(false);
}

//雨丝循环
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

//空格跳跃
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

//磁场：按键释放
void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space)
    {
        if (mode == HardMode && currentWeather == Magnetic)
            magneticSpacePressed = false;
    }
    QMainWindow::keyReleaseEvent(event);
}