#include<stdio.h>
#include<locale.h>
#include<stdlib.h>
#include<ncurses.h>
#include<menu.h>
#include<pthread.h>
#include<unistd.h>
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]));

volatile int seconds_left = 60;
volatile int running = 1;
volatile int should_exit = 0;

const char *digit_glyphs[11][5] = {
    { " ██ ", "█  █", "█  █", "█  █", " ██ " },
    { "  █ ", " ██ ", "  █ ", "  █ ", " ███" },
    { " ██ ", "█  █", "  █ ", " █  ", "████" }, 
    { "███ ", "   █", " ██ ", "   █", "███ " },
    { "█  █", "█  █", "████", "   █", "   █" },
    { "████", "█   ", "███ ", "   █", "███ " },
    { " ██ ", "█   ", "███ ", "█  █", " ██ " },
    { "████", "   █", "  █ ", " █  ", " █  " },
    { " ██ ", "█  █", " ██ ", "█  █", " ██ " },
    { " ██ ", "█  █", " ███", "   █", " ██ " },
    { "    ", " ██ ", "    ", " ██ ", "    " }
};

void draw_digit(int digit, int row, int col){
  for(int i = 0; i<5; i++){
    mvprintw(row+i, col, "%s", digit_glyphs[digit][i]);
  }
}

void draw_time(int seconds_left, int row, int col){
    int minutes = seconds_left / 60;
    int secs = seconds_left % 60;

    int m1 = minutes / 10;
    int m2 = minutes % 10;
    int s1 = secs / 10;
    int s2 = secs % 10;

    int x = col;
    draw_digit(m1, row, x); x += 5;
    draw_digit(m2, row, x); x += 5;
    draw_digit(10, row, x); x += 5;
    draw_digit(s1, row, x); x += 5;
    draw_digit(s2, row, x);
}
char *options[] = {
      "Focus mode",
      "Short break",
      "Long break",
      "Exit"
      };
ITEM **my_items;
int n_choices, i, c;
MENU *my_menu;

void *countdown_thread() {
    while (seconds_left > 0 && running) {
        sleep(1);
        seconds_left--;
    }
    return NULL;
}

void set_timer(int duration){
  unpost_menu(my_menu);
  erase();
  refresh();
  running = 1;
  seconds_left = duration;
  pthread_t tid;
  pthread_create(&tid, NULL, countdown_thread, NULL);
  
  timeout(200);
  while(seconds_left > 0){
    erase();
    int cur_y, cur_x;
    getmaxyx(stdscr, cur_y, cur_x);
    int row = (cur_y - 5) / 2;
    int col = (cur_x - 25) / 2;
    draw_time(seconds_left, row, col);
    refresh();
    int ch = getch();
    if(ch == 'q'){ running = 0; break;}
  }
  pthread_join(tid, NULL);
  timeout(-1);
}

void focus_mode(void){set_timer(25*60);}
void short_break(void){set_timer(5*60);}
void long_break(void){set_timer(10*60);}
void exit_app(void){should_exit = 1;}

int main(){
  ITEM *cur_item;
  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();
    keypad(stdscr, TRUE);
  n_choices = ARRAY_SIZE(options);
  my_items = (ITEM**)calloc(n_choices + 1, sizeof(ITEM*)) ;
  void (*actions[])(void) = {focus_mode, short_break, long_break, exit_app};
  for(i = 0; i < n_choices; ++i){
	        my_items[i] = new_item(options[i], "");
          set_item_userptr(my_items[i], actions[i]);
  }
	my_items[n_choices] = (ITEM *)NULL;
	my_menu = new_menu((ITEM **)my_items);
	mvprintw(LINES - 2, 0, "F1 to Exit");
  int max_x, max_y;
  getmaxyx(stdscr, max_y, max_x);
  int menu_height = 4, menu_width = 20;
  int start_y = (max_y - menu_height) / 2;
  int start_x = (max_x - menu_width) / 2;

  WINDOW* menu_win = newwin(menu_height+2, menu_width+2, start_y, start_x);
  keypad(menu_win, true);
  box(menu_win, 0, 0);
  mvwprintw(menu_win, 0, 2, " Pomodoro ");
  set_menu_win(my_menu, menu_win);
  set_menu_sub(my_menu, derwin(menu_win, menu_height, menu_width, 1, 1));

  start_color();
  init_pair(1, COLOR_CYAN, COLOR_BLACK);
  init_pair(2, COLOR_BLACK, COLOR_CYAN);
  set_menu_fore(my_menu, COLOR_PAIR(2) | A_BOLD);
  set_menu_back(my_menu, COLOR_PAIR(1));
  set_menu_spacing(my_menu, 2, 1, 3);
  
	post_menu(my_menu);
      wnoutrefresh(stdscr);
    wnoutrefresh(menu_win);
    doupdate();

  while((c = getch()) != KEY_F(1)){
    switch(c){
        case KEY_RESIZE: {
          endwin();
          refresh();

          int new_max_y, new_max_x;
          getmaxyx(stdscr, new_max_y, new_max_x);

          int new_start_y = (new_max_y - menu_height) / 2;
          int new_start_x = (new_max_x - menu_width) / 2;
          
          unpost_menu(my_menu);
          wresize(menu_win, menu_height + 2, menu_width + 2);
          mvwin(menu_win, new_start_y, new_start_x);
          werase(menu_win);
          box(menu_win, 0, 0);
          mvwprintw(menu_win, 0, 2, " Pomodoro ");

          erase();
          mvprintw(LINES - 2, 0, "F1 to Exit");
          post_menu(my_menu);
          break;
      }
      case KEY_DOWN:
        menu_driver(my_menu, REQ_DOWN_ITEM);
        break;
      case KEY_UP:
        menu_driver(my_menu, REQ_UP_ITEM);
        break;
      case 10:
        case KEY_ENTER: {
          cur_item = current_item(my_menu);
          void (*action)(void) = item_userptr(cur_item);
          action();
          if(should_exit) goto cleanup;
          erase();
          post_menu(my_menu);
          wnoutrefresh(stdscr);
          wnoutrefresh(menu_win);
          doupdate();
          break;
      }
    }
    post_menu(my_menu);
    wnoutrefresh(stdscr);
    wnoutrefresh(menu_win);
    doupdate();
  }
 cleanup:
    unpost_menu(my_menu);
    free_menu(my_menu);
    for(i = 0; i < n_choices; ++i)
        free_item(my_items[i]);
    free(my_items);
    endwin();
    return 0;

}
