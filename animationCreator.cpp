//i686-w64-mingw32-g++ -I src/include -L src/lib -o animator animationCreator.cpp  -lmingw32 -lSDL2main -lSDL2 -lfreetype
#include<SDL2/SDL.h>

#include<ft2build.h>
#include FT_FREETYPE_H

#include<fstream>
#include<iostream>
#include<vector>
#include<string>
#include<cmath>
#include<bitset>
#include<filesystem>
#include<algorithm>

int WIDTH = 1600;
int HEIGHT = 800;
int UPS = 60;

SDL_Color bg = {30,30,30,255};

FT_Library ft;
SDL_Color colors[256];

SDL_Window *wind;
SDL_Renderer *rend;
SDL_Event evt;

FT_FaceRec_ *face;
std::string CD = "./assets/images";

enum GUItypes{
    TEXT = 0,
    BUTTON = 1
};

struct transform_struct{
    int x;
    int y;
    int w;
    int h;
    double scale_x = 1;
    double scale_y = 1;
    double angle = 0;
    //SDL_Point rotationCenter;
    //#SDL_FLIP status?
};

struct sprite_struct{
    //order in layers?
    std::string path;
    transform_struct transform;
    std::vector<int8_t> alphas;
    SDL_Texture *texture;
    SDL_Texture *transparencyMask;
};
//think of them as layered images
std::vector<sprite_struct> sprites;

void renderSprites(bool transparencyMask);
void moveSprites();

void loadBMP(std::string path);
void loadFont(int fontSize);

void selectDirectory(std::string label);
void browseDirectory(std::string &cd, SDL_Rect box, int fontSize);
void cdBack(void* in);  

std::string selectedSprite = "";
void inspector(SDL_Rect box, int fontSize);
void selectActiveSprite(std::string in);

bool onRect(SDL_Rect rect, int x, int y);
bool onRect(SDL_Rect rect);
std::string intToString(int numb);
int stringToInt(std::string string);
std::string doubleToString(double in, int precision);
double stringToDouble(std::string in);

class surfaceManipulation{
public:
    struct pixel{
        int x;
        int y;
    };
    struct color{
        Uint8 r;
        Uint8 g;
        Uint8 b;
        Uint8 a;
    };
    /** 
     * Prepares surface for a high batch of changes.
     * Format for RGBA surface: `SDL_PIXELFORMAT_RGBA32`
    */
    void createSurface(int width, int height, int depth, SDL_PixelFormatEnum format);
    /**
     * Creates a texture and deletes the surface.
    */
    SDL_Texture* createTextureAndDeleteSurface(SDL_Renderer *renderer);
    /**
     * Draws to a specifc pixel provided colors.
    */
    void drawToSurface(pixel coords, color RGBA);
    /**
     * Gets RGBA values from a specified surface's given pixel.
    */
    SDL_Color getSurfaceColors(pixel coords, SDL_Surface *surface, bool freeSurface);

private:
    SDL_Surface* surface;
    Uint8 *data;
};

void surfaceManipulation::createSurface(int width, int height, int depth, SDL_PixelFormatEnum format){
    surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, depth, format);
    SDL_LockSurface(surface);
}
void surfaceManipulation::drawToSurface(pixel coords, color RGBA){
    if(RGBA.a == 0) return;
    data = (Uint8*)surface->pixels + coords.y*surface->pitch + coords.x * surface->format->BytesPerPixel;
    *((Uint32*)data) = (Uint32)(RGBA.r << 0 | RGBA.g << 8 | RGBA.b << 16 | RGBA.a << 24);
}
SDL_Texture* surfaceManipulation::createTextureAndDeleteSurface(SDL_Renderer *renderer){
    SDL_Texture* _return;
    SDL_UnlockSurface(surface);
    _return = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return _return;
}
SDL_Color surfaceManipulation::getSurfaceColors(pixel coords, SDL_Surface *surface, bool freeSurface){
    Uint8 red{}, green{}, blue{}, alfa{};
    Uint32 values = *(Uint32*)((Uint8*)((Uint8*)surface->pixels + coords.y*surface->pitch + coords.x*surface->format->BytesPerPixel));
    SDL_GetRGBA(values, surface->format, (Uint8*)(&red), (Uint8*)(&green), (Uint8*)(&blue), (Uint8*)(&alfa));
    if(freeSurface) SDL_FreeSurface(surface);
    return {red, green, blue, alfa};
}

class UI2{
public:
    struct renderedTexts_struct{
        SDL_Texture* texture;
        std::string sentence = "";
        int lastAccessed = 0; //-frames/updates ago.
        SDL_Rect dimensions;
    };
    static std::vector<renderedTexts_struct> renderedTexts;

    //Last renderText() information.
    struct{
        //Whole texture:
        int w, h;
        int fontSize;
        //total width at the end of each symbol.
        std::vector<SDL_Point> widthSymEnd; 
    }textInfo;

    struct loadedFaces_struct{
        std::string path;
        int fontSize;
        FT_FaceRec_ *face;
        FT_Library ft;
    };
    std::vector<loadedFaces_struct> loadedFaces;

    //UI
    SDL_Texture* renderText(std::string sentence, SDL_Rect textBox, int fontsize, bool newLines);
    FT_FaceRec_* useFont(std::string path, int fontSize);

    static void updateLife();
    static SDL_Texture* findExistingText(std::string &sentence, SDL_Rect &textBox);
};
std::vector<UI2::renderedTexts_struct> UI2::renderedTexts{};

class UI{
public:
    //UI
    void renderText(std::string sentence, SDL_Rect textBox, int fontSize, bool newLines);
    void slider(std::string label, int fontSize, SDL_Rect sliderBox, int &value, int minValue, int maxValue);
    void button(std::string label, SDL_Rect buttonbox, int fontSize, void(*onClick)(void*), void* param);
    void buttonDrawn(std::string label, SDL_Rect buttonbox, int fontSize, void(*onClick)(void*), void* param);
    void scrollBox(GUItypes type, SDL_Rect box, std::vector<std::string> &entries, int fontSize, void (*onClick)(std::string));
    void textInput(std::string &in, bool onlyNUMB, SDL_Rect box, int fontsize);
    
    void inspectorSpecificValue(double &value, std::string name, SDL_Rect box, int fontSize);
private:
    //scrollBox() variables.
        int delta;
        bool scrollBarPressed = false;
        //shift is scroll bar position;
        int shift = 0; 
        //Check if new entries  
        std::vector<std::string> old{};
    //textInput variables.
        std::string input = "";
        bool selectedText = false;
        int symIndx = 0;
        std::vector<int> symEndPos;
        int numbOfCommas = 0;
};
UI explorer_class;
UI inspector_class;

UI specStat1;
UI specStat2;
UI specStat3;

struct save_info{
    std::string sprite_category, sprite_name;
    int rotation;
    int realtive_x, relative_y;
    int dividend;
};

void timeline(SDL_Rect box);
void clearFrames();
void saveFrame();

void saveToFile();

void pushToFront(void* data);
void pushToBack(void* data);
void unload(void* data);

int main(int argc, char *argv[]){
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER);
    FT_Init_FreeType(&ft);

    wind = SDL_CreateWindow("animator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    rend = SDL_CreateRenderer(wind, -1, 0);
    SDL_SetRenderDrawBlendMode(rend, SDL_BlendMode::SDL_BLENDMODE_BLEND);

    for(int i = 0; i < 256; i++)
        colors[i].r = colors[i].g = colors[i].b = colors[i].a= i;


    while(true){
        SDL_PollEvent(&evt);
        if(evt.type == SDL_QUIT) break;
        UI2::updateLife();

        SDL_SetRenderDrawColor(rend, bg.r,bg.g,bg.b,bg.a);
        SDL_RenderClear(rend);
        SDL_SetRenderDrawColor(rend, 255,255,255,255);
        browseDirectory(CD, {10, 10, 270, 200}, 18);
        inspector({WIDTH-WIDTH/5, 30, WIDTH/5 - 10, HEIGHT - 60}, 18);
        moveSprites();
        renderSprites(false);

        timeline({10, HEIGHT-100, WIDTH-WIDTH/5-10, 70});
        
        SDL_RenderPresent(rend);
        SDL_Delay(1000/float(UPS));
    }

    FT_Done_FreeType(ft);
    SDL_Quit();
    return 0;
}

void UI2::updateLife(){
    for(int i = 0; i < renderedTexts.size(); i++){
        renderedTexts[i].lastAccessed++;
        if(renderedTexts[i].lastAccessed >= 2){
            SDL_DestroyTexture(renderedTexts[i].texture);
            renderedTexts.erase(renderedTexts.begin() + i);
        }
    }
    renderedTexts.shrink_to_fit();
    return;
}

void UI::buttonDrawn(std::string label, SDL_Rect buttonbox, int fontSize, void(*onClick)(void*), void* param){
    button(label, buttonbox, fontSize, onClick, param);
    int width = symEndPos[symEndPos.size()-1];
    buttonbox = {buttonbox.x-2, buttonbox.y, width+4, fontSize+3};
    SDL_SetRenderDrawColor(rend, 255,255,255,255);
    SDL_RenderDrawRect(rend, &buttonbox);
}
void UI::button(std::string label, SDL_Rect buttonbox, int fontSize, void(*onClick)(void*), void* param){
    if(onRect(buttonbox) && evt.type == SDL_MOUSEBUTTONDOWN && evt.button.button == SDL_BUTTON_LEFT)
        onClick(param);
    renderText(label, buttonbox, fontSize, false);
}

void browseDirectory(std::string &cd, SDL_Rect box, int fontSize){
    //Get current files in the directory.
    std::vector<std::string> files;
    for(const auto &entry : std::filesystem::directory_iterator(cd)){
        files.push_back(entry.path().generic_string());
    }

    //Draw boxes
    SDL_RenderDrawRect(rend, &box);
    SDL_RenderDrawLine(rend, box.x, box.y + fontSize + 2, box.x + box.w - 1, box.y + fontSize + 2);

    //cdBack
    explorer_class.button("...", {box.x + box.w - fontSize, box.y, fontSize, fontSize}, fontSize, &cdBack, nullptr);

    //Print current directory
    explorer_class.renderText(cd, {box.x + 1, box.y + 1, box.w - 1, fontSize}, fontSize, false);
    explorer_class.scrollBox(GUItypes::BUTTON, {box.x, box.y + fontSize + 2, box.w, box.h -fontSize-2}, files, fontSize, &selectDirectory);
}

void cdBack(void* in){
    if(CD.size() > 2 && CD[CD.size()-1] == '/') CD.pop_back();
    for(int i = CD.size()-1; i >= 1; i--){
        if(CD[i] != '/') CD.pop_back();
        else
            return;
    }
}
bool onRect(SDL_Rect rect){
    int tmpMx, tmpMy;
    SDL_GetMouseState(&tmpMx, &tmpMy);
    return onRect(rect, tmpMx, tmpMy);
}
bool onRect(SDL_Rect rect, int x, int y){
    if(rect.x <= x && rect.x + rect.w > x && rect.y <= y && rect.y + rect.h > y)
        return true;
    return false;
}

std::string reverseString(std::string in){
    std::string _return = "";
    for(int i = in.size() -1; i >= 0; i--)
        _return += in[i];
    return _return;
}

void selectDirectory(std::string label){
    //Check if its a folder.
    std::string extension = "";
    bool a_folder = true;

    //Check for some exceptions
    if(label == "./Makefile") return;

    for(int i = label.size() - 1; i > 0; i--){
        extension+=label[i];
        if(label[i] == '/') break;
        if(label[i] == '.'){
            extension = reverseString(extension);
            a_folder = false;
            break;
        }
    }    
    if(a_folder) CD = label;
    else 
        if(extension == ".bmp")
            loadBMP(label);
    return;
}

void loadBMP(std::string path){    
    //check if such one already exists
    for(sprite_struct &obj : sprites)
        if(obj.path == path)
            return;

    //Load surface and save information
    SDL_Surface *surf = SDL_LoadBMP(path.c_str());
    sprites.push_back({path, {WIDTH/2-surf->w/2, HEIGHT/2 - surf->h/2, surf->w, surf->h}, std::vector<int8_t>(std::ceil(float(surf->w*surf->h)/8)), SDL_CreateTextureFromSurface(rend, surf)});
    
    //Surface for creating transparency mask.
    surfaceManipulation transpMask;
    transpMask.createSurface(surf->w, surf->h, 32, SDL_PIXELFORMAT_RGBA32);

    //Save all 'intangible' points by filling a vector with 11111111's, indicating non_complete_transparency.
    std::fill(sprites[sprites.size()-1].alphas.begin(), sprites[sprites.size()-1].alphas.end(), 255);

    //And finding all points where the image is completely transparent and change vector[byte][bit] value to 0, indicating complete_transparency.
    SDL_LockSurface(surf);
    int *red{new int(0)}, *green{new int(0)}, *blue{new int(0)}, *alfa{new int(0)};
    for(int _y = 0; _y < surf->h; _y++)
        for(int _x = 0; _x < surf->w; _x++){
            Uint32 _colorValues = *(Uint32*)((Uint8*)((Uint8*)surf->pixels + _y*surf->pitch + _x*surf->format->BytesPerPixel));
            SDL_GetRGBA(_colorValues, surf->format, (Uint8*)red, (Uint8*)green, (Uint8*)blue, (Uint8*)alfa);

            if(*alfa == 0){
                int byteNumb = (_y*surf->w + _x)/8;
                int bitNumb = (_y*surf->w + _x)%8;
                std::string binary = std::bitset<8>(sprites[sprites.size()-1].alphas[byteNumb]).to_string();
                binary[bitNumb] = '0';
                sprites[sprites.size()-1].alphas[byteNumb] = (int8_t)std::bitset<8>(binary).to_ulong();
                transpMask.drawToSurface({_x, _y}, {255, 0, 0, 100});
            }
            else
                transpMask.drawToSurface({_x, _y}, {0, 255, 0, 100});
        }
    sprites[sprites.size()-1].transparencyMask = transpMask.createTextureAndDeleteSurface(rend);

    //Clean up
    delete red, green, blue, alfa;
    SDL_UnlockSurface(surf);
    SDL_FreeSurface(surf);
}

void renderSprites(bool transparencyMask){
    for(sprite_struct &obj : sprites){
        SDL_Rect pos = {obj.transform.x, obj.transform.y, int(obj.transform.w*obj.transform.scale_x), int(obj.transform.h*obj.transform.scale_y)};
        SDL_RenderCopyEx(rend, obj.texture, NULL, &pos, obj.transform.angle, NULL, SDL_FLIP_NONE);
        if(transparencyMask) SDL_RenderCopyEx(rend, obj.transparencyMask, NULL, &pos, obj.transform.angle, NULL, SDL_FLIP_NONE);
    }
}

void UI::scrollBox(GUItypes type, SDL_Rect box, std::vector<std::string> &entries, int fontSize, void (*onClick)(std::string)){
    int sliderWidth = 8;
    int y;

    if(old != entries){
        old = entries;
        shift = 0;
    }

    //Display the bar slot
    SDL_Rect sliderBox;
    SDL_Rect sliderSlot = {box.x + box.w - sliderWidth, box.y, sliderWidth, box.h};
    SDL_RenderDrawRect(rend, &sliderSlot);

    //Calculate the heigth of the bar and display it;
    if((1 + fontSize*entries.size()) > box.h){
        double height = (double(box.h*box.h)/((1+fontSize)*(entries.size()+2)));

        sliderBox = {box.x + box.w - sliderWidth, box.y + shift, sliderWidth, (int)height};
        SDL_SetRenderDrawColor(rend, 0,0,0,140);
        SDL_RenderFillRect(rend, &sliderBox);

    //Scroll Functionallity
        if(evt.type == SDL_MOUSEBUTTONDOWN && evt.button.button == SDL_BUTTON_LEFT && onRect(sliderBox)){
            scrollBarPressed = true;
            SDL_GetMouseState(NULL, &delta);
        }
        if(evt.type == SDL_MOUSEBUTTONUP && evt.button.button == SDL_BUTTON_LEFT)
            scrollBarPressed = false;

        if(scrollBarPressed){
            SDL_GetMouseState(NULL, &y);
            shift += y - delta;
            delta = y;
            if(shift < 0) shift = 0;
            if(shift+(int)height > box.h) shift = box.h - (int)height;
        }
    }

    //Show directories and files
    if(type == GUItypes::BUTTON){
        for(int i = 0; i < entries.size(); i++){
            int y_position = box.y + 1 - (double(shift)/box.h)*fontSize*entries.size() + fontSize*i;

            if(box.y-y_position <= 0){
                if(y_position - fontSize> box.h) break;
                SDL_Rect butBox = {box.x, y_position, box.w-sliderWidth, fontSize};
                if(onRect(butBox) && evt.type == SDL_MOUSEBUTTONDOWN && evt.button.button == SDL_BUTTON_LEFT)
                    onClick(entries[i]);
                renderText(entries[i], butBox, fontSize, false);
            }
        }
    }
}

void loadFont(int fontSize){
    FT_Open_Args args;
    args.flags = FT_OPEN_PATHNAME;
    char fontpath[]= "./assets/fonts/OpenSans-Regular.ttf";
    args.pathname = fontpath;
    if(FT_Open_Face(ft, &args, 0, &face)) std::cout << "Failed to load fonts!\n";
    face->glyph->format = FT_GLYPH_FORMAT_BITMAP;
    FT_Set_Pixel_Sizes(face, fontSize, fontSize);
}

void UI::renderText(std::string sentence, SDL_Rect textBox, int fontSize, bool newLines){
    symEndPos.clear();
    symEndPos.push_back(0);
    std::vector<std::string> words;
    std::string ind_word{""};
    FT_Bitmap ftbitmap;

    loadFont(fontSize);

    //Save each word and whitespace in a vector
    for(int i = 0; i < sentence.size(); i++){
        if(sentence[i] == ' '){
            words.push_back(ind_word);
            ind_word = "";
        }
        else ind_word += sentence[i];
    }
    //save the last one
    words.push_back(ind_word);

    //Display all characters
    int totalWidth = 1;
    int linePos = 1;

    for(std::string word : words){
        for(char symb : word){
            FT_Load_Char(face, symb, FT_LOAD_RENDER);

            ftbitmap = face->glyph->bitmap;

            //Create a surface and apply palette's colros
            SDL_Surface* glyph = SDL_CreateRGBSurfaceFrom(ftbitmap.buffer, ftbitmap.width, ftbitmap.rows, 8, ftbitmap.pitch, 0, 0, 0, 0xFF);
            SDL_SetPaletteColors(glyph->format->palette, colors, 0, 256);
            SDL_SetSurfaceBlendMode(glyph, SDL_BlendMode::SDL_BLENDMODE_ADD);    

            //Letterbox
            int belowBaseLine = (face->glyph->metrics.height - face->glyph->metrics.horiBearingY)/55;
            SDL_Rect pos = {textBox.x + totalWidth, int(textBox.y + linePos + (fontSize-fontSize/5) - glyph->h + belowBaseLine), glyph->w, glyph->h};

            totalWidth+=glyph->w;
            if(newLines && totalWidth >= textBox.w){
                linePos+=fontSize;
                totalWidth = 1 + glyph->w;
                pos.x = textBox.w + 1;
                pos.y = int(textBox.y + linePos + (fontSize-fontSize/5) - glyph->h + belowBaseLine);
            }

            //Display the symbol
            SDL_Texture *txtr = SDL_CreateTextureFromSurface(rend, glyph);
            //Don't render outside the bounds!
            SDL_Rect renderArea = {0, 0, std::min(textBox.w-totalWidth, glyph->w), std::min(textBox.h-linePos, glyph->h)};
            SDL_RenderCopy(rend, txtr, &renderArea, &pos);

            //clean up
            SDL_FreeSurface(glyph);
            SDL_DestroyTexture(txtr);
            symEndPos.push_back(totalWidth);
        }
        totalWidth+=fontSize/4;
    }

    FT_Done_Face(face);
}

bool isOnTransparentPoint(sprite_struct &obj){
    int x, y;
    SDL_GetMouseState(&x, &y);
    double angle = (360-obj.transform.angle)*3.14/180;
    int w = std::roundf(obj.transform.w * obj.transform.scale_x);
    int h = std::roundf(obj.transform.h * obj.transform.scale_y);

    int relative_x = std::roundf((x-obj.transform.x-w/2) * std::cosf(angle) - (y-obj.transform.y-h/2) * std::sinf(angle));
    int relative_y = std::roundf((x-obj.transform.x-w/2) * std::sinf(angle) + (y-obj.transform.y-h/2) * std::cosf(angle));
    
    relative_x+=w/2;
    relative_y+=h/2;
    int indx = ((double)relative_y/obj.transform.scale_y)*obj.transform.w + (double)relative_x/obj.transform.scale_x;
    relative_x+=obj.transform.x;
    relative_y+=obj.transform.y;

    if(!onRect({obj.transform.x, obj.transform.y, w, h}, relative_x, relative_y)) return false;
    if(std::bitset<8>(obj.alphas[indx/8]).to_string()[indx%8] == '1') return true;
    return false;
}

void moveSprites(){
    static bool onButton = false;
    static int index = 0;
    static int prev_x, prev_y;
    int c_x, c_y;
    SDL_GetMouseState(&c_x, &c_y);

    for(int i = sprites.size()-1; i>=0; i--){
        if(evt.type == SDL_MOUSEBUTTONDOWN && evt.button.button == SDL_BUTTON_LEFT){
            if(isOnTransparentPoint(sprites[i])){
                prev_x = c_x;
                prev_y = c_y;
                onButton = true;
                index = i;
                break;
            }
        }
        if(evt.type == SDL_MOUSEBUTTONUP && evt.button.button == SDL_BUTTON_LEFT){
            onButton = false;            
            break;
        }
    }

    if(onButton){
        sprites[index].transform.x += c_x-prev_x;
        sprites[index].transform.y += c_y-prev_y;

        prev_x = c_x;
        prev_y = c_y;
    }
}

void selectActiveSprite(std::string in){
    if(selectedSprite == in) selectedSprite = "";
    else selectedSprite = in;
}

void inspector(SDL_Rect box, int fontSize){
    int x, y;
    SDL_GetMouseState(&x, &y);
    
    //Borders and lines
    SDL_SetRenderDrawColor(rend, 255,255,255,255);
    inspector_class.renderText("Inspector", {box.x + 2, box.y - fontSize - 3, box.w, fontSize}, fontSize, false);
    SDL_RenderDrawRect(rend, &box);
    SDL_RenderDrawLine(rend, box.x, box.y + box.h*2/10-1, box.x + box.w-1, box.y + box.h*2/10-1);
    SDL_RenderDrawLine(rend, box.x + 15, box.y+box.h*2/10+25+3*fontSize, box.x + box.w - 15, box.y+box.h*2/10+25+3*fontSize);
    SDL_RenderDrawLine(rend, box.x, box.y+box.h*2/10+6*fontSize+45, box.x + box.w, box.y+box.h*2/10+6*fontSize+45);

    // * Section "List of active sprites"

    //Get all loaded paths into a string.
    std::vector<std::string> loaded;
    for(int i = sprites.size()-1; i >= 0; i--)
        loaded.push_back(sprites[i].path);
    //Create a scroll box:
    if(loaded.size() == 0){
        inspector_class.renderText("Active sprites.", {box.x + 2, box.y + 5, box.w - 2, fontSize+4}, fontSize+4, false);
        inspector_class.renderText("Select a .bmp file from the explorer to open one.", {box.x + 2, box.y + 7 + fontSize+4, box.w - 2, fontSize-4}, fontSize-4, false);
    }
    else inspector_class.scrollBox(GUItypes::BUTTON, {box.x, box.y, box.w, box.h*2/10}, loaded, fontSize-4, &selectActiveSprite);

    // * Section "Selected sprite - it's details"
    int detailsAreaY = box.y+box.h*2/10-1+12;
    std::string selected = "enoN";
    for(int i = selectedSprite.size()-1; i >0; i--){
        if(selectedSprite.size()-1 == i) selected = "";
        if(selectedSprite[i] == '/') break;
        else selected+=selectedSprite[i];
    }
    selected = reverseString(selected);
    
    inspector_class.renderText("Selected sprite:", {box.x + 15, detailsAreaY, box.w - 15, fontSize}, fontSize, false);
    inspector_class.renderText(selected, {box.x + 30, detailsAreaY+fontSize+3, box.w - 30, fontSize}, fontSize, false);
    inspector_class.renderText(selectedSprite, {box.x + 30, detailsAreaY+2*fontSize+6, box.w - 30, fontSize-4}, fontSize-4, false);

    //Selected sprite's text input boxes for scale and rotation.
    if(selected != "None"){
        //Get new y position and index of the selected sprite.
        detailsAreaY = box.y+box.h*2/10+3*fontSize+31; 
        int selectedIndex;
        for(int i = 0; i < sprites.size(); i++) if(sprites[i].path == selectedSprite) selectedIndex = i;
        
        //Convert value to string, show it as an editable text box, translate the value, which can be modified, back to an int.
        specStat1.inspectorSpecificValue(sprites[selectedIndex].transform.scale_x, "Scale X: ", {box.x + 30, detailsAreaY, 170, fontSize}, fontSize);
        specStat2.inspectorSpecificValue(sprites[selectedIndex].transform.scale_y, "Scale Y: ", {box.x + 30, detailsAreaY+fontSize+3, 170, fontSize}, fontSize);
        specStat3.inspectorSpecificValue(sprites[selectedIndex].transform.angle, "Angle: ", {box.x + 30, detailsAreaY+2*fontSize+6, 170, fontSize}, fontSize);

        //Buttons "push to front", "push to back", "unload".
        inspector_class.buttonDrawn("Up", {box.x + 170, detailsAreaY, 25, fontSize}, 14, &pushToFront, &selectedIndex);
        inspector_class.buttonDrawn("Down", {box.x + 195, detailsAreaY, 50, fontSize}, 14, &pushToBack, &selectedIndex);
        inspector_class.buttonDrawn("Del", {box.x + 245, detailsAreaY, 30, fontSize}, 14, &unload, &selectedIndex);
    }

    // * Section "List of sliders"
    detailsAreaY = box.y+box.h*2/10+6*fontSize+50;
    inspector_class.renderText("Tracked angle sliders: ", {box.x + 15, detailsAreaY, box.x-30, fontSize}, fontSize, false);
}

void pushToFront(void* data){
    int index = *static_cast<int*>(data);
    if(index + 1 == sprites.size()) return; 
    if(index + 2 == sprites.size()) sprites.resize(index+2);
    sprites.insert(sprites.begin() + index + 2, sprites[index]);
    sprites.erase(sprites.begin()+index);
    sprites.shrink_to_fit();
}
void pushToBack(void* data){
    int index = *static_cast<int*>(data);
    if(index == 0) return;
    sprites.insert(sprites.begin() + index-1, sprites[index]);
    sprites.erase(sprites.begin() + index + 1);
}
void unload(void* data){
    int index = *static_cast<int*>(data);
    SDL_DestroyTexture(sprites[index].transparencyMask);
    SDL_DestroyTexture(sprites[index].texture);
    sprites.erase(sprites.begin() + index);
    sprites.shrink_to_fit();
    selectedSprite = "";
}

void UI::inspectorSpecificValue(double &value, std::string name, SDL_Rect box, int fontSize){
    std::string tmp = doubleToString(value, 2);
    renderText(name, box, fontSize, false);
    textInput(tmp, true, {box.x+symEndPos[symEndPos.size()-1] + fontSize/5, box.y, box.w, fontSize}, fontSize);
    value = stringToDouble(tmp);
}

void UI::textInput(std::string &in, bool onlyNUMB, SDL_Rect box, int fontsize){
    int x, y;
    SDL_GetMouseState(&x, &y);

    if(evt.type == SDL_MOUSEBUTTONDOWN && evt.button.button == SDL_BUTTON_LEFT){
        if(onRect(box)){
            selectedText = true;
            input = in;
            symIndx = in.size();
        }
        else{
            selectedText = false;
            input = "";
        }
    }

    //Find how many commas or dots the value has.
    numbOfCommas = 0;
    for(char symb : input) if(symb == '.' || symb == ',') numbOfCommas++;

    //Get text input
    if(selectedText && evt.type == SDL_TEXTINPUT){
        std::string newWords = evt.text.text;
        if(onlyNUMB){
            //Ignore all numbers;
            for(int i = 0; i < newWords.size(); i++){
                if((((int)newWords[i]<48||(int)newWords[i]>57)&&newWords[i]== '.'&&newWords[i]==',') || (numbOfCommas > 0 && (newWords[i]==','||newWords[i]=='.')))
                    newWords.erase(newWords.begin() + i);
            }
        }
        newWords.shrink_to_fit();
        if(newWords.size() != 0){
            input.insert(symIndx, newWords);
            symIndx++;
        }
    }

    //Check for specific key presses
    if(selectedText && evt.type == SDL_KEYDOWN){
        switch(evt.key.keysym.sym){
            case SDLK_BACKSPACE:
                if(input.size() > 0 && symIndx > 0){
                    input.erase(input.begin()+symIndx-1);
                    symIndx--;
                }
                break;
            case SDLK_ESCAPE:
                selectedText = false;
                input = "";
                break;
            case SDLK_RETURN:
                selectedText = false;
                in = input;
                break;
            case SDLK_LEFT:
                symIndx--;
                break;
            case SDLK_RIGHT:
                symIndx++;
                break;
        }
        input.shrink_to_fit();
        if(symIndx>input.size()) symIndx = input.size();
        if(symIndx<0) symIndx = 0;
    }
    
    //Render value.
    if(selectedText) renderText(input, box, fontsize, false);
    else renderText(in, box, fontsize, false);

    //Draw underline
    SDL_SetRenderDrawColor(rend, 255,255,255,255);
    SDL_RenderDrawLine(rend, box.x, box.y + box.h, box.x + symEndPos[symEndPos.size()-1], box.y + box.h);

    //Current pos `mark`(?)
    if(selectedText)
        SDL_RenderDrawLine(rend, box.x + symEndPos[symIndx], box.y, box.x + symEndPos[symIndx], box.y + box.h);

}

std::string intToString(int numb){
    std::string _return_backwards = "";

    if(numb == 0)
        _return_backwards += "0";
    while(numb != 0){
        _return_backwards += char(numb%10+48);
        numb/=10;
    }

    std::string _return = "";
    for(int i = _return_backwards.size()-1; i>=0; i--)
        _return+=_return_backwards[i];

    return _return;
}

int stringToInt(std::string string){
    if(string.size() == 0) return 1;

    int _return = 0;
    int multiplier = 1;
    for(int i = string.size()-1; i >= 0; i--){
        if(i == 0 && string[i] == '-') _return*=-1;
        else if(i!=0 && string[i] == '-') return 0;

        _return += ((int)string[i]-48)*multiplier;
        multiplier*=10;
    }

    return _return;
}

//can be a dot or a comma.
double stringToDouble(std::string in){
    double _return = 0;
    if(in.size() == 0) return 1;

    int k = 0;
    if(in[0] == '-') k = 1;
    double multiplier = 0.1;
    while(true){
        if(in.size() == k) break;
        if(in[k] == ',' || in[k] == '.')
            break;
        k++;
        multiplier*=10;
    }
    
    for(int i = 0; i<in.size(); i++){
        if(in[i] != '-' && in[i] != ',' && in[i] != '.'){
            _return += ((int)in[i]-48)*multiplier;
            multiplier/=10;
        }
    }

    if(in[0] == '-') _return*=-1;
    return _return;
}

std::string doubleToString(double in, int precision){
    std::string _return = "";
    if(in == 0) return "0";

    //Seperate integer and fractional parts.
    int whole = int(in);
    double fraction = in - whole;

    //Save the integer part.
    _return = intToString(whole);
    _return += ".";

    //Save fractional part to the provided precision.
    _return += intToString(int(fraction*std::pow(10, precision)));

    return _return;
}

//I'm testing whether this approach is better, if it is, apply everywhere.
surfaceManipulation timeLineSurf;
struct visualData 
{
    bool wasUpdated = true;
    SDL_Texture *texture;
};

visualData timeLineBackground;
UI2 test;
void timeline(SDL_Rect box){
    if(timeLineBackground.wasUpdated){
        surfaceManipulation::color white = {255,255,255,255};

        SDL_DestroyTexture(timeLineBackground.texture);
        timeLineSurf.createSurface(box.w, box.h, 32, SDL_PIXELFORMAT_RGBA32);
        for(int x = 0; x < box.w; x++)
            for(int y = 0; y < box.h; y++)
                timeLineSurf.drawToSurface({x, y}, {bg.r, bg.g, bg.b, bg.a});
        
        for(int x = 0; x< box.w; x++){
            timeLineSurf.drawToSurface({x, 0}, white);
            timeLineSurf.drawToSurface({x, box.h-1}, white);
        }
        for(int y = 0; y < box.h; y++){
            timeLineSurf.drawToSurface({0, y}, white);
            timeLineSurf.drawToSurface({box.w-1, y}, white);
        }

        timeLineBackground.texture = timeLineSurf.createTextureAndDeleteSurface(rend);
        timeLineBackground.wasUpdated = false;
    }
    SDL_RenderCopy(rend, timeLineBackground.texture, NULL, &box);
    SDL_Rect dk = {500, 500, 100, 100};
    SDL_RenderCopy(rend, test.renderText("something", dk, 18, false), NULL, &dk);
}

SDL_Texture* UI2::renderText(std::string sentence, SDL_Rect textBox, int fontsize, bool newLines){
    //Return an existing surface, if there is one.
    SDL_Texture* textr = findExistingText(sentence, textBox);
    if(textr != nullptr) return textr;
    SDL_DestroyTexture(textr);

    //needed variables
    textInfo.widthSymEnd.clear();
    std::vector<std::string> words;
    std::string ind_word{""};

    FT_FaceRec_* FACE = useFont("./assets/fonts/OpenSans-Regular.ttf", fontsize);

    //Save each word and whitespaces in a vector
    for(char symb : sentence){
        if(symb == ' '){
            words.push_back(ind_word);
            ind_word = "";
        }
        else ind_word += symb;
    }
    //save the last one.
    words.push_back(ind_word);

    //Create a surface where the sentence will be stored.
    surfaceManipulation manip;
    SDL_Color col;
    manip.createSurface(textBox.w, textBox.h, 32, SDL_PIXELFORMAT_RGBA32);

    //Get all information about the words we want to display.
    int totalWidth = 1;
    int totalHeight = 1;
    int belowBaseLine = 0;
    for(std::string word : words){
        for(char symb : word){
            FT_Load_Char(FACE, symb, FT_LOAD_RENDER);
            belowBaseLine = (FACE->glyph->metrics.height - FACE->glyph->metrics.horiBearingY)/55;
            SDL_Surface *glyph;

            if(newLines && totalWidth + FACE->glyph->metrics.width >=textBox.w-1){
                //Take into consideration '-' symbol.
                FT_Load_Char(FACE, '-', FT_LOAD_RENDER);
                belowBaseLine = (FACE->glyph->metrics.height - FACE->glyph->metrics.horiBearingY)/55;
                
                glyph = SDL_CreateRGBSurfaceFrom(FACE->glyph->bitmap.buffer, FACE->glyph->bitmap.width, FACE->glyph->bitmap.rows, 8, FACE->glyph->bitmap.pitch, 0, 0, 0, 0xFF);
                for(int x = 0; x < FACE->glyph->bitmap.width; x++){
                    for(int y = 0; y < FACE->glyph->bitmap.rows; y++){
                        col = manip.getSurfaceColors({x,y}, glyph, false);
                        manip.drawToSurface({x+totalWidth, y+totalHeight}, {col.r, col.g, col.b, col.a});
                    }
                }
                SDL_FreeSurface(glyph);

                FT_Load_Char(FACE, symb, FT_LOAD_RENDER);
                belowBaseLine = (FACE->glyph->metrics.height - FACE->glyph->metrics.horiBearingY)/55;

                totalHeight += 1;
                totalWidth = 1;
            }
            glyph = SDL_CreateRGBSurfaceFrom(FACE->glyph->bitmap.buffer, FACE->glyph->bitmap.width, FACE->glyph->bitmap.rows, 8, FACE->glyph->bitmap.pitch, 0, 0, 0, 0xFF);
            for(int x = 0; x < FACE->glyph->bitmap.width; x++){
                for(int y = FACE->glyph->bitmap.rows; y < FACE->glyph->bitmap.rows; y--){
                    col = manip.getSurfaceColors({x,y}, glyph, false);
                    //cia aukstyn kojom jie!
                    manip.drawToSurface({x+totalWidth, y+totalHeight}, {col.r, col.g, col.b, col.a});
                }
            }
            SDL_FreeSurface(glyph);

            //Update some info.
            totalWidth+=FACE->glyph->metrics.width;
            textInfo.widthSymEnd.push_back({totalWidth, totalHeight});
        }
        totalWidth += fontsize/4;
    }
    renderedTexts.push_back({manip.createTextureAndDeleteSurface(rend), sentence, 0, textBox});
    return renderedTexts[renderedTexts.size()-1].texture;
}

bool checkIfRectsEqual(SDL_Rect rect1, SDL_Rect rect2){
    if(rect1.x == rect2.x && rect1.w == rect2.w && rect1.h == rect2.h && rect1.y == rect2.y) return true;
    return false;
}

SDL_Texture* UI2::findExistingText(std::string &sentence, SDL_Rect &textBox){
    for(renderedTexts_struct &obj : renderedTexts){
        if(obj.sentence == sentence && checkIfRectsEqual(textBox, obj.dimensions)){
            obj.lastAccessed = 0;
            return obj.texture;
        }
    }
    return nullptr;
}

FT_FaceRec_* UI2::useFont(std::string path, int fontSize){
    //Check if a specific sized font is already loaded.
    for(loadedFaces_struct &obj : loadedFaces)
        if(obj.path == path && obj.fontSize == fontSize) return obj.face;
    
    //Load a font of specific size.
    loadedFaces.push_back({path, fontSize});
    int i = loadedFaces.size()-1;

    FT_Open_Args args;
    args.flags = FT_OPEN_PATHNAME;
    args.pathname = path.data();

    FT_Init_FreeType(&loadedFaces[i].ft);
    if(FT_Open_Face(loadedFaces[i].ft, &args, 0, &loadedFaces[i].face)) std::cout << "Failed to load font ('" << path << "') of size " << fontSize << ". Tried index: " << i <<"\n";
    
    loadedFaces[i].face->glyph->format = FT_GLYPH_FORMAT_BITMAP;
    FT_Set_Pixel_Sizes(loadedFaces[i].face, fontSize, fontSize);

    return loadedFaces[i].face;
}  