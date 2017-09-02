/* This file is part of MysteryGiftDump

Copyright (C) 2017 Bernardo Giordano
	 
>    This program is free software: you can redistribute it and/or modify
>    it under the terms of the GNU General Public License as published by
>    the Free Software Foundation, either version 3 of the License, or
>    (at your option) any later version.
>
>    This program is distributed in the hope that it will be useful,
>    but WITHOUT ANY WARRANTY; without even the implied warranty of
>    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
>    GNU General Public License for more details.
>
>    You should have received a copy of the GNU General Public License
>    along with this program.  If not, see <http://www.gnu.org/licenses/>.
>    See LICENSE for information.
*/

#include <stdlib.h>
#include <stdio.h>
#include <3ds.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#include "graphic.h"
#include "util.h"

#define GAMES 6

void exitServices() {
	hidExit();
	srvExit();
	fsEnd();
	sdmcExit();
	aptExit();
	romfsExit();
	pp2d_exit();
}

void initServices() {
	aptInit();
	sdmcInit();
	romfsInit();
	fsStart();
	srvInit();
	hidInit();
	gfxInitDefault();
	pp2d_init();
	GUIElementsInit();
	
	mkdir("sdmc:/3ds", 777);
	mkdir("sdmc:/3ds/data", 777);
	mkdir("sdmc:/3ds/data/MysteryGiftDump", 777);
}

u16 wcx_get_id(u8* wcx) { 
	return *(u16*)(wcx); 
}

ssize_t wcx_get_title(u8* wcx, u8* dst) {
	u16 src[36];
	for (int i = 0; i < 36; i++)
		src[i] = *(u16*)(wcx + 0x2 + i*2);

	ssize_t len = utf16_to_utf8(dst, src, 72);
	return len;
}

void file_write(char* path, u8 *buf, int size) {
	FILE *file = fopen(path, "wb");
	fwrite(buf, 1, size, file);
	fclose(file);
}

void dumpMg2wcx(u8* mainbuf, int game) {
	char dmppath[100];
	time_t unixTime = time(NULL);
	struct tm* timeStruct = gmtime((const time_t *)&unixTime);		
	snprintf(dmppath, 100, "sdmc:/3ds/data/MysteryGiftDump/MGDump_%02i%02i%02i%02i%02i%02i", 
			timeStruct->tm_year + 1900, 
			timeStruct->tm_mon + 1, 
			timeStruct->tm_mday, 
			timeStruct->tm_hour, 
			timeStruct->tm_min, 
			timeStruct->tm_sec);
	mkdir(dmppath, 777);
	chdir(dmppath);
	
	int offset = 0;
	if (game == GAME_X || game == GAME_Y)
		offset = 0x1BD00;
	else if (game == GAME_OR || game == GAME_AS)
		offset = 0x1CD00;
	else if (game == GAME_SUN || game == GAME_MOON)
		offset = 0x65C00 + 0x100;
	
	u8 empty[264];
	memset(empty, 0, 264);
	
	char step[20];
	for (int i = 0, tot = game < 4 ? 24 : 48; i < tot; i++) {
		sprintf(step, "Step %d/%d", i + 1, tot);
		freezeMsg(step);
		
		u8 tmp[264];
		memcpy(tmp, mainbuf + offset + i*264, 264);
		if (memcmp(tmp, empty, 264)) {
			u8 title[72];
			char path[150];
			
			memset(title, 0, 72);
			memset(path, 0, 150);
			
			ssize_t len = wcx_get_title(tmp, title);
			if (len > 0 && wcx_get_id(tmp) != 145)
				sprintf(path, "#%d - %.*s.%s", (int)wcx_get_id(tmp), len, title, game < 4 ? "wc6" : "wc7");
			else
				sprintf(path, "#%d.%s", (int)wcx_get_id(tmp), game < 4 ? "wc6" : "wc7");

			file_write(path, tmp, 264);
		}
	}
}

int main() {
	u8* mainbuf;
	u64 mainSize = 0;
	int game = 0;
	const u64 ids[] = {0x0004000000055D00, 0x0004000000055E00, 0x000400000011C400, 0x000400000011C500, 0x0004000000164800, 0x0004000000175E00};
	
	initServices();
	
	Handle mainHandle;
	FS_Archive saveArch;
	while (aptMainLoop() && !(hidKeysDown() & KEY_A)) {
		hidScanInput();
		
		if (hidKeysDown() & KEY_B) {
			exitServices();
			return 0;
		}
		
		if (hidKeysDown() & KEY_LEFT) {
			if (game == 0) game = GAMES - 1;
			else if (game > 0) game--;	
		}
		
		if (hidKeysDown() & KEY_RIGHT) {
			if (game == GAMES - 1) game = 0;
			else if (game < GAMES - 1) game++;
		}
		
		gameSelectorMenu(game);
	}

	freezeMsg("Loading save...");
	if (!(openSaveArch(&saveArch, ids[game]))) {
		infoDisp("Game not found!");
		exitServices();
		return -1;
	}
	FSUSER_OpenFile(&mainHandle, saveArch, fsMakePath(PATH_ASCII, "/main"), FS_OPEN_READ | FS_OPEN_WRITE, 0);		
	FSFILE_GetSize(mainHandle, &mainSize);
	switch(game) {
		case GAME_X : { if (mainSize != 415232)    infoDisp("Incorrect size for this game!"); break; }
		case GAME_Y : { if (mainSize != 415232)    infoDisp("Incorrect size for this game!"); break; }
		case GAME_OR : { if (mainSize != 483328)   infoDisp("Incorrect size for this game!"); break; }
		case GAME_AS : { if (mainSize != 483328)   infoDisp("Incorrect size for this game!"); break; }
		case GAME_SUN : { if (mainSize != 441856)  infoDisp("Incorrect size for this game!"); break; }
		case GAME_MOON : { if (mainSize != 441856) infoDisp("Incorrect size for this game!"); break; }
		exitServices();
		return -1;
	}
	mainbuf = (u8*)malloc(mainSize);
	FSFILE_Read(mainHandle, NULL, 0, mainbuf, mainSize);
	
	dumpMg2wcx(mainbuf, game);
	infoDisp("Mystery Gifts dumped!");
	free(mainbuf);
	exitServices();
	return 0;
}