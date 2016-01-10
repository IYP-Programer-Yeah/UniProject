#include "Genio.h"
#include <time.h>

#undef main

#define Start_Menu		1
#define Play_Mode		2
#define Pause_Menu		3
#define Won_Menu		4
#define Lost_Menu		5
#define Player_Menu		6
#define Clear_For_Exit	7

#define Slope			1
#define Cliff			2
#define Road			3

static int mouseX, mouseY, Event;
static int GameState = Start_Menu;

struct Animation
{
	G_Texture* Texture;

	G_Rect *Src;

	int FrameCount;
	int FrameDuration;

	int StopFrame;
	int PausedFrame;

	Uint32 StartTime;

	Animation()
	{
		StopFrame = -2;
		PausedFrame = 0;
	}

	~Animation()
	{
		G_DestroyTexture(Texture);
		if (StopFrame != -2)
			delete[] Src;
	}

	void load(char* path, int frame_count, int frame_duration, int x, int y, int w, int h, int ir = -1, int ig = -1, int ib = -1)
	{
		if (ir == -1)
			Texture = G_LoadImage(path);
		else
			Texture = G_LoadImage(path, ir, ig, ib);
		StopFrame = 0;
		Src = new G_Rect[frame_count];
		for (int i = 0; i < frame_count; i++)
		{
			Src[i].x = x + w / frame_count*i;
			Src[i].y = y;
			Src[i].w = w / frame_count;
			Src[i].h = h;
		}
		FrameDuration = frame_duration;
		FrameCount = frame_count;
	}

	void play()
	{
		StartTime = G_GetTicks();
		StopFrame = -1;
	}
	void stop()
	{
		StopFrame = 0;
		PausedFrame = 0;
	}
	void pause()
	{
		if (StopFrame == -1)
			PausedFrame = ((G_GetTicks() - StartTime) / FrameDuration + PausedFrame) % FrameCount;
		StopFrame = 0;
	}

};

struct Background
{
	Animation *Pic;
};

struct MovingBackground
{
	float v;
	G_Rect Dst;

	G_Rect TempDst, TempSrc;

	int PausedPosition;

	bool Playing;

	Uint32 StartTime;

	Animation *Pic;

	bool FirstHalf;

	MovingBackground()
	{
		PausedPosition = 0;
		Playing = false;
		FirstHalf = false;
	}

	void Play()
	{
		StartTime = G_GetTicks();
		Playing = true;
	}
	void Pause()
	{
		PausedPosition += Playing ? int(v*(G_GetTicks() - StartTime)) % Pic->Src[0].w : 0;
		Playing = false;
	}
	void Stop()
	{
		PausedPosition = 0;
		Playing = false;
	}
};

struct Player
{
	float x, y;
	float Vx, Vy;
	float Ax, Ay;
	G_Rect Pos;
	Animation *Anim;
	bool Right;

	int LastTick;
	int dt;

	bool IsOnFloor;

	void update_pos()
	{
		dt = G_GetTicks() - LastTick;
		LastTick = G_GetTicks();
		Vx += float(dt)*Ax;
		Vy += float(dt)*Ay;

		x += float(dt)*Vx;
		y += float(dt)*Vy;
	}
};

struct Button
{
	G_Rect Dst;
	Animation *States[3];
	bool IsOn;
	bool Pressed;
	bool Puls;

	Button()
	{
		IsOn = false;
		Pressed = false;
	}

	void Update()
	{
		Puls = false;
		IsOn = false;
		if (mouseX >= Dst.x&&mouseX <= Dst.x + Dst.w&&mouseY >= Dst.y&&mouseY <= Dst.y + Dst.h)
			IsOn = true;
		if (IsOn&&Event == G_MOUSEBUTTONDOWN&&G_Mouse == G_BUTTON_LEFT)
			Pressed = true;
		if (Event == G_MOUSEBUTTONUP&&G_Mouse == G_BUTTON_LEFT)
		{
			if (Pressed&&IsOn)
				Puls = true;
			Pressed = false;
		}
	}
};

struct CheckBox
{
	bool Checked;
	bool Pressed;

	Animation *States[2];

	G_Rect Dst;

	CheckBox()
	{
		Checked = false;
		Pressed = false;
	}

	void Update()
	{
		if (mouseX >= Dst.x&&mouseX <= Dst.x + Dst.w&&mouseY >= Dst.y&&mouseY <= Dst.y + Dst.h)
		{
			if (Event == G_MOUSEBUTTONDOWN&&G_Mouse == G_BUTTON_LEFT)
				Pressed = true;
			if (Pressed && Event == G_MOUSEBUTTONUP&&G_Mouse == G_BUTTON_LEFT)
			{
				Pressed = false;
				Checked = !Checked;
			}
		}
	}
};

struct Pic
{
	G_Rect Dst;
	Animation *Anim;
	SDL_RendererFlip Flip;
	Pic()
	{
		Flip = SDL_FLIP_NONE;
	}
};

struct drawable
{
	G_Texture* Texture;

	G_Rect* Src;
	G_Rect* Dst;

	SDL_RendererFlip Flip;

	drawable()
	{
	}

	drawable(Background &inp)
	{
		if (inp.Pic[0].StopFrame != -2)
		{
			if (inp.Pic[0].StopFrame == -1)
				Src = &inp.Pic[0].Src[((G_GetTicks() - inp.Pic[0].StartTime) / inp.Pic[0].FrameDuration + inp.Pic[0].PausedFrame) % inp.Pic[0].FrameCount];
			else
				Src = &inp.Pic[0].Src[inp.Pic[0].PausedFrame];
			Dst = NULL;
			Texture = inp.Pic[0].Texture;
		}
		else
			Texture = NULL;
	}
	drawable(MovingBackground &inp)
	{
		if (inp.Pic[0].StopFrame != -2)
		{
			if (!inp.FirstHalf)
			{
				if (inp.Pic[0].StopFrame == -1)
					inp.TempSrc = inp.Pic[0].Src[((G_GetTicks() - inp.Pic[0].StartTime) / inp.Pic[0].FrameDuration + inp.Pic[0].PausedFrame) % inp.Pic[0].FrameCount];
				else
					inp.TempSrc = inp.Pic[0].Src[inp.Pic[0].PausedFrame];

				Src = &inp.TempSrc;

				inp.TempDst = inp.Dst;
				Dst = &inp.TempDst;

				int CurrentPositon = int(inp.v*(G_GetTicks() - inp.StartTime)) % Src->w;
				CurrentPositon = Src->w - ((inp.Playing ? CurrentPositon : 0) + inp.PausedPosition) % Src->w;

				int CurrentPositionDst = int(CurrentPositon*Dst->w / Src->w) - 1;

				Src->w -= CurrentPositon;

				Dst->x += CurrentPositionDst;
				Dst->w -= CurrentPositionDst;

				Texture = inp.Pic[0].Texture;
			}
			else
			{
				if (inp.Pic[0].StopFrame == -1)
					inp.TempSrc = inp.Pic[0].Src[((G_GetTicks() - inp.Pic[0].StartTime) / inp.Pic[0].FrameDuration + inp.Pic[0].PausedFrame) % inp.Pic[0].FrameCount];
				else
					inp.TempSrc = inp.Pic[0].Src[inp.Pic[0].PausedFrame];

				Src = &inp.TempSrc;

				inp.TempDst = inp.Dst;
				Dst = &inp.TempDst;

				int CurrentPositon = int(inp.v*(G_GetTicks() - inp.StartTime)) % Src->w;
				CurrentPositon = Src->w - ((inp.Playing ? CurrentPositon : 0) + inp.PausedPosition) % Src->w;

				int CurrentPositionDst = int(CurrentPositon*Dst->w / Src->w);

				Src->x += Src->w - CurrentPositon;
				Src->w = CurrentPositon;

				Dst->w = CurrentPositionDst;

				Texture = inp.Pic[0].Texture;
			}

			inp.FirstHalf = !inp.FirstHalf;
		}
		else
			Texture = NULL;
	}
	drawable(Player &inp)
	{
		inp.Pos.x = int(inp.x);
		inp.Pos.y = int(inp.y);
		if (inp.Anim[0].StopFrame != -2)
		{
			if (inp.Anim[0].StopFrame == -1)
				Src = &inp.Anim[0].Src[((G_GetTicks() - inp.Anim[0].StartTime) / inp.Anim[0].FrameDuration + inp.Anim[0].PausedFrame) % inp.Anim[0].FrameCount];
			else
				Src = &inp.Anim[0].Src[inp.Anim[0].PausedFrame];
			Dst = &inp.Pos;
			Texture = inp.Anim[0].Texture;
			if (inp.Right)
				Flip = SDL_FLIP_NONE;
			else
				Flip = SDL_FLIP_HORIZONTAL;
		}
		else
			Texture = NULL;
	}
	drawable(Button &inp)
	{
		Animation *Anim;
		if (inp.Pressed)
			Anim = inp.States[2];
		else if (inp.IsOn)
			Anim = inp.States[1];
		else
			Anim = inp.States[0];

		if (Anim[0].StopFrame != -2)
		{
			if (Anim[0].StopFrame == -1)
				Src = &Anim[0].Src[((G_GetTicks() - Anim[0].StartTime) / Anim[0].FrameDuration + Anim[0].PausedFrame) % Anim[0].FrameCount];
			else
				Src = &Anim[0].Src[Anim[0].PausedFrame];
			Dst = &inp.Dst;
			Texture = Anim[0].Texture;
			Flip = SDL_FLIP_NONE;
		}
		else
			Texture = NULL;
	}
	drawable(CheckBox &inp)
	{
		Animation *Anim;
		if (inp.Checked)
			Anim = inp.States[0];
		else
			Anim = inp.States[1];

		if (Anim[0].StopFrame != -2)
		{
			if (Anim[0].StopFrame == -1)
				Src = &Anim[0].Src[((G_GetTicks() - Anim[0].StartTime) / Anim[0].FrameDuration + Anim[0].PausedFrame) % Anim[0].FrameCount];
			else
				Src = &Anim[0].Src[Anim[0].PausedFrame];
			Dst = &inp.Dst;
			Texture = Anim[0].Texture;
			Flip = SDL_FLIP_NONE;
		}
		else
			Texture = NULL;
	}
	drawable(Pic &inp)
	{
		Animation *Anim= inp.Anim;

		if (Anim[0].StopFrame != -2)
		{
			if (Anim[0].StopFrame == -1)
				Src = &Anim[0].Src[((G_GetTicks() - Anim[0].StartTime) / Anim[0].FrameDuration + Anim[0].PausedFrame) % Anim[0].FrameCount];
			else
				Src = &Anim[0].Src[Anim[0].PausedFrame];
			Dst = &inp.Dst;
			Texture = Anim[0].Texture;
			Flip = inp.Flip;
		}
		else
			Texture = NULL;
	}
};

struct Tile
{
	int h;
	float x;
	int Terrain;
};

void draw(drawable inp);

struct Map
{
	Tile Tiles[4];
	float v;
	int LastTick;

	Pic TileStyles[3];

	void Reset()
	{
		int temp_height = rand() % int(278 * 1.25 - 75) + 75;
		for (int i = 0; i < 4; i++)
		{
			Tiles[i].x = i*int(150 * 1.25) * 3;
			Tiles[i].h = temp_height;
			Tiles[i].Terrain = Road;
		}
		LastTick = G_GetTicks();
	}

	void Update()
	{
		int dt = G_GetTicks() - LastTick;
		LastTick = G_GetTicks();

		for (int i = 0; i < 4; i++)
			Tiles[i].x -= v*float(dt);

		if (Tiles[1].x < 0)
		{
			Tiles[0] = Tiles[1];
			Tiles[1] = Tiles[2];
			Tiles[2] = Tiles[3];
			Tiles[3].x = Tiles[2].x + int(150 * 1.25) * 3;
			Tiles[3].Terrain = rand() % 3 + 1;

			if (Tiles[2].Terrain == Road)
				Tiles[3].h = Tiles[2].h;
			else  if (Tiles[2].Terrain == Cliff)
				Tiles[3].h = rand() % int(278 * 1.25 - 150) + 150;
			else
				Tiles[3].h = Tiles[2].h - int(99 * 1.25);

			if (Tiles[3].Terrain == Slope&&Tiles[3].h<int(175 * 1.25))
				Tiles[3].Terrain = Cliff;
		}
	}
	void Draw()
	{
		for (int i = 0; i < 3; i++)
		{
			switch (Tiles[i].Terrain)
			{
			case Road:
				TileStyles[0].Dst.w = int(150 * 1.25);
				TileStyles[0].Dst.h = int(278 * 1.25);
				TileStyles[0].Dst.y = 600 - Tiles[i].h;
				for (int j = 0; j < 3; j++)
				{
					TileStyles[0].Dst.x = Tiles[i].x + int(150 * 1.25)*j;
					draw(TileStyles[0]);
				}

				break;
			case Cliff:
				TileStyles[1].Dst.w = int(150 * 1.25);
				TileStyles[1].Dst.h = int(278 * 1.25);
				TileStyles[1].Dst.y = 600 - Tiles[i].h;
				TileStyles[1].Dst.x = Tiles[i].x;
				TileStyles[1].Flip = SDL_FLIP_NONE;
				draw(TileStyles[1]);

				TileStyles[1].Dst.w = int(150 * 1.25);
				TileStyles[1].Dst.h = int(278 * 1.25);
				TileStyles[1].Dst.y = 600 - Tiles[i + 1].h;
				TileStyles[1].Dst.x = Tiles[i].x + int(150 * 1.25)*2;
				TileStyles[1].Flip = SDL_FLIP_HORIZONTAL;
				draw(TileStyles[1]);

				break;
			case Slope:
				TileStyles[0].Dst.w = int(150 * 1.25);
				TileStyles[0].Dst.h = int(278 * 1.25);
				TileStyles[0].Dst.y = 600 - Tiles[i].h;
				TileStyles[0].Dst.x = Tiles[i].x;
				TileStyles[0].Flip = SDL_FLIP_NONE;
				draw(TileStyles[0]);

				TileStyles[2].Dst.w = int(150 * 1.25);
				TileStyles[2].Dst.h = int(374 * 1.25);
				TileStyles[2].Dst.y = 600 - Tiles[i].h;
				TileStyles[2].Dst.x = Tiles[i].x + int(150 * 1.25);
				TileStyles[2].Flip = SDL_FLIP_NONE;
				draw(TileStyles[2]);

				TileStyles[0].Dst.w = int(150 * 1.25);
				TileStyles[0].Dst.h = int(278 * 1.25);
				TileStyles[0].Dst.y = 600 - Tiles[i + 1].h;
				TileStyles[0].Dst.x = Tiles[i].x + int(150 * 1.25) * 2;
				TileStyles[0].Flip = SDL_FLIP_NONE;
				draw(TileStyles[0]);
				break;
			}
		}
	}
	int GetY(int X)
	{
		for (int i = 0; i < 3; i++)
			if (int(Tiles[i].x) <= X && (int(Tiles[i].x) + int(150 * 1.25) * 3) >= X)
			{
				switch (Tiles[i].Terrain)
				{
				case Road:
					
					return Tiles[i].h;

					break;
				case Cliff:

					if (int(Tiles[i].x) <= X && (int(Tiles[i].x) + int(150 * 1.25)) >= X)
						return Tiles[i].h;
					else if (int(Tiles[i].x) + int(150 * 1.25) <= X && (int(Tiles[i].x) + int(150 * 1.25) * 2) >= X)
						return 0;
					else
						return Tiles[i + 1].h;

					break;
				case Slope:

					if (int(Tiles[i].x) <= X && (int(Tiles[i].x) + int(150 * 1.25)) >= X)
						return Tiles[i].h;
					else if (int(Tiles[i].x) + int(150 * 1.25) <= X && (int(Tiles[i].x) + int(150 * 1.25) + 55) >= X)
					{
						float ratio = float(X - (int(Tiles[i].x) + int(150 * 1.25))) / 55.0f;
						return  ratio * Tiles[i + 1].h + (1.0f - ratio)* Tiles[i].h;
					}
					else
						return Tiles[i + 1].h;
					
					break;
				}
			}
	}
};

Player ThePlayer;
Animation TheGuyAnim;
Animation TheGirlAnim;

Background StartMenuBCK;
Animation StartMenuBCKAnim;

MovingBackground GameBCK;
Animation GameBCKAnim;

Button StartButton;
Animation StartButtonAnim;
Animation StartButtonAnimPressed;

Pic GuyInStartMenu;
Animation GuyInStartMenuAnim;

Pic LogoInStartMenu;
Animation LogoInStartMenuAnim;

CheckBox Sound;
Animation SoundOn;
Animation SoundOff;

CheckBox Music;
Animation MusicOn;
Animation MusicOff;

Background ChoosePlayerBCK;
Animation ChoosePlayerBCKAnim;

CheckBox GirlSelection;
CheckBox BoySelection;
Animation GirlChosen;
Animation GirlNotChosen;
Animation BoyChosen;
Animation BoyNotChosen;

Button BackBtnPlayerSelection;
Animation BackBtnPlayerSelectionAnim;
Animation BackBtnPlayerSelectionAnimPressed;

Button PlayBtnPlayerSelection;
Animation PlayBtnPlayerSelectionAnim;
Animation PlayBtnPlayerSelectionAnimPressed;

Button PauseBtn;
Animation PuaseBtnAnim;

Background Shade;
Animation ShadeAnim;

Button ResumeBtnPause;
Animation ResumeBtnPauseAnim;
Animation ResumeBtnPauseAnimPressed;

Button RetryBtnPause;
Animation RetryBtnPauseAnim;
Animation RetryBtnPauseAnimPressed;

Button QuitBtnPause;
Animation QuitBtnPauseAnim;
Animation QuitBtnPauseAnimPressed;

Map MovingMap;
Animation TileAnims[3];

int PlayerHealth = 3;

void DoPhysics(Player *P)
{
	P->IsOnFloor = false;

	float y = P->y;
	P->update_pos();
	int h = MovingMap.GetY(ThePlayer.x + ThePlayer.Pos.w / 2);
	if (h != 0)
	{
		int FloorY = 550 - h;
		if (FloorY <= P->y && FloorY >= y)
		{
			P->IsOnFloor = true;
			P->y = FloorY;
			P->Vy = 0;
		}
	}
}

void draw(drawable inp)
{
	if (inp.Texture != NULL)
	{
		if (inp.Dst == NULL)
			G_DrawEx(inp.Texture, inp.Src, inp.Dst, inp.Flip, true);
		else
			G_DrawEx(inp.Texture, inp.Src, inp.Dst, inp.Flip);
	}
}

void load()
{
	StartMenuBCKAnim.load("Pics\\home_bg.png", 1, 100, 0, 0, 511, 307);
	StartMenuBCK.Pic = &StartMenuBCKAnim;

	GameBCKAnim.load("Pics\\changjing4.jpg", 1, 100, 0, 0, 900, 505);
	GameBCK.Pic = &GameBCKAnim;

	GameBCK.v = 0.05;

	GameBCK.Dst.x = -100;
	GameBCK.Dst.y = 0;
	GameBCK.Dst.w = 1200;
	GameBCK.Dst.h = 600;

	StartButtonAnim.load("Pics\\menu.png", 2, 100, 0, 0, 600, 100);
	StartButtonAnimPressed.load("Pics\\menu.png", 1, 100, 0, 0, 300, 100);

	StartButtonAnim.play();

	StartButton.States[0] = &StartButtonAnim;
	StartButton.States[1] = &StartButtonAnim;
	StartButton.States[2] = &StartButtonAnimPressed;

	StartButton.Dst.w = 300;
	StartButton.Dst.h = 100;
	StartButton.Dst.x = 350;
	StartButton.Dst.y = 450;

	GuyInStartMenuAnim.load("Pics\\man.png", 1, 100, 0, 0, 182, 251);
	GuyInStartMenu.Anim = &GuyInStartMenuAnim;
	GuyInStartMenu.Dst.w = 182*1.25;
	GuyInStartMenu.Dst.h = 251*1.25;
	GuyInStartMenu.Dst.x = 659;
	GuyInStartMenu.Dst.y = 200;

	LogoInStartMenuAnim.load("Pics\\logo.png", 1, 100, 0, 0, 511, 228);
	LogoInStartMenu.Anim = &LogoInStartMenuAnim;
	LogoInStartMenu.Dst.w = 511*1.25;
	LogoInStartMenu.Dst.h = 228*1.25;
	LogoInStartMenu.Dst.x = 175;
	LogoInStartMenu.Dst.y = 100;

	SoundOn.load("pics\\SoundOn.png", 1, 100, 0, 0, 55, 55);
	SoundOff.load("pics\\SoundOff.png", 1, 100, 0, 0, 55, 55);
	Sound.States[0] = &SoundOn;
	Sound.States[1] = &SoundOff;
	Sound.Dst.w = 55 * 1.25;
	Sound.Dst.h = 55 * 1.25;
	Sound.Dst.x = 920;
	Sound.Dst.y = 520;

	MusicOn.load("pics\\MusicOn.png", 1, 100, 0, 0, 55, 55);
	MusicOff.load("pics\\MusicOff.png", 1, 100, 0, 0, 55, 55);
	Music.States[0] = &MusicOn;
	Music.States[1] = &MusicOff;
	Music.Dst.w = 55 * 1.25;
	Music.Dst.h = 55 * 1.25;
	Music.Dst.x = 830;
	Music.Dst.y = 520;

	BoyChosen.load("Pics\\boybtn.png", 1, 100, 0, 0, 274, 274);
	BoyNotChosen.load("Pics\\boybtn.png", 1, 100, 274, 0, 274, 274);
	BoySelection.States[0] = &BoyChosen;
	BoySelection.States[1] = &BoyNotChosen;
	BoySelection.Dst.w = 274 * 1.25;
	BoySelection.Dst.h = 274 * 1.25;
	BoySelection.Dst.x = 575;
	BoySelection.Dst.y = 100;


	GirlChosen.load("Pics\\girlbtn.png", 1, 100, 0, 0, 274, 274);
	GirlNotChosen.load("Pics\\girlbtn.png", 1, 100, 274, 0, 274, 274);
	GirlSelection.States[0] = &GirlChosen;
	GirlSelection.States[1] = &GirlNotChosen;
	GirlSelection.Dst.w = 274 * 1.25;
	GirlSelection.Dst.h = 274 * 1.25;
	GirlSelection.Dst.x = 100;
	GirlSelection.Dst.y = 100;

	ChoosePlayerBCKAnim.load("Pics\\shop_bg.jpg", 1, 100, 0, 0, 800, 480);
	ChoosePlayerBCK.Pic = &ChoosePlayerBCKAnim;

	BackBtnPlayerSelectionAnimPressed.load("Pics\\BackBtnPressed.png", 1, 100, 0, 0, 130, 100);
	BackBtnPlayerSelectionAnim.load("Pics\\BackBtn.png", 1, 100, 0, 0, 130, 100);
	BackBtnPlayerSelection.States[0] = &BackBtnPlayerSelectionAnim;
	BackBtnPlayerSelection.States[1] = &BackBtnPlayerSelectionAnim;
	BackBtnPlayerSelection.States[2] = &BackBtnPlayerSelectionAnimPressed;
	BackBtnPlayerSelection.Dst.w = 130 * 1.25;
	BackBtnPlayerSelection.Dst.h = 100 * 1.25;
	BackBtnPlayerSelection.Dst.x = 50;
	BackBtnPlayerSelection.Dst.y = 450;

	PlayBtnPlayerSelectionAnimPressed.load("Pics\\PlayBtnPressed.png", 1, 100, 0, 0, 120, 100);
	PlayBtnPlayerSelectionAnim.load("Pics\\PlayBtn.png", 1, 100, 0, 0, 120, 100);
	PlayBtnPlayerSelection.States[0] = &PlayBtnPlayerSelectionAnim;
	PlayBtnPlayerSelection.States[1] = &PlayBtnPlayerSelectionAnim;
	PlayBtnPlayerSelection.States[2] = &PlayBtnPlayerSelectionAnimPressed;
	PlayBtnPlayerSelection.Dst.w = 120 * 1.25;
	PlayBtnPlayerSelection.Dst.h = 100 * 1.25;
	PlayBtnPlayerSelection.Dst.x = 800;
	PlayBtnPlayerSelection.Dst.y = 450;

	PuaseBtnAnim.load("Pics\\PauseBtn.png", 1, 100, 0, 0, 45, 35);
	PauseBtn.States[0] = &PuaseBtnAnim;
	PauseBtn.States[1] = &PuaseBtnAnim;
	PauseBtn.States[2] = &PuaseBtnAnim;
	PauseBtn.Dst.w = 45 * 1.25;
	PauseBtn.Dst.h = 35 * 1.25;
	PauseBtn.Dst.x = 10;
	PauseBtn.Dst.y = 10;

	TheGuyAnim.load("Pics\\body.png", 8, 100, 0, 0, 304, 60);
	TheGirlAnim.load("Pics\\f_body.png", 8, 100, 0, 0, 320, 60);
	ThePlayer.Anim = &TheGuyAnim;

	ThePlayer.x = 0;
	ThePlayer.y = 500;
	ThePlayer.Pos.w = 100;
	ThePlayer.Pos.h = 100;

	ShadeAnim.load("Pics\\Shade.png", 1, 100, 0, 0, 10, 10);
	Shade.Pic = &ShadeAnim;

	ResumeBtnPauseAnim.load("Pics\\Resume.png", 1, 100, 0, 0, 200, 110);
	ResumeBtnPauseAnimPressed.load("Pics\\ResumePressed.png", 1, 100, 0, 0, 200, 110);
	ResumeBtnPause.States[0] = &ResumeBtnPauseAnim;
	ResumeBtnPause.States[1] = &ResumeBtnPauseAnim;
	ResumeBtnPause.States[2] = &ResumeBtnPauseAnimPressed;
	
	ResumeBtnPause.Dst.x = 400;
	ResumeBtnPause.Dst.y = 75;
	ResumeBtnPause.Dst.w = 200 * 1.25;
	ResumeBtnPause.Dst.h = 110 * 1.25;

	RetryBtnPauseAnim.load("Pics\\Retry.png", 1, 100, 0, 0, 200, 110);
	RetryBtnPauseAnimPressed.load("Pics\\RetryPressed.png", 1, 100, 0, 0, 200, 110);
	RetryBtnPause.States[0] = &RetryBtnPauseAnim;
	RetryBtnPause.States[1] = &RetryBtnPauseAnim;
	RetryBtnPause.States[2] = &RetryBtnPauseAnimPressed;

	RetryBtnPause.Dst.x = 400;
	RetryBtnPause.Dst.y = 225;
	RetryBtnPause.Dst.w = 200 * 1.25;
	RetryBtnPause.Dst.h = 110 * 1.25;

	QuitBtnPauseAnim.load("Pics\\Quit.png", 1, 100, 0, 0, 200, 110);
	QuitBtnPauseAnimPressed.load("Pics\\QuitPressed.png", 1, 100, 0, 0, 200, 110);
	QuitBtnPause.States[0] = &QuitBtnPauseAnim;
	QuitBtnPause.States[1] = &QuitBtnPauseAnim;
	QuitBtnPause.States[2] = &QuitBtnPauseAnimPressed;

	QuitBtnPause.Dst.x = 400;
	QuitBtnPause.Dst.y = 375;
	QuitBtnPause.Dst.w = 200 * 1.25;
	QuitBtnPause.Dst.h = 110 * 1.25;

	TileAnims[0].load("Pics\\jadde.png", 1, 100, 0, 0, 150, 278);
	TileAnims[1].load("Pics\\L_darre.png", 1, 100, 0, 0, 150, 278);
	TileAnims[2].load("Pics\\shib.png", 1, 100, 0, 0, 150, 374);
	MovingMap.TileStyles[0].Anim = &TileAnims[0];
	MovingMap.TileStyles[1].Anim = &TileAnims[1];
	MovingMap.TileStyles[2].Anim = &TileAnims[2];
	
	MovingMap.v = 0.2;
}

void Init()
{
	srand(time(0));

	G_Rect WinPos;
	WinPos.x = WinPos.y = SDL_WINDOWPOS_UNDEFINED;
	WinPos.w = 1000;
	WinPos.h = 600;

	G_InitSDL();

	G_CreateWindow("Zombie Dash", WinPos, 0, 0, 100);

	load();

}

void HandleEvent()
{
	Event = G_Event();

	if (Event == G_MOUSEMOTION)
	{
		mouseX = G_motion.x;
		mouseY = G_motion.y;
	}
	if (Event == G_QUIT)
		GameState = Clear_For_Exit;

}

void ResetGame()
{
	GameBCK.Stop();
	GameBCK.Play();

	ThePlayer.Anim->stop();
	ThePlayer.Anim->play();

	MovingMap.Reset();

	ThePlayer.Right = true;
	ThePlayer.LastTick = G_GetTicks();

	ThePlayer.Vx = 0;
	ThePlayer.Vy = 0;
	ThePlayer.Ax = 0;
	ThePlayer.Ay = 0.001;

	ThePlayer.x = 50;
	ThePlayer.y = 500 - MovingMap.GetY(ThePlayer.x + ThePlayer.Pos.w / 2);

	PlayerHealth = 3;

}

void Start()
{
	draw(StartMenuBCK);
	draw(StartButton);
	draw(LogoInStartMenu);
	draw(GuyInStartMenu);
	draw(Sound);
	draw(Music);

	StartButton.Update();
	Sound.Update();
	Music.Update();

	if (StartButton.Puls)
		GameState = Player_Menu;
}

void Play()
{
	draw(GameBCK);
	draw(GameBCK);
	draw(PauseBtn);
	MovingMap.Draw();
	draw(ThePlayer);

	MovingMap.Update();


	DoPhysics(&ThePlayer);

	if (Event == G_KEYDOWN && G_Keyboard == GK_SPACE&&ThePlayer.IsOnFloor)
	{
		ThePlayer.Vy = -1;
		ThePlayer.y -= 20;
	}

	

	PauseBtn.Update();
	if (PauseBtn.Puls)
	{
		GameState = Pause_Menu;
		GameBCK.Pause();
		ThePlayer.Anim->pause();
	}
}

void Pause()
{
	draw(GameBCK);
	draw(GameBCK);
	MovingMap.Draw();
	draw(ThePlayer);
	draw(Shade);
	draw(ResumeBtnPause);
	draw(RetryBtnPause);
	draw(QuitBtnPause);

	MovingMap.LastTick = G_GetTicks();
	ThePlayer.LastTick = G_GetTicks();

	ResumeBtnPause.Update();
	RetryBtnPause.Update();
	QuitBtnPause.Update();

	if (ResumeBtnPause.Puls)
	{
		GameState = Play_Mode;
		GameBCK.Play();
		ThePlayer.Anim->play();
	}
	if (RetryBtnPause.Puls)
	{
		ResetGame();
		GameState = Play_Mode;
	}
	if (QuitBtnPause.Puls)
	{
		ResetGame();
		GameState = Start_Menu;
	}
}

void ChoosePlayer()
{
	draw(ChoosePlayerBCK);
	draw(GirlSelection);
	draw(BoySelection);
	draw(PlayBtnPlayerSelection);
	draw(BackBtnPlayerSelection);

	BackBtnPlayerSelection.Update();
	PlayBtnPlayerSelection.Update();

	if (!GirlSelection.Checked)
	{
		GirlSelection.Update();
		BoySelection.Checked = !GirlSelection.Checked;
	}
	if (!BoySelection.Checked)
	{
		BoySelection.Update();
		GirlSelection.Checked = !BoySelection.Checked;
	}

	if (BackBtnPlayerSelection.Puls)
	{
		GameState = Start_Menu;
		GirlSelection.Checked = false;
	}
	if (PlayBtnPlayerSelection.Puls)
	{
		if (BoySelection.Checked)
			ThePlayer.Anim = &TheGuyAnim;
		if (GirlSelection.Checked)
			ThePlayer.Anim = &TheGirlAnim;
		
		ResetGame();

		GameState = Play_Mode;

		GirlSelection.Checked = false;
	}
}

void Won()
{

}

void Lost()
{

}

void ClearExit()
{

}






void main()
{

	Init();

	while (GameState != Clear_For_Exit)
	{
		HandleEvent();
		switch (GameState)
		{
		case Start_Menu:
			Start();
			break;
		case Player_Menu:
			ChoosePlayer();
			break;
		case Play_Mode:
			Play();
			break;
		case Pause_Menu:
			Pause();
			break;
		case Won_Menu:
			Won();
			break;
		case Lost_Menu:
			Lost();
			break;
		}
		G_Update();
	}
	ClearExit();
}