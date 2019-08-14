/*
	Snake Renewal 新版贪吃蛇
	作者：Umbrella丶2
	项目开始时间：2019-7-25 9:00:00
	项目结束时间：2019-7-25 17:53:30
	运用技术：
	1.双缓冲，解决控制台闪烁问题
	2.文件I/O读写，实现排行榜数据的读取与储存。
	3.基础排序算法，冒泡。
	4.有限状态机，处理游戏状态的转换。
	命名规范，常量均大写+下划线，全局变量使用CamelCase，局部变量使用camelCase。
	借鉴资料：
	双缓冲：https://blog.csdn.net/weixinhum/article/details/72179593
	控制台窗口居中：https://stackoverflow.com/questions/51344985/how-to-center-output-console-window-in-c
	拒绝匈牙利命名法/呲牙
	这是我用来复习C随便写的一个小项目，供参考，应该还有很多可以优化的地方/呲牙。
	I/O部分写的时间最久，4小时，坑估计都被我踩完了/呲牙。
	C#是世界上最好的语言/呲牙。
*/

#include "stdio.h"
#include "stdlib.h"
#include "io.h"
#include <conio.h>
#include <Windows.h>
#include <winuser.h>
#include "wincon.h"
#include <time.h>

// 定义窗口宽高
#define SCREEN_WIDTH 60
#define SCREEN_HEIGHT 35
#define GAME_WIDTH 59
#define GAME_HEIGHT 34
// 定义按键常量
#define KEY_ENTER 13
#define KEY_ESCAPE 27
#define KEY_BACKSPACE 8
#define KEY_UP 72
#define KEY_DOWN 80
#define KEY_LEFT 75
#define KEY_RIGHT 77
// 定义游戏常量
#define RANKING_SCORE_NUM 10 // 排行榜能够记录的分数数量
#define SCORE_MAXIMUM 9999 // 最大分数
#define GAME_SPEED 60 // 限制游戏的刷新频率，这里我默认为60FPS，即1秒刷新60次 
#define MOVEMENT_INTERVAL 100 // 游戏刷新周期，毫秒
#define FOOD_SCORE 50 // 食物的分数

HANDLE ScreenBufferPrev, ScreenBufferPost;
HANDLE* ScreenOutput;
short ScreenBufferState = 0;

COORD Coord;
DWORD Bytes = 0;

// [y][x]
unsigned char Data[SCREEN_HEIGHT][SCREEN_WIDTH];

double intervalCount = 0;

// 定义游戏枚举
// 填充字符的方向
enum DrawLineDirection
{
	Horizontal = 1, // 水平
	Vertical // 竖直
};

enum GameState
{
	Title = 1,
	Menu,
	InGame,
	GameOver,
	Ranking
};
// 当前游戏状态
enum GameState CurGameState = Title;

// 游戏变量
// 菜单
short MenuIndex = 0;
// 游戏
int SnakeLength = 1; // 身体长度
short SnakeDir = 0; // 1 left 2 right 3 up 4 down 下一次移动的方向
COORD SnakePosition[GAME_WIDTH * GAME_HEIGHT]; // 蛇头+身的位置
COORD FoodPosition = {0, 0}; // 食物的位置
COORD PrevTailPosition = {0, 0}; // 蛇尾上一次所在的位置
int CurGameScore = 0; // 当前游戏分数
short IsGameOver = 0; // 是否游戏结束

// 排行榜
int RankingScores[RANKING_SCORE_NUM];

void ScreenDraw(); // 总绘制函数
void GameLogic(); // 总游戏逻辑
// 以下用了点OOP思想
// 绘制部分
void Screen_DrawTitle(); // 绘制标题画面
void Screen_DrawMenu(); // 绘制菜单
void Screen_DrawRanking(); // 绘制排行榜
void Screen_DrawGame(); // 绘制游戏
void Screen_DrawGameOver(); // 绘制游戏结束画面
// 文件类
void File_WriteScoreByDelimiter(int scores[], const char* filePath, char delimiter); // 输出分数到指定文件，用delimiter进行分数分割
void File_ReadScoreByDelimiter(const char* filePath, int* scores, const char delimiter); // 从指定文件读入分数到指定数组，用delimiter进行读取分割
// 工具类
void Utils_MoveCenter(); // 将窗口居中
void Utils_DrawWindowBorder(); // 绘制游戏边框
void Utils_DrawWindow(COORD windowPosition, COORD size); // 绘制游戏子窗口
void Utils_FillCharInLine(enum DrawLineDirection direction, COORD startPosition, short drawLength, unsigned char drawChar); // 用drawChar在startCoord位置朝着direction方向填充，长度为drawLength
void Utils_SetText(COORD textPosition, const char* text); // 在textPosition位置填写text，填充顺序为从左往右
void Utils_ResetContent(); // 重置用于显示的字符数组
void Utils_DoubleBuffer(); // 双缓冲
void Utils_BubbleSort(int* numArray, size_t numCount);// 冒泡排序，可能会换成快排
void Utils_Swap(int* a, int* b); // 交换函数，使用位运算实现 
// 游戏类
void Game_InitGameData(); // 初始化游戏数据
void Game_SaveScore(int score); // 储存分数
void Game_WorldBehave(); // 处理移动逻辑 
void Game_SpawnFood(); // 生成一个新的食物 
void Game_AddNewTail(); // 使蛇的长度增加1 
void Game_MoveBodies(); // 移动蛇身，我们的逻辑顺序是，先处理蛇头的移动，然后再处理蛇身的移动 
// 核心部分
void Core_DoubleBufferInitialize(); // 初始化双缓冲 

int main(int argc, char* argv[])
{
	Game_InitGameData();
	Core_DoubleBufferInitialize();
	for (;;)
	{
		// 这里我采用的是先处理游戏逻辑再将结果画到控制台上 
		GameLogic();
		ScreenDraw();
	}
	return 0;
}

void Core_DoubleBufferInitialize()
{
	// 设置控制台窗口大小
	system("mode con cols=60 lines=35"); // 设置控制台的大小，这里我设置成60列35行 
	SetConsoleOutputCP(437); // 改变控制台使用的字符集，使其能够使用我们用来画边框的字符·但改变后我们无法通过控制台输出中文字符。 
	SetConsoleTitle("Snake"); // 设置控制台窗口的标题 
	// 初始化缓冲
	ScreenBufferPrev = CreateConsoleScreenBuffer(GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	ScreenBufferPost = CreateConsoleScreenBuffer(GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	CONSOLE_CURSOR_INFO cursorInfo;
	cursorInfo.bVisible = 0;
	cursorInfo.dwSize = 1;
	SetConsoleCursorInfo(ScreenBufferPrev, &cursorInfo);
	SetConsoleCursorInfo(ScreenBufferPost, &cursorInfo);
	// 设置缓冲区的大小
	COORD size = {SCREEN_WIDTH, SCREEN_HEIGHT};
	SetConsoleScreenBufferSize(ScreenBufferPrev, size);
	SetConsoleScreenBufferSize(ScreenBufferPost, size);
	// 让控制台居中
	Utils_MoveCenter();
}

void ScreenDraw()
{
	Utils_ResetContent();
	switch (CurGameState) // 处理当前游戏状态 
	{
		case Title:
			Screen_DrawTitle();
			break;
		case Menu:
			Screen_DrawMenu();
			break;
		case InGame:
			Screen_DrawGame();
			break;
		case Ranking:
			Screen_DrawRanking();
			break;
		case GameOver:
			Screen_DrawGameOver();
			break;
	}
	Utils_DoubleBuffer();
}

void GameLogic()
{
	if (kbhit()) // 按键检测，只有当任何按键被按下后才会响应 
	{
		char key = _getch(); // 取得当前按下的按键 
		if (key == KEY_ESCAPE)
		{
			exit(0);
			return;
		}
		switch (CurGameState)
		{
			case Title:
				if (key == KEY_ENTER)
				{
					CurGameState = Menu;
				}
				break;
			case Menu:
				if (key == KEY_UP)
				{
					MenuIndex--;
					if (MenuIndex < 0)
					{
						MenuIndex = 1;
					}
				}
				if (key == KEY_DOWN)
				{
					MenuIndex++;
					if (MenuIndex > 1)
					{
						MenuIndex = 0;
					}
				}
				if (key == KEY_ENTER)
				{
					switch (MenuIndex)
					{
						case 0:// 开始游戏
							CurGameState = InGame;
							SnakePosition[0].X = 15;
							SnakePosition[0].Y = 15;
							SnakeDir = 0;
							SnakeLength = 1;
							IsGameOver = 0;
							Game_SpawnFood();
							break;
						case 1:// 排行榜
							CurGameState = Ranking;
							break;
					}
				}
				break;
			case InGame:
				if (!IsGameOver)
				{
					if (key == KEY_UP)
					{
						SnakeDir = 3;
					}
					if (key == KEY_DOWN)
					{
						SnakeDir = 4;
					}
					if (key == KEY_LEFT)
					{
						SnakeDir = 1;
					}
					if (key == KEY_RIGHT)
					{
						SnakeDir = 2;
					}
				}
				break;
			case Ranking:
				if (key == KEY_BACKSPACE)
				{
					CurGameState = Menu;
				}
				break;
			case GameOver:
				if (key == KEY_BACKSPACE)
				{
					Game_SaveScore(CurGameScore);
					CurGameScore = 0;
					CurGameState = Ranking;
				}
				break;
		}
	}

	if (CurGameState == InGame)
	{
		Game_WorldBehave();
		Sleep(MOVEMENT_INTERVAL);
	}
}

void Screen_DrawTitle()
{
	Utils_DrawWindowBorder();
	COORD drawCoord;
	drawCoord.X = SCREEN_WIDTH / 2 - 10 - 1;
	drawCoord.Y = SCREEN_HEIGHT - 2;
	Utils_SetText(drawCoord, "Press enter to start.");
}

void Screen_DrawMenu()
{
	Utils_DrawWindowBorder();
	COORD drawCoord;
	drawCoord.X = SCREEN_WIDTH / 2 - 4 - 1;
	drawCoord.Y = 8;
	Utils_SetText(drawCoord, "* MENU *");
	drawCoord.X -= 2;
	drawCoord.Y = 12;
	if (MenuIndex == 0)
	{
		Utils_SetText(drawCoord, "* START GAME");
		drawCoord.Y++;
		Utils_SetText(drawCoord, "  RANKING");
	}
	else
	{
		Utils_SetText(drawCoord, "  START GAME");
		drawCoord.Y++;
		Utils_SetText(drawCoord, "* RANKING");
	}
}

void Screen_DrawGame()
{
	Data[SnakePosition[0].Y][SnakePosition[0].X] = '@';
	for (int i = 1; i < SnakeLength; i++)
	{
		Data[SnakePosition[i].Y][SnakePosition[i].X] = '$';
	}
	Data[FoodPosition.Y][FoodPosition.X] = '*';
	Utils_DrawWindowBorder();
}

void Screen_DrawRanking()
{
	Utils_DrawWindowBorder();
	COORD drawCoord = { SCREEN_WIDTH / 2 - 5 - 1, 8};
	Utils_SetText(drawCoord, "* RANKING *");
	drawCoord.X -= 4;
	drawCoord.Y += 2;
	COORD size = {20, 15};
	Utils_DrawWindow(drawCoord, size);
	drawCoord.X += 4;
	drawCoord.Y += 1;
	Utils_SetText(drawCoord, "No.     Score");
	for (int i = 0; i < RANKING_SCORE_NUM; i++)
	{
		char temp[4];
		drawCoord.Y += 1;
		Utils_SetText(drawCoord, itoa(i + 1, temp, 10));
		drawCoord.X += 8;
		Utils_SetText(drawCoord, itoa(RankingScores[i], temp, 10));
		drawCoord.X -= 8;
	}
	drawCoord.X = SCREEN_WIDTH / 2 - 11;
	drawCoord.Y = SCREEN_HEIGHT - 2;
	Utils_SetText(drawCoord, "Press Backspace to back");
}

void Screen_DrawGameOver()
{
	Utils_DrawWindowBorder();
	COORD drawCoord = {15, 15};
	Utils_SetText(drawCoord, "GAME OVER");
	drawCoord.Y += 5;
	Utils_SetText(drawCoord, "Press Backspace to see ranking.");
}

void Game_InitGameData()
{
	for (int i = 0; i < 10; i++)
	{
		RankingScores[i] = 0;
	}
	if (access("scores.dat", 0) == 0)
	{
		File_ReadScoreByDelimiter("scores.dat", RankingScores, '#');
	}
}

void Game_SaveScore(int score)
{
	int tempArray[RANKING_SCORE_NUM + 1];
	for (int i = 0; i < RANKING_SCORE_NUM + 1; i++)
	{
		tempArray[i] = RankingScores[i];
	}
	tempArray[RANKING_SCORE_NUM] = score;
	Utils_BubbleSort(tempArray, RANKING_SCORE_NUM + 1);
	for (int i = 0; i < RANKING_SCORE_NUM; i++)
	{
		RankingScores[i] = tempArray[RANKING_SCORE_NUM - i];
	}
	File_WriteScoreByDelimiter(RankingScores, "scores.dat", '#');
}

void Game_WorldBehave()
{
	Game_MoveBodies();
	switch (SnakeDir)
	{
		case 1:
			SnakePosition[0].X--;
			if (SnakePosition[0].X <= 0)
			{
				CurGameState = GameOver;
				IsGameOver = 1;
			}
			break;
		case 2:
			SnakePosition[0].X++;
			if (SnakePosition[0].X >= GAME_WIDTH)
			{
				CurGameState = GameOver;
				IsGameOver = 1;
			}
			break;
		case 3:
			SnakePosition[0].Y--;
			if (SnakePosition[0].Y <= 0)
			{
				CurGameState = GameOver;
				IsGameOver = 1;
			}
			break;
		case 4:
			SnakePosition[0].Y++;
			if (SnakePosition[0].Y >= GAME_HEIGHT)
			{
				CurGameState = GameOver;
				IsGameOver = 1;
			}
			break;
	}
	
	if (Data[SnakePosition[0].Y][SnakePosition[0].X] == '*')
	{
		CurGameScore += FOOD_SCORE;
		Game_MoveBodies();
		Game_AddNewTail();
		Game_SpawnFood();
	}
}

void Game_SpawnFood()
{
	BOOL checked = FALSE;
	while (!checked)
	{
		srand((unsigned)time(NULL));// 重置随机数种子 
		FoodPosition.X = 2 + rand() % (GAME_WIDTH - 5);
		FoodPosition.Y = 2 + rand() % (GAME_HEIGHT - 5);
		if (Data[FoodPosition.Y][FoodPosition.X] != '@' && Data[FoodPosition.Y][FoodPosition.X] != '$')
		{
			checked = TRUE;
		}
	}
}

void Game_AddNewTail()
{
	SnakeLength++;
	SnakePosition[SnakeLength - 1].X = PrevTailPosition.X;
	SnakePosition[SnakeLength - 1].Y = PrevTailPosition.Y;
}

void Game_MoveBodies()
{
	PrevTailPosition.X = SnakePosition[SnakeLength - 1].X;
	PrevTailPosition.Y = SnakePosition[SnakeLength - 1].Y;
	for (int i = SnakeLength - 1; i > 0; i--)
	{
		SnakePosition[i].X = SnakePosition[i - 1].X;
		SnakePosition[i].Y = SnakePosition[i - 1].Y;
	}
}

void File_WriteScoreByDelimiter(int scores[], const char* filePath, char delimiter)
{
	FILE* filePtr = fopen(filePath, "w");
	char output[55];
	short index = 0;
	for (int i = 0; i < RANKING_SCORE_NUM; i++)
	{
		char* scoreStr;
		scoreStr = (char*)malloc(5);
		itoa(scores[i], scoreStr, 10);
		strcat(scoreStr, &delimiter);
		if (index == 0)
		{
			strcpy(output, scoreStr);
			index = 1;
		}
		else
		{
			strcat(output, scoreStr);
		}
		free(scoreStr);
	}
	fputs(output, filePtr);
	fclose(filePtr);
}

void File_ReadScoreByDelimiter(const char* filePath, int* scores, const char delimiter)
{
	FILE* filePtr = fopen(filePath, "r");
	unsigned short readCount = 0;
	char* score;
	score = (char*)malloc(8);
	short index = 0;
	for (int i = 0; i < 255; i++)
	{
		char letter;
		letter = fgetc(filePtr);
		if (letter == -1)
		{
			break;
		}
		if (letter == delimiter)
		{
			score[++index] = '\0';
			scores[readCount++] = atoi(score);
			index = 0;
		}
		else
		{
			score[index++] = letter;
		}
	}
	free(score);
	fclose(filePtr);
}

void Utils_DrawWindowBorder()
{
	COORD pos = {0, 0};
	COORD size = {SCREEN_WIDTH, SCREEN_HEIGHT};
	Utils_DrawWindow(pos, size);
}

void Utils_DrawWindow(COORD windowPosition, COORD size)
{
	COORD tempPos = windowPosition;
	// 绘制顶部
	Data[tempPos.Y][tempPos.X] = 201;
	tempPos.X++;
	enum DrawLineDirection drawDirection = Horizontal;
	Utils_FillCharInLine(drawDirection, tempPos, size.X - 2, 205);// 顶部横线
	tempPos.Y = windowPosition.Y + size.Y - 1;
	Utils_FillCharInLine(drawDirection, tempPos, size.X - 2, 205);// 底部横线
	Data[windowPosition.Y][windowPosition.X + size.X - 1] = 187;
	// 绘制中间
	tempPos.Y = windowPosition.Y + 1;
	for (int i = 0; i < 2; i++)
	{
		tempPos.X = windowPosition.X + (size.X - 1) * i;
		drawDirection = Vertical;
		Utils_FillCharInLine(drawDirection, tempPos, size.Y - 2, 186);
	}
	// 绘制底部
	Data[windowPosition.Y + size.Y - 1][windowPosition.X] = 200;
	Data[windowPosition.Y + size.Y - 1][windowPosition.X + size.X - 1] = 188;
}

void Utils_FillCharInLine(enum DrawLineDirection direction, COORD startPosition, short drawLength, unsigned char drawChar)
{
	for (int i = 0; i < drawLength; i++)
	{
		switch (direction)
		{
			case Horizontal:
				Data[startPosition.Y][startPosition.X + i] = drawChar;
				break;
			case Vertical:
				Data[startPosition.Y + i][startPosition.X] = drawChar;
				break;
		}
	}
}

void Utils_SetText(COORD textPosition, const char* text)
{
	for (int i = 0; i < strlen(text); i++)
	{
		Data[textPosition.Y][textPosition.X + i] = text[i];
	}
}

void Utils_ResetContent()
{
	for (int i = 0; i < SCREEN_WIDTH; i++)
	{
		for (int j = 0; j < SCREEN_HEIGHT; j++)
		{
			Data[j][i] = ' ';
		}
	}
}

void Utils_DoubleBuffer()
{
	ScreenBufferState = !ScreenBufferState;
	if (ScreenBufferState)
	{
		ScreenOutput = &ScreenBufferPrev;
	}
	else
	{
		ScreenOutput = &ScreenBufferPost;
	}
	for (int i = 0; i < SCREEN_HEIGHT; i++)
	{
		Coord.Y = i;
		WriteConsoleOutputCharacter(*ScreenOutput, (char*)Data[i], SCREEN_WIDTH, Coord, &Bytes);
	}
	SetConsoleActiveScreenBuffer(*ScreenOutput);
}

void Utils_BubbleSort(int* numArray, size_t numCount)
{
	for (int i = 0; i < numCount - 1; i++)
	{
		for (int j = 0; j < numCount - i - 1; j++)
		{
			if (numArray[j] > numArray[j + 1])
			{
				Utils_Swap(&numArray[j], &numArray[j + 1]);
			}
		}
	}
}

void Utils_Swap(int* a, int* b)
{
	*a = *a ^ *b;
	*b = *a ^ *b;
	*a = *a ^ *b;
}

//extern "C" WINBASEAPI HWND WINAPI GetConsoleWindow ();
void Utils_MoveCenter()
{
	RECT rectClient, rectWindow;
	HWND handle = GetConsoleWindow();
	GetClientRect(handle, &rectClient);
	GetWindowRect(handle, &rectWindow);
	int posx = GetSystemMetrics(SM_CXSCREEN) / 2 - (rectWindow.right - rectWindow.left) / 2;
	int posy = GetSystemMetrics(SM_CYSCREEN) / 2 - (rectWindow.bottom - rectWindow.top) / 2;
	MoveWindow(handle, posx, posy, rectClient.right - rectClient.left, rectClient.bottom - rectClient.top, TRUE);
}
