#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMouseEvent>
#include <ctime>
#include <QMessageBox>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    srand(time(0)); // 初始化随机数种子

    // 创建游戏舞台和窗口
    scene = new QGraphicsScene(this);
    scene->setSceneRect(0, 0, 400, 600); // 场景强制和窗口一样大

    view = new QGraphicsView(scene, this);
    view->setAlignment(Qt::AlignLeft | Qt::AlignTop); // 靠左上角
    setCentralWidget(view);
    setFixedSize(400, 600);
    setWindowTitle("FlappyBird");

    // 给视图安装事件过滤器，监听鼠标点击
    view->installEventFilter(this);

    // 加载所有图片资源
    bg.load(":/pic/bg.png");
    bird.load(":/pic/0.png");
    pipeUp.load(":/pic/pipeUp.png");
    pipeDown.load(":/pic/pipeDown.png");
    menuBg.load(":/pic/start.png");
    btnNormal.load(":/pic/normal.png");
    btnHard.load(":/pic/hard.png");

    // 初始化重力和定时器
    speed = 0;
    timer = new QTimer(this);

    //初始化计数器和游戏状态
    pipeTimerCount=0;
    isGameOver=false;

    // 程序启动先进入开始界面
    createMenu();
}

MainWindow::~MainWindow()
{
    delete ui;
}

//开始界面
void MainWindow::createMenu(){
    scene->clear();
    isGameOver=true;
    scene->addPixmap(bg);
    scene->addPixmap(menuBg)->setPos(0,0);

    QGraphicsPixmapItem* btnN =scene->addPixmap(btnNormal);
    btnN->setPos(100,360);
    btnN->setData(0,NormalMode);

    QGraphicsPixmapItem* btnH =scene->addPixmap(btnHard);
    btnH->setPos(100,450);
    btnH->setData(0,HardMode);
}

// 事件过滤器：处理菜单鼠标点击
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if(obj == view && event->type() == QEvent::MouseButtonPress && isGameOver)
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        QGraphicsItem* item = scene->itemAt(view->mapToScene(mouseEvent->pos()), QTransform());
        if(item)
        {
            GameMode mode = (GameMode)item->data(0).toInt();
            startGame(mode);
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

// 开始游戏 初始化游戏场景与定时器逻辑
void MainWindow::startGame(GameMode mode)
{
    scene->clear();
    currentMode = mode;
    isGameOver = false;
    score = 0;
    gameStart = false;   // 开局默认未开始，小鸟不动

    // 添加背景
    scene->addPixmap(bg);

    // 添加小鸟
    birdItem = scene->addPixmap(bird);
    birdItem->setPos(100, 200); // 初始悬浮位置

    // 清空旧水管数据
    pipeList.clear();
    pipeTimerCount = 0;
    speed = 0;

    // 普通模式：所有速度固定
    if(currentMode == NormalMode)
    {
        pipeSpawnInterval = 100;
        pipeMoveSpeed = 3.5;
    }
    else
    {
        // 困难模式：初始和普通一样，后续加速
        pipeSpawnInterval = 100;
        pipeMoveSpeed = 3.0;
    }

    // 定时器：控制小鸟下落
    connect(timer, &QTimer::timeout, this, [this](){
        // 游戏结束：小鸟定格，不再移动、不刷新位置
        if(isGameOver)
        {
            return;
        }

        // 没按第一次空格：小鸟静止，不下落、不生成管道
        if(!gameStart)
        {
            return;
        }

        // 小鸟重力下落
        speed += 0.9;
        birdItem->setPos(100, birdItem->y() + speed);

        // 防止小鸟掉出屏幕上下边界
        if(birdItem->y() > 550){
            isGameOver = true;
            QMessageBox::information(this, "游戏结束", QString("最终得分：%1分").arg(score));
            this->close();
            return;
        }
        if(birdItem->y() < 0){
            isGameOver = true;
            QMessageBox::information(this, "游戏结束", QString("最终得分：%1分").arg(score));
            this->close();
            return;
        }

        // 循环生成水管
        pipeTimerCount++;
        if(pipeTimerCount >= pipeSpawnInterval){
            spawnPipe();
            pipeTimerCount = 0;

            // 困难模式 加速变难
            if(currentMode == HardMode)
            {
                if(pipeMoveSpeed < 7.0)
                {
                    pipeMoveSpeed += 0.28;
                }
                if(pipeSpawnInterval > 55)
                {
                    pipeSpawnInterval -= 3;
                }
            }
        }

        // 管道左移
        for(int i=0;i<pipeList.size();i++){
            QGraphicsPixmapItem* pipe = pipeList[i];
            pipe->setPos(pipe->x() - pipeMoveSpeed, pipe->y());

            // 计分
            if(pipe->x() < 100 && pipe->data(1).toInt() == 0){
                pipe->setData(1, 1);
                score++;
            }

            // 移出屏幕删除
            if(pipe->x()<-100){
                scene->removeItem(pipe);
                pipeList.removeAt(i);
                delete pipe;
                i--;
            }
        }

        // 上下管道碰到任意一个都结束游戏
        for(int i=0;i<pipeList.size();i++){
            QGraphicsPixmapItem* pipe = pipeList[i];
            if(birdItem->collidesWithItem(pipe))
            {
                // 碰撞瞬间定格在当前位置
                isGameOver = true;
                // 弹出分数窗口，点OK关闭
                QMessageBox::information(this, "游戏结束", QString("最终得分：%1分").arg(score));
                this->close();
                break;
            }
        }
    });

    timer->start(20);
}

// 游戏结束返回主菜单
void MainWindow::backToMenu()
{
    timer->stop();
    disconnect(timer);
    createMenu();
}

// 空格跳跃函数
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // 游戏结束后禁止空格操作
    if(isGameOver)
    {
        return;
    }

    if(event->key() == Qt::Key_Space){

        // 第一次按空格：正式开始游戏
        if(!gameStart)
        {
            gameStart = true;
            speed = -10; // 开局直接跳一下
            return;
        }

        if(isGameOver){
            //游戏结束，按空格重新开始
            isGameOver=false;
            gameStart = false;  // 重置为未开始
            speed=0;
            birdItem->setPos(100,200); //鸟归位
            //清空水管
            for(int i=0;i<pipeList.size();i++){
                QGraphicsPixmapItem* pipe = pipeList[i];
                scene->removeItem(pipe);
                delete pipe;
            }
            pipeList.clear();
            pipeTimerCount=0;
            score = 0;
        }
        else
        {
            // 正常跳跃
            speed=-10;
        }
    }
    // 调用基类函数
    QMainWindow::keyPressEvent(event);
}

//生成水管函数
void MainWindow:: spawnPipe(){
    const int PIPE_HEIGHT = 320;   // 管道高度
    const int FIX_GAP = 145;       // 缝隙保持你要的大小

    // 随机缝隙
    int gapY = rand() % 120 + 120;

    //上管道
    QGraphicsPixmapItem* upPipe = scene->addPixmap(pipeUp);
    upPipe->setData(1, 0);
    upPipe->setPos(400, gapY - PIPE_HEIGHT);
    pipeList.append(upPipe);

    //下管道
    QGraphicsPixmapItem* downPipe = scene->addPixmap(pipeDown);
    downPipe->setData(1, 0);
    downPipe->setPos(400, gapY + FIX_GAP);
    pipeList.append(downPipe);
}