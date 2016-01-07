#include "Genio.h"

#undef main

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
	Animation BackGround;
};

struct Block
{
	Animation *Pic;
	G_Rect Dst;
	float x, y;
	float Vx, Vy;
	Block()
	{
		Vx = 0;
		Vy = 0;
	}
	void update_pos(int dt)
	{
		x += dt*Vx;
		y += dt*Vy;
	}
};

struct Rope
{
	Animation *Pic;
	G_Rect Dst;
};

struct drawable
{
	G_Texture* Texture;

	G_Rect* Src;
	G_Rect* Dst;

	drawable()
	{
	}

	drawable(Background &inp)
	{
		if (inp.BackGround.StopFrame != -2)
		{
			if (inp.BackGround.StopFrame == -1)
				Src = &inp.BackGround.Src[((G_GetTicks() - inp.BackGround.StartTime) / inp.BackGround.FrameDuration + inp.BackGround.PausedFrame) % inp.BackGround.FrameCount];
			else
				Src = &inp.BackGround.Src[inp.BackGround.PausedFrame];
			Dst = NULL;
			Texture = inp.BackGround.Texture;
		}
		else
			Texture = NULL;
	}

	drawable(Block &inp)
	{
		inp.Dst.x = int(inp.x);
		inp.Dst.y = int(inp.y);

		if (inp.Pic[0].StopFrame != -2)
		{
			if (inp.Pic[0].StopFrame == -1)
				Src = &inp.Pic[0].Src[((G_GetTicks() - inp.Pic[0].StartTime) / inp.Pic[0].FrameDuration + inp.Pic[0].PausedFrame) % inp.Pic[0].FrameCount];
			else
				Src = &inp.Pic[0].Src[inp.Pic[0].PausedFrame];
			Dst = &inp.Dst;
			Texture = inp.Pic[0].Texture;
		}
		else
			Texture = NULL;
	}
	drawable(Rope &inp)
	{
		inp.Dst.x = inp.Dst.x;
		inp.Dst.y = inp.Dst.y;

		if (inp.Pic[0].StopFrame != -2)
		{
			if (inp.Pic[0].StopFrame == -1)
				Src = &inp.Pic[0].Src[((G_GetTicks() - inp.Pic[0].StartTime) / inp.Pic[0].FrameDuration + inp.Pic[0].PausedFrame) % inp.Pic[0].FrameCount];
			else
				Src = &inp.Pic[0].Src[inp.Pic[0].PausedFrame];
			Dst = &inp.Dst;
			Texture = inp.Pic[0].Texture;
		}
		else
			Texture = NULL;
	}
};

void draw(drawable inp)
{
	if (inp.Texture != NULL)
	{
		if (inp.Dst == NULL)
			G_Draw(inp.Texture, inp.Src, inp.Dst, true);
		else
			G_Draw(inp.Texture, inp.Src, inp.Dst);
	}
}

void main()
{

}