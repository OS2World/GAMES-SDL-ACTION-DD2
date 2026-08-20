/*

    Dodgin' Diamond 2, a shot'em up arcade
    Copyright (C) 2003,2004 Juan J. Martinez <jjm@usebox.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/
#include<stdlib.h>
#include<stdio.h>
#include"main.h"
#include"SDL.h"
#include"SDL_mixer.h"
#include"menu.h"

#ifdef __OS2__
#include<sys/stat.h>
#include<string.h>
#endif

#define APP_NAME	"Dodgin' Diamond ]["

#define FPS	60

#include "control.h"
#include "SDL_plus.h"
#include "engine.h"
#include "cfg.h"

#ifdef WIN32
static const char COPYRIGHT[]="Dodgin' Diamond 2 - Copyright (c) 2003,2004 Juan J. Martinez <jjm@usebox.net> This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License; either version 2 of the License, or (at your option) any later version, as published by the Free Software Foundation (www.fsf.org).";
#endif

SDL_Surface *screen, *gfx;
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *screen_texture = NULL;

extern pDesc player[2];
SDL_Joystick *joy[2]={ NULL, NULL };
SDL_Event event;
Uint32 tick, ntick;
float scroll=0,scroll2=0;

bool pause;
Uint32 pause_tick;

extern bool boss;

cfg conf;
score hiscore[10];

Mix_Chunk *efx[8]={ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
Mix_Music *bgm=NULL, *bgm_boss=NULL;
int sound;
bool done;

#ifdef __OS2__
static FILE *logfp=NULL;
#define LOG(...) do { if(logfp){ fprintf(logfp,__VA_ARGS__); fflush(logfp); } } while(0)
#else
#define LOG(...) fprintf(stderr,__VA_ARGS__)
#endif

#ifdef __OS2__
static void mkdirp_impl(const char *path)
{
	char buf[1024];
	char *p;
	strncpy(buf, path, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';
	for (p = buf + 1; *p; p++) {
		if (*p == '/' || *p == '\\') {
			char saved = *p; *p = '\0';
			mkdir(buf, 0755);
			*p = saved;
		}
	}
	mkdir(path, 0755);
}

static void dd2_config_path(char *buf, size_t len)
{
	char dir[490];
	char *p;
	const char *xdg = getenv("XDG_CONFIG_HOME");
	if (xdg && xdg[0])
		snprintf(dir, sizeof(dir), "%s/dd2", xdg);
	else {
		const char *home = getenv("HOME");
		snprintf(dir, sizeof(dir), "%s/.config/dd2", home ? home : ".");
	}
	for (p = dir; *p; p++)
		if (*p == '\\') *p = '/';
	mkdirp_impl(dir);
	snprintf(buf, len, "%s/dd2.cfg", dir);
}
#endif

/* load all the sound stuff */
void
soundLoad()
{
	int i;
	char buffer[512];

	sprintf(buffer,"%s/bgm1.xm",DD2_DATA);
	bgm=Mix_LoadMUS(buffer);
	if(!bgm)
		LOG("Unable load bgm: %s\n", SDL_GetError());

	sprintf(buffer,"%s/bgm2.xm",DD2_DATA);
	bgm_boss=Mix_LoadMUS(buffer);
	if(!bgm_boss)
		LOG("Unable load bgm_boss: %s\n", SDL_GetError());

	for(i=0;i<NUM_EFX;i++) {
		sprintf(buffer,"%s/efx%i.wav",DD2_DATA,i+1);
		efx[i]=Mix_LoadWAV(buffer);
		if(!efx[i]) {
			LOG("Unable load efx%d: %s\n", i+1, SDL_GetError());
		} else
			Mix_VolumeChunk(efx[i],MIX_MAX_VOLUME/2);
	}
}

void
gameLoop()
{
	int afterdeath=0;

	for(done=false,tick=SDL_GetTicks();!done && afterdeath<400;) {

		while(SDL_PollEvent(&event)) {
			if (event.type==SDL_QUIT)
				done=true;

			/* Alt+Enter: toggle fullscreen */
			if(event.type==SDL_KEYDOWN
				&& event.key.keysym.sym==SDLK_RETURN
				&& (event.key.keysym.mod & KMOD_ALT)) {
				DD2_ToggleFullscreen();
				continue;
			}

			/* Ctrl+X: exit immediately */
			if(event.type==SDL_KEYDOWN
				&& event.key.keysym.sym==SDLK_x
				&& (event.key.keysym.mod & KMOD_CTRL)) {
				exit(0);
			}

			/* joystick pause button */
			if(player[0].joy && joy[0])
			{
				SDL_JoystickUpdate();
				if(SDL_JoystickGetButton(joy[0], 1))
				{
					event.type=SDL_KEYDOWN;
					event.key.keysym.sym=SDLK_p;
				}
			}
			else
				if(player[1].joy && joy[1])
				{
					SDL_JoystickUpdate();
					if(SDL_JoystickGetButton(joy[1], 1))
					{
						event.type=SDL_KEYDOWN;
						event.key.keysym.sym=SDLK_p;
					}
				}

			if(event.type==SDL_KEYDOWN) {
				if(event.key.keysym.sym==SDLK_ESCAPE) {
					done=true;
					continue;
				}
				else
				{
					if(event.key.keysym.sym==SDLK_p && pause_tick<SDL_GetTicks())
					{
						writeCString(gfx, screen, 98, 20, "game paused", 0);
						DD2_Flip();
						pause=pause ? false : true;
						pause_tick=SDL_GetTicks()+200;
						continue;
					}
					else
						if(event.key.keysym.sym==SDLK_F12)
							SDL_SaveBMP(screen,"scnshot.bmp");
				}
			}
		}

		/* player control */
		if(player[0].shield) {
			if(joy[0] && player[0].joy)
				control_player_joy(joy[0],&player[0]);
			else
				control_player(&player[0]);
		}

		if(player[1].shield) {
			if(joy[1] && player[1].joy)
				control_player_joy(joy[1],&player[1]);
			else
				control_player(&player[1]);
		}

		if(pause)
			continue;

		/* frame rate calculation */
		ntick=SDL_GetTicks();
		if(ntick-tick>=1000/FPS) {
			tick=ntick;

			/* scroll here */
			{
				SDL_Rect a,b;

				if(scroll>0)
					scroll-=0.5;
				else
					scroll=200;

				b.x=1;
				b.w=SCREENW;
				a.x=0;

				if(!scroll) {
					a.y=0;
					b.y=204;
					b.h=SCREENH;
					SDL_BlitSurface(gfx, &b, screen, &a);
				} else {
					a.y=0;
					b.y=204+(int)scroll;
					b.h=SCREENH-(int)scroll;
					SDL_BlitSurface(gfx, &b, screen, &a);
					a.y=SCREENH-(int)scroll;
					b.y=204;
					b.h=(int)scroll;
					SDL_BlitSurface(gfx, &b, screen, &a);
				}

				/* scroll parallax here */

				if(scroll2>0)
					scroll2-=2;
				else
					scroll2=200;

				b.x=324;
				b.w=25;
				a.x=0;

				if(!scroll2) {
					a.y=0;
					b.y=204;
					b.h=SCREENH;
					SDL_BlitSurface(gfx, &b, screen, &a);

					b.x=358;
					a.x=SCREENW-25;
					SDL_BlitSurface(gfx, &b, screen, &a);
				} else {
					a.y=0;
					b.y=204+(int)scroll2;
					b.h=SCREENH-(int)scroll2;
					SDL_BlitSurface(gfx, &b, screen, &a);
					a.y=SCREENH-(int)scroll2;
					b.y=204;
					b.h=(int)scroll2;
					SDL_BlitSurface(gfx, &b, screen, &a);

					b.x=358;
					a.x=SCREENW-25;
					a.y=0;
					b.y=204+(int)scroll2;
					b.h=SCREENH-(int)scroll2;
					SDL_BlitSurface(gfx, &b, screen, &a);
					a.y=SCREENH-(int)scroll2;
					b.y=204;
					b.h=(int)scroll2;
					SDL_BlitSurface(gfx, &b, screen, &a);
				}
			}
			/* enemy here */
			engine_enemy();

			/* fire here */
			engine_fire();

			/* character here */
			if(player[0].shield)
				engine_player(&player[0]);

			if(player[1].shield)
				engine_player(&player[1]);

			if(!(player[0].shield | player[1].shield))
				afterdeath++;

			engine_obj();

			engine_vefx();

			/* panel */
			drawPanel(gfx,screen,player);

			DD2_Flip();
		} else {
			SDL_Delay(1);
		}
	}
}

int
main (int argc, char *argv[])
{
	int i,j,k;
	char buffer[512];

#ifndef WIN32
	if(argc==2)
		if(argv[1][0]=='-' && argv[1][1]=='v') {
			printf("%s v%s\nCopyright (c) 2003,2004 Juan J. Martinez <jjm@usebox.net>\n", PACKAGE, VERSION);
			printf("This is free software, and you are welcome\nto redistribute it"
				   " under certain conditions; read COPYING for details.\n");
			return 1;
		}

#ifdef __OS2__
	logfp=fopen(DD2_DATA "/dd2.log","w");
	if(!logfp) logfp=fopen("dd2.log","w");
	LOG("DD2 starting\n");

	dd2_config_path(buffer, sizeof(buffer));
	if(!loadCFG(buffer,&conf)) {
		sprintf(buffer,"%s/dd2.cfg",DD2_DATA);
		if(!loadCFG(buffer,&conf))
			fprintf(stderr,"unable to read configuration, using defaults\n");
	}
#else
	/* try local configuration */
	sprintf(buffer,"%.500s/.dd2rc",getenv("HOME"));
	if(!loadCFG(buffer,&conf)) {
		/* if there's no local, use global */
		sprintf(buffer,"%s/dd2.cfg",DD2_DATA);
		if(!loadCFG(buffer,&conf))
			fprintf(stderr,"unable to read configuration, using defaults\n");
	}
#endif
#else
	sprintf(buffer,"%s/dd2.cfg",DD2_DATA);
	if(!loadCFG(buffer,&conf))
		fprintf(stderr,"unable to read configuration, using defaults\n");
#endif

	/* read hi-scores */
	sprintf(buffer,"%s/dd2-hiscore",DD2_DATA);
	if(!loadScore(buffer,hiscore))
		fprintf(stderr,"unable to read hi-scores, using defaults\n");

	/* always init video and joystick first */
	i=SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK);
	if(i<0) {
		fprintf(stderr,"Unable to init SDL: %s\n", SDL_GetError());
		return 1;
	}
	atexit(SDL_Quit);

	/* init audio separately so its failure doesn't kill the game */
	sound=0;
	if(conf.sound!=NO_SOUND) {
		LOG("[audio] conf.sound=%d\n", conf.sound);
		if(SDL_InitSubSystem(SDL_INIT_AUDIO)<0) {
			LOG("[audio] SDL_InitSubSystem failed: %s\n", SDL_GetError());
		} else {
			static const int freqs[]={44100,22050,16000,0};
			int fi=0;
			switch(conf.sound) {
				default:
				case SOUND_HI:  fi=0; break;
				case SOUND_MED: fi=1; break;
				case SOUND_LOW: fi=2; break;
			}
			for(; freqs[fi]; fi++) {
				LOG("[audio] trying Mix_OpenAudio(%d)\n", freqs[fi]);
				if(Mix_OpenAudio(freqs[fi], MIX_DEFAULT_FORMAT, 2, 2048)>=0) {
					sound=1;
					LOG("[audio] Mix_OpenAudio(%d) OK\n", freqs[fi]);
					soundLoad();
					LOG("[audio] soundLoad done, efx[0]=%p bgm=%p\n",(void*)efx[0],(void*)bgm);
					break;
				}
				LOG("[audio] Mix_OpenAudio(%d) failed: %s\n", freqs[fi], SDL_GetError());
			}
			if(!sound)
				LOG("[audio] no working audio frequency found\n");
		}
	}

	/* create SDL2 window at 1024x768 minimum */
	{
		int winW = 1024, winH = 768;
		window = SDL_CreateWindow(APP_NAME,
		                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		                          winW, winH,
		                          SDL_WINDOW_RESIZABLE);
		if (!window) {
			fprintf(stderr, "Unable to create window: %s\n", SDL_GetError());
			return 1;
		}
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
		if (!renderer) {
			renderer = SDL_CreateRenderer(window, -1, 0);
			if (!renderer) {
				fprintf(stderr, "Unable to create renderer: %s\n", SDL_GetError());
				return 1;
			}
		}
		SDL_RenderSetLogicalSize(renderer, SCREENW, SCREENH);
		screen = SDL_CreateRGBSurface(0, SCREENW, SCREENH, 16,
		                              0xF800, 0x07E0, 0x001F, 0);
		if (!screen) {
			fprintf(stderr, "Unable to create screen surface: %s\n", SDL_GetError());
			return 1;
		}
		screen_texture = SDL_CreateTexture(renderer,
		                                   SDL_PIXELFORMAT_RGB565,
		                                   SDL_TEXTUREACCESS_STREAMING,
		                                   SCREENW, SCREENH);
		if (!screen_texture) {
			fprintf(stderr, "Unable to create screen texture: %s\n", SDL_GetError());
			return 1;
		}
		if (conf.fullscreen)
			SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}

	/* init the joystick */
	if(SDL_WasInit(SDL_INIT_JOYSTICK) & SDL_INIT_JOYSTICK)
		if(SDL_NumJoysticks()>=1)
		{
			joy[0]=SDL_JoystickOpen(0);
			if(SDL_NumJoysticks()>1)
				joy[1]=SDL_JoystickOpen(1);
		}

	/* hide the mouse */
	SDL_ShowCursor(SDL_DISABLE);

	/* load console gfx */
	sprintf(buffer,"%s/gfx.bmp",DD2_DATA);
	gfx=loadBMP(buffer);
	if(!gfx) {
		fprintf(stderr,"Unable load gfx: %s\n", SDL_GetError());
		return 1;
	}
	/* set transparent color */
	if(SDL_SetColorKey(gfx, SDL_TRUE, SDL_MapRGB(gfx->format, 255, 0, 255))<0) {
		fprintf(stderr,"Unable to setup gfx: %s\n", SDL_GetError());
		return 1;
	}

	/* main LOOP */
	while(menu()) {

		/* init the engine */
		engine_init();

		SDL_FillRect(screen,NULL,SDL_MapRGB(screen->format,0,0,0));
		DD2_Flip();

		if(sound && bgm) {
			Mix_VolumeMusic(MIX_MAX_VOLUME);
			Mix_FadeInMusic(bgm,-1,2000);
			SDL_Delay(2000);
		}

		player[0].joy=(int)conf.control[0]==JOYSTICK;
		player[1].joy=(int)conf.control[1]==JOYSTICK;

		pause=0;
		pause_tick=0;
		boss=0;
		gameLoop();

		if(sound && bgm) {
			Mix_FadeOutMusic(2000);
			SDL_Delay(3000);
		}

		for(i=0;i<2;i++) {
			/* check if there's a place for this score */
			for(j=9;j>=0 && hiscore[j].score<player[i].score;j--);

			/* the player will be in the hall of fame? */
			if(j<9) {
				for(k=8;k>j;k--)
					hiscore[k+1]=hiscore[k];

				/* put the new score */
				hiscore[j+1].score=player[i].score;
				hiscore[j+1].stage=player[i].stage;

				hiscore[j+1].name[0]=0;
				if(!getName(hiscore[j+1].name, j+2,i+1))
					break; /* probably a problem if the user closes the window */

				/* show the hall of fame */
				hiscores();
			}
		}
	}

	if(sound) {
		if(bgm)
			Mix_FreeMusic(bgm);
		if(bgm_boss)
			Mix_FreeMusic(bgm_boss);

		for(i=0;i<NUM_EFX;i++)
			if(efx[i])
				Mix_FreeChunk(efx[i]);

		Mix_CloseAudio();
	}

	/* release the joystick */
	if(joy[0] && SDL_JoystickGetAttached(joy[0]))
		SDL_JoystickClose(joy[0]);
	if(joy[1] && SDL_JoystickGetAttached(joy[1]))
		SDL_JoystickClose(joy[1]);

	/* free all! */
	SDL_FreeSurface(gfx);
	SDL_FreeSurface(screen);
	SDL_DestroyTexture(screen_texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	/* free engine memory */
	engine_release();

	/* now update conf changes */
#ifdef WIN32
	sprintf(buffer,"%s/dd2.cfg",DD2_DATA);
#elif defined(__OS2__)
	dd2_config_path(buffer, sizeof(buffer));
#else
	/* save conf into local */
	sprintf(buffer,"%.500s/.dd2rc",getenv("HOME"));
#endif
	saveCFG(buffer,&conf);

	/* save hi-scores */
	sprintf(buffer,"%s/dd2-hiscore",DD2_DATA);
	if(!saveScore(buffer,hiscore))
		fprintf(stderr,"unable to save hi-scores\ndo you have permissions to write into %s?\n"
			,buffer);

	return 0;
}
