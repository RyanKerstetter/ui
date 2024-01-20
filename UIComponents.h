#include <algorithm>
#include <functional>
#include <map>
#include <raylib.h>
#include <iostream>
#include <stack>
#include <string>
#include <vector>
using namespace std;


enum Layout{
    HORIZONTAL,
    VERTICAL,
    FREE,
};

enum Allignment{
    LEFT,
    RIGHT,
    MIDDLE,
};

class Style;
class StaticContainer;

stack<Rectangle> scissor_stack;
StaticContainer* root;

inline void DrawTextureToScreenCoords(Texture t,Vector2 pos,Vector2 dim){
    DrawTexturePro(t,Rectangle{0,0,(float)t.width,(float)t.height},Rectangle{pos.x,pos.y,dim.x,dim.y},Vector2{0,0},0,WHITE);
}

void setScissor(Rectangle r);
void endScissor();
void startGameLoop();


class UIComponent {
public:
    static int id_counter;
    static Color default_background_color;
    static Color primary_color;
    static Color secondary_color;
    int id;
    int z_index;
    vector<Style*> styles;
    UIComponent* parent; 
    Vector2 offset;
    Vector2 dimension;
    bool show;
    UIComponent(UIComponent* parent, Vector2 offset, Vector2 dimension);
    virtual ~UIComponent(); 
    virtual void Draw() = 0;
    virtual void Update() = 0;
    virtual bool setBounds(Vector2 offset, Vector2 dimension) = 0;
    virtual void addStyle(Style* style,int pos=-1);
    Rectangle getGlobalBounds();
    void UIDraw();
    Vector2 getGlobalOffset();
};

class Style{
public:
    virtual void DrawBelow(UIComponent* c);
    virtual void DrawAbove(UIComponent* c);
    virtual void OnAdd(UIComponent* c);
};

class Border: public Style{ 
public:
    int margin;
    Color margin_color;
    bool U,D,L,R;
    Border(int margin,Color margin_color);
    void DrawAbove(UIComponent* c) override;
    void OnAdd(UIComponent* c) override;
};

class Background: public Style{
public:
    Color color;
    Background(Color background_color);
    void DrawBelow(UIComponent* c) override;
};

class UIImage: public Style{
public:
    Texture texture;
    UIImage(Texture t);
    void DrawBelow(UIComponent* c) override;
};

class Clip: public Style{
public:
    bool relative;
    Rectangle view;
    Clip(Rectangle view,bool relative=false);
    void DrawBelow(UIComponent* c) override;
    void DrawAbove(UIComponent* c) override;
};

class Scroll: public Style{
public:
    UIComponent* parent;
    Rectangle view;
    Vector2 offset;
    Scroll(Rectangle view,Vector2 offset);
    void DrawBelow(UIComponent* c) override;
    void DrawAbove(UIComponent* c) override;
};

class KeyboardListener{
public:
    static KeyboardListener* focus;
    static int last_char;
    KeyboardListener();
    ~KeyboardListener();
    bool isFocused();
    void HandleKey(int key);
    virtual void KeyPress(int key);
    virtual void KeyRelease(int key);
    virtual void KeyType(int key);
};

class Counter{
public:
    int count;
    int max_val;
    bool repeat;
    Counter(int max_val,bool repeat);
    void Tick();
    void Reset();
    bool Done();
};

class Draggable{
public:
    static Draggable* focus;
    Draggable();
    ~Draggable();
    virtual void onDrag(Vector2 mouse_pos,Vector2 delta,MouseButton button) = 0;
};

class MouseListener{
public:
    UIComponent* listener_parent;
    MouseListener();
    void init();
    void UpdateClip();
    virtual bool onClick(Vector2 mouse_pos,MouseButton button);
    virtual bool onHover(Vector2 mouse_pos);
    bool Click(Vector2 mouse_pos,MouseButton button);
    bool Hover(Vector2 mouse_pos);
    bool operator<(const MouseListener& other) const;
};

class Container: public UIComponent, public MouseListener{
public:
    vector<UIComponent*> components;
    vector<MouseListener*> listeners;
    Layout layout;
    Container(Vector2 offset, Vector2 dimension,Layout l,Color background_color);
    ~Container();
    virtual void addComponent(UIComponent* component,bool fill=false) = 0;
    void clear();
    void addListener(MouseListener* listener);
    void addBoth(UIComponent* component);
    bool removeComponent(int id);
    bool removeListener(int id);
    void Draw();
    void Update();
    virtual bool setBounds(Vector2 offset, Vector2 dimension) = 0;
    bool onClick(Vector2 mouse_pos,MouseButton button) override;
    bool onHover(Vector2 mouse_pos) override;
};

class StaticContainer: public Container{
public:
    StaticContainer(Vector2 offset, Vector2 dimension,Layout l,Color background_color=UIComponent::default_background_color);
    void addComponent(UIComponent* component, bool fill=false);
    bool setBounds(Vector2 offset, Vector2 dimension);
};

class DynamicContainer: public Container{
public:
    DynamicContainer(Vector2 offset,Layout l,Color background_color);
    void addComponent(UIComponent* component, bool fill=false);
    bool setBounds(Vector2 offset, Vector2 dimension);
};

class Text: public UIComponent{
public:
    string str;
    int size;
    Color text_color;
    Allignment a;
    Text(Vector2 offset,string text,int font_size=20,Allignment a=LEFT,Color text_color=BLACK);
    void Draw();
    void Update();
    bool setBounds(Vector2 offset, Vector2 dimension);
    Vector2 calculateDimension();
    void setText(string text);
};

class Box: public UIComponent{
public:
    Box(Rectangle rec);
    void Draw();
    void Update();
    bool setBounds(Vector2 offset, Vector2 dimension);
};

class Button: public UIComponent, public MouseListener{
private:
    static Color hover_color;
public:
    Background* background;
    std::function<void(Vector2 mouse_pos,MouseButton button)> click_callback;
    UIComponent* component;
    Color default_color;
    Button(UIComponent* c,std::function<void(Vector2 mouse_pos,MouseButton button)> click,Color col);
    ~Button();
    void Draw();
    void Update();
    void addStyle(Style* style,int pos=-1) override;
    bool setBounds(Vector2 offset, Vector2 dimension);
    bool onClick(Vector2 mouse_pos,MouseButton button) override;
    bool onHover(Vector2 mouse_pos) override;
};

class TitleBar: public UIComponent, public MouseListener,public Draggable{
public:
    static int title_bar_height;
    StaticContainer* container;
    Background* background;
    string title;
    Vector2 start_drag;
    TitleBar(Vector2 offset, Vector2 dimension,string title);
    ~TitleBar();
    void Draw();
    void Update();
    void addComponents();
    bool setBounds(Vector2 offset, Vector2 dimension);
    bool onClick(Vector2 mouse_pos,MouseButton button);
    bool onHover(Vector2 mouse_pos);
    void onDrag(Vector2 mouse_pos,Vector2 delta,MouseButton button);
};

class Window: public UIComponent, public MouseListener{
public:
    StaticContainer* content;
    StaticContainer* root_container;
    TitleBar* title_bar;
    Border* border;
    bool is_dragging;
    Window(Vector2 offset, Vector2 dimension,string title);
    ~Window();
    void addComponent(UIComponent* component);
    void addListener(MouseListener* listener);
    void addBoth(UIComponent* component);
    void Draw();
    void Update();
    void Kill();
    void addComponents();
    bool setBounds(Vector2 offset, Vector2 dimension);
    bool onClick(Vector2 mouse_pos,MouseButton button) override;
    bool onHover(Vector2 mouse_pos) override;
};

class TextBox: public UIComponent, public KeyboardListener, public MouseListener{
private:
    static int repeat_delay;
    static int start_delay; 
    static map<char,char> shift_map;
public:
    Text* text;
    int padding;
    int cols;
    std::function<void(string str)> submit_callback;
    int cursor_pos;
    Counter* frame_counter;
    Counter* repeat_counter;
    Counter* start_counter;
    bool start_count_done;
    bool reset_on_enter;
    Background* background;
    Border* border;
    TextBox(Vector2 offset, double font_size,int cols,std::function<void(string str)> submit_callback,bool reset,string str,Color text_color);
    ~TextBox();
    void Draw();
    void Update();
    bool setBounds(Vector2 offset, Vector2 dimension);
    Vector2 calculateDimension();
    bool onClick(Vector2 mouse_pos,MouseButton button) override;
    void KeyPress(int key) override;
    void KeyRelease(int key) override;
    void KeyType(int key) override;
    void HandleKey(int key);
};

class CheckBox: public UIComponent, public MouseListener{
private:
    static Color default_margin_color;
    static Color hover_margin_color; 
    static Color checked_color;
    static Color unchecked_color;
public:
    Background* background;
    Border* border;
    Color margin_color;
    bool checked;
    CheckBox(Vector2 offset, Vector2 dimension,bool checked);
    ~CheckBox();
    void Draw();
    void Update();
    bool setBounds(Vector2 offset, Vector2 dimension);
    bool onClick(Vector2 mouse_pos,MouseButton button) override;
    bool onHover(Vector2 mouse_pos) override;
};

class Field: public UIComponent,public MouseListener{
public:
    Container* container;
    TextBox* text_box;
    Text* text;
    Box* spacer;
    Field(Vector2 pos,Vector2 dimension,string text,int cols,float font_size,std::function<void(string str)> submit,Color text_color);
    ~Field();
    bool onClick(Vector2 pos,MouseButton button) override;
    void Draw();
    void Update();
    bool setBounds(Vector2 pos,Vector2 dimension);
};

class Slider: public UIComponent, public MouseListener, public Draggable{
private:
    static Color background_color;
    static Color slider_color;
public:
    Background* background;
    std::function<void(float)> change_callback;
    float slider_scale;
    float current;
    Layout l;
    Slider(Vector2 offset,Vector2 dimension,std::function<void(float)> change_callback,float current=0, float slider_scale=.1,Layout l=VERTICAL);
    ~Slider();
    void Draw();
    void Update();
    bool setBounds(Vector2 offset,Vector2 dimensino);
    bool onClick(Vector2 mouse_pos,MouseButton button) override;
    void onDrag(Vector2 mouse_pos,Vector2 delta,MouseButton button) override;
};

class ScrollPane: public UIComponent, public MouseListener{
public:
    UIComponent* view;
    Slider* scroll_bar;
    Clip* clip;
    Vector2 original_offset;
    bool isListener;
    ScrollPane(UIComponent* c,Vector2 dim);
    ~ScrollPane();
    void Draw();
    void Update();
    bool setBounds(Vector2 offset,Vector2 dimesnion);
    bool onClick(Vector2 mouse_pos,MouseButton button) override; 
    bool onHover(Vector2 mouse_pos) override;
};


