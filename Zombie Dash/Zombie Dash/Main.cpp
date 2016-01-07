#include "Genio.h"

#undef main

#define Start_Menu		1
#define Play_Mode		2
#define Pause_Menu		3
#define Won_Menu		4
#define Lost_Menu		5
#define Player_Menu		6
#define Clear_For_Exit	7

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
	Animation Pic;
};

struct Player
{
	float x, y;
	float Vx, Vy;
	G_Rect Pos;
	Animation *Anim;
	bool Right;
	void update_pos(int dt)
	{
		x += float(dt)*Vx;
		y += float(dt)*Vy;
	}
};

struct Button
{
	int X, Y;
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
		if (IsOn&&Event == G_MOUSEBUTTONUP&&G_Mouse == G_BUTTON_LEFT)
		{
			if (Pressed)
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
		if (inp.Pic.StopFrame != -2)
		{
			if (inp.Pic.StopFrame == -1)
				Src = &inp.Pic.Src[((G_GetTicks() - inp.Pic.StartTime) / inp.Pic.FrameDuration + inp.Pic.PausedFrame) % inp.Pic.FrameCount];
			else
				Src = &inp.Pic.Src[inp.Pic.PausedFrame];
			Dst = NULL;
			Texture = inp.Pic.Texture;
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
};


Player TheGuy;
Animation TheGuyAnim;

Player TheGirl;
Animation TheGirlAnim;


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
	TheGuyAnim.load("Pics\\body.png", 8, 100, 0, 0, 304, 60);
	TheGuy.Anim = &TheGuyAnim;

	TheGuy.x = 0;
	TheGuy.y = 0;
	TheGuy.Pos.w = 100;
	TheGuy.Pos.h = 100;

	TheGuy.Right = true;

	TheGirlAnim.load("Pics\\f_body.png", 8, 100, 0, 0, 320, 60);
	TheGirl.Anim = &TheGirlAnim;

	TheGirl.x = 100;
	TheGirl.y = 0;
	TheGirl.Pos.w = 100;
	TheGirl.Pos.h = 100;

	TheGirl.Right = true;

}

void Init()
{
	G_Rect WinPos;
	WinPos.x = WinPos.y = SDL_WINDOWPOS_UNDEFINED;
	WinPos.w = 1000;
	WinPos.h = 600;

	G_InitSDL();

	G_CreateWindow("Zombie Dash", WinPos, 0, 0, 100);

	load();



	TheGuy.Anim->play();
	TheGirl.Anim->play();

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

void Start()
{
	draw(TheGuy);
	draw(TheGirl);
}

void Play()
{
	
}

void Pause()
{

}

void ChoosePlayer()
{

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