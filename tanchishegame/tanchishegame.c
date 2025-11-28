#define _CRT_SECURE_NO_WARNINGS 1

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include <string.h>
#include <limits.h>
#include<mmsystem.h>
#pragma comment(lib,"winmm.lib")

#define ROW 28 //游戏区行数
#define COL 60 //游戏区列数
#define KONG 0 //标记空（什么也没有）
#define WALL 1 //标记墙
#define FOOD 2 //标记食物
#define HEAD 3 //标记蛇头
#define BODY 4 //标记蛇身
#define OBSTACLE 5 //标记障碍物
#define UP 72 //方向键：上
#define DOWN 80 //方向键：下
#define LEFT 75 //方向键：左
#define RIGHT 77 //方向键：右
#define SPACE 32 //暂停
#define ESC 27 //退出
#define F1 59 //F1加速
#define F2 60 //F2减速  

#define HEAD2 5 //ai蛇头标记
#define BODY2 6 //ai蛇身标记


//排行榜结构体
typedef struct {
    char name[20];
    int score;
    int time_left; //限时模式剩余时间（0表示普通模式）
    char difficulty[10];
} RankEntry;

//蛇的结构体
struct Snake {
    int len; //记录蛇身长度
    int x; //蛇头横坐标
    int y; //蛇头纵坐标
} snake;

//蛇身结构体
struct Body {
    int x; //蛇身横坐标
    int y; //蛇身纵坐标
} body[ROW * COL]; //开辟足以存储蛇身的结构体数组

//蛇的结构体(双人)
struct Snake2 {
    int len; //记录蛇身长度
    int x; //蛇头横坐标
    int y; //蛇头纵坐标
} snake2;

//蛇身结构体(双人)
struct Body2 {
    int x; //蛇身横坐标
    int y; //蛇身纵坐标
} body2[ROW * COL]; //开辟足以存储蛇身的结构体数组

int face[ROW][COL]; //标记游戏区各个位置的状态
int max_score = 0;  //最高分
int max_score2 = 0; //最高分(玩家2)
int current_score = 0; //当前得分
int current_score2 = 0; //当前得分（玩家2）
int speed = 300;   //初始速度，值越小越快
int game_mode = 0; //游戏模式：1-单人普通，2-双人，3-AI对战，4-单人限时
int difficulty = 1; //难度：1-简单，2-中等，3-困难
int time_limit = 60; //限时模式倒计时（秒）
int remaining_time; //剩余时间
char player_name[20]; //玩家姓名
RankEntry rank_list[10]; //排行榜（最多10人）
int rank_count = 0; //排行榜人数

//ai游戏
void gameai();
//ai游戏区设计
void InitInterfaceai();
//ai游戏判断得分与结束
void JudgeFuncai(int x, int y);
//ai游戏主体逻辑函数
void Gameaizhu();
//打印蛇与覆盖蛇
void DrawSnake1ai(int flag);
//移动蛇
void MoveSnake1ai(int x, int y);
//执行按键
void run(int x, int y);
//随机生成食物
void RandFoodai();
// 第二条蛇（AI）函数原型与并发支持
void InitOtherSnakeai();
void DrawOtherSnakeai(int flag);
void MoveOtherSnakeai(int x, int y);
void AutoMoveOneStep2();
DWORD WINAPI AutoThreadProc(LPVOID lpParam);
void ToggleAI();
// 并发保护
CRITICAL_SECTION g_cs;
// AI 线程句柄与运行标志
HANDLE g_aiThread = NULL;
volatile BOOL ai_running = FALSE;
// AI 得分
int ai_score = 0;
int face[ROW][COL]; //标记游戏区各个位置的状态
int max, grade;

int fen[2] = { 1, 1 }, shijian = 3000, t; //全局变量
//int a = rand()% 5; //初始蛇的长度随机数a



//颜色函数
void color(int c) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

//隐藏光标
void HideCursor() {
    CONSOLE_CURSOR_INFO curInfo;
    curInfo.dwSize = 1;
    curInfo.bVisible = FALSE;
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleCursorInfo(handle, &curInfo);
}

//光标跳转
void CursorJump(int x, int y) {
    COORD pos;
    pos.X = x;
    pos.Y = y;
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleCursorPosition(handle, pos);
}

//读取最高分
void ReadHighScore() {
    FILE* fp = fopen("snake_score.txt", "r");
    if (fp != NULL) {
        fscanf(fp, "%d", &max_score);
        fclose(fp);
    }
}

//读取最高分(双人)
void ReadHighScore2() {
    FILE* fp = fopen("snake2_score2.txt", "r");
    if (fp != NULL) {
        fscanf(fp, "%d", &max_score2);
        fclose(fp);
    }
}

//写入最高分
void WriteHighScore() {
    if (current_score > max_score) {
        max_score = current_score;
        FILE* fp = fopen("snake_score.txt", "w");
        if (fp != NULL) {
            fprintf(fp, "%d", max_score);
            fclose(fp);
        }
    }
}

//写入最高分(双人)
void WriteHighScore2() {
    if (current_score2 > max_score2) {
        max_score2 = current_score2;
        FILE* fp = fopen("snake2_score2.txt", "w");
        if (fp != NULL) {
            fprintf(fp, "%d", max_score2);
            fclose(fp);
        }
    }
}

//读取排行榜
void ReadRankList() {
    FILE* fp = fopen("snake_rank.txt", "r");
    if (fp == NULL) return;

    rank_count = 0;
    while (rank_count < 10 && fscanf(fp, "%s %d %d %s",
        rank_list[rank_count].name,
        &rank_list[rank_count].score,
        &rank_list[rank_count].time_left,
        &rank_list[rank_count].difficulty) != EOF) {
        rank_count++;
    }
    fclose(fp);
}

//保存排行榜
void SaveRankList() {
    //添加新记录
    RankEntry new_entry;
    strncpy(new_entry.name, player_name, sizeof(new_entry.name) - 1);
    new_entry.name[sizeof(new_entry.name) - 1] = '\0';
    new_entry.score = current_score;
    new_entry.time_left = (game_mode == 4) ? remaining_time : 0;

    switch (difficulty) {
    case 1: strcpy(new_entry.difficulty, "简单"); break;
    case 2: strcpy(new_entry.difficulty, "中等"); break;
    case 3: strcpy(new_entry.difficulty, "困难"); break;
    }

    //插入排序（按分数降序）
    int i = rank_count;
    while (i > 0 && new_entry.score > rank_list[i - 1].score) {
        if (i < 10) rank_list[i] = rank_list[i - 1];
        i--;
    }
    if (i < 10) {
        rank_list[i] = new_entry;
        if (rank_count < 10) rank_count++;
    }

    //写入文件
    FILE* fp = fopen("snake_rank.txt", "w");
    if (fp == NULL) return;

    for (i = 0; i < rank_count; i++) {
        fprintf(fp, "%s %d %d %s\n",
            rank_list[i].name,
            rank_list[i].score,
            rank_list[i].time_left,
            rank_list[i].difficulty);
    }
    fclose(fp);
}

//显示排行榜
void ShowRankList() {
    system("cls");
    color(14);
    CursorJump(COL - 10, 2);
    printf("贪吃蛇排行榜");

    //绘制边框
    color(6);
    for (int i = 4; i <= 16; i++) {
        for (int j = COL - 30; j <= COL + 30; j++) {
            CursorJump(j, i);
            if (i == 4 || i == 16) printf("-");
            else if (j == COL - 30 || j == COL + 30) printf("|");
            else printf(" ");
        }
    }

    //表头
    color(10);
    CursorJump(COL - 25, 5); printf("排名");
    CursorJump(COL - 15, 5); printf("姓名");
    CursorJump(COL - 5, 5); printf("分数");
    CursorJump(COL + 5, 5); printf("模式");
    CursorJump(COL + 15, 5); printf("难度");

    //显示排名
    color(15);
    for (int i = 0; i < rank_count; i++) {
        CursorJump(COL - 24, 7 + i); printf("%d", i + 1);
        CursorJump(COL - 15, 7 + i); printf("%s", rank_list[i].name);
        CursorJump(COL - 5, 7 + i); printf("%d", rank_list[i].score);

        if (rank_list[i].time_left > 0) {
            CursorJump(COL + 5, 7 + i); printf("限时");
        }
        else {
            CursorJump(COL + 5, 7 + i); printf("普通");
        }

        CursorJump(COL + 15, 7 + i); printf("%s", rank_list[i].difficulty);
    }

    color(3);
    CursorJump(COL - 15, 18);
    printf("按任意键返回主菜单...");
    _getch();
}

//生成障碍物
void GenerateObstacles() {
    int obstacle_count = 0;
    //根据难度设置障碍物数量
    switch (difficulty) {
    case 1: obstacle_count = 5; break;  //简单：5个障碍物
    case 2: obstacle_count = 15; break; //中等：15个障碍物
    case 3: obstacle_count = 30; break; //困难：30个障碍物
    }

    color(8); //灰色障碍物
    int x, y;
    for (int i = 0; i < obstacle_count; i++) {
        do {
            //随机生成障碍物位置（不包括墙、蛇身、蛇头）
            x = rand() % (COL - 4) + 2;
            y = rand() % (ROW - 4) + 2;
        } while (face[y][x] != KONG);

        face[y][x] = OBSTACLE;
        CursorJump(2 * x, y);
        printf("■");
    }
}

//选择难度
void SelectDifficulty() {
    system("cls");
    color(14);
    CursorJump(COL, ROW / 2 - 3);
    printf("选择难度");

    color(15);
    CursorJump(COL - 10, ROW / 2 - 1);
    printf("1. 简单 (速度慢，障碍物少)");
    CursorJump(COL - 10, ROW / 2 + 1);
    printf("2. 中等 (速度中等，障碍物中等)");
    CursorJump(COL - 10, ROW / 2 + 3);
    printf("3. 困难 (速度快，障碍物多)");

    color(10);
    CursorJump(COL - 5, ROW / 2 + 5);
    printf("请选择 [1-3]:");

    while (1) {
        if (_kbhit()) {
            int key = _getch();
            if (key >= '1' && key <= '3') {
                difficulty = key - '0';
                //设置对应初始速度
                switch (difficulty) {
                case 1: speed = 400; break; //简单：慢
                case 2: speed = 300; break; //中等：中
                case 3: speed = 200; break; //困难：快
                }
                break;
            }
        }
    }
}

//选择限时时间
void SelectTimeLimit() {
    system("cls");
    color(14);
    CursorJump(COL, ROW / 2 - 3);
    printf("选择限时时间");

    color(15);
    CursorJump(COL - 10, ROW / 2 - 1);
    printf("1. 30秒");
    CursorJump(COL - 10, ROW / 2 + 1);
    printf("2. 60秒");
    CursorJump(COL - 10, ROW / 2 + 3);
    printf("3. 90秒");

    color(10);
    CursorJump(COL - 5, ROW / 2 + 5);
    printf("请选择 [1-3]:");

    while (1) {
        if (_kbhit()) {
            int key = _getch();
            switch (key) {
            case '1': time_limit = 30; break;
            case '2': time_limit = 60; break;
            case '3': time_limit = 90; break;
            default: continue;
            }
            remaining_time = time_limit;
            break;
        }
    }
}

//输入玩家姓名
void InputPlayerName() {
    system("cls");
    color(14);
    CursorJump(COL - 10, ROW / 2 - 2);
    printf("游戏结束！");

    color(15);
    CursorJump(COL - 15, ROW / 2);
    printf("请输入你的姓名（最多10个字符）：");

    scanf("%s", player_name);
    //限制姓名长度
    player_name[10] = '\0';
    getchar(); //吸收回车
}

//游戏欢迎界面（修改：增加限时模式选项）
int WelcomeMenu() {
    system("cls");
    //播放音乐    
    mciSendString(TEXT("open \"C:\\Users\\lenovo\\Desktop\\欢迎背景.mp3\" type mpegvideo alias music1"), NULL, 0, NULL);
    mciSendString(TEXT("play music1 repeat"), NULL, 0, NULL);
    //画欢迎界面的蛇
    color(6);
    CursorJump(35, 0); printf("/^\\/ ^\\");
    CursorJump(34, 1); printf("|_0|  0|");
    CursorJump(33, 1); color(2); printf("_");
    CursorJump(25, 2); color(12); printf("\\/");
    CursorJump(31, 2); color(2); printf("/");
    CursorJump(38, 2); color(6); printf("\\_/");
    CursorJump(41, 2); color(10); printf("\\");
    CursorJump(26, 3); color(12); printf("\\____");
    CursorJump(31, 3); color(2); printf("|");
    CursorJump(32, 3); printf("_________/");
    CursorJump(42, 3); color(10); printf("\\");
    CursorJump(31, 4); color(2); printf("\\_______");
    CursorJump(43, 4); color(10); printf("\\");
    CursorJump(38, 5); printf(" |    |        贪吃蛇游戏             \\");
    CursorJump(38, 6); printf(" /    /                               \\ \\");
    CursorJump(36, 7); printf(" /    /                                  \\  \\");
    CursorJump(34, 8); printf(" /    /                _----_                \\  \\");
    CursorJump(33, 9); printf(" /    /             _--~       --_            |   |");
    CursorJump(33, 10); printf("(    (        _----~     _--_      --_      _/    |");
    CursorJump(34, 11); printf("\\    ~-——-~     --~---    ~--      ~-_-~       /");
    CursorJump(35, 12); printf("\\           -—~              ~—-         -—~");
    CursorJump(36, 13); printf("~—-----—~                      ~—---—~        ");
    //菜单边框
    color(14);
    for (int i = 14; i <= 29; i++) {
        for (int j = 35; j <= 81; j++) {
            CursorJump(j, i);
            if (i == 14 || i == 29) printf("-");
            else if (j == 35 || j == 81) printf("|");
            else printf(" ");
        }
    }

    //菜单选项
    color(15);
    CursorJump(39, 15); printf("1. 单人普通模式");
    CursorJump(39, 17); printf("2. 双人模式 ");
    CursorJump(39, 19); printf("3. AI 对战");
    CursorJump(39, 21); printf("4. 单人限时模式");
    CursorJump(39, 23); printf("5. 游戏说明");
    CursorJump(39, 25); printf("6. 查看排行榜");
    CursorJump(39, 27); printf("7. 退出游戏");

    //选择提示
    color(10);
    CursorJump(39, 28); printf("请选择 [1-7]:");

    int choice;
    scanf("%d", &choice);
    getchar(); //吸收回车
    return choice;
}

//游戏说明界面（修改：添加新功能说明）
void GameInstructions() {
    system("cls");
    //播放音乐    
    mciSendString(TEXT("open \"C:\\Users\\lenovo\\Desktop\\欢迎背景.mp3\" type mpegvideo alias music1"), NULL, 0, NULL);
    mciSendString(TEXT("play music1 repeat"), NULL, 0, NULL);
    color(13);
    CursorJump(54, 1); printf("游戏说明");

    color(2);
    //边框
    for (int i = 3; i <= 29; i++) {
        for (int j = 28; j <= 87; j++) {
            CursorJump(j, i);
            if (i == 3 || i == 29) printf("=");
            else if (j == 28 || j == 87) printf("||");
        }
    }

    color(3);
    CursorJump(32, 5); printf("单人模式操作说明：");
    CursorJump(32, 7); printf("↑↓←→键：控制蛇的移动方向");
    CursorJump(32, 9); printf("Space键：暂停/继续游戏");
    CursorJump(32, 11); printf("F1键：加速游戏");
    CursorJump(32, 13); printf("F2键：减速游戏");
    CursorJump(32, 15); printf("ESC键：退出游戏");

    color(6);
    CursorJump(58, 5); printf("双人模式操作说明：");
    CursorJump(58, 7); printf("↑↓←→键：玩家1控制蛇的移动方向");
    CursorJump(58, 9); printf("wsad键：玩家2控制蛇的移动方向");
    CursorJump(58, 11); printf("Space键：暂停/继续游戏");
    CursorJump(58, 13); printf("F1键：加速游戏");
    CursorJump(58, 15); printf("F2键：减速游戏");
    CursorJump(58, 17); printf("ESC键：退出游戏");

    color(5);
    CursorJump(35, 18); printf("新增功能说明：");
    CursorJump(35, 20); printf("1. 难度选择：简单/中等/困难（不同速度和障碍物数量）");
    CursorJump(35, 22); printf("2. 限时模式：30/60/90秒倒计时，时间到游戏结束");
    CursorJump(35, 24); printf("3. 障碍物：灰色方块，碰到即结束游戏");
    CursorJump(35, 26); printf("4. 排行榜：记录玩家姓名、分数、模式和难度");

    color(12);
    CursorJump(35, 28); printf("按下回车键返回主菜单...");
    getchar();
}

//初始化游戏界面（修改：添加限时显示）
void InitInterface() {
    //清空游戏区
    for (int i = 0; i < ROW; i++) {
        for (int j = 0; j < COL; j++) {
            face[i][j] = KONG;
        }
    }

    //画墙
    color(6);
    for (int i = 0; i < ROW; i++) {
        for (int j = 0; j < COL; j++) {
            if (j == 0 || j == COL - 1 || i == 0 || i == ROW - 1) {
                face[i][j] = WALL;
                CursorJump(2 * j, i);
                printf("■");
            }
        }
    }

    //显示分数和速度
    color(7);
    CursorJump(0, ROW); printf("当前得分: %d", current_score);
    CursorJump(COL * 2 - 20, ROW); printf("最高分: %d", max_score);
    CursorJump(COL - 10, ROW); printf("速度: %d", 1000 - speed);

    //限时模式显示倒计时
    if (game_mode == 4) {
        CursorJump(COL / 2 - 10, ROW);
        printf("剩余时间: %d秒", remaining_time);
    }
}

//初始化蛇
void InitSnake()
{
    //初始长度为3
    snake.len = 3;

    //初始位置在屏幕中心
    snake.x = COL / 2;
    snake.y = ROW / 2;

    //初始化蛇身
    for (int i = 0; i < snake.len; i++)
    {
        body[i].x = snake.x - i - 1;
        body[i].y = snake.y;
        face[body[i].y][body[i].x] = BODY;
    }

    //标记蛇头
    face[snake.y][snake.x] = HEAD;
}

//初始化蛇1(双人)
void InitSnake1()
{
    //初始长度为3
    snake.len = 3;
    snake.x = COL / 2 - 10;
    snake.y = ROW / 2;

    //初始化蛇身
    for (int i = 0; i < snake.len; i++)
    {
        body[i].x = snake.x - i - 1;
        body[i].y = snake.y;
        face[body[i].y][body[i].x] = BODY;
    }

    //标记蛇头
    face[snake.y][snake.x] = HEAD;
}

//初始化蛇2(双人)
void InitSnake2()
{
    // 初始化玩家2（右侧）
    snake2.len = 3;
    snake2.x = COL / 2 + 10;
    snake2.y = ROW / 2;

    //初始化蛇身
    for (int i = 0; i < snake2.len; i++)
    {
        body2[i].x = snake2.x + i + 1;
        body2[i].y = snake2.y;
        face[body2[i].y][body2[i].x] = BODY;
    }

    //标记蛇头
    face[snake2.y][snake2.x] = HEAD;
}

//随机生成食物
void GenerateFood() {
    int x, y;
    do {
        //随机生成食物位置（不包括墙、蛇身、障碍物）
        x = rand() % (COL - 2) + 1;
        y = rand() % (ROW - 2) + 1;
    } while (face[y][x] != KONG); //确保食物不在蛇身上、墙上或障碍物上

    face[y][x] = FOOD;
    color(12); //红色食物
    CursorJump(2 * x, y);
    printf("■");
}

//绘制蛇
void DrawSnake()
{
    //绘制蛇头
    color(14); //黄色蛇头
    CursorJump(2 * snake.x, snake.y);
    printf("★");

    //绘制蛇身
    color(2); //绿色蛇身
    for (int i = 0; i < snake.len; i++) {
        CursorJump(2 * body[i].x, body[i].y);
        printf("■");
    }
}

//绘制蛇(双人)
void DrawSnake2()
{
    //绘制蛇头
    color(14); //黄色蛇头
    CursorJump(2 * snake2.x, snake2.y);
    printf("◆");

    //绘制蛇身
    color(2); //绿色蛇身
    for (int i = 0; i < snake2.len; i++) {
        CursorJump(2 * body2[i].x, body2[i].y);
        printf("■");
    }
}

//移动蛇（修改：增加障碍物碰撞检测和限时处理）
void MoveSnake(int dx, int dy)
{
    // 1. 保存当前蛇尾位置（用于新增身体块初始化）
    int tail_x = body[snake.len - 1].x;
    int tail_y = body[snake.len - 1].y;

    // 2. 保存蛇头移动前的旧位置（用于更新第一节身体）
    int old_head_x = snake.x;
    int old_head_y = snake.y;

    // 3. 更新蛇头位置
    snake.x += dx;
    snake.y += dy;

    // 4. 碰撞检测（撞墙/撞自身/撞障碍物）
    if (face[snake.y][snake.x] == WALL || face[snake.y][snake.x] == BODY || face[snake.y][snake.x] == OBSTACLE) {
        game_mode = 0; // 结束游戏
        //播放音乐     
        mciSendString(TEXT("open \"C:\\Users\\lenovo\\Desktop\\失败.mp3\" type mpegvideo alias music4"), NULL, 0, NULL);
        mciSendString(TEXT("play music4"), NULL, 0, NULL);
        return;
    }
    else {
        mciSendString(TEXT("close music4"), NULL, 0, NULL);
    }

    // 5. 处理蛇身移动逻辑
    int is_eat_food = 0; // 标记是否吃到食物
    int FoodSoundPlayed = 0; // 标记是否播放过吃食物的声音
    if (face[snake.y][snake.x] == FOOD) {
        // 5.1 吃到食物：增加长度+初始化新蛇尾
        is_eat_food = 1;
        FoodSoundPlayed = 1;
        //播放音乐     
        mciSendString(TEXT("open \"C:\\Users\\lenovo\\Desktop\\吃东西.mp3\" type mpegvideo alias music3"), NULL, 0, NULL);
        mciSendString(TEXT("play music3"), NULL, 0, NULL);
        snake.len++; // 长度+1
        body[snake.len - 1].x = tail_x; // 新蛇尾 = 原蛇尾位置
        body[snake.len - 1].y = tail_y;
        face[tail_y][tail_x] = BODY; // 标记新蛇尾为身体（防止食物生成在这里）

        // 更新分数和食物
        current_score += 10;
        GenerateFood();
        color(7);
        CursorJump(0, ROW);
        printf("当前得分: %d", current_score);
    }
    else {
        // 5.2 没吃到食物：擦除原蛇尾（因为蛇要移动，尾巴会离开原位置）
        face[tail_y][tail_x] = KONG;
        CursorJump(2 * tail_x, tail_y);
        printf("  "); // 用空格覆盖原蛇尾
        FoodSoundPlayed = 0;//重置播放标记
        mciSendString(TEXT("close music3"), NULL, 0, NULL);
    }

    // 6. 移动所有蛇身（从后往前，后一节复制前一节的位置）
    for (int i = snake.len - 1; i > 0; i--) {
        body[i] = body[i - 1];
    }

    // 7. 更新第一节身体（原蛇头位置变为第一节身体）
    body[0].x = old_head_x;
    body[0].y = old_head_y;
    face[old_head_y][old_head_x] = BODY; // 标记为身体

    // 8. 更新蛇头标记
    face[snake.y][snake.x] = HEAD;

    // 9. 重新绘制蛇
    DrawSnake();
}

//移动蛇2(双人)
void MoveSnake2(int dx, int dy)
{
    // 1. 保存当前蛇尾位置（用于新增身体块初始化）
    int tail_x = body2[snake2.len - 1].x;
    int tail_y = body2[snake2.len - 1].y;

    // 2. 保存蛇头移动前的旧位置（用于更新第一节身体）
    int old_head_x = snake2.x;
    int old_head_y = snake2.y;

    // 3. 更新蛇头位置
    snake2.x += dx;
    snake2.y += dy;

    // 4. 碰撞检测（撞墙/撞自身）
    if (face[snake2.y][snake2.x] == WALL || face[snake2.y][snake2.x] == BODY)
    {
        game_mode = 0; // 结束游戏
        //播放音乐     
        mciSendString(TEXT("open \"C:\\Users\\lenovo\\Desktop\\失败.mp3\" type mpegvideo alias music4"), NULL, 0, NULL);
        mciSendString(TEXT("play music4"), NULL, 0, NULL);
        return;
    }
    else {
        mciSendString(TEXT("close music4"), NULL, 0, NULL);
    }

    // 5. 处理蛇身移动逻辑
    int is_eat_food = 0; // 标记是否吃到食物
    int FoodSoundPlayed = 0; // 标记是否播放过吃食物的声音
    if (face[snake2.y][snake2.x] == FOOD) {
        // 5.1 吃到食物：增加长度+初始化新蛇尾
        is_eat_food = 1;
        FoodSoundPlayed = 1;
        //播放音乐     
        mciSendString(TEXT("open \"C:\\Users\\lenovo\\Desktop\\吃东西.mp3\" type mpegvideo alias music3"), NULL, 0, NULL);
        mciSendString(TEXT("play music3"), NULL, 0, NULL);
        snake2.len++; // 长度+1
        body2[snake2.len - 1].x = tail_x; // 新蛇尾 = 原蛇尾位置
        body2[snake2.len - 1].y = tail_y;
        face[tail_y][tail_x] = BODY; // 标记新蛇尾为身体（防止食物生成在这里）

        // 更新分数和食物
        current_score2 += 10;
        GenerateFood();
        color(7);
        CursorJump(0, ROW);
        printf("当前得分: %d", current_score2);
    }
    else {
        // 5.2 没吃到食物：擦除原蛇尾（因为蛇要移动，尾巴会离开原位置）
        face[tail_y][tail_x] = KONG;
        CursorJump(2 * tail_x, tail_y);
        printf("  "); // 用空格覆盖原蛇尾
        FoodSoundPlayed = 0;//重置播放标记
        mciSendString(TEXT("close music4"), NULL, 0, NULL);
    }

    // 6. 移动所有蛇身（从后往前，后一节复制前一节的位置）
    for (int i = snake2.len - 1; i > 0; i--) {
        body2[i] = body2[i - 1];
    }

    // 7. 更新第一节身体（原蛇头位置变为第一节身体）
    body2[0].x = old_head_x;
    body2[0].y = old_head_y;
    face[old_head_y][old_head_x] = BODY; // 标记为身体

    // 8. 更新蛇头标记
    face[snake2.y][snake2.x] = HEAD;

    // 9. 重新绘制蛇
    DrawSnake2();
}

//游戏结束界面(单人)（修改：添加姓名输入和排行榜）
void GameOver()
{
    Sleep(500);
    InputPlayerName(); //输入姓名

    system("cls");

    color(12);
    CursorJump(COL, ROW / 2 - 4);
    printf("游戏结束!");

    color(7);
    CursorJump(COL - 5, ROW / 2 - 2);
    printf("玩家：%s", player_name);
    CursorJump(COL, ROW / 2);
    printf("最终得分: %d", current_score);

    //显示模式和难度
    CursorJump(COL - 8, ROW / 2 + 2);
    if (game_mode == 4) {
        printf("模式：限时%d秒", time_limit);
    }
    else {
        printf("模式：普通模式");
    }

    CursorJump(COL - 8, ROW / 2 + 4);
    char diff_str[10];
    switch (difficulty) {
    case 1: strcpy(diff_str, "简单"); break;
    case 2: strcpy(diff_str, "中等"); break;
    case 3: strcpy(diff_str, "困难"); break;
    }
    printf("难度：%s", diff_str);

    //更新最高分和排行榜
    WriteHighScore();
    ReadRankList(); //读取现有排行榜
    SaveRankList(); //保存新记录

    CursorJump(COL, ROW / 2 + 6);
    printf("最高分: %d", max_score);

    if (current_score == max_score) {
        color(14);
        CursorJump(COL, ROW / 2 + 8);
        printf("恭喜你创造了新纪录!");
    }
    else {
        color(10);
        CursorJump(COL, ROW / 2 + 8);
        printf("距离最高分还差: %d分", max_score - current_score);
    }

    color(3);
    CursorJump(COL - 10, ROW / 2 + 10);
    printf("按任意键返回主菜单...");
    _getch();
}

//游戏结束界面(双人)
void GameOver2()
{
    Sleep(500);
    system("cls");

    color(12);
    CursorJump(COL, ROW / 2 - 6);
    printf("游戏结束!");

    color(7);
    CursorJump(COL - 3, ROW / 2 - 2);
    printf("玩家1最终得分: %d", current_score);

    color(7);
    CursorJump(COL - 3, ROW / 2);
    printf("玩家2最终得分: %d", current_score2);

    //更新最高分
    WriteHighScore();
    WriteHighScore2();

    CursorJump(COL - 18, ROW / 2 + 2);
    printf("玩家1最高分: %d", max_score);

    CursorJump(COL + 13, ROW / 2 + 2);
    printf("玩家2最高分: %d", max_score2);

    if (current_score == max_score)
    {
        color(14);
        CursorJump(COL - 3, ROW / 2 + 4);
        printf("恭喜玩家1创造了新纪录!");
    }
    else
    {
        color(10);
        CursorJump(COL - 3, ROW / 2 + 4);
        printf("玩家1距离最高分还差: %d分", max_score - current_score);
    }

    if (current_score2 == max_score2)
    {
        color(14);
        CursorJump(COL - 3, ROW / 2 + 6);
        printf("恭喜玩家2创造了新纪录!");
    }
    else
    {
        color(10);
        CursorJump(COL - 3, ROW / 2 + 6);
        printf("玩家2距离最高分还差: %d分", max_score2 - current_score2);
    }

    color(3);
    CursorJump(COL - 3, ROW / 2 + 8);
    printf("按任意键返回主菜单...");
    _getch();
}

//单人普通模式主函数（修改：添加难度选择和障碍物）
void SinglePlayerMode() {
    SelectDifficulty(); //选择难度

    system("cls");
    system("title 贪吃蛇 - 单人普通模式");
    system("mode con cols=124 lines=32"); //设置窗口大小

    HideCursor();//隐藏控制台中的光标
    ReadHighScore();//从文件中读取最高分数并将其存储于变量
    InitInterface();//初始化游戏界面
    InitSnake();//初始化蛇的长度、位置以及蛇身的初始状态。
    GenerateObstacles();//生成障碍物
    GenerateFood();//随机生成食物的位置
    DrawSnake();//绘制蛇

    int direction = RIGHT; //初始方向向右
    int is_paused = 0;     //暂停状态
    clock_t last_time = clock(); //用于计时

    while (game_mode == 1)
    {
        //播放音乐
        mciSendString(TEXT("open \"C:\\Users\\lenovo\\Desktop\\游戏背景.mp3\" type mpegvideo alias music2"), NULL, 0, NULL);
        mciSendString(TEXT("play music2 repeat"), NULL, 0, NULL);
        //键盘控制
        if (_kbhit())
        {
            int key = _getch();
            switch (key)
            {
            case UP:
                if (direction != DOWN) direction = UP;
                break;
            case DOWN:
                if (direction != UP) direction = DOWN;
                break;
            case LEFT:
                if (direction != RIGHT) direction = LEFT;
                break;
            case RIGHT:
                if (direction != LEFT) direction = RIGHT;
                break;
            case SPACE:
                is_paused = !is_paused; //切换暂停状态
                if (is_paused) {
                    color(12);
                    CursorJump(COL - 8, ROW + 2);
                    printf("已暂停");
                }
                else {
                    color(10);
                    CursorJump(COL - 8, ROW + 2);
                    printf("      "); //清除暂停提示
                    last_time = clock(); //恢复计时
                }
                break;
            case F1:
                //加速，最低速度为100
                if (speed > 100) speed -= 50;
                color(7);
                CursorJump(COL - 10, ROW);
                printf("速度: %d", 1000 - speed);
                break;
            case F2:
                //减速，最高速度为500
                if (speed < 500) speed += 50;
                color(7);
                CursorJump(COL - 10, ROW);
                printf("速度: %d", 1000 - speed);
                break;
            case ESC:
                game_mode = 0; //退出游戏
                break;
            }
        }

        //如果未暂停，则移动蛇
        if (!is_paused)
        {
            switch (direction) {
            case UP:
                MoveSnake(0, -1);
                break;
            case DOWN:
                MoveSnake(0, 1);
                break;
            case LEFT:
                MoveSnake(-1, 0);
                break;
            case RIGHT:
                MoveSnake(1, 0);
                break;
            }
        }

        //检查游戏是否结束
        if (game_mode == 0) break;

        Sleep(speed); //控制移动速度
    }
    //音乐结束
    mciSendString(TEXT("close music2"), NULL, 0, NULL);
    //游戏结束
    GameOver();
}

//单人限时模式主函数
void TimeLimitMode() {
    SelectDifficulty(); //选择难度
    SelectTimeLimit(); //选择限时时间

    system("cls");
    system("title 贪吃蛇 - 单人限时模式");
    system("mode con cols=124 lines=32"); //设置窗口大小

    HideCursor();//隐藏控制台中的光标
    ReadHighScore();//从文件中读取最高分数并将其存储于变量
    InitInterface();//初始化游戏界面
    InitSnake();//初始化蛇的长度、位置以及蛇身的初始状态。
    GenerateObstacles();//生成障碍物
    GenerateFood();//随机生成食物的位置
    DrawSnake();//绘制蛇

    int direction = RIGHT; //初始方向向右
    int is_paused = 0;     //暂停状态
    clock_t last_time = clock(); //用于计时

    while (game_mode == 4)
    {
        //播放音乐
        mciSendString(TEXT("open \"C:\\Users\\lenovo\\Desktop\\游戏背景.mp3\" type mpegvideo alias music2"), NULL, 0, NULL);
        mciSendString(TEXT("play music2 repeat"), NULL, 0, NULL);
        //键盘控制
        if (_kbhit())
        {
            int key = _getch();
            switch (key)
            {
            case UP:
                if (direction != DOWN) direction = UP;
                break;
            case DOWN:
                if (direction != UP) direction = DOWN;
                break;
            case LEFT:
                if (direction != RIGHT) direction = LEFT;
                break;
            case RIGHT:
                if (direction != LEFT) direction = RIGHT;
                break;
            case SPACE:
                is_paused = !is_paused; //切换暂停状态
                if (is_paused) {
                    color(12);
                    CursorJump(COL - 8, ROW + 2);
                    printf("已暂停");
                }
                else {
                    color(10);
                    CursorJump(COL - 8, ROW + 2);
                    printf("      "); //清除暂停提示
                    last_time = clock(); //恢复计时
                }
                break;
            case F1:
                //加速，最低速度为100
                if (speed > 100) speed -= 50;
                color(7);
                CursorJump(COL - 10, ROW);
                printf("速度: %d", 1000 - speed);
                break;
            case F2:
                //减速，最高速度为500
                if (speed < 500) speed += 50;
                color(7);
                CursorJump(COL - 10, ROW);
                printf("速度: %d", 1000 - speed);
                break;
            case ESC:
                game_mode = 0; //退出游戏
                break;
            }
        }

        //倒计时更新
        if (!is_paused) {
            clock_t current_time = clock();
            if ((current_time - last_time) >= 1000) { //每秒更新一次
                remaining_time--;
                color(7);
                CursorJump(COL / 2 - 10, ROW);
                printf("剩余时间: %d秒", remaining_time);

                if (remaining_time <= 10) {
                    color(12); //最后10秒变红
                    CursorJump(COL / 2 - 10, ROW);
                    printf("剩余时间: %d秒", remaining_time);
                }

                if (remaining_time <= 0) {
                    game_mode = 0; //时间到，游戏结束
                    break;
                }
                last_time = current_time;
            }
        }

        //如果未暂停，则移动蛇
        if (!is_paused && game_mode == 4)
        {
            switch (direction) {
            case UP:
                MoveSnake(0, -1);
                break;
            case DOWN:
                MoveSnake(0, 1);
                break;
            case LEFT:
                MoveSnake(-1, 0);
                break;
            case RIGHT:
                MoveSnake(1, 0);
                break;
            }
        }

        //检查游戏是否结束
        if (game_mode == 0) break;

        Sleep(speed); //控制移动速度
    }
    //音乐结束
    mciSendString(TEXT("close music2"), NULL, 0, NULL);
    //游戏结束
    GameOver();
}

//双人模式主函数
void DoublePlayerMode()
{
    system("cls");
    system("title 贪吃蛇 - 双人模式");
    system("mode con cols=124 lines=32");//设置窗口大小

    HideCursor();//隐藏控制台中的光标
    ReadHighScore();//从文件中读取最高分数并将其存储于变量
    InitInterface();//初始化游戏界面
    GenerateFood();//随机生成食物的位置
    InitSnake1();//初始化蛇1(左侧)
    InitSnake2();//初始化蛇2(右侧)
    DrawSnake(); // 绘制蛇1
    DrawSnake2(); // 绘制蛇2

    int direction1 = RIGHT; //玩家1初始方向向右
    int direction2 = LEFT; //玩家2初始方向向左
    int is_paused = 0;     //暂停状态

    while (game_mode == 2)
    {
        //播放音乐
        mciSendString(TEXT("open \"C:\\Users\\lenovo\\Desktop\\游戏背景.mp3\" type mpegvideo alias music2"), NULL, 0, NULL);
        mciSendString(TEXT("play music2 repeat"), NULL, 0, NULL);
        //键盘控制
        if (_kbhit())
        {
            int key = _getch();
            switch (key)
            {
                // 玩家1 使用方向键
            case UP:
                if (direction1 != DOWN) direction1 = UP;//避免蛇瞬间反向撞到自己
                break;
            case DOWN:
                if (direction1 != UP) direction1 = DOWN;
                break;
            case LEFT:
                if (direction1 != RIGHT) direction1 = LEFT;
                break;
            case RIGHT:
                if (direction1 != LEFT) direction1 = RIGHT;
                break;
                // 玩家2 使用 wsad 键
            case 'w':
                if (direction2 != DOWN) direction2 = UP;
                break;
            case 's':
                if (direction2 != UP) direction2 = DOWN;
                break;
            case 'a':
                if (direction2 != RIGHT) direction2 = LEFT;
                break;
            case 'd':
                if (direction2 != LEFT) direction2 = RIGHT;
                break;
            case SPACE:
                is_paused = !is_paused; //切换暂停状态
                if (is_paused) {
                    color(12);
                    CursorJump(COL - 3, ROW + 2);
                    printf("已暂停");
                }
                else {
                    color(10);
                    CursorJump(COL - 3, ROW + 2);
                    printf("      "); //清除暂停提示
                }
                break;
            case F1:
                //加速，最低速度为100
                if (speed > 100) speed -= 50;
                color(7);
                CursorJump(COL - 10, ROW);
                printf("速度: %d", 1000 - speed);
                break;
            case F2:
                //减速，最高速度为500
                if (speed < 500) speed += 50;
                color(7);
                CursorJump(COL - 10, ROW);
                printf("速度: %d", 1000 - speed);
                break;
            case ESC:
                game_mode = 0; //退出游戏
                break;
            }
        }
        //如果未暂停，则移动两条蛇（先移动玩家1，再移动玩家2）
        if (!is_paused)
        {
            int alive1 = 1;
            int alive2 = 1;

            //玩家1移动
            switch (direction1)
            {
            case UP:
                MoveSnake(0, -1);
                break;
            case DOWN:
                MoveSnake(0, 1);
                break;
            case LEFT:
                MoveSnake(-1, 0);
                break;
            case RIGHT:
                MoveSnake(1, 0);
                break;
            }

            //玩家2移动
            switch (direction2)
            {
            case UP:
                MoveSnake2(0, -1);
                break;
            case DOWN:
                MoveSnake2(0, 1);
                break;
            case LEFT:
                MoveSnake2(-1, 0);
                break;
            case RIGHT:
                MoveSnake2(1, 0);
                break;
            }

            // 如果任意一方死亡，结束
            if (!alive1 || !alive2)
            {
                //游戏结束
                game_mode = 0;;
                break;
            }

            // 绘制两条蛇的新位置
            DrawSnake();
            DrawSnake2();

            // 更新底部分数显示
            color(7);
            CursorJump(0, ROW);
            printf("P1: %d", current_score);
            CursorJump(10, ROW);
            printf("P2: %d", current_score2);
        }
        Sleep(speed); //控制移动速度
    }
    //音乐结束
    mciSendString(TEXT("close music2"), NULL, 0, NULL);
    //游戏结束
    GameOver2();
}

//主函数（修改：添加新菜单选项处理）
int main() {
    srand((unsigned int)time(NULL)); //初始化随机数种子
    ReadRankList(); //初始化时读取排行榜

    while (1) {
        int choice = WelcomeMenu();
        //重置游戏状态
        current_score = 0;
        current_score2 = 0;
        speed = 300;
        difficulty = 1;
        time_limit = 60;

        switch (choice) {
        case 1:
            //单人普通模式
            game_mode = 1;
            mciSendString(TEXT("close music1"), NULL, 0, NULL);
            SinglePlayerMode();
            break;
        case 2:
            //双人模式
            game_mode = 2;
            mciSendString(TEXT("close music1"), NULL, 0, NULL);
            DoublePlayerMode();
            break;
        case 3:
            //ai对战
            system("cls");
            mciSendString(TEXT("close music1"), NULL, 0, NULL);
            system("title 贪吃蛇"); //设置cmd窗口的名字
            system("mode con cols=124 lines=30");;//设置控制台的宽高
            gameai();
            break;
        case 4:
            //单人限时模式
            game_mode = 4;
            mciSendString(TEXT("close music1"), NULL, 0, NULL);
            TimeLimitMode();
            break;
        case 5:
            //游戏说明
            GameInstructions();
            break;
        case 6:
            //查看排行榜
            ShowRankList();
            break;
        case 7:
            //退出游戏
            system("cls");
            color(14);
            CursorJump(COL, ROW / 2);
            printf("感谢游玩，再见!");
            Sleep(1000);
            exit(0);
        default:
            //无效选择
            color(12);
            CursorJump(37, 33);
            printf("请输入1-7之间的有效数字!");
            Sleep(1000);
        }
    }

    return 0;
}

void gameai() {
    //播放音乐
    mciSendString(TEXT("open \"C:\\Users\\lenovo\\Desktop\\游戏背景.mp3\" type mpegvideo alias music2"), NULL, 0, NULL);
    mciSendString(TEXT("play music2 repeat"), NULL, 0, NULL);
    HideCursor(); //隐藏光标

    system("cls");
    // 重置 AI 得分，避免上一局遗留分数
    ai_score = 0;
    InitInterfaceai(); //初始化界面
    InitSnake(); //初始化蛇
    // 初始化并启动第二条（AI）蛇
    InitializeCriticalSection(&g_cs);
    InitOtherSnakeai();
    srand((unsigned int)time(NULL)); //设置随机数生成起点
    RandFoodai(); //随机生成食物
    DrawSnake1ai(1); //打印玩家蛇
    DrawOtherSnakeai(1); //打印 AI 蛇
    // 启动 AI 线程（并行自动移动），记录句柄并设置运行标志
    ai_running = TRUE;
    g_aiThread = CreateThread(NULL, 0, AutoThreadProc, NULL, 0, NULL);

    // 启动玩家控制的主游戏循环
    Gameaizhu(); //开始游戏

}
void RandFoodai() { //rand是典型随机函数
    int i, j;
    do {
        //随机生成食物的横纵坐标
        i = rand() % ROW;
        j = rand() % COL;
    } while (face[i][j] != KONG); //确保生成食物的位置为空，若不为空则重新生成
    face[i][j] = FOOD; //将食物位置进行标记
    color(12); //颜色设置为红色
    CursorJump(2 * j, i); //光标跳转到生成的随机位置处
    printf("■"); //打印食物
}
void InitInterfaceai() {
    color(6); //颜色为土黄色
    for (int i = 0; i < ROW; i++) { //双层循环设计墙的位置
        for (int j = 0; j < COL; j++) {
            if (j == 0 || j == COL - 1) {
                face[i][j] = WALL; //标记该位置为墙
                CursorJump(2 * j, i); //一个方块占两个横坐标和一个纵坐标
                printf("■");//打印方块
            }
            else if (i == 0 || i == ROW - 1) { //首尾
                face[i][j] = WALL; //标记该位置为墙
                CursorJump(2 * j, i);
                printf("■");//打印方块
            }
            else {
                face[i][j] = KONG; //标记该位置为空

            }
        }
    }
    color(7); //颜色设置为白色
    CursorJump(0, ROW);
    printf("当前得分:%d", grade);
    CursorJump(COL, ROW);
    printf("历史最高得分:%d", max);
    // AI 得分显示
    CursorJump(20, ROW);
    printf("AI得分:%d", ai_score);
}
void JudgeFuncai(int x, int y) {

    //若蛇头即将到达的位置是食物，则得分
    if (face[snake.y + y][snake.x + x] == FOOD)
    {
        shijian -= 100;
        //播放音乐     
        mciSendString(TEXT("open \"C:\\Users\\lenovo\\Desktop\\吃东西.mp3\" type mpegvideo alias music3"), NULL, 0, NULL);
        mciSendString(TEXT("play music3"), NULL, 0, NULL);
        Sleep(500);
        mciSendString(TEXT("close music3"), NULL, 0, NULL);
        snake.len++; //蛇身加长
        grade += fen[1]; //更新当前得分
        color(7); //颜色设置为白色
        CursorJump(0, ROW);
        printf("当前得分:%d", grade); //重新打印当前得分
        RandFoodai(); //重新随机生成食物
    }
    //若蛇头即将到达的位置是墙或者蛇身，则游戏结束
    else if (face[snake.y + y][snake.x + x] == WALL || face[snake.y + y][snake.x + x] == BODY)
    {
        //音乐结束
        mciSendString(TEXT("close music2"), NULL, 0, NULL);
        ai_running = FALSE;
        //播放音乐     
        mciSendString(TEXT("open \"C:\\Users\\hp\\Desktop\\失败.mp3\" type mpegvideo alias music4"), NULL, 0, NULL);
        mciSendString(TEXT("play music4"), NULL, 0, NULL);
        Sleep(1000);
        mciSendString(TEXT("close music4"), NULL, 0, NULL);
        Sleep(1000); //留给玩家反应时间
        system("cls"); //清空屏幕
        // 停止 AI 线程并清除 AI 蛇和食物标记（使它们在屏幕上消失）
        if (g_aiThread) {
            WaitForSingleObject(g_aiThread, 500);
            CloseHandle(g_aiThread);
            g_aiThread = NULL;
        }
        EnterCriticalSection(&g_cs);
        for (int _i = 0; _i < ROW; ++_i) {
            for (int _j = 0; _j < COL; ++_j) {
                if (face[_i][_j] == FOOD || face[_i][_j] == HEAD2 || face[_i][_j] == BODY2) {
                    face[_i][_j] = KONG;
                    CursorJump(2 * _j, _i);
                    printf("  ");
                }
            }
        }
        LeaveCriticalSection(&g_cs);
        color(7); //颜色设置为白色
        // 在结束画面显示玩家与 AI 的分数，并给出胜负提示
        CursorJump(2 * (COL / 3), ROW / 2 - 1);
        printf("你的得分:%d   AI得分:%d", grade, ai_score);
        CursorJump(2 * (COL / 3), ROW / 2 - 0);
        if (grade > ai_score) {
            printf("  你获胜！");
        }
        else if (grade == ai_score) {
            printf("  平局！");
        }
        else {
            printf("  AI获胜！");
        }
        grade = 0;
        while (1) { //询问玩家是否再来一局
            char ch;
            CursorJump(2 * (COL / 3), ROW / 2 + 3);
            printf("(回到主界面请按下r)");
            CursorJump(2 * (COL / 3), ROW / 2 + 5);
            printf("要再耍一把么?(y/n):");
            scanf("%c", &ch);
            if (ch == 'y' || ch == 'Y') {
                system("cls");
                getchar();
                gameai();
            }
            else if (ch == 'r' || ch == 'R') {
                system("cls");
                getchar();
                main();
            }
            else if (ch == 'n' || ch == 'N') {
                CursorJump(2 * (COL / 3), ROW / 2 + 7);
                exit(0);
            }
            else {
                CursorJump(2 * (COL / 3), ROW / 2 + 5);
                printf("选择错误，请再次选择");
            }
        }
    }
}
void run(int x, int y) {
    int t = 0;
    while (1) {
        if (t == 0 && fen[1] == 1)
            t = shijian; //这里t越小，蛇移动速度越快（可以根据次设置游戏难度）
        else if (fen[1] > fen[0])
            t = shijian - (fen[1] - fen[0]) * 100;
        if (fen[1] < fen[0])
            t = shijian + (fen[0] - fen[1]) * 100;
        /*if (t == 0)
            t = 3500;*/ //这里t越小，蛇移动速度越快（可以根据次设置游戏难度）
        while (--t) {
            if (_kbhit() != 0) //若键盘被敲击，则退出循环
                break;
        }
        if (t == 0) { //键盘未被敲击
            JudgeFuncai(x, y); //判断到达该位置后，是否得分与游戏结束
            MoveSnake1ai(x, y); //移动蛇
        }
        else { //键盘被敲击
            break; //返回Game函数读取键值
        }
    }
}
//游戏主体逻辑函数
void Gameaizhu() {
    int n = RIGHT; //开始游戏时，默认向右移动
    int tmp = 0; //记录蛇的移动方向
    goto first; //第一次进入循环先向默认方向前进
    while (1) {
        n = _getch(); //读取键值
        //在执行前，需要对所读取的按键进行调整
        switch (n) {
        case UP:
        case DOWN: //如果敲击的是“上”或“下”
            if (tmp != LEFT && tmp != RIGHT) { //并且上一次蛇的移动方向不是“左”或“右”
                n = tmp; //那么下一次蛇的移动方向设置为上一次蛇的移动方向
            }
            break;
        case LEFT:
        case RIGHT: //如果敲击的是“左”或“右”
            if (tmp != UP && tmp != DOWN) { //并且上一次蛇的移动方向不是“上”或“下”
                n = tmp; //那么下一次蛇的移动方向设置为上一次蛇的移动方向
            }


        case SPACE:
        case ESC:
        case 'r':
        case 'R':
            break; //这四个无需调整
        default:
            n = tmp; //其他键无效，默认为上一次蛇移动的方向
            break;
        }
    first: //第一次进入循环先向默认方向前进
        switch (n) {
        case UP: //方向键：上
            run(0, -1); //向上移动
            tmp = UP; //记录当前蛇的移动方向
            break;
        case DOWN: //方向键：下
            run(0, 1); //向下移动
            tmp = DOWN; //记录当前蛇的移动方向
            break;
        case LEFT: //方向键：左
            run(-1, 0); //向左移动
            tmp = LEFT; //记录当前蛇的移动方向
            break;
        case RIGHT: //方向键：右
            run(1, 0); //向右移动
            tmp = RIGHT; //记录当前蛇的移动方向
            break;
        case 'S':
        case 's':
            t = shijian - (fen[1] - fen[0]) * 100;
            break;
        case 'd':
        case 'D':
            t = shijian + (fen[0] - fen[1]) * 100;
            break;
        case SPACE: //暂停
            // 按空格：切换 AI 蛇的运行/暂停状态
            ToggleAI();
            break;
        case ESC: //退出
            system("cls"); //清空屏幕
            color(7); //颜色设置为白色
            CursorJump(COL - 8, ROW / 2);
            printf("  游戏结束  ");
            CursorJump(COL - 8, ROW / 2 + 2);
            exit(0);
        case 'r':
        case 'R': //重新开始
            system("cls"); //清空屏幕
            main(); //重新执行主函数
        }
    }
}

//打印和行走的蛇
void DrawSnake1ai(int flag) {
    if (flag == 1) { //打印蛇
        color(14); //设置为黄色五角星
        CursorJump(2 * snake.x, snake.y);
        printf("★"); //打印蛇头
        for (int i = 0; i < snake.len; i++) {
            color(2);//设置绿色
            CursorJump(2 * body[i].x, body[i].y);
            printf("◆"); //打印蛇身
        }
    }
    else { //覆盖蛇
        if (body[snake.len - 1].x != 0) { //防止len++后将(0, 0)位置的墙覆盖
            //将蛇尾覆盖为空格即可
            CursorJump(2 * body[snake.len - 1].x, body[snake.len - 1].y);
            printf("  ");
        }
    }
}

//移动ai游戏的玩家蛇
void MoveSnake1ai(int x, int y) {
    EnterCriticalSection(&g_cs);
    DrawSnake1ai(0); //先覆盖当前所显示的蛇
    face[body[snake.len - 1].y][body[snake.len - 1].x] = KONG; //蛇移动后蛇尾重新标记为空
    face[snake.y][snake.x] = BODY; //蛇移动后蛇头的位置变为蛇身
    //蛇移动后各个蛇身位置坐标需要更新
    for (int i = snake.len - 1; i > 0; i--) {
        body[i].x = body[i - 1].x;
        body[i].y = body[i - 1].y;
    }
    //蛇移动后蛇头位置变为第0个蛇身的位置
    body[0].x = snake.x;
    body[0].y = snake.y;
    //蛇头的位置更改
    snake.x = snake.x + x;
    snake.y = snake.y + y;
    DrawSnake1ai(1); //打印移动后的蛇
    LeaveCriticalSection(&g_cs);
}

// === 第二条（AI）蛇的实现 ===
struct Snake snakeai;
struct Body bodyai[ROW * COL];

void InitOtherSnakeai()
{
    snakeai.len = 4;
    snakeai.x = 2;
    snakeai.y = 2;
    for (int i = 0; i < snakeai.len; ++i) {
        bodyai[i].x = snakeai.x - 1 - i;
        if (bodyai[i].x < 1) bodyai[i].x = 1 + i;
        bodyai[i].y = snakeai.y;
    }
    EnterCriticalSection(&g_cs);
    face[snakeai.y][snakeai.x] = HEAD2;
    for (int i = 0; i < snakeai.len; ++i) face[bodyai[i].y][bodyai[i].x] = BODY2;
    LeaveCriticalSection(&g_cs);
}

void DrawOtherSnakeai(int flag)
{
    if (flag == 1) {
        color(14);
        CursorJump(2 * snakeai.x, snakeai.y);
        printf("■");
        for (int i = 0; i < snakeai.len; ++i) {
            color(2);
            CursorJump(2 * bodyai[i].x, bodyai[i].y);
            printf("◆");
        }
    }
    else {
        if (bodyai[snakeai.len - 1].x != 0) {
            CursorJump(2 * bodyai[snakeai.len - 1].x, bodyai[snakeai.len - 1].y);
            printf("  ");
        }
    }
}

void MoveOtherSnakeai(int x, int y)
{
    EnterCriticalSection(&g_cs);
    DrawOtherSnakeai(0);
    face[bodyai[snakeai.len - 1].y][bodyai[snakeai.len - 1].x] = KONG;
    face[snakeai.y][snakeai.x] = BODY2;
    for (int i = snakeai.len - 1; i > 0; --i) {
        bodyai[i].x = bodyai[i - 1].x;
        bodyai[i].y = bodyai[i - 1].y;
    }
    bodyai[0].x = snakeai.x;
    bodyai[0].y = snakeai.y;
    snakeai.x += x;
    snakeai.y += y;
    face[snakeai.y][snakeai.x] = HEAD2;
    DrawOtherSnakeai(1);
    LeaveCriticalSection(&g_cs);
}

void AutoMoveOneStep2()
{
    int tx = -1, ty = -1;
    int bestd = INT_MAX;
    for (int i = 0; i < ROW; ++i) for (int j = 0; j < COL; ++j) {
        if (face[i][j] == FOOD) {
            int d = abs(i - snakeai.y) + abs(j - snakeai.x);
            if (d < bestd) { bestd = d; ty = i; tx = j; }
        }
    }
    if (tx == -1 || ty == -1) return;
    int prevx = bodyai[0].x, prevy = bodyai[0].y;
    int curdx = snakeai.x - prevx, curdy = snakeai.y - prevy;
    int dx = tx - snakeai.x, dy = ty - snakeai.y;
    int cand[4][2]; int candn = 0;
    if (abs(dx) >= abs(dy)) {
        if (dx > 0) { cand[candn][0] = 1; cand[candn][1] = 0; candn++; }
        else if (dx < 0) { cand[candn][0] = -1; cand[candn][1] = 0; candn++; }
        if (dy > 0) { cand[candn][0] = 0; cand[candn][1] = 1; candn++; }
        else if (dy < 0) { cand[candn][0] = 0; cand[candn][1] = -1; candn++; }
    }
    else {
        if (dy > 0) { cand[candn][0] = 0; cand[candn][1] = 1; candn++; }
        else if (dy < 0) { cand[candn][0] = 0; cand[candn][1] = -1; candn++; }
        if (dx > 0) { cand[candn][0] = 1; cand[candn][1] = 0; candn++; }
        else if (dx < 0) { cand[candn][0] = -1; cand[candn][1] = 0; candn++; }
    }
    if (candn < 4) {
        int extra[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };
        for (int k = 0; k < 4 && candn < 4; ++k) {
            int ex = extra[k][0], ey = extra[k][1], found = 0;
            for (int p = 0; p < candn; ++p) if (cand[p][0] == ex && cand[p][1] == ey) found = 1;
            if (!found) { cand[candn][0] = ex; cand[candn][1] = ey; candn++; }
        }
    }
    int chosen_dx = 0, chosen_dy = 0, found = 0;
    for (int i = 0; i < candn; ++i) {
        int cx = cand[i][0], cy = cand[i][1];
        if (curdx == -cx && curdy == -cy) continue;
        int nx = snakeai.x + cx, ny = snakeai.y + cy;
        if (nx < 0 || nx >= COL || ny < 0 || ny >= ROW) continue;
        if (face[ny][nx] == BODY2) continue;
        if (face[ny][nx] == WALL) continue;
        chosen_dx = cx; chosen_dy = cy; found = 1; break;
    }
    if (!found) {
        int alt[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };
        for (int i = 0; i < 4; ++i) {
            int cx = alt[i][0], cy = alt[i][1];
            if (curdx == -cx && curdy == -cy) continue;
            int nx = snakeai.x + cx, ny = snakeai.y + cy;
            if (nx < 0 || nx >= COL || ny < 0 || ny >= ROW) continue;
            if (face[ny][nx] == BODY2) continue;
            if (face[ny][nx] == WALL) continue;
            chosen_dx = cx; chosen_dy = cy; found = 1; break;
        }
    }
    if (!found) { chosen_dx = curdx; chosen_dy = curdy; }

    int nx = snakeai.x + chosen_dx, ny = snakeai.y + chosen_dy;
    if (nx >= 0 && nx < COL && ny >= 0 && ny < ROW && face[ny][nx] == FOOD) {
        //播放音乐     
        mciSendString(TEXT("open \"C:\\Users\\lenovo\\Desktop\\吃东西.mp3\" type mpegvideo alias music3"), NULL, 0, NULL);
        mciSendString(TEXT("play music3"), NULL, 0, NULL);
        Sleep(500);
        mciSendString(TEXT("close music3"), NULL, 0, NULL);
        snakeai.len++;
        // AI 吃到食物：增加 AI 得分并更新界面
        ai_score++;
        EnterCriticalSection(&g_cs);
        CursorJump(20, ROW);
        color(13);
        printf("AI得分:%d", ai_score);
        color(7);
        LeaveCriticalSection(&g_cs);
        RandFoodai();
    }
    MoveOtherSnakeai(chosen_dx, chosen_dy);
}

DWORD WINAPI AutoThreadProc(LPVOID lpParam)
{
    (void)lpParam;
    while (ai_running) {
        AutoMoveOneStep2();
        Sleep(250);
    }
    return 0;
}

// 切换 AI 运行状态（由空格键触发）
void ToggleAI()
{
    EnterCriticalSection(&g_cs);
    if (ai_running) {
        // 停止 AI
        ai_running = FALSE;
        if (g_aiThread) {
            WaitForSingleObject(g_aiThread, 500);
            CloseHandle(g_aiThread);
            g_aiThread = NULL;
        }
        // 更新状态显示
        CursorJump(30, ROW);
        color(12);
        printf("AI: 暂停   ");
        color(7);
    }
    else {
        // 启动 AI
        ai_running = TRUE;
        g_aiThread = CreateThread(NULL, 0, AutoThreadProc, NULL, 0, NULL);
        CursorJump(30, ROW);
        color(10);
        printf("AI: 运行   ");
        color(7);
    }
    LeaveCriticalSection(&g_cs);
}