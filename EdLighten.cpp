#include "EdLighten.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <queue>
#include <cmath>

using namespace imaging;

out_file_type imaging::to_file_type(std::string file_type){
    if (file_type == "bmp") return bmp;
    else if (file_type == "jpg") return jpg;
    return png;
}

pixel imaging::to_pixel(int x){
    if (x > BRIGHT) return (pixel) 255;
    if (x < DARK) return (pixel) 0;
    return (pixel) x;
}

//R,B,G = DARK (0) ... BRIGHT (255), H = 0...359, S,L = 0 ... 255
HSL_value imaging::RGB_to_HSL(RGB_value rbg){
    int R = (int) (rbg[0]); int G = (int) (rbg[1]); int B = (int) (rbg[2]);
    int max_pixel, middle_pixel, min_pixel, h1, h2;
    if (R >= G && G >= B){ //orange
        max_pixel = R; middle_pixel = G; min_pixel = B; h1 = 0; h2 = 1;
    }
    else if (G > R && R >= B){ //Chartreuse
        max_pixel = G; middle_pixel = R; min_pixel = B; h1 = 2; h2 = -1;
    }
    else if (G >= B && B > R) { //Spring green
        max_pixel = G; middle_pixel = B; min_pixel = R; h1 = 2; h2 = 1;
    }
    else if (B > G && G > R){  //Azure
        max_pixel = B; middle_pixel = G; min_pixel = R; h1 = 4; h2 = -1;
    }
    else if (B >= R && R >= G){ //Violet
        max_pixel = B; middle_pixel = R; min_pixel = G; h1 = 4; h2 = 1;
    }
    else{ //Rose if (R >= B && B >= G)
        max_pixel = R; middle_pixel = B; min_pixel = G; h1 = 6; h2 = -1;
    }
    int h = (max_pixel == min_pixel)? 0 : 60 * h1 + h2*60*(middle_pixel - min_pixel)/(max_pixel - min_pixel);
    pixel l = (pixel) ((max_pixel + min_pixel)/2);
    pixel s = (l == 0)? (pixel) 0 : to_pixel(BRIGHT*(max_pixel - min_pixel)/(BRIGHT - std::abs(2* (int)l - BRIGHT)));
    HSL_value hsl; hsl.H = h; hsl.S = s; hsl.L = l; 
    return hsl; 
}

//R,B,G = DARK (0) ... BRIGHT (255), H = 0...359, S,L = 0 ... 255
RGB_value imaging::HSL_to_RGB(HSL_value hsl){
    int h = hsl.H; int s = hsl.S; int l = hsl.L;
    float range = s*(BRIGHT - std::abs(2*l - BRIGHT))/BRIGHT;
    int min_pixel = l - range /2; 
    pixel R,G,B;
    if (0 <= h && h < 60){ //orange
        R = to_pixel(min_pixel + range); B = to_pixel(min_pixel); G = to_pixel(range*h/60.0 + min_pixel);
    }
    else if (60 <= h && h < 120){ //Chartreuse
        G = to_pixel(min_pixel + range); B = to_pixel(min_pixel); R = to_pixel(range*(120-h)/60.0 + min_pixel);
    }
    else if (120 <= h && h < 180) { //Spring green
        G = to_pixel(min_pixel + range); R = to_pixel(min_pixel); B = to_pixel(range*(h-120)/60.0 + min_pixel);
    }
    else if (180 <= h && h < 240){  //Azure
        B = to_pixel(min_pixel + range); R = to_pixel(min_pixel); G = to_pixel(range*(240-h)/60.0 + min_pixel);
    }
    else if (240 <= h && h < 300){ //Violet
        B = to_pixel(min_pixel + range); G = to_pixel(min_pixel); R= to_pixel(range*(h-240)/60.0 + min_pixel);
    }
    else{ //Rose if (R >= B && B >= G)
        R = to_pixel(min_pixel + range); G = to_pixel(min_pixel); B = to_pixel(range*(360-h)/60.0 + min_pixel);
    }
    RGB_value rgb = {R,G,B};
    return rgb;
}
namespace{
    pixel piece_lin_tran(pixel p, int add_value){
        pixel tran_p;
        if (add_value > 0) tran_p = to_pixel((BRIGHT - p)*(add_value/100.0) + p);
        else tran_p = to_pixel(p*(1 + add_value/100.0));
        return tran_p;
    }
}


void image::free_image(){
    free(data);
    width = 0; height = 0; channels = 0;
}

//image from file
image::image(std::string file_name, int desired_channels){
	int w, h, ch;
	data = stbi_load(file_name.c_str(), &w, &h, &ch, desired_channels);
    if (data == NULL) exit(1);
    width = w; height = h; channels = (desired_channels == 0)? ch : desired_channels;
}

image image::duplicate(){
    image img = image(width, height, channels);
    std::copy(data, data + width * height * channels, img.data);
    return img;
}

//plain image
image::image(int input_width, int input_height, int input_desired_channels){
    try{
        width = input_width; height = input_height; channels = input_desired_channels;
        if (width < 1 || height < 1 || channels < 1 || channels > 4)
            throw 1;
    }
    catch(...){
        printf( "Invaild parameters!/n" );
        width = 1; height = 1; channels = 1;
    }
    data = (pixel*) malloc(sizeof(pixel)*width*height*channels);
    if (data == NULL) exit(1);
}

//plain image
image::image(int input_width, int input_height, int transparency, RGB_value color){
    if (transparency < DARK) transparency = DARK;
    if (transparency > BRIGHT) transparency = BRIGHT;
    try{
        width = input_width; height = input_height; channels = 4;
        if (width < 1 || height < 1)
            throw 1;
    }
    catch(...){
        printf( "Invaild parameters!/n" );
        width = 1; height = 1; channels = 4;
    }
    data = (pixel*) malloc(sizeof(pixel)*width*height*channels);
    if (data == NULL) exit(1);
    for (int i = 0; i < width * height * 4; i += 4){
        data[i] = color[0];
        data[i+1] = color[1];
        data[i+2] = color[2];
        data[i+3] = transparency;
    }
}

pixel * image::get_all_pixels() {
    pixel * temp = (pixel *) malloc(width*height*channels*sizeof(pixel));
    std::copy(data, data + width * height * channels, temp);
    return temp;
} 
//x = 0 ... width -1, y = 0 ... height - 1, colour_no = 0 ... channel -1
pixel image::get_pixel(int x, int y, int channel_no){
    if (x < 0 || x >= width || y < 0 || y >= height || channel_no < 0 || channel_no >= channels) return (pixel) DARK;
    return data[(x + y * width) * channels + channel_no];
}

void image::change_pixel(int x, int y, int channel_no, pixel color){
    if (x < 0 || x >= width || y < 0 || y >= height || channel_no < 0 || channel_no >= channels) return;
    data[(x + y * width) * channels + channel_no] = color;
}

void image::change_pixel(int position, pixel color){
    if (position < 0 || position >= get_size()) return;
    data[position] = color;
}

//enum out_file_type{bmp, jpg, png};
int image::write_image_custom_ext(std::string file_name, out_file_type type, int compress){
    switch(type){
        case jpg:
            return stbi_write_jpg(file_name.c_str(), width, height, channels, data, compress);
        case png:
            return stbi_write_png(file_name.c_str(), width, height, channels, data, width*channels);
        default:
            return stbi_write_bmp(file_name.c_str(), width, height, channels, data);
    }
}

//enum out_file_type{bmp, jpg, png};
int image::write_image(std::string file_name, out_file_type type, int compress){
    switch(type){
        case jpg:
            return stbi_write_jpg((file_name + ".jpg").c_str(), width, height, channels, data, compress);
        case png:
            return stbi_write_png((file_name + ".png").c_str(), width, height, channels, data, width*channels);
        default:
            return stbi_write_bmp((file_name + ".bmp").c_str(), width, height, channels, data);
    }
}

image image::to_four_channels(){
    if (channels == 0) return *this;
    if (channels == 4) return *this;
    if (channels == 3) return this->add_alpha_channel();
    image grey_img = (channels == 2)? this->duplicate(): this->duplicate().add_alpha_channel();
    image tran_img = image(width, height, 4);
    for (int i = 0, j = 0; i < width * height * 4; i += 4, j += 2){
        tran_img.data[i] = grey_img.data[j];
        tran_img.data[i+1] = grey_img.data[j];
        tran_img.data[i+2] = grey_img.data[j];
        tran_img.data[i+3] = grey_img.data[j+1];
    }
    grey_img.free_image();
    this->free_image();
    return tran_img;
}

 image image::to_grey(){
    if (channels == 0) return *this;
    if (channels < 3) return *this;
    int grey_channels = (channels == 2 || channels == 4)? 2 : 1;
    image tran_img = image(width, height, grey_channels);
     if (channels == 2 || channels == 4) 
        for (int i = 0, j = 0 ; i < width * height * channels; i += channels, j += grey_channels)
            tran_img.data[j+grey_channels-1] = data[i+channels-1]; //alpha
    for (int i = 0, j = 0 ; i < width*height*channels; i += channels, j += grey_channels) 
        tran_img.data[j] = to_pixel((data[i] + data[i+1] + data[i+2]) / 3);
    this->free_image();
    return tran_img;
}; 

//from -100 to 100
image image::brightness(int add_value){
    if (channels == 0) return *this;
    if (add_value == 0) return *this;
    image tran_img = image(width, height, channels);
    if (channels == 2 || channels == 4) 
        for (int i = 0; i < width * height * channels; i += channels) tran_img.data[i+channels-1] = data[i+channels-1]; //alpha
    if (channels < 3)
        for (int i = 0; i < width * height * channels; i += channels)
            tran_img.data[i] = piece_lin_tran(data[i], add_value);
    else{
        for (int i = 0; i < width * height * channels; i += channels){
            HSL_value hsl = RGB_to_HSL({data[i], data[i+1], data[i+2]});
            hsl.L = piece_lin_tran(hsl.L, add_value);
            RGB_value rgb = HSL_to_RGB(hsl);
            tran_img.data[i] = rgb[0]; tran_img.data[i+1] = rgb[1]; tran_img.data[i+2] = rgb[2]; 
        }
    }
    this->free_image();
    return tran_img;
}

//from -100 to 100
image image::contrast(int add_value){
    if (channels == 0) return *this;
    if (add_value == 0) return *this;
    image tran_img = image(width, height, channels);
    int min_pixel = BRIGHT; int max_pixel = DARK;
    if (channels == 1 || channels == 2){
        if (channels == 2) 
            for (int i = 0; i < width * height * channels; i += channels) tran_img.data[i+1] = data[i+1]; //alpha
        for (int i = 0; i < width * height * channels; i += channels){
            if (max_pixel < (int) data[i]) max_pixel = (int) data[i];
            else if (min_pixel > (int) data[i]) min_pixel = (int) data[i];
        }
        int ori_diff = max_pixel - min_pixel;
        float tran_diff = (add_value > 0)? (BRIGHT - ori_diff)*add_value/100.0 + ori_diff 
                                                : ori_diff*(1 + add_value/100.0);
        int tran_min = (add_value > 0)? min_pixel *(1 - add_value/100.0)
                                                : min_pixel - ori_diff/200.0* add_value;
        for (int i = 0; i < width * height * channels; i += channels)
            tran_img.data[i] = to_pixel(((int) data[i] - min_pixel)*tran_diff/ori_diff + tran_min) ;
    }
    else {
        image grey_img = this->duplicate().to_grey();
        image grey_tran_img = grey_img.duplicate().contrast(add_value);
        if (channels == 4) 
            for (int i = 0; i < width * height * channels; i += channels) tran_img.data[i+3] = data[i+3]; //alpha
        for (int i = 0, j = 0; i < width * height * channels; i += channels, j += grey_img.channels){
            HSL_value hsl = RGB_to_HSL({data[i], data[i+1],data[i+2]});
            hsl.L = to_pixel(hsl.L* grey_tran_img.data[j] / grey_img.data[j]);
            RGB_value rgb = HSL_to_RGB(hsl);
            tran_img.data[i] = rgb[0]; tran_img.data[i+1] = rgb[1]; tran_img.data[i+2] = rgb[2]; 
        }
        grey_img.free_image(); grey_tran_img.free_image(); 
    }
    this->free_image();
    return tran_img;
}

//from -100 to 100
image image::saturation(int add_value){
    if (channels == 0) return *this;
    if (add_value == 0) return *this;
    if (channels < 3) return *this;
    image tran_img = image(width, height, channels);
    if (channels == 4) 
        for (int i = 0; i < width * height * channels; i += channels) tran_img.data[i+3] = data[i+3]; //alpha
    for (int i = 0; i < width * height * channels; i += channels){
        HSL_value hsl = RGB_to_HSL({data[i], data[i+1],data[i+2]});
        hsl.S = to_pixel(piece_lin_tran(hsl.S, add_value));
        RGB_value rgb = HSL_to_RGB(hsl);
        tran_img.data[i] = rgb[0]; tran_img.data[i+1] = rgb[1]; tran_img.data[i+2] = rgb[2]; 
    }
    this->free_image();
    return tran_img;
}

//from 0 to 255
image image::smoothness(int tolerance, int radius){
    if (channels == 0) return *this;
    if (tolerance < 0 || radius < 1 ) return *this;
    image tran_img = image(width, height, channels);
    int color_channels = channels;
    if (channels == 2 || channels == 4) {
        color_channels--;
        for (int i = 0; i < width * height * channels; i += channels) tran_img.data[i+channels-1] = data[i+channels-1]; //alpha
    }
    for (int j = 0; j < height ; j++){
        for (int i = 0; i < width; i++){
            for (int ch = 0; ch < color_channels; ch++){ //sigma filter
                int sum = 0; int count = 0; pixel current = data[(i+j*width)*channels+ch];
                for (int q = std::max(0,j-radius); q <= std::min(height-1, j+radius); q++){
                    for (int p = std::max(0,i-radius); p <= std::min(width-1, i+radius); p++){
                        if (std::abs(current - data[(p+q*width)*channels+ch]) < tolerance) {
                            sum += data[(p+q*width)*channels+ch]; count++;
                        }
                    }
                }
                if (count != 0) tran_img.data[(i+j*width)*channels+ch] = to_pixel(sum/count);
                else tran_img.data[(i+j*width)*channels+ch] = current;
            }
        }
    }
    this->free_image();
    return tran_img;
};

//from -100 to 100
image image::transparency(int add_value){
    if (channels == 0) return *this;
    if (add_value == 0) return *this;
    int tran_channels = (channels == 1 || channels == 3)? channels + 1: channels;
    image tran_img = image(width, height, tran_channels);
    switch (channels){
        case 1:
            for (int i = 0, j = 0; i < width * height * channels; i += channels, j += tran_channels){
                tran_img.data[j] = data[i];
                tran_img.data[j+1] = piece_lin_tran(BRIGHT,-add_value);
            }
            break;
        case 2:
            for (int i = 0, j = 0; i < width * height * channels; i += channels, j += tran_channels){
                tran_img.data[j] = data[i];
                tran_img.data[j+1] = piece_lin_tran(data[i+1],-add_value);
            }
            break;
        case 3:
            for (int i = 0, j = 0; i < width * height * channels; i += channels, j += tran_channels){
                tran_img.data[j] = data[i];
                tran_img.data[j+1] = data[i+1];
                tran_img.data[j+2] = data[i+2];
                tran_img.data[j+3] = piece_lin_tran(BRIGHT,-add_value);
            }
            break;        
        case 4:
            for (int i = 0, j = 0; i < width * height * channels; i += channels, j += tran_channels){
                tran_img.data[j] = data[i];
                tran_img.data[j+1] = data[i+1];
                tran_img.data[j+2] = data[i+2];
                tran_img.data[j+3] = piece_lin_tran(data[i+3],-add_value);
            }
            break;            
        default:
            break;
    }
    this->free_image();
    return tran_img;
}

image image::paint(std::vector<point> include_pts, std::array<pixel,3> color){
    if (channels == 0) return *this;
    if (include_pts.empty()) return *this;
    image tran_img = image(width, height, channels);
    for (int i = 0; i < width * height * channels; i++) tran_img.data[i] = data[i];
    if (channels < 3) color[0] = (color[0] + color[1] + color[2])/3;
    int color_channels = (channels < 3)? 1 : 3;
    for (point pt : include_pts){
        int x = pt[0]; int y = pt[1];
        if (x < 0 || x >= width || y < 0 || y >= height) continue;
        for (int i = 0; i < color_channels; i++) tran_img.data[(x + y*width)*channels + i] = color[i]; 
    }
    this->free_image();
    return tran_img;
}

//from 0 to 100
image image::sharpness(int add_value){
    if (channels == 0) return *this;
    if (add_value <= 0) return *this;
    image blur_img = this->duplicate().smoothness(50,2);
    image tran_img = image(width, height, channels);
    for(int i = 0; i < width*height*channels;i++)
        tran_img.data[i] = to_pixel(data[i] * (1+2*add_value/100.0) - blur_img.data[i] * 2*add_value/100.0);
    blur_img.free_image();
    this->free_image();
    return tran_img;
}

std::vector<std::vector<bool>> image::empty_segment(){
    std::vector<std::vector<bool>> segment(height, std::vector<bool>(width, false));
    return segment;
}

bool image::close_to(int torlance, RGB_value color, int x, int y){
    if (channels == 1 || channels == 2) return abs(data[(x+y*width)*channels] - color[0]) <= torlance;
    if (channels == 3|| channels == 4)
        return ((abs(data[(x+y*width)*channels] - color[0]) <= torlance) 
            && (abs(data[(x+y*width)*channels+1] - color[1]) <= torlance) 
            && (abs(data[(x+y*width)*channels+2] - color[2]) <= torlance)); 
    return false;
}

//torlance 0 ... 255
std::vector<std::vector<bool>> image::segment_include(int torlance, point include, std::vector<std::vector<bool>> segment){
    if (torlance < 0) return segment;
    std::queue<point> remains({include});
    int x = include[0]; int y = include[1];
    if (x < 0 || x >=width || y < 0 || y >= height) return segment;
    RGB_value color = {0,0,0};
    if (channels == 1 || channels == 2){
        pixel temp = data[(x+y*width)*channels];
        color[0] = temp; color[1] = temp; color[2] = temp;
    }
    if (channels == 3|| channels == 4){
        color[0] = data[(x+y*width)*channels]; color[1] = data[(x+y*width)*channels+1]; color[2] = data[(x+y*width)*channels+2];
    }
    while (!remains.empty()){
        point now = remains.front();
        int now_x = now[0]; int now_y = now[1];
        remains.pop();
        segment[now_y][now_x] = true;
        if (now_x > 0) {
            if (!segment[now_y][now_x-1] && close_to(torlance, color, now_x-1, now_y)){
                point side = {now_x-1,now_y};
                remains.push(side);
                segment[now_y][now_x-1] = true;
            }
        }
        if (now_x < width - 1) {
            if (!segment[now_y][now_x+1] && close_to(torlance, color, now_x+1, now_y)){
                point side = {now_x+1,now_y};
                remains.push(side);
                segment[now_y][now_x+1] = true;
            }
        }        
        if (now_y > 0) {
            if (!segment[now_y-1][now_x] && close_to(torlance, color, now_x, now_y-1)){
                point side = {now_x,now_y-1};
                remains.push(side);
                segment[now_y-1][now_x] = true;
            }
        }
        if (now_y < height - 1) {
            if (!segment[now_y+1][now_x] && close_to(torlance, color, now_x, now_y+1)){
                point side = {now_x,now_y+1};
                remains.push(side);
                segment[now_y+1][now_x] = true;
            }
        }        
    }
    return segment;
}

std::vector<std::vector<bool>> image::segment_exclude(int torlance, point exclude, std::vector<std::vector<bool>> segment){
    std::vector<std::vector<bool>> minus_segment = segment_include(torlance, exclude, empty_segment());
    for (int y = 0; y < height; y++){
        for (int x = 0; x < width; x++)
            segment[y][x] = (segment[y][x] && !minus_segment[y][x]);
    }
    for (auto row: minus_segment) row.clear();
    minus_segment.clear();
    return segment;
}

image image::select(std::vector<std::vector<bool>> segment, RGB_value non_select_color, int non_select_transparency){
    if (channels == 0) return *this;
    int tran_channels = (channels == 1 || channels == 3)? channels + 1: channels;
    image tran_img = image(width, height, tran_channels);
    if (channels != tran_channels)
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
                tran_img.data[(x+y*width+1)*tran_channels - 1 ] = BRIGHT;
    for (int y = 0; y < height; y++){
        for (int x = 0; x < width; x++){
            if (segment[y][x])
                for (int i = 0; i < channels; i++) tran_img.data[(x+y*width)*tran_channels + i] = data[(x+y*width)*channels + i];
            else {
                tran_img.data[(x+y*width + 1)*tran_channels - 1] = non_select_transparency;
                if (tran_channels == 4)
                    for (int i = 0; i < tran_channels; i++) tran_img.data[(x+y*width)*tran_channels + i] = non_select_color[i];
                else tran_img.data[(x+y*width)*tran_channels] = to_pixel((non_select_color[0] + non_select_color[1] + non_select_color[2])/3);
            }
        }
    }
    this->free_image();
    return tran_img;
}

image image::invert_select(std::vector<std::vector<bool>> segment, RGB_value non_select_color, int non_select_transparency){
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            segment[y][x] = !segment[y][x];
    return select(segment, non_select_color, non_select_transparency);
}

image image::filters(filter_step filters){
    if (filters.to_grey)
        return this->to_grey().smoothness(filters.smoothness_torlance,filters.smoothness_radius).brightness(filters.brightness).contrast(filters.contrast)
            .saturation(filters.saturation).sharpness(filters.sharpness).transparency(filters.transparency);
    else 
        return this->smoothness(filters.smoothness_torlance,filters.smoothness_radius).brightness(filters.brightness).contrast(filters.contrast)
            .saturation(filters.saturation).sharpness(filters.sharpness).transparency(filters.transparency);        
}

image image::transforms(transform_step transforms){
    if (channels == 0) return *this;
    if (transforms.x_scale == 0) transforms.x_scale = 0.01;
    if (transforms.y_scale == 0) transforms.y_scale = 0.01;
    int tran_channels = (channels == 1 || channels == 3)? channels + 1: channels;
    image tran_img = image(transforms.new_width, transforms.new_height, tran_channels);
    if (channels != tran_channels)
        for (int y = 0; y < transforms.new_height; y++)
            for (int x = 0; x < transforms.new_width; x++)
                tran_img.data[(x+y*transforms.new_width+1)*tran_channels - 1] = BRIGHT;
    for (int y = 0; y < transforms.new_height; y++){
        for (int x = 0; x < transforms.new_width; x++){
            int x_get = (transforms.start_x + x - width/2)/transforms.x_scale*std::cos(transforms.rotate) 
                                    + (transforms.start_y + y - height/2)/transforms.x_scale*std::sin(transforms.rotate) 
                                    + width/2;
            int y_get = - (transforms.start_x + x - width/2)/transforms.y_scale*std::sin(transforms.rotate) 
                                    + (transforms.start_y + y - height/2)/transforms.y_scale*std::cos(transforms.rotate) 
                                    + height/2;
            if (x_get >= 0 && x_get < width && y_get >= 0 && y_get < height){
                for (int i = 0; i < channels; i++) tran_img.data[(x+y*transforms.new_width)*tran_channels + i] = data[(x_get+y_get*width)*channels + i];
            }
            else{
                for (int i = 0; i < tran_channels; i++) tran_img.data[(x+y*transforms.new_width)*tran_channels + i] = DARK;
            }
        }
    }
    this->free_image();
    return tran_img;
}

image image::add_alpha_channel(){
    if (channels == 0) return *this;
    if (channels == 2 || channels == 4) return *this;
    int tran_channels = channels + 1;
    image tran_img = image(width, height, tran_channels);
    switch (channels){
        case 1:
            for (int i = 0, j = 0; i < width * height * channels; i += channels, j += tran_channels){
                tran_img.data[j] = data[i];
                tran_img.data[j+1] = BRIGHT;
            }
            break;
        case 3:
            for (int i = 0, j = 0; i < width * height * channels; i += channels, j += tran_channels){
                tran_img.data[j] = data[i];
                tran_img.data[j+1] = data[i+1];
                tran_img.data[j+2] = data[i+2];
                tran_img.data[j+3] = BRIGHT;
            }
            break;          
        default:
            break;
    }
    this->free_image();
    return tran_img;
}

image image::add(image img, int start_x, int start_y, int out_width, int out_height, int offset_x, int offset_y){
    if (channels == 0) return *this;
    int tran_channels = (img.channels >= 3 || channels >= 3) ? 4 : 2;
    image tran_img(out_width, out_height, tran_channels);
    image img_under = (channels == 2 || channels == 4)? this->duplicate(): this->duplicate().add_alpha_channel();
    image img_over = (img.channels == 2 || img.channels == 4)? img.duplicate(): img.duplicate().add_alpha_channel();
    for (int y = start_y;  y < out_height + start_y; y++){
        for (int x = start_x; x < out_width + start_x; x++){
            pixel under_alpha_pixel; pixel over_alpha_pixel;
            bool under_empty = (x < 0 || x >= img_under.width || y < 0 || y >= img_under.height);
            int over_x = x - offset_x; int over_y = y - offset_y;
            bool over_empty = (over_x < 0 || over_x >= img_over.width || over_y < 0 || over_y >= img_over.height);

            if (under_empty) under_alpha_pixel = DARK;
            else under_alpha_pixel = img_under.data[(x + y*img_under.width) * img_under.channels + img_under.channels - 1];
            if (over_empty) over_alpha_pixel = DARK;
            else over_alpha_pixel = img_over.data[(over_x + over_y*img_over.width) * img_over.channels + img_over.channels - 1];
            float under_appear_ratio = (1-over_alpha_pixel/BRIGHT) *under_alpha_pixel;
            float alpha_pixel = under_appear_ratio + over_alpha_pixel; 
            tran_img.data[((x - start_x) + (y-start_y)*out_width)*tran_channels + tran_channels - 1] = to_pixel(alpha_pixel);
            for (int i = 0; i < tran_channels - 1; i++) {
                pixel under_pixel; pixel over_pixel;
                if (under_empty) under_pixel = DARK;
                else under_pixel = img_under.data[(x + y*img_under.width) * img_under.channels + i % (img_under.channels - 1)];
                if (over_empty) over_pixel = DARK;
                else over_pixel = img_over.data[(over_x + over_y*img_over.width) * img_over.channels + i % (img_over.channels - 1)];
                tran_img.data[((x - start_x) + (y-start_y)*out_width)*tran_channels + i] 
                    = to_pixel((under_appear_ratio*under_pixel + over_alpha_pixel * over_pixel)/alpha_pixel); 
            }
        }
    }
    img_under.free_image(); 
    img_over.free_image();
    this->free_image();
    return tran_img;
}

image imaging::edited(layer layer_info){
    return layer_info.original.duplicate().filters(layer_info.filters).transforms(layer_info.transforms);
}