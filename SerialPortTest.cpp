// Test hardware handshake lines on serial port
// g++ -lncurses -o SerialPortTest SerialPortTest.cpp
//

#include <ctype.h>
#include <dirent.h>
#include <math.h>  
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h> 
#include <termios.h>
#include <time.h>   
#include <unistd.h>

class MyPort{
private:
  FILE * serial_io;
  struct termios new_tio;
public:
  void OpenPort(char * port,char * message ){
    if(serial_io != NULL)ClosePort();
    serial_io = fopen(port, "r+");
    if(serial_io==NULL){
      strcpy(message,"Cannot open serial port");
    } else {
      if (0!=flock(fileno(serial_io),LOCK_EX|LOCK_NB)){
	strcpy(message,"Serial IO port locked");
	ClosePort();
      } else {
	if(0!= fcntl(fileno(serial_io), F_SETFL, O_RDWR | O_NONBLOCK )){
	  strcpy(message,"Serial IO port cannot set flags.");
	  ClosePort();
	} else {
	  cfmakeraw(&new_tio);
	  // new_tio.c_cflag = B9600 | CS8 | CSTOPB | CLOCAL | CREAD ; 
	  new_tio.c_cflag = B9600 | CS8 | CSTOPB | CREAD ; 
	  tcflush(fileno(serial_io), TCIFLUSH);
	  tcsetattr(fileno(serial_io),TCSANOW,&new_tio);
	  strcpy(message,"Serial IO port open");
	} 
      }
    }
  }
  
  bool ReadPort(char * NewString){
    int j=0;
    if(IsPortOpen())j=fread(NewString,1,60,serial_io);
    NewString[j]=0;
    return(j>0);
  }
  void WritePort(char * NewString){
    int i;
    if(IsPortOpen()){
      for (i=0;i<strlen(NewString);i++)fputc(NewString[i],serial_io);
      fputc('\n',serial_io);
      fflush(serial_io);
    }
  }
  void ClosePort(){
    if(IsPortOpen()){
      fclose(serial_io);
      serial_io=NULL;
    }
  }
  // man ioctl_tty
  void SetDTR(bool NewState){
    int DTR_flag;
    int fd;
    if(IsPortOpen()){
      fd=fileno(serial_io);
      DTR_flag = TIOCM_DTR;
      if(NewState){
	ioctl(fd,TIOCMBIS,&DTR_flag);
      } else {
	ioctl(fd,TIOCMBIC,&DTR_flag);
      }
    }
  }
  void SetRTS(bool NewState){
    int RTS_flag;
    int fd;
    if(IsPortOpen()){
      fd=fileno(serial_io);
      RTS_flag = TIOCM_RTS;
      if(NewState){
	ioctl(fd,TIOCMBIS,&RTS_flag);
      } else {
	ioctl(fd,TIOCMBIC,&RTS_flag);
      }
    }
  }
  bool GetCTS(){
    int fd;
    int SerialState;
    if(IsPortOpen()){
      fd=fileno(serial_io);
      ioctl(fd, TIOCMGET, &SerialState);
      return(SerialState & TIOCM_CTS);
    } else return(false);
  }
  bool GetDCD(){
    int fd;
    int SerialState;
    if(IsPortOpen()){
      fd=fileno(serial_io);
      ioctl(fd, TIOCMGET, &SerialState);
      return(SerialState & TIOCM_CAR);
    } else return(false);
  }
  bool GetRI(){
    int fd;
    int SerialState;
    if(IsPortOpen()){
      fd=fileno(serial_io);
      ioctl(fd, TIOCMGET, &SerialState);
      return(SerialState & TIOCM_RNG);
    } else return(false);
  }
  bool GetDTR(){
    int fd;
    int SerialState;
    if(IsPortOpen()){
      fd=fileno(serial_io);
      ioctl(fd, TIOCMGET, &SerialState);
      return(SerialState & TIOCM_DTR);
    } else return(false);
  }
  bool GetRTS(){
    int fd;
    int SerialState;
    if(IsPortOpen()){
      fd=fileno(serial_io);
      ioctl(fd, TIOCMGET, &SerialState);
      return(SerialState & TIOCM_RTS);
    } else return(false);
  }
  bool IsPortOpen(){
    return(serial_io!=NULL);
  }
  MyPort():serial_io(NULL){}
  ~MyPort(){}
}MyPort;

#define MINIMUM_ROW 20
#define MINIMUM_COL 60
class CurseWindow{
private:
  const char *menu_choices[6] = { 
    "Select Port              ",
    "Display Mode (Ascii,Hex) ",
    "Toggle DTR               ",
    "Toggle RTS               ",
    "Output string            ",
    "Exit                     " };
  WINDOW *upper_window_border;
  WINDOW *lower_window_border;
  WINDOW *upper_window;
  WINDOW *lower_window;
  WINDOW *CreateNewWindow(int height, int width, int starty, int startx,bool border){
    WINDOW *local_win;
    local_win = newwin(height, width, starty, startx);
    if(border){
      box(local_win, 0 , 0);
    } else {
      scrollok(local_win,TRUE);
    }
    wrefresh(local_win);	  
    return (local_win);
  }
  int row,col;
  bool FrameChanged(bool update){
    int r,c;
    bool newsize=false;
    getmaxyx(stdscr, r, c);
    if((r!=row)|(c!=col)){
      if(update){
	row=r;
        col=c;
      }
      newsize=true;
    }
    return(newsize);
  }
  bool FrameReset(){
    bool too_small;
    InitCurse=true;
    initscr();
    clear(); 
    noecho();
    refresh();
    keypad(stdscr, TRUE);
    FrameChanged(true);
    too_small= (row<MINIMUM_ROW)|(col<MINIMUM_COL);
    if(too_small)printf("Screen too small\n");
    return(!too_small);
  }
#define QUERY_COL 4
#define VAL_COL 28
  void QueryMenu(const char * query,char * reply){
    int msize;
    msize=sizeof(menu_choices[0])-1;
    echo();
    mvwprintw(lower_window,msize-1,QUERY_COL,query);
    wscanw(lower_window,"%s",reply);  
    wrefresh(lower_window);
    mvwprintw(lower_window,msize-1,QUERY_COL,"                                    ");
    noecho(); 
    wrefresh(lower_window);
  }
  void LogWindowMessage(char * t,char * s){
    wprintw(upper_window,"%s%s\n",t,s);
    wrefresh(upper_window); 
  }
  void LogWindowMessageHex(char * t,char * s){
    int i,j;
    wprintw(upper_window,"%s",t);
    for(i=0;i<strlen(s);i++) wprintw(upper_window,"[%02x]",s[i]);
    wprintw(upper_window,"\n");
    wrefresh(upper_window); 
  }
  
  time_t TimeNow;
  bool CheckTimeChanged(){
    time_t old_time;
    old_time=TimeNow;
    time(&TimeNow);
    return(old_time!=TimeNow);
  }
  int MenuSelected;
  int DisplayMode;
  char SerialPort[20];
  void UpdateMenuWindow(){
    int i,msize;
    bool reverse;
    char disp_mode[20];
    char text_time[20];
    msize=sizeof(menu_choices[0])-1;
    scrollok(lower_window,FALSE);
    keypad(lower_window,TRUE);
    for(i = 0; i < (msize-1); ++i){
      if(reverse=(MenuSelected == (i + 1))) wattron(lower_window, A_REVERSE); 
      mvwprintw(lower_window, i, 2, "%s", menu_choices[i]);
      if(reverse)wattroff(lower_window, A_REVERSE);	
    }
    switch(DisplayMode){
    default:
    case 0:
      strcpy(disp_mode,"ASCII");
      break;
    case 1:
      strcpy(disp_mode,"HEX");
      break;
    }
    strftime(text_time,20,"%H:%M:%S",gmtime(&TimeNow));
    mvwprintw(lower_window, 0,VAL_COL, "= %s  %s     ",
	      SerialPort,
	      (char*)(MyPort.IsPortOpen()?"[Open]":"[Closed]")); 
    mvwprintw(lower_window, 1,VAL_COL, "= %s  ",disp_mode); 
    mvwprintw(lower_window, 2,VAL_COL, "= %d  ",(MyPort.GetDTR()?1:0));
    mvwprintw(lower_window, 3,VAL_COL, "= %d  ",(MyPort.GetRTS()?1:0)); 
    mvwprintw(lower_window, 5,VAL_COL-10, "  %s  ",text_time); 
    wrefresh(lower_window);
  }
  void ShowHandshake(){
    char cts[7],dcd[7],ri[7];
    if(MyPort.GetCTS()){
      strcpy(cts,"CTS");
    } else {
      strcpy(cts,"cts");
    }
    if(MyPort.GetDCD()){
      strcpy(dcd,"DCD");
    } else {
      strcpy(dcd,"dcd");
    }
    if(MyPort.GetRI()){
      strcpy(ri,"RI");
    } else {
      strcpy(ri,"ri");
    }
    mvwprintw(lower_window,5,VAL_COL+3," %s %s %s     ",cts,dcd,ri);
    wrefresh(lower_window);
  }
  void NextMenu(int step){
    MenuSelected+=step;
    int msize;
    msize=sizeof(menu_choices[0])-1;
    if(MenuSelected<1)MenuSelected=msize-1;
    if(MenuSelected>(msize-1))MenuSelected=1;
    UpdateMenuWindow();
  }
  bool CheckMenuSelected(){
    bool chosen=false;
    halfdelay(10);
    switch(wgetch(lower_window)) {
    case KEY_UP:
      NextMenu(-1);
      break;
    case KEY_DOWN:
      NextMenu(1);
      break;
    case 10:
      chosen=true;       
      break;
    default:   
      break;
    }
    return(chosen);
  }
  bool InitCurse;
  bool WindowOK;
 public:
  bool OpenWindow(){
    bool redraw=false;
    int msize;
    msize=sizeof(menu_choices[0])-1;
    if(!InitCurse){
      WindowOK=FrameReset();
      redraw=true;
    } else {
      if(FrameChanged(false)){
        WindowOK=FrameReset();
	redraw=true;
      }
    }
    if(WindowOK & redraw){
        upper_window_border = CreateNewWindow(row-(msize+2) , col    , 0             , 0   , true);
        upper_window  = CreateNewWindow(row-(msize+4) , col-2  , 1             , 1   , false);
        lower_window_border = CreateNewWindow(msize+2       , col    , row-(msize+2) , 0   , true);
        lower_window  = CreateNewWindow(msize         , col-2  , row-(msize+1) , 1   , false);
    }
    UpdateMenuWindow();
    return(WindowOK);
  }
  bool CheckInputs(){
    char newstring[50];
    char qstring[70];
    bool dont_quit=true;
    while(!OpenWindow());
    strcpy(newstring,"");
    ShowHandshake();
    if(CheckTimeChanged()){
      UpdateMenuWindow();
    } else {
      if(MyPort.ReadPort(newstring)){
	LogWindowMessage((char *)"< ",newstring);
      } else {
	if(CheckMenuSelected()){
	  switch(MenuSelected){
	  default:
	    break;
	  case 1:
	    sprintf(qstring,"New port ID [%s] ?",SerialPort);
	    QueryMenu(qstring,newstring);
	    if(0!=strcmp(newstring,"")){
	      strcpy(SerialPort,newstring);
	      LogWindowMessage((char *)"# new serial port = ",SerialPort);
	      MyPort.OpenPort(SerialPort,newstring);
	    } else {
	      if(MyPort.IsPortOpen()){
		MyPort.ClosePort();
		strcpy(newstring,"Port closed");
	      } else {
		MyPort.OpenPort(SerialPort,newstring);
	      }
	    }
	    LogWindowMessage((char *)"# ",newstring);
	    break;
	  case 2:
	    if(DisplayMode>=1){
	      DisplayMode=0;
	    } else {
	      DisplayMode++;
	    }
	    break;
	  case 3:
	    MyPort.SetDTR(!MyPort.GetDTR());
	    LogWindowMessage((char *)"> ",(char *)(MyPort.GetDTR()?"DTR +":"DTR -"));
	    break;
	  case 4:
	    MyPort.SetRTS(!MyPort.GetRTS());
	    LogWindowMessage((char *)"> ",(char *)(MyPort.GetRTS()?"RTS +":"RTS -"));
	    break;
	  case 5:
	    QueryMenu("Write string?",newstring);
	    switch(DisplayMode){
	    default:
	    case 0: 
	      LogWindowMessage((char *)"> ",newstring);
	      break;
	    case 1:
	      LogWindowMessageHex((char *)"> ",newstring);
	      break;
	    }
	    if(MyPort.IsPortOpen())MyPort.WritePort(newstring);
	    break;
	  case 6:
	    dont_quit=false;
	    break;
	  }
	  UpdateMenuWindow();
	}
      }
    }
    return(dont_quit);
  }
  CurseWindow():WindowOK(false),
		InitCurse(false),
		MenuSelected(1),
		DisplayMode(0),
		SerialPort("/dev/ttyUSB0"){
    initscr();
  }
  ~CurseWindow(){
    endwin();
  }
}CW;

int main(int argc, char *argv[]){
  int i;
  // open default serial port
  // remember port state
  // read data from port in check_menu_time routine
  while(CW.CheckInputs());
  endwin();
  printf("Build date %s, %s\n",__DATE__,__TIME__);
}
