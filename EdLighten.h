#pragma once
#if !defined(IMAGING)
#define IMAGING

#include <vector>
#include <array> 
#include <string>

namespace imaging {
	#define BRIGHT 255.0
	#define DARK 0.0

	enum out_file_type{bmp, jpg, png};
	out_file_type to_file_type(std::string file_type);

	struct filter_step {
		bool to_grey = false;
		int smoothness_torlance = 0; int smoothness_radius = 0;
		int brightness = 0;
		int contrast = 0;
		int saturation = 0;
		int sharpness = 0;
		int transparency = 0;
	};

	struct transform_step {
		int start_x = 0;
		int start_y = 0;
		int new_width;
		int new_height;
		float rotate = 0;
		float x_scale = 1;
		float y_scale = 1;

	};



	typedef unsigned char pixel;
	typedef std::array<int, 2> point;

	typedef std::array<pixel, 3> RGB_value;
	struct HSL_value{
		int H;
		pixel S;
		pixel L;
	};


	pixel to_pixel(int x);
	HSL_value RGB_to_HSL(RGB_value rbg);
	RGB_value HSL_to_RGB(HSL_value hsl);

	class image{
		pixel* data;
		int width;
		int height;
		int channels;

		bool close_to(int torlance, RGB_value color, int x, int y);
		image add_alpha_channel();

		public:
		void free_image();
		image duplicate();
		image(std::string file_name, int desired_channels); //image from file
		image(std::string file_name) : image(file_name, 0) {}; //image from file
		image(int input_width, int input_height, int input_desired_channels);
		image(int input_width, int input_height, int transparency, RGB_value color); //get plain color image
		image() {data = NULL ;width = 1; height = 1; channels = 1;};
		int get_width() {return width;};
		int get_height() {return height;};
		int get_channels() {return channels;};
		int get_area() {return width*height;};
		int get_size() {return  width*height*channels;};
		pixel * get_all_pixels();
		pixel get_pixel(int x, int y, int channel_no); //1/2: grey, alpha; 3/4: red, green, blue, alpha
		void change_pixel(int x, int y, int channel_no, pixel color);
		void change_pixel(int position, pixel color);
		int write_image(std::string file_name, out_file_type type, int compress);
		int write_image_custom_ext(std::string file_name, out_file_type type, int compress);
		
		image to_four_channels();
		image to_grey(); 
		image brightness(int add_value);
		image contrast(int add_value);
		image saturation(int add_value);
		image smoothness(int torlance, int radius);
		image transparency(int add_value);
		image paint(std::vector<point> include_pts, RGB_value color);
		image sharpness(int add_value);

		std::vector<std::vector<bool>> empty_segment();
		std::vector<std::vector<bool>> segment_include(int torlance, point include, std::vector<std::vector<bool>> segment);
		std::vector<std::vector<bool>> segment_exclude(int torlance, point exclude, std::vector<std::vector<bool>> segment);
		image select(std::vector<std::vector<bool>> segment, RGB_value non_select_color, int non_select_transparency);
		image invert_select(std::vector<std::vector<bool>> segment, RGB_value non_select_color, int non_select_transparency);
		
		image filters(filter_step filters);
		image transforms(transform_step transforms);
		
		image add(image img, int start_x, int start_y, int out_width, int out_height, int offset_x, int offset_y);
	};

	struct layer {
		image original;
		transform_step transforms;
		filter_step filters;
		layer() {original = image();};
		layer(image img) {original = img;  transforms = transform_step(); filters = filter_step();};
		layer(image img, transform_step T, filter_step F) {original = img; transforms = T; filters = F;};
	};
	image edited(layer layer_info);
}
#endif