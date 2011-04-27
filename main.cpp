#include <cstdlib>
#include <string>.
#include <vector>
#include <time.h>
#include <fstream>
#include <sstream>
#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
#include </usr/include/mysql/mysql.h>

#include "font.h"

#define BOX_W	100
#define BOX_H	120
#define BOX_Z	80
#define BOX_nX	80
#define BOX_nY	100
#define BOX_sX	200
#define BOX_sY	50
#define SCREENW 800
#define SCREENH 600
#define L_X		34
#define L_Y		40

#define LAN true
#define DEBUG(stra) debug("%i:%s\n", (void*)__LINE__, (void*)stra);

#define DBTABLE "data"
#define DOORSCRIPT "./ovi.sh"
#define DBHOST1 "dalek.local"
#define DBHOST2 "87.94.164.79"


enum MODE { NUM, ACT };

float mReload = 50000;
using namespace std;
string key;
SDL_Event event;
MODE mode = NUM;
string lenghts[10] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
time_t sec, startSec, reloadSec;
bool running = true, pressed = true;
int mp = 0, mx = 0, my = 0, mpk = 0;
string val, nkey, query;
int reload = 0;

MYSQL *connection, mysql;
MYSQL_RES *result;
MYSQL_ROW row;
int query_state;
bool conSuccess = true;

vector<string> keys;

template <class T>
inline std::string toString (const T& t)
{
    std::stringstream ss;
    ss << t;
    return ss.str();
}

void debug (string str){
	printf(str.c_str());
	if (str.compare("\n")) printf("\n");
}
void debug (char *str){
	debug(string(str));
}
void debug (string str, void* val){
	char strg[512];
	sprintf (strg, str.c_str(), val);
	debug(strg);
}
void debug (string str, void* val, void* val2){
	char strg[512];
	sprintf (strg, str.c_str(), val, val2);
	debug(strg);
}

string rand_str_array(string arr) {
	int x = arr.length(), y;
	string temp;
	srand(time(0));
	while(x!=0){
	int y=rand()%x;
		temp+=arr[y];
		arr[y]=arr[x-1];
		arr=arr.substr(0,(x-1));
		x--;
	}
	return temp;
}

int main ( int argc, char** argv ) {
	atexit(SDL_Quit);
	if ( SDL_Init( SDL_INIT_VIDEO ) < 0 ) exit(-1);
	string vals("0123456789abcdef");
	vals = rand_str_array(vals);
	SDL_Surface* screen = SDL_SetVideoMode(SCREENW, SCREENH, 16, SDL_HWSURFACE|SDL_DOUBLEBUF);
	if ( !screen ) exit(-1);
	debug("Alustetaan ohjelmaa");
	SDL_SetCursor(NULL);
	debug("Kursori vaihdettu");

	debug("Alustetaan MySQL-yhteyttä");
	mysql_init(&mysql);
	debug("Haetaan yhteyttä sijaintiin %s", (void*)DBHOST1);
	connection = mysql_real_connect(&mysql,DBHOST1,DBUSER,DBPASSWD,DBTABLE,0,0,0);
	if (connection == NULL) {
		debug("%s", (void*)mysql_error(&mysql));
		debug("Yritetään uuteen sijaintiin %s", (void*)DBHOST2);
		mysql_kill(&mysql, 0);
		debug("Tapettiin vanha yhteys");
		mysql_init(&mysql);
		debug("Alustetaan yhteyttä uudestaan");
		connection = mysql_real_connect(&mysql,DBHOST2,DBUSER,DBPASSWD,DBTABLE,0,0,0);
		if (connection == NULL) {
			conSuccess = false;
			printf("error, %i: %s\n", __LINE__, mysql_error(&mysql));
		}
	}
	debug("Yhdistettu MySQL-tietokantaan");

	debug("Luetaan avaimia");
	if (conSuccess) {
		debug("Noudetaan uusia avaimia");
		ofstream keysFile;
		keysFile.open("./keys.dat");
		debug("Noudetaan ulkoisesta tietokannasta");
		if (mysql_query(&mysql, "SELECT * FROM doorUsers")) {
			debug("Ei voitu noutaa avaimia");
			printf("error, %i: %s\n", __LINE__, mysql_error(&mysql));
		}
		result = mysql_use_result(&mysql);
		while ((row = mysql_fetch_row(result)) != NULL) {
			keys.push_back(string(row[1]));
			keysFile << string(row[1]).c_str() << "\n";
		}
		debug("Avaimet sijoitettu paikalliseen tiedostojärjestelmään");
		keysFile.close();
	} else {
		debug("Noudetaan paikallisesta tiedostojärjestelmästä");
		ifstream keysFile;
		keysFile.open("./keys.dat");
		string line;
		if (keysFile.is_open()) {
			while ( keysFile.good() ) {
				getline (keysFile,line);
				if (line.length()>3) keys.push_back(line);
			}
		}
		keysFile.close();
	}

	gfxPrimitivesSetFont(SDL_gfx_font_10x20_fnt,10,20);
	debug("Fontti vaihdettu");
	startSec = reloadSec = time (NULL);
	for (int i = 1; running == true; reload++, i++) {
		sec = time (NULL);
		SDL_PollEvent(&event);
		mp = SDL_GetMouseState(&mx, &my);
		if (mp && !mpk) mpk = 2;
		else if (mpk==2) mpk = 1;
		else if (!mp && mpk==1) mpk = 0;

		usleep(200);
		if (sec-reloadSec >= 10) {
			reloadSec = time(NULL);
			mode = NUM;
			reload = 0;
			pressed = true;
			mp = 0;
			mpk = 0;
			vals = rand_str_array(vals);
		}

		if (mpk == 2 || pressed == true) {
			SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, 0, 0, 0));
			pressed = false;
			if (mode == ACT) {
				key.clear();

				if (my>10 && my < 10+BOX_Z&& mpk == 2) {
					debug("Vaihdetaan tilaa");
					system(DOORSCRIPT);
					if (conSuccess) {
						query = string("INSERT INTO door (time,user) VALUES ('");
						query += toString(time(NULL));
						query += "','";
						query += toString(nkey);
						query += "')";
						debug("Suoritetaan kysely '%s'", (void*)query.c_str());
						if (mysql_query(&mysql, query.c_str())) {
							debug("Tietokantaan ei saatu yhteyttä");
						}
					}
				}
				boxRGBA(screen, 0, 10, SCREENW, 10+BOX_Z,128,128,128,255);
				stringRGBA(screen, 30, 30 , "Vaihda oven tilaa", 0, 0, 0, 255);

				if (my>20+BOX_Z && my < 20+2*BOX_Z && mpk == 2) {
					debug("Palataan numeronäkymään");
					mode = NUM;
					pressed = false;
					mpk = 0;
					mp = 0;
				}
				boxRGBA(screen, 0, 20+BOX_Z, SCREENW, 20+2*BOX_Z,128,128,128,255);
				stringRGBA(screen, 30, 30+(BOX_Z+10) , "Lopeta", 0, 0, 0, 255);

				if (my>30+2*BOX_Z && my < 30+3*BOX_Z && mpk == 2) {
					debug("Poistutaan ohjelmasta");
					return 0;

				}
				boxRGBA(screen, 0, 30+2*BOX_Z, SCREENW, 30+3*BOX_Z,128,128,128,255);
				stringRGBA(screen, 30, 30+(20+2*BOX_Z) , "Sammuta", 0, 0, 0, 255);

			} else {
				for (int x = 0; x < 4; x++){
					for (int y = 0; y < 4; y++){
						val.clear();
						val = vals.substr(x*4+y,1);
						if (!pressed && (mx>BOX_sX+x*BOX_W && mx < BOX_sX+BOX_nX+(x)*BOX_W && my>BOX_sY+y*BOX_H && my < BOX_sY+BOX_nY+(y)*BOX_H) && mpk == 2) {
							key.push_back(val.at(0));
							reload = 0;
							pressed = true;
						}
					}
				}
				if (key.size()>=8 || !(mx+20>BOX_sX&&mx-20< BOX_sX+BOX_nX+3*BOX_W&&my+20>BOX_sY&& my-20 < BOX_sY+BOX_nY+3*BOX_H)) {
					vals = rand_str_array(vals);
					key.clear();
				}
				if (pressed) {
					vals = rand_str_array(vals);
				}
				for (int x = 0; x < 4; x++){
					for (int y = 0; y < 4; y++){
						val.clear();
						val = vals.substr(x*4+y,1);
						if (!pressed && (mx>BOX_sX+x*BOX_W && mx < BOX_sX+BOX_nX+(x)*BOX_W && my>BOX_sY+y*BOX_H && my < BOX_sY+BOX_nY+(y)*BOX_H) && mpk == 2) {
							boxRGBA(screen, BOX_sX+x*BOX_W, BOX_sY+y*BOX_H, BOX_sX+x*BOX_W+BOX_nX, BOX_sY+y*BOX_H+BOX_nY,0,255,0,255);
						} else boxRGBA(screen, BOX_sX+x*BOX_W, BOX_sY+y*BOX_H, BOX_sX+x*BOX_W+BOX_nX, BOX_sY+y*BOX_H+BOX_nY,128,128,128,255);
						stringRGBA(screen, BOX_sX+L_X+x*BOX_W, BOX_sY+L_Y+y*BOX_H, val.c_str(), 0, 0, 0, 255);
					}
				}
				stringRGBA(screen, 10, SCREENH-30, lenghts[key.size()].c_str(), 255,255,255, 255);
				for (int e = 0; e < keys.size(); e++) {
					if (!keys[e].compare(key)) {
						printf("Koodi löydetty: %s!\n", key.c_str());
						nkey = key;
						key.clear();
						mode = ACT;
						reloadSec = time(NULL);
					}
				}
			}
			SDL_Flip(screen);
		}
	}
	mysql_free_result(result);
	mysql_close(connection);
	DEBUG("Poistutaan ohjelmasta");
	exit(1);
	return 0;
}

