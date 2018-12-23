#include <sourcerer.h>

struct GameInfo_T * {
  
};

struct Environment_T * environment;

int main(int argc, char * argv[]) {
  environment = build_environment();
  parse_code(environment, "botidea.c");
  
  struct GameInfo_T * gameinfo;
  
  gameinfo = step_game();
  on_step(gameinfo);
  
}

int mark_new_change() {
  
}

int judge_change() {
                             
}

int on_step(struct GameInfo_T * game_info) {
  SOURCERER_FUNC_INIT("struct GameInfo_T*", game_info, "", NULL, "", NULL,"", NULL,"", NULL,"", NULL,"", NULL,"", NULL)
  
}
