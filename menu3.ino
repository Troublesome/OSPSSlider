/** 
  * Program to handle menu system and represent it on LCD display
  */

#include <glcd.h>
#include <fonts/allFonts.h>

#define STACK_SIZE 16

/**
  * define pins
  */
enum {
  button_down = 11,
  button_up = 16,
  button_enter = 12,
  button_back = 17
};


/**
  * stack implementation to keep track of menu state
  * @param default_value represents default state in the menu
  */
template <class T>
class Stack {

private:
  T stack[STACK_SIZE];
  int pos;

public:

  Stack(T default_val)
  {
    pos = -1;
    push(default_val);
  }

  void push (T value)
  {
    stack[++pos] = value;
  }

  T pop(void)
  {
    if (pos == 0)
      return stack[0];

    return stack[pos--];
  }

  T peek(void)
  {
    return stack[pos];
  }
};


/**
  * base class for gui representation on display (interface)
  */
class Gui
{
public:
  virtual void draw() = 0;
  virtual void keypress(int key) = 0;
};

extern Stack<Gui *> gui_stack;

struct MenuElement {
  char *name;
  Gui *submenu;
};

/**
  * Menu class implements Gui class to show menu screen on display
  */
class Menu:
public Gui 
{
  MenuElement *elements;
  int cursor;

public:

  Menu(MenuElement *elements) {
    this->elements = elements;
   this->cursor = 0;
  } 

  void draw() {
  GLCD.ClearScreen();
  
  for(int i = 0; elements[i].name != NULL; i++){
    GLCD.SelectFont(System5x7);
    GLCD.CursorTo(5, 1+i*2);
    GLCD.Puts(elements[i].name);
  }
  
  GLCD.CursorTo(3, cursor*2+1);
  GLCD.print("*");
  

  }

// switch menu states
  void keypress(int key) {
    switch(key) {
    case button_up:
      if (cursor > 0)
        cursor--;
      break;
    case button_down:
      // step down
      if (elements[cursor+1].name != NULL)
        cursor++;
      break;
    case button_enter:
      if (elements[cursor].submenu == NULL) return;
      gui_stack.push(elements[cursor].submenu);
      break;
    case button_back:
      gui_stack.pop();
      cursor = 0;
      break;
    }
  }
};

/**
  * Draw a circle on the screen
  */
class Circle : public Gui{
public:
  void draw(){
    GLCD.ClearScreen();
    GLCD.FillCircle(64, 32, 15, BLACK);
  }
  
  void keypress(int key){
    gui_stack.pop();
  }
};

Circle circle;


/**
  * Gui element to set stop position for the slider
  */
class StopPos : public Gui{
  public:
    void draw(){
      GLCD.ClearScreen();
      GLCD.SelectFont(System5x7);
      GLCD.CursorTo(2, 1);
      GLCD.print("Set Stop Position");
      GLCD.CursorTo(4, 4);
      GLCD.print("<");
      GLCD.CursorTo(16, 4);
      GLCD.print(">");
    }
    
    void keypress(int key){
      gui_stack.pop();
    }
};

StopPos stopPos;


/**
  * Gui element to set start position for the slider
  */
class StartPos : public Gui{
  public:
    void draw(){
      GLCD.ClearScreen();
      GLCD.SelectFont(System5x7);
      GLCD.CursorTo(2, 1);
      GLCD.print("Set Start Position");
      GLCD.CursorTo(4, 4);
      GLCD.print("<");
      GLCD.CursorTo(16, 4);
      GLCD.print(">");
    }
    
    void keypress(int key){
      switch(key){
          case button_enter:
            gui_stack.pop();
            gui_stack.push(&stopPos);
            break;
          case button_back:
            gui_stack.pop();
            break;
      }
    }
};

StartPos startPos;

/**
  * Array with menu definitions
  */
struct MenuElement settings_menu_elements[] = {
  {
    "Setup Wizard",  &startPos     }
  ,
  {
    "Advanced Mode", NULL          }
  ,
  {
    NULL,NULL          }
};
Menu settings_menu(settings_menu_elements);

struct MenuElement test_menu_elements[] = {
  {
    "Auto", NULL          }
  ,
  {
    "Manual", NULL          }
  ,
  {
    NULL, NULL          }
};
Menu test_menu(test_menu_elements);


struct MenuElement help_menu_elements[] = {
  {
    "Help1", NULL          }
  ,
  {
    "Help2", NULL          }
  ,
  {
    NULL, NULL          }
};
Menu help_menu(help_menu_elements);


struct MenuElement root_menu_elements[] = {
  {
    "Settings", &settings_menu          }
  ,
  {
    "Run", &test_menu          }
  ,
  {
    "Help", NULL          }
  ,
  {
    NULL,NULL          }
};
Menu root_menu(root_menu_elements);

Stack<Gui *> gui_stack(&root_menu);


/**
  *Welcome screen
  */
void welcome(void){
  GLCD.SelectFont(System5x7);
  GLCD.CursorTo(7, 2);
  GLCD.print("WELCOME");
  GLCD.SelectFont(Arial14);
  GLCD.CursorTo(4, 2);
  GLCD.print("OSPS");
  delay(1000);
  GLCD.ClearScreen();
}

/**
  * buttons initiations
  */
void init_buttons(void){
  pinMode(button_down, INPUT);
  digitalWrite(button_down, HIGH);
  pinMode(button_up, INPUT);
  digitalWrite(button_up, HIGH);
  pinMode(button_enter, INPUT);
  digitalWrite(button_enter, HIGH);
  pinMode(button_back, INPUT);
  digitalWrite(button_back, HIGH);
}

int getEncoderTurn(int newA, int newB) {
// return -1, 0, or +1
  static int oldA = LOW;
  static int oldB = LOW;
  int result = 0;
  
  
  if (newA != oldA || newB != oldB) {
  // something has changed
    if (oldA == LOW && newA == HIGH) {
        result = -(oldB * 2 - 1);
    }
  }
   oldA = newA;
   oldB = newB;
   return result;
}


/**
  * read buttons and show top element in the stack
  */
void update_gui()
{
  
  static char input_pins[] = { button_up, button_down, button_enter, button_back };
  static char input_debounce[] = {0,0,0,0};
  static char input_states[] = {0,0,0,0};

  //delay(5);
  
  char updated = 0;
  for(int i=0; i<sizeof(input_pins); i++) {
    char newstate = digitalRead(input_pins[i]);
    if (newstate != input_states[i]) {
      if(++input_debounce[i] > 3) {
        input_states[i] = newstate;
        input_debounce[i] = 0;
        updated = 1;
      }
    }
  }
  
  
  if (!updated) return;
  
  switch(getEncoderTurn(input_states[0],input_states[1])) {
    case -1:
      gui_stack.peek()->keypress(button_down);
      break;
    case 1:
      gui_stack.peek()->keypress(button_up);
      break;
    default:
      break;
  }
  
  if (digitalRead(button_enter) == LOW) {
    gui_stack.peek()->keypress(button_enter);
  }
  
  else if (digitalRead(button_back) == LOW) {
    gui_stack.peek()->keypress(button_back);
  }

    gui_stack.peek()->draw();

}




/**
  * Initiate and run program
  */
void setup()
{
  GLCD.Init();
  GLCD.SelectFont(System5x7);
  welcome();
  init_buttons();
  
  /* initiate main menu for the 1:st time */
  gui_stack.peek()->draw();  
  
}

void loop()
{
  update_gui();
}






