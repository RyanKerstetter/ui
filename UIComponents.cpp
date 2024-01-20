#include <functional>
#include <map>
#include "UIComponents.h"
#include <queue>
#include <algorithm>
#include <raylib.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

Font font;

int KeyboardListener::last_char = 0;
KeyboardListener* KeyboardListener::focus = nullptr;
Draggable* Draggable::focus = nullptr;
int UIComponent::id_counter = 0;
Color UIComponent::default_background_color = RAYWHITE;
Color UIComponent::primary_color = LIGHTGRAY;
Color UIComponent::secondary_color = DARKGRAY;
Color Button::hover_color = GRAY;
int TitleBar::title_bar_height = 20;
int TextBox::repeat_delay = 12;
int TextBox::start_delay = 15;
map<char,char> TextBox::shift_map = {{'1','!'},{'2','@'},{'3','#'},{'4','$'},{'5','%'},{'6','^'},{'7','&'},{'8','*'},{'9','('},{'0',')'},
                                     {'-','_'},{'=','+'},{'[','{'},{']','}'},{'\\','|'},{';',':'},{'\'','"'},{',','<'},{'.','>'},{'/','?'}};
Color CheckBox::checked_color = BLUE;
Color CheckBox::unchecked_color = UIComponent::default_background_color;
Color CheckBox::hover_margin_color = BLACK;
Color CheckBox::default_margin_color = UIComponent::secondary_color;
Color Slider::background_color = UIComponent::primary_color;
Color Slider::slider_color = UIComponent::secondary_color;

struct CompareMouseListeners{
    bool operator()(MouseListener* l1,MouseListener* l2){
        return l1->listener_parent->z_index < l2->listener_parent->z_index;
    }
};

struct CompareUIComponents{
    bool operator()(UIComponent* c1,UIComponent* c2){
        return c1->z_index > c2->z_index;
    }
};

void setScissor(Rectangle r){
    if(!scissor_stack.empty()){
        int x = max((int)r.x,(int)scissor_stack.top().x);
        int y = max((int)r.y,(int)scissor_stack.top().y);
        int width = min((int)r.width+r.x,(int)scissor_stack.top().width+scissor_stack.top().x) - x;
        int height = min((int)r.height+r.y,(int)scissor_stack.top().height+scissor_stack.top().y) - y;
        r = Rectangle{static_cast<float>(x),static_cast<float>(y),static_cast<float>(width),static_cast<float>(height)};
    }
    BeginScissorMode((int)r.x,(int)r.y,(int)r.width,(int)r.height);
    scissor_stack.push(r);
}

void endScissor(){
    Rectangle r = scissor_stack.top();
    scissor_stack.pop();
    if(scissor_stack.empty())
        EndScissorMode();
    else{
        Rectangle r = scissor_stack.top();
        BeginScissorMode((int)r.x,(int)r.y,(int)r.width,(int)r.height);
    }
}

void startGameLoop(){
    Vector2 last_mouse_pos = GetMousePosition();
    Vector2 drag_origin = {-1,-1};
    int drag_button = -1;
    while(!WindowShouldClose()){
        root->Update();
        int key = GetKeyPressed();
        Vector2 mouse_pos = GetMousePosition();
        root->onHover(mouse_pos);
        if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
            drag_origin = mouse_pos;
            drag_button = MOUSE_LEFT_BUTTON;
            root->onClick(mouse_pos,MOUSE_LEFT_BUTTON);
        }
        if(IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)){
            drag_origin = mouse_pos;
            drag_button = MOUSE_RIGHT_BUTTON;
            root->onClick(mouse_pos,MOUSE_RIGHT_BUTTON);
        }

        if(IsMouseButtonDown(MOUSE_LEFT_BUTTON) && drag_button == MOUSE_LEFT_BUTTON){
            if(Draggable::focus != nullptr)
                Draggable::focus->onDrag(drag_origin,{mouse_pos.x - drag_origin.x,mouse_pos.y - drag_origin.y},MOUSE_LEFT_BUTTON);
        }
        if(IsMouseButtonDown(MOUSE_RIGHT_BUTTON) && drag_button == MOUSE_RIGHT_BUTTON){
            if(Draggable::focus != nullptr)
                Draggable::focus->onDrag(drag_origin,{mouse_pos.x - drag_origin.x,mouse_pos.y - drag_origin.y},MOUSE_RIGHT_BUTTON);
        }

        if(!IsMouseButtonDown(MOUSE_LEFT_BUTTON) && drag_button == MOUSE_LEFT_BUTTON 
        || !IsMouseButtonDown(MOUSE_RIGHT_BUTTON) && drag_button == MOUSE_RIGHT_BUTTON){
            drag_origin = {-1,-1};
            drag_button = -1;
            Draggable::focus = nullptr;
        } 
        
        if(KeyboardListener::focus != nullptr)
            KeyboardListener::focus->HandleKey(key);
        BeginDrawing();
        ClearBackground(WHITE);
        root->UIDraw();
        EndDrawing();
    }
    CloseWindow();
}


UIComponent::UIComponent(UIComponent* parent,Vector2 offset,Vector2 dimension){
    this->parent = parent;
    this->offset = offset;
    this->dimension = dimension;
    this->id = id_counter++;
    this->z_index = 0;
    show = true;
}

UIComponent::~UIComponent(){
    for(auto s : styles)
        delete s;
}

void UIComponent::addStyle(Style* style,int pos){
    style->OnAdd(this);
    if(pos == -1)
        styles.push_back(style);
    else
        styles.insert(styles.begin() + pos,style);
}

Rectangle UIComponent::getGlobalBounds(){
    Vector2 offset = getGlobalOffset();
    return Rectangle{offset.x,offset.y,dimension.x,dimension.y};
}

void UIComponent::UIDraw(){
    if(show){
        Vector2 global_offset = getGlobalOffset();
        setScissor({global_offset.x,global_offset.y,dimension.x,dimension.y});
        for(auto s : styles){
            s->DrawBelow(this);
        }
        Draw();
        for(auto s : styles)
            s->DrawAbove(this);
        endScissor();
    }
}

Vector2 UIComponent::getGlobalOffset(){
    if(parent == nullptr){
        return offset;
    }
    return {offset.x + parent->getGlobalOffset().x,offset.y + parent->getGlobalOffset().y};
}

KeyboardListener::KeyboardListener(){}

KeyboardListener::~KeyboardListener(){
    if(focus == this)
        focus = nullptr;
}

void KeyboardListener::HandleKey(int key){
    if(!isFocused())
        return;
    if(IsKeyDown(last_char))
        KeyType(last_char);
    if(!IsKeyDown(last_char))
        KeyRelease(last_char);
    if(key != 0){
        KeyPress(key);
        last_char = key;
    }
}

bool KeyboardListener::isFocused(){
    return focus == this; 
}

void KeyboardListener::KeyPress(int key){
    return;
}

void KeyboardListener::KeyRelease(int key){
    return;
}

void KeyboardListener::KeyType(int key){
    return;
}

Counter::Counter(int max_val,bool repeat){
    this->count = 0;
    this->max_val = max_val;
    this->repeat = repeat;
}

void Counter::Tick(){
    count = min(count + 1,max_val);
    if(count >= max_val && repeat)
        count = 0;
}

bool Counter::Done(){
    return count == max_val;
}

void Counter::Reset(){
    count = 0;
}

MouseListener::MouseListener(){}


void MouseListener::init(){
    listener_parent = dynamic_cast<UIComponent*>(this);
    if(listener_parent == nullptr){
        cout << "Error: Mouse listener parent is not a UI component" << endl;
        return;
    }
    UIComponent* l = listener_parent;
}

void MouseListener::UpdateClip(){

}

bool MouseListener::Click(Vector2 mousePos,MouseButton button){
    Vector2 off = listener_parent->getGlobalOffset();
    Vector2 dim = listener_parent->dimension;
    Rectangle clip_rect = Rectangle{off.x,off.y,dim.x,dim.y};
    if(CheckCollisionPointRec(mousePos,clip_rect)){
        if(onClick(mousePos,button))
            return true;
    }
    return false;
}

bool MouseListener::Hover(Vector2 mousePos){
    Vector2 off = listener_parent->getGlobalOffset();
    Vector2 dim = listener_parent->dimension;
    Rectangle clip_rect = Rectangle{off.x,off.y,dim.x,dim.y};
    if(CheckCollisionPointRec(mousePos,clip_rect)){
        if(onHover(mousePos))
            return true;
    }
    return false;
}

bool MouseListener::onClick(Vector2 mouse_pos,MouseButton button){
    return true;
}

bool MouseListener::onHover(Vector2 mouse_pos){
    return true;
}

bool MouseListener::operator<(const MouseListener& other) const{
    return listener_parent < other.listener_parent;
}

Container::Container(Vector2 offset,Vector2 dimension,Layout layout,Color background_color) 
:UIComponent(nullptr,offset,dimension),MouseListener(){
    this->layout = layout;
    components = vector<UIComponent*>();
    listeners = vector<MouseListener*>();
    init();
}

Container::~Container(){
    for(UIComponent* c : components)
        delete c;
}

void Container::clear(){
    for(UIComponent* c : components)
        delete c;
    components.clear();
    listeners.clear();
}

void Container::Draw(){
    priority_queue<UIComponent*,vector<UIComponent*>,CompareUIComponents> max_heap;
    for(auto c : components){
        max_heap.push(c);
    }

    while(!max_heap.empty()){
        UIComponent* c = max_heap.top();
        max_heap.pop();
        c->UIDraw();
    }
}

void Container::Update(){
    for(auto c : components){
        c->Update();
    }
}

bool Container::removeComponent(int id){
    for(int i = 0; i < components.size(); i++){
        UIComponent* c = components[i];
        if(c->id == id){
            components.erase(remove(components.begin(),components.end(),c),components.end());
            removeListener(id);
            delete c;
            return true;
        }
    }
    return false;
}

bool Container::removeListener(int id){
    for(int i = 0; i < listeners.size(); i++){
        MouseListener* l = listeners[i];
        UIComponent* c = dynamic_cast<UIComponent*>(l);
        if(c == nullptr)
            cout << "Error: Mouse listener is not a UI component" << endl;
        if(c->id == id){
            listeners.erase(remove(listeners.begin(),listeners.end(),l),listeners.end());
            return true;
        }
    }
    return false;
}

void Container::addListener(MouseListener* l){
    listeners.push_back(l);
}

void Container::addBoth(UIComponent* c){
    MouseListener* listener = dynamic_cast<MouseListener*>(c);
    if(listener == nullptr){
        cout << "Error: Component is not a mouse listener" << endl;
        return;
    }
    addComponent(c);
    addListener(listener);
}

bool Container::setBounds(Vector2 offset,Vector2 dimension){
    this->offset = offset;
    this->dimension = dimension;
    return true;
}

bool Container::onClick(Vector2 mousePos,MouseButton button){
    priority_queue<MouseListener*,vector<MouseListener*>,CompareMouseListeners> min_heap; 
    for(auto l : listeners)
        min_heap.push(l);

    while(!min_heap.empty()){
        MouseListener* l = min_heap.top();
        min_heap.pop();    
        if(l->Click(mousePos,button))
            return true;
    }
    return false;
}

bool Container::onHover(Vector2 mousePos){
    priority_queue<MouseListener*,vector<MouseListener*>,CompareMouseListeners> min_heap;
    for(auto l : listeners)
        min_heap.push(l);

    while(!min_heap.empty()){
        MouseListener* l = min_heap.top();
        min_heap.pop();    
        if(l->Hover(mousePos))
            return true;
    }
    return false;
}

StaticContainer::StaticContainer(Vector2 offset,Vector2 dimension,Layout layout,Color background_color):
Container(offset,dimension,layout,background_color){}

void StaticContainer::addComponent(UIComponent* c,bool fill){
    Vector2 off = c->offset;
    Vector2 dim = c->dimension;
    if(layout == HORIZONTAL){
        off = {0,0};
        if(components.size() > 0){
            off = components[components.size() - 1]->offset;
            off.x += components[components.size() - 1]->dimension.x;
        }
        dim = {c->dimension.x,dimension.y};
        if(fill)
            dimension.x = c->dimension.x - off.x;
    } else if(layout == VERTICAL){
        off = {0,0};
        if(components.size() > 0){
            off = components[components.size() - 1]->offset;
            off.y += components[components.size() - 1]->dimension.y;
        }
        dim = {dimension.x,c->dimension.y};
        if(fill)
            dimension.y = c->dimension.y - off.y;
    }
    bool worked = c->setBounds(off,dim);
    if(!worked)
        cout << "Error: Component could not be resized" << endl;
    c->parent = this;
    components.push_back(c);
}

bool StaticContainer::setBounds(Vector2 off,Vector2 dim){
    double max_x = components.size() > 0 ? components[components.size()-1]->offset.x + components[components.size()-1]->dimension.x : 0;
    double max_y = components.size() > 0 ? components[components.size()-1]->offset.y + components[components.size()-1]->dimension.y : 0;
    if(layout == HORIZONTAL && dim.x < max_x)
        return false;
    if(layout == VERTICAL && dim.y < max_y)
        return false;
    this->offset = off;
    this->dimension = dim;
    return true;
}

DynamicContainer::DynamicContainer(Vector2 offset,Layout layout,Color background_color):
Container(offset,{0,0},layout,background_color){}

void DynamicContainer::addComponent(UIComponent* c,bool fill){
    Vector2 off = {0,0};
    Vector2 new_container_dim = dimension;
    if(layout == FREE){
        cout << "Error: Dynamic container must have a layout" << endl;
        return;
    }
    if(layout == HORIZONTAL){
        if(components.size() > 0){
            off = components[components.size() - 1]->offset;
            off.x += components[components.size() - 1]->dimension.x;
        }
        new_container_dim = {off.x + c->dimension.x,max(dimension.y,c->dimension.y)};
    } else if(layout == VERTICAL){
        if(components.size() > 0){
            off = components[components.size() - 1]->offset;
            off.y += components[components.size() - 1]->dimension.y;
        }
        new_container_dim = {max(dimension.x,c->dimension.x),off.y + c->dimension.y};
    }
    c->offset = off;
    c->parent = this;
    components.push_back(c);
    dimension = new_container_dim;
}

bool DynamicContainer::setBounds(Vector2 off,Vector2 dim=Vector2{0,0}){
    if(dim.x != 0 || dim.y != 0){
        cout << "Error: Dynamic container cannot be resized" << endl;
        return false;
    }
    this->offset = off;
    return true;
}

Text::Text(Vector2 offset,string text,int font_size,Allignment a,Color text_color) : UIComponent(nullptr,offset,{0,0}){
    this->str= text;
    this->text_color = text_color;
    this->size = font_size;
    this->a = a;
    dimension = calculateDimension();
}

void Text::Draw(){
    Vector2 offset = getGlobalOffset();
    Vector2 text_dim = calculateDimension();
    Vector2 dim = dimension;
    if(dim.x == text_dim.x && dim.y == text_dim.y){
        DrawTextEx(font,str.c_str(),offset,size,2,text_color);
        return;
    }
    if(a == MIDDLE)
        offset = {offset.x + (dim.x - text_dim.x) / 2,offset.y + (dim.y - text_dim.y) / 2};
    if(a == RIGHT)
        offset = {offset.x + (dim.x - text_dim.x),offset.y + (dim.y - text_dim.y)};
    DrawTextEx(font,str.c_str(),offset,size,2,text_color);
}

void Text::Update(){
    return;
}

bool Text::setBounds(Vector2 off,Vector2 dim){
    this->offset = off;
    if(dim.x < dimension.x || dim.y < dimension.y){
        return false;
    }
    this->dimension = dim;
    return true;
}

Vector2 Text::calculateDimension(){
    Vector2 dim = MeasureTextEx(font,str.c_str(),size,2);
    return dim;
}

void Text::setText(string text){
    this->str = text;
    dimension = calculateDimension();
}

Box::Box(Rectangle rec) 
:UIComponent(nullptr,{rec.x,rec.y},{rec.width,rec.height}){}

void Box::Draw(){
}

void Box::Update(){}

bool Box::setBounds(Vector2 off,Vector2 dim){
    this->offset = off;
    this->dimension = dim;
    return true;
}

UIImage::UIImage(Texture texture){
    this->texture = texture;
}

void UIImage::DrawBelow(UIComponent* c){
    Vector2 offset = c->getGlobalOffset();
    Vector2 dim = c->dimension;
    DrawTextureToScreenCoords(texture,offset,dim);
}

Button::Button(UIComponent* c,std::function<void(Vector2,MouseButton)> click_callback,Color col)
:UIComponent(nullptr,c->offset,c->dimension),MouseListener(){
    this->component = c;
    c->parent = this;
    this->click_callback = click_callback;
    background = new Background(col);
    addStyle(background);
    default_color = col;
    init();
}

Button::~Button(){
    delete component;
}

void Button::Draw(){
    component->UIDraw();
}

void Button::Update(){
    component->Update();
    background->color = default_color;
}

bool Button::setBounds(Vector2 off,Vector2 dim){
    //bool worked = component->setBounds({0,0},dim);
    //if(!worked){
    //    cout << "Error: Component could not be resized" << endl;
    //    return false;
    //}
    this->offset = off;
    this->dimension = dim;
    return true;
}

void Button::addStyle(Style* style,int pos){
    component->addStyle(style,pos);
}

bool Button::onClick(Vector2 mousePos,MouseButton button){
    cout << "Clicked" << endl;
    click_callback(mousePos,button);
    return true;
}

bool Button::onHover(Vector2 mousePos){
    background->color = hover_color;
    return true;
}

void Style::DrawAbove(UIComponent* c){}

void Style::DrawBelow(UIComponent* c){}

void Style::OnAdd(UIComponent* c){}   

Border::Border(int margin,Color margin_col){
    this->margin = margin;
    this->margin_color = margin_col;
    U = true; D = true; L = true; R = true;
}

void Border::DrawAbove(UIComponent* c){
    Vector2 global_offset = c->getGlobalOffset();
    Vector2 dimension = c->dimension;
    if(U)
        DrawRectangle(global_offset.x,global_offset.y,dimension.x,margin,margin_color);
    if(D)
        DrawRectangle(global_offset.x,global_offset.y + dimension.y-margin,dimension.x,margin,margin_color);
    if(L)
        DrawRectangle(global_offset.x,global_offset.y,margin,dimension.y,margin_color);
    if(R)
        DrawRectangle(global_offset.x+dimension.x-margin,global_offset.y,margin,dimension.y,margin_color);
}

void Border::OnAdd(UIComponent* c){
    c->offset = {c->offset.x - margin,c->offset.y - margin};
    c->dimension = {c->dimension.x + 2*margin,c->dimension.y + 2*margin};
}

Background::Background(Color background_color){
    this->color = background_color;
}

void Background::DrawBelow(UIComponent* c){
    Vector2 global_offset = c->getGlobalOffset();
    Vector2 dimension = c->dimension;
    DrawRectangle(global_offset.x,global_offset.y,dimension.x,dimension.y,color);
}

Clip::Clip(Rectangle r,bool relative){
    this->view = r;
    this->relative = relative;
}

void Clip::DrawBelow(UIComponent* c){
    Vector2 c_offset = relative ? c->getGlobalOffset() : Vector2{0,0};
    Rectangle r = {c_offset.x + view.x,c_offset.y + view.y,view.width,view.height};
    setScissor(r);
}

void Clip::DrawAbove(UIComponent* c){
    endScissor();
}

Scroll::Scroll(Rectangle view,Vector2 offset){
    this->view = view;
    this->offset = offset;
}

void Scroll::DrawBelow(UIComponent* c){
    setScissor(view);
}

void Scroll::DrawAbove(UIComponent* c){
    endScissor();
}

Draggable::Draggable(){}

Draggable::~Draggable(){
    if(focus == this)
        focus = nullptr;
}

TitleBar::TitleBar(Vector2 offset, Vector2 dimension, string title)
:UIComponent(nullptr,offset,dimension),MouseListener(),Draggable(){
    this->title = title;
    background = new Background(UIComponent::primary_color);
    addStyle(background);
    addComponents();
    init();
}

TitleBar::~TitleBar(){
    delete container;
}

void TitleBar::addComponents(){
    container = new StaticContainer(Vector2{0,0},dimension,HORIZONTAL); 
    Text* title_name = new Text(Vector2{0,0},title,20,LEFT,BLACK);
    Button* close_window = new Button(new Text(Vector2{0,0},"X",20,MIDDLE,BLACK),[this](Vector2 mousePos,MouseButton button){
        Window* window_parent = dynamic_cast<Window*>(this->parent->parent);
        //                                          Button -> Container -> TitleBar -> Container -> Window
        if(window_parent == nullptr){
            cout << "Error: Not a window" << endl;
            return;
        }
        cout << "Killing" << endl;
        window_parent->Kill();
    },RED);
    float spacer_width = dimension.x - title_name->dimension.x - close_window->dimension.x;
    Box* spacer = new Box({0,0,spacer_width,0});
    container->addComponent(title_name);
    container->addComponent(spacer);
    container->addComponent(close_window);
    container->addListener(close_window);
    container->parent = this;
}

void TitleBar::Draw(){
    container->UIDraw();
}

void TitleBar::Update(){
    container->Update();
}

bool TitleBar::setBounds(Vector2 off,Vector2 dim){
    this->offset = off;
    this->dimension = dim;
    if(dim.x == dimension.x && dim.y == dimension.y)
        return true;
    addComponents();
    return true;
}

bool TitleBar::onClick(Vector2 mousePos,MouseButton button){
    Draggable::focus = this;
    start_drag = parent->parent->offset;
    return container->onClick(mousePos,button);
}

bool TitleBar::onHover(Vector2 mousePos){
    container->onHover(mousePos);
    return true;
}

void TitleBar::onDrag(Vector2 startPos,Vector2 delta,MouseButton button){
    if(button == MOUSE_RIGHT_BUTTON)
        return;
    Window* window_parent = dynamic_cast<Window*>(parent->parent);
    //                               Window root -> Window
    if(window_parent == nullptr){
        cout << "Error: Not a window" << endl;
        cout << typeid(parent->parent).name() << endl;
        return;
    }
    window_parent->offset = {start_drag.x + delta.x,start_drag.y + delta.y};
}

Window::Window(Vector2 offset, Vector2 dimension, string title)
:UIComponent(nullptr,offset,dimension),MouseListener(){
    border = new Border(2,UIComponent::secondary_color);
    addStyle(border);
    addComponents();
    init();
    cout << dimension.x << ":" << dimension.y << endl;
}

Window::~Window(){
    delete root_container;
}

void Window::addComponents(){
    root_container = new StaticContainer({0,0},dimension,VERTICAL,RAYWHITE); 
    title_bar = new TitleBar({0,0},{dimension.x,(float)TitleBar::title_bar_height},"Window");
    content = new StaticContainer({0,(float)TitleBar::title_bar_height},{dimension.x,dimension.y - TitleBar::title_bar_height},FREE);
    root_container->addComponent(title_bar);
    root_container->addListener(title_bar);
    root_container->addComponent(content);
    root_container->addListener(content);
    root_container->parent = this;
}

void Window::addBoth(UIComponent* c){
    MouseListener* l = dynamic_cast<MouseListener*>(c);
    if(l == nullptr){
        cout << "Window::addBoth Error: Component is not a mouse listener" << endl;
        return;
    }
    addComponent(c);
    addListener(l);
}

void Window::Draw(){
    root_container->UIDraw();
}

void Window::Update(){
    root_container->Update();
}

bool Window::setBounds(Vector2 off,Vector2 dim){
    this->offset = off;
    this->dimension = dim;
    if(dim.x == dimension.x && dim.y == dimension.y)
        return true;
    addComponents();
    return true;
}

void Window::Kill(){
    Container* parent = dynamic_cast<Container*>(this->parent);
    if(parent == nullptr){
        cout << "Error: Window parent is not a container" << endl;
        return;
    }
    parent->removeComponent(id);
}

bool Window::onClick(Vector2 mousePos,MouseButton button){
    return root_container->Click(mousePos,button);
}

bool Window::onHover(Vector2 mousePos){
    return root_container->onHover(mousePos);
}


void Window::addComponent(UIComponent* c){
    content->addComponent(c);
}

void Window::addListener(MouseListener* l){
    content->addListener(l);
}

TextBox::TextBox(Vector2 offset,double font_size,int cols,std::function<void(string str)> submit_callback,bool reset = false, string text="",Color text_color=BLACK)
    :UIComponent(nullptr,offset,{0,0}),MouseListener(),KeyboardListener(){
    padding = 2;
    this->text = new Text(Vector2{5,0},text,font_size,LEFT,text_color);
    this->text->parent = this;
    this->cols = cols;
    this->submit_callback = submit_callback;
    this->reset_on_enter = reset;
    cursor_pos = 0;
    frame_counter = new Counter(60,true);
    repeat_counter = new Counter(repeat_delay,true);
    start_counter = new Counter(start_delay,false);
    dimension = calculateDimension();

    background = new Background(UIComponent::default_background_color);
    addStyle(background);
    border = new Border(2,UIComponent::secondary_color);
    addStyle(border);
    init();
}

TextBox::~TextBox(){
    delete frame_counter;
    delete repeat_counter;
    delete start_counter;
    delete text;
}

void TextBox::KeyPress(int key){
    HandleKey(key);
    start_counter->count = 1;
    start_count_done = false;
}

void TextBox::KeyType(int key) {
    if(start_count_done){
        if(repeat_counter->count == 0){
            HandleKey(key);
        }
    }
    if(start_counter->count >= start_delay){
        HandleKey(key);
        start_count_done = true;
        repeat_counter->count = 0;
    }
}

void TextBox::KeyRelease(int key){
    repeat_counter->count = repeat_delay;
    start_counter->count = 0;
    start_count_done = false;
}

void TextBox::HandleKey(int key){
    frame_counter->count = 0;
    bool shift_pressed = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    if(key == KEY_BACKSPACE){
        if(text->str.size() > 0 && cursor_pos > 0){
            text->str.erase(cursor_pos - 1,1);
            cursor_pos--;
        }
    } else if(key >= 32 && key <= 126){
        if(text->str.size() < cols){
            char c = (char)key;
            if(c >= 'A' && c <= 'Z' && !shift_pressed)
                c += 32;
            else if (shift_pressed && shift_map.find(c) != shift_map.end())
                c = shift_map[c];
            text->str.insert(cursor_pos,1,c);
            cursor_pos++;
        }
    } else if(key == KEY_LEFT){
        if(cursor_pos > 0)
            cursor_pos--;
        frame_counter->count = 0;
    } else if(key == KEY_RIGHT){
        if(cursor_pos < text->str.size())
            cursor_pos++;
        frame_counter->count = 0;
    } else if(key == KEY_ENTER){
        if(submit_callback != nullptr)
            submit_callback(text->str);
        if(reset_on_enter){
            text->str = "";
            cursor_pos = 0;
        }
    }
    text->dimension = text->calculateDimension();
} 

void TextBox::Draw(){
    Vector2 offset = getGlobalOffset();
    Vector2 dim = dimension;
    Vector2 text_width = MeasureTextEx(font," ",text->size,2); 
    double cursor_x = offset.x + (padding + text_width.x) * cursor_pos + 5;
    text->UIDraw();
    if(frame_counter->count < 30 && isFocused()){
        DrawLine(cursor_x,offset.y,cursor_x,offset.y + text->size,text->text_color);
    }
}

void TextBox::Update(){
    frame_counter->Tick();
    repeat_counter->Tick();
    start_counter->Tick();
}

Vector2 TextBox::calculateDimension(){
    Vector2 dim = MeasureTextEx(font," ",text->size,2);
    dim.x = (dim.x  + padding) * cols + 5;
    dim.y = text->size;
    return dim;
}

bool TextBox::setBounds(Vector2 off,Vector2 dim){
    this->offset = off;
    this->dimension = dim;
    return true;
}

bool TextBox::onClick(Vector2 mousePos,MouseButton button) {
    Vector2 offset = getGlobalOffset();
    Vector2 text_width = MeasureTextEx(font," ",text->size,2);
    double char_width = text_width.x + padding;
    int char_pos = (mousePos.x - offset.x + char_width / 2) / char_width;
    cursor_pos = char_pos > text->str.size() ? text->str.size() : char_pos;
    frame_counter->count = 0;
    focus = this;
    return true;
}

CheckBox::CheckBox(Vector2 offset,Vector2 dimension,bool checked):UIComponent(nullptr,offset,dimension),MouseListener(){
    this->offset = offset;
    this->dimension = dimension;
    this->checked = checked;
    background = new Background(checked ? checked_color : unchecked_color);
    border = new Border(2,default_margin_color);
    addStyle(background);
    addStyle(border);
    margin_color = default_margin_color;
    init();
}

CheckBox::~CheckBox(){}

void CheckBox::Draw(){}

void CheckBox::Update(){
    background->color = checked ? checked_color : unchecked_color;
    border->margin_color = margin_color;
}

bool CheckBox::setBounds(Vector2 off,Vector2 dim){
    this->offset = off;
    this->dimension = dim;
    return true;
}

bool CheckBox::onClick(Vector2 mousePos,MouseButton button){
    checked = !checked;
    return true;
}

bool CheckBox::onHover(Vector2 mousePos){
    border->margin_color = hover_margin_color;
    margin_color = hover_margin_color;
    return true;
}

Field::Field(Vector2 pos,Vector2 dimension,string text,int cols,float font_size,std::function<void(string str)> submit,Color text_color):UIComponent(nullptr,pos,dimension){
    this->container = new StaticContainer(pos,dimension,HORIZONTAL);
    this->text = new Text({0,0},text,font_size,LEFT,text_color);
    this->text_box = new TextBox({0,0},font_size,cols,submit,false,"",text_color);
    float spacer_x = dimension.x - (text_box->dimension.x + this->text->dimension.x);
    this->spacer = new Box({0,0,spacer_x,font_size});
    container->addComponent(this->text);
    container->addComponent(this->spacer);
    container->addBoth(this->text_box);
    container->parent = this;
    init();
}

Field::~Field(){
    delete container;
}

bool Field::onClick(Vector2 pos,MouseButton button){
    return container->Click(pos,button);
}

void Field::Draw(){
    container->Draw();
}

void Field::Update(){
    container->Update();
}

bool Field::setBounds(Vector2 pos,Vector2 dimension){
    this->offset = pos;
    return true;
}

Slider::Slider(Vector2 offset,Vector2 dimension,std::function<void(float)> change_callback,float current,float slider_scale,Layout l):UIComponent(nullptr,offset,dimension),MouseListener(),Draggable(){
    this->l = l;
    this->change_callback = change_callback;
    this->current = current;
    background = new Background(background_color);
    this->slider_scale = slider_scale;
    addStyle(background);
    init();
}

Slider::~Slider(){}

void Slider::Draw(){
    Vector2 offset = getGlobalOffset();
    float slider_percent = current;
    float slider_width = l == HORIZONTAL ? dimension.x * slider_scale : dimension.y * slider_scale; 
    offset.x = l == HORIZONTAL ? offset.x + slider_percent * (dimension.x - slider_width) : offset.x;
    offset.y = l == VERTICAL ? offset.y + slider_percent * (dimension.y - slider_width) : offset.y;
    Vector2 slider_bounds = l == HORIZONTAL ? Vector2{slider_width,dimension.y} : Vector2{dimension.x,slider_width};
    DrawRectangleV(offset,slider_bounds,DARKGRAY);
}

bool Slider::setBounds(Vector2 offset,Vector2 dimension){
    this->dimension = dimension;
    this->offset = offset;
    return true;
}

bool Slider::onClick(Vector2 mouse_pos,MouseButton button){
    Vector2 offset = getGlobalOffset();
    Vector2 click_pos = {mouse_pos.x - offset.x,mouse_pos.y - offset.y};
    float click_percent = l == HORIZONTAL ? click_pos.x / dimension.x : click_pos.y / dimension.y;
    change_callback(click_percent);
    current = click_percent;
    Draggable::focus = this;
    return true;
}

void Slider::onDrag(Vector2 mouse_pos,Vector2 delta,MouseButton button){
    Vector2 offset = getGlobalOffset();
    Vector2 click_pos = {mouse_pos.x + delta.x - offset.x,mouse_pos.y + delta.y - offset.y};
    float click_percent = l == HORIZONTAL ? click_pos.x / dimension.x : click_pos.y / dimension.y;
    click_percent = max(.0f,min(1.0f,click_percent));
    change_callback(click_percent);
    current = click_percent;
}

void Slider::Update(){}
ScrollPane::ScrollPane(UIComponent* c,Vector2 dim):UIComponent(nullptr,c->offset,{0,0}),MouseListener(){
    this->view = c;
    clip = new Clip({c->getGlobalOffset().x,c->getGlobalOffset().y,dim.x,dim.y},false);
    c->addStyle(clip,0);
    dimension = {dim.x + 10,dim.y};
    scroll_bar = new Slider({c->offset.x + c->dimension.x,c->offset.y},{10,dim.y},[this](float val){
        view->offset.y = -val * (view->dimension.y - dimension.y);
    });
    isListener = dynamic_cast<MouseListener*>(c) != nullptr;
    c->parent = this;
    c->offset = {0,0};
        
    init();
}

ScrollPane::~ScrollPane(){
    delete view;
    delete scroll_bar;
}

void ScrollPane::Draw(){
    view->UIDraw();
    scroll_bar->UIDraw();
}

void ScrollPane::Update(){
    view->Update();
    scroll_bar->Update();
}

bool ScrollPane::setBounds(Vector2 offset,Vector2 dimension){
    this->offset = offset;
    clip->view = {getGlobalOffset().x,getGlobalOffset().y,dimension.x,dimension.y};
    return true;
}

bool ScrollPane::onClick(Vector2 mouse_pos,MouseButton button){
    if(isListener)
        return dynamic_cast<MouseListener*>(view)->Click(mouse_pos,button) || scroll_bar->Click(mouse_pos,button);
    return scroll_bar->Click(mouse_pos,button);
}

bool ScrollPane::onHover(Vector2 mouse_pos){
    if(isListener)
        return dynamic_cast<MouseListener*>(view)->Hover(mouse_pos) || scroll_bar->Hover(mouse_pos);
    return scroll_bar->Hover(mouse_pos);
}

