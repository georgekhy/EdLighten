// Dear ImGui: standalone example application for DirectX 11
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "stb_image.h"
#include "EdLighten.h"
#include <math.h>
#include <algorithm>
#include<string>  

#include "imgui.h"
#include "imfilebrowser.h"


#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>



// Data
static ID3D11Device*            g_pd3dDevice = NULL;
static ID3D11DeviceContext*     g_pd3dDeviceContext = NULL;
static IDXGISwapChain*          g_pSwapChain = NULL;
static ID3D11RenderTargetView*  g_mainRenderTargetView = NULL;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void HelpMarker(const char* desc);

// Simple helper function to load an image into a DX11 texture with common settings
bool LoadTextureFromImaging(imaging::image img, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height);
static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) ;
static inline ImVec2 ImRotate(const ImVec2& v, float cos_a, float sin_a) ;
void ImageRotated(ImTextureID tex_id, ImVec2 center, ImVec2 size, float angle);

// Main code
int main(int, char**)
{
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"ImGui Example", NULL };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX11 Example", WS_OVERLAPPEDWINDOW, 30, 30, 960 , 800, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() t them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need t the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Our state

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    
    //some helper var/const initialised
    #define PI 3.14159265358979323846f
    #define MAX_layers 2
    static std::vector<std::string> filenames = {};
    static int my_image_width = 0;
    static int my_image_height = 0;
    static int canvas_size[2] = {256, 256};
    static int selected_layer = 0;

    struct canvas_layer{
        imaging::layer image_info;
        int layer_size[2] = {1,1};
        int reflect_x = 1;
        int reflect_y = 1;
        float rotate_angle_deg = 0;
        float abs_scale_x = 1;
        float abs_scale_y = 1;
        int not_to_grey = 1;
        int off_x = 0;
        int off_y = 0;
        canvas_layer(imaging::layer layer_input) {
            image_info = layer_input;
            image_info.transforms.new_width = image_info.original.get_width();
            image_info.transforms.new_height = image_info.original.get_height();};
        canvas_layer() = default;
    };
    static imaging::image to_load = imaging::image();
    static imaging::image current_image = imaging::image();
    static std::vector<canvas_layer> canvas_layers = {};
    static ID3D11ShaderResourceView* my_texture = NULL;

    static ImGuiWindowFlags flags_fullscreen = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove;

    static bool replacing = false;
    static bool adding = false;
    static bool segmentating = false;
    static bool saving = false;
    static bool updating = false;
    static bool combining = false;
    static bool first = false;
    static bool _true = true;

    //File Browser
    static ImGuiFileBrowserFlags flags_save = ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_CreateNewDir;
    static ImGui::FileBrowser fileDialog_open;
    static ImGui::FileBrowser fileDialog_save(flags_save);
    // (optional) set browser properties
    fileDialog_open.SetTypeFilters({ ".jpg", ".bmp", ".png" });
    fileDialog_save.SetTypeFilters({ ".jpg", ".bmp", ".png" });
    fileDialog_open.SetTitle("Open File");
    fileDialog_save.SetTitle("Save");

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();


        //Full window
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::Begin("EdLighten!", &_true,flags_fullscreen);
        //Menu
        if (!segmentating){
        if (ImGui::BeginMainMenuBar()){
            if (ImGui::BeginMenu("Menu")){
                if( ImGui::MenuItem("New Canvas") ){
                    //free the pop
                    for (auto layer: canvas_layers){
                        layer.image_info.original.free_image();
                    }
                    std::vector<canvas_layer>().swap(canvas_layers);
                }
                
                if(ImGui::BeginMenu("Open File")){
                    if (canvas_layers.size() > 0){
                        if (ImGui::MenuItem("Replace the current layer")) {
                            replacing = true;
                            fileDialog_open.Open();
                        }
                    }
                    if (canvas_layers.size() < MAX_layers){
                        if (ImGui::MenuItem("Add a new layer")) {
                            adding = true;
                            fileDialog_open.Open();
                        }
                    }
                    ImGui::EndMenu();
                }
                
                if (ImGui::MenuItem("Save Canvas")){
                    saving = true;
                    fileDialog_save.Open();
                }
                if (canvas_layers.size() == 1) 
                    if (ImGui::MenuItem("Segmentation")) {
                        segmentating = true;
                        first = true;
                    }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
        }

        ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.9f);

        //controls
        ImGui::BeginChild("ChildL", ImVec2(ImGui::GetContentRegionAvail().x * 0.2f, ImGui::GetContentRegionAvail().y));

        //canvas size
        ImGui::Text("Canvas size");
        ImGui::InputInt2("##Canvas size", canvas_size);
        ImGui::SameLine();
        HelpMarker("Canvas width x height");
        

        //Background before the layer
        ImGui::Text("Background Colour");
        static float canvas_background_colours[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        ImGui::ColorEdit4("##Background Colour", canvas_background_colours); ImGui::SameLine();
        HelpMarker("Fill the background with the colour for empty space.");

        if (canvas_layers.size() > 0){
        //layer
        ImGui::Text("Tempering the:");
        ImGui::RadioButton("Layer 0 (Back)", &selected_layer, 0);
        if (canvas_layers.size() == 1) selected_layer = 0;
        else ImGui::RadioButton("Layer 1 (Front)", &selected_layer, 1);

        if (ImGui::Button("Delete the layer")){
            canvas_layers[selected_layer].image_info.original.free_image();
            canvas_layers.erase (canvas_layers.begin() + selected_layer);
        }
        //Filters
        
        ImGui::Text("Filters");
        ImGui::SliderInt("##Smoothness Torlance",&canvas_layers[selected_layer].image_info.filters.smoothness_torlance,0, 255, "Smo.Tor: %d");
        ImGui::SliderInt("##Smoothness Radius",&canvas_layers[selected_layer].image_info.filters.smoothness_radius,0,3, "Smo.rad: %d"); ImGui::SameLine();
        HelpMarker("Smoothness Torlance and Radius");
        ImGui::SliderInt("##Brightness",&canvas_layers[selected_layer].image_info.filters.brightness,-50,50, "Bri: %d"); ImGui::SameLine();
        HelpMarker("Brightness"); 
        ImGui::SliderInt("##Contrast",&canvas_layers[selected_layer].image_info.filters.contrast,-50,50, "Con: %d"); ImGui::SameLine();
        HelpMarker("Contrast");
        ImGui::SliderInt("##Saturation",&canvas_layers[selected_layer].image_info.filters.saturation,-50,50, "Sat: %d"); ImGui::SameLine();
        HelpMarker("Saturation");
        ImGui::SliderInt("##Sharpness",&canvas_layers[selected_layer].image_info.filters.sharpness,-50,50, "Sha: %d"); ImGui::SameLine();
        HelpMarker("Sharpness"); 
        ImGui::SliderInt("##Transparency",&canvas_layers[selected_layer].image_info.filters.transparency,-100,100, "Tra: %d"); ImGui::SameLine();
        HelpMarker("Transparency");

        
        ImGui::RadioButton("Colour", &canvas_layers[selected_layer].not_to_grey, 1); ImGui::SameLine();
        ImGui::RadioButton("Grey", &canvas_layers[selected_layer].not_to_grey, 0);
        if (canvas_layers[selected_layer].not_to_grey == 0) canvas_layers[selected_layer].image_info.filters.to_grey = true;
        else  canvas_layers[selected_layer].image_info.filters.to_grey = false;

        //Transformations: rotate, scaling, etc.
        ImGui::Text("Transformations");


        ImGui::SliderFloat("##Rotate", &canvas_layers[selected_layer].rotate_angle_deg,-180.0f,180.0f, "Rat: %.0f deg"); ImGui::SameLine();
        canvas_layers[selected_layer].image_info.transforms.rotate = canvas_layers[selected_layer].rotate_angle_deg/180.0f*PI;
        HelpMarker("Rotation");
        ImGui::SliderFloat("##x_scale",&canvas_layers[selected_layer].abs_scale_x,0.1f,10.0f, "x_sca: %.2f",ImGuiSliderFlags_Logarithmic); ImGui::SameLine();
        HelpMarker("Scaling Horizontally"); 
        ImGui::SliderFloat("##y_scale",&canvas_layers[selected_layer].abs_scale_y,0.1f,10.0f, "y_sca: %.2f",ImGuiSliderFlags_Logarithmic); ImGui::SameLine();
        HelpMarker("Scaling Vertically");
        canvas_layers[selected_layer].layer_size[0] = (int) (canvas_layers[selected_layer].abs_scale_x*canvas_layers[selected_layer].image_info.original.get_width());
        canvas_layers[selected_layer].layer_size[1] = (int) (canvas_layers[selected_layer].abs_scale_y*canvas_layers[selected_layer].image_info.original.get_height());
        
        ImGui::Text("OR to the size: ");
        ImGui::InputInt2("##Layer size", canvas_layers[selected_layer].layer_size);
        canvas_layers[selected_layer].abs_scale_x = canvas_layers[selected_layer].layer_size[0]/ ((float) canvas_layers[selected_layer].image_info.original.get_width());
        canvas_layers[selected_layer].abs_scale_y = canvas_layers[selected_layer].layer_size[1]/ ((float) canvas_layers[selected_layer].image_info.original.get_height());

        if (ImGui::Button("Reflect Horizontally")) canvas_layers[selected_layer].reflect_x *= -1;
        if (ImGui::Button("Reflect Vertically")) canvas_layers[selected_layer].reflect_y *= -1;
        canvas_layers[selected_layer].image_info.transforms.x_scale = canvas_layers[selected_layer].abs_scale_x * canvas_layers[selected_layer].reflect_x;
        canvas_layers[selected_layer].image_info.transforms.y_scale = canvas_layers[selected_layer].abs_scale_y * canvas_layers[selected_layer].reflect_y;

        ImGui::Text("Offset of layers");
        ImGui::InputInt("##Offset of layersx", &canvas_layers[selected_layer].off_x);ImGui::SameLine();
        HelpMarker("Offset respect to the background. Positive -> to the right");
        ImGui::InputInt("##Offset of layersy", &canvas_layers[selected_layer].off_y);ImGui::SameLine();
        HelpMarker("Offset respect to the background. Positive -> to the bottom.");
        canvas_layers[selected_layer].image_info.transforms.start_x = -canvas_layers[selected_layer].off_x;
        canvas_layers[selected_layer].image_info.transforms.start_y = -canvas_layers[selected_layer].off_y;

        if (canvas_layers.size() == 2)
            if (ImGui::Button("Swap layers"))
                std::swap(canvas_layers[0], canvas_layers[1]);
        
        if (ImGui::Button("Preview")) updating = true;

        if (ImGui::Button("Combine two layers")){
            updating = true;
            combining = true;
        }
        }

        ImGui::EndChild();
        ImGui::SameLine();

        ImGui::BeginChild("ChildL2", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y));

        if (!ImGui::IsItemVisible()){
        ImGui::Text("Mouse Left: Click to select the upper left corner of the output; Mouse Right: drag to scroll");
        static ImVec2 scrolling(50.0f, 50.0f);
        ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
        ImVec2 canvas_sz = ImGui::GetContentRegionAvail();   // Resize canvas to what's available
        ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);
        ImVec2 canvas_p2 = ImVec2((scrolling.x < 0)? canvas_p0.x : canvas_p0.x + scrolling.x, 
                                    (scrolling.y < 0)? canvas_p0.y : canvas_p0.y + scrolling.y);
        ImVec2 canvas_p3 = ImVec2((canvas_p0.x + scrolling.x + canvas_size[0] >= canvas_p1.x)? 
                                    canvas_p1.x : canvas_p0.x + scrolling.x + canvas_size[0],
                                    (canvas_p0.y + scrolling.y + canvas_size[1] >= canvas_p1.y)? 
                                    canvas_p1.y : canvas_p0.y + scrolling.y + canvas_size[1]);
        ImVec2 pic_p0 = ImVec2((scrolling.x < 0)? -scrolling.x/canvas_size[0] : 0.0f,
                                    (scrolling.y < 0)? -scrolling.y/canvas_size[1] : 0.0f);
        ImVec2 pic_p1 = ImVec2((canvas_p0.x + scrolling.x + canvas_size[0] >= canvas_p1.x)?
                                    (canvas_sz.x - scrolling.x)/canvas_size[0] : 1.0f,
                                    (canvas_p0.y + scrolling.y + canvas_size[1] >= canvas_p1.y)?
                                    (canvas_sz.y - scrolling.y)/canvas_size[1] : 1.0f);

        // Draw background color
        ImGuiIO& io = ImGui::GetIO();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
    
        ImGui::InvisibleButton("canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
        const bool is_hovered = ImGui::IsItemHovered(); // Hovered
        const bool is_active = ImGui::IsItemActive();   // Held
        if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
            if (ImGui::IsMousePosValid()){
                int delta_x = io.MousePos.x - canvas_p0.x - scrolling.x;
                int delta_y = io.MousePos.y - canvas_p0.y - scrolling.y;
                for (auto layer:canvas_layers){
                    canvas_layers[selected_layer].off_x -= delta_x;
                    canvas_layers[selected_layer].off_y -= delta_y;
                }
                scrolling.x += delta_x;
                scrolling.y += delta_y;
                
            }
            updating= true;
        }
        if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f)){
            scrolling.x += io.MouseDelta.x;
            scrolling.y += io.MouseDelta.y;
        }
        //Draw grid
        const float GRID_STEP = 64.0f;
        for (float x = fmodf(scrolling.x, GRID_STEP); x < canvas_sz.x; x += GRID_STEP)
            draw_list->AddLine(ImVec2(canvas_p0.x + x, canvas_p0.y), ImVec2(canvas_p0.x + x, canvas_p1.y), IM_COL32(200, 200, 200, 40));
        for (float y = fmodf(scrolling.y, GRID_STEP); y < canvas_sz.y; y += GRID_STEP)
            draw_list->AddLine(ImVec2(canvas_p0.x, canvas_p0.y + y), ImVec2(canvas_p1.x, canvas_p0.y + y), IM_COL32(200, 200, 200, 40));
        

        //Draw canvas background
        draw_list->AddRectFilled(canvas_p2,canvas_p3,IM_COL32(canvas_background_colours[0]*BRIGHT, canvas_background_colours[1]*BRIGHT,
                                                                 canvas_background_colours[2]*BRIGHT, canvas_background_colours[3]*BRIGHT));

        //update the image if needed
        if (updating){
            to_load.free_image();
            to_load = imaging::image(canvas_size[0], canvas_size[1],4);
            for (auto current_layer:canvas_layers){
                current_image = imaging::edited(current_layer.image_info);
                to_load = to_load.add(current_image,0,0,canvas_size[0],canvas_size[1],0,0);
                current_image.free_image();
            }
            if (combining){
                canvas_layers.pop_back(); canvas_layers.pop_back();
                canvas_layer combined = canvas_layer(imaging::layer(to_load));
                combined.image_info.transforms.new_width = canvas_size[0];
                combined.image_info.transforms.new_height = canvas_size[1];
                canvas_layers.push_back(combined);
                combining = false;
            }
            bool ret = LoadTextureFromImaging(to_load , &my_texture, &my_image_width, &my_image_height);
            IM_ASSERT(ret);
            updating = false;
        }
        draw_list->AddImage((void*)my_texture,canvas_p2,canvas_p3, pic_p0, pic_p1);
        
        // Draw border
        draw_list->AddRect(canvas_p2, canvas_p3, IM_COL32(127, 127, 127, 127));
        draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));
        }

        ImGui::EndChild();
        
        ImGui::End(); 

        fileDialog_open.Display();
        if(fileDialog_open.HasSelected()) {
            if (replacing){
                canvas_layer the_replacement = canvas_layer(imaging::image(fileDialog_open.GetSelected().string(),4));
                std::swap(canvas_layers[selected_layer], the_replacement);
                the_replacement.image_info.original.free_image();
                replacing = false;
                updating = true;
            }
            if (adding){
                canvas_layer new_layer = canvas_layer(imaging::image(fileDialog_open.GetSelected().string(),4));
                canvas_layers.push_back(new_layer);
                adding = false;
                updating = true;
            }
            fileDialog_open.ClearSelected();
        }
        fileDialog_save.Display();
        if(fileDialog_save.HasSelected()) {
            imaging::RGB_value rgb = {imaging::to_pixel(canvas_background_colours[0]*BRIGHT),
                                        imaging::to_pixel(canvas_background_colours[1]*BRIGHT),
                                        imaging::to_pixel(canvas_background_colours[2]*BRIGHT)};
            imaging::image to_save = imaging::image(canvas_size[0], canvas_size[1], canvas_background_colours[3]*BRIGHT, rgb);
            for (auto current_layer:canvas_layers){
                current_image = imaging::edited(current_layer.image_info);
                to_save = to_save.add(current_image,0,0,canvas_size[0],canvas_size[1],0,0);
                current_image.free_image();
            }
            std::string save_file_name = fileDialog_save.GetSelected().string();
            std::string extension = save_file_name.substr(save_file_name.length() - 4, 3);
            to_save.write_image(save_file_name.substr(0, save_file_name.length() - 4),imaging::to_file_type(extension),100);
            to_save.free_image();
            saving = false;
            fileDialog_save.ClearSelected();
        }

        if (segmentating){
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::Begin("Segmentating", &_true,flags_fullscreen);
            static int select_to_add = 1;
            static int segment_mod = 0;
            static int canvas_seg_size[2] = {1,1};
            static int seg_torlance = 0;
            static float seg_col[4]= {1.0f,1.0f,1.0f,1.0f};
            static imaging::image to_segment = imaging::image(1,1,1);
            static imaging::image seg_preview = imaging::image(1,1,1);
            static std::vector<std::vector<bool>> select_segment = std::vector<std::vector<bool>>();

            ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.4f);
            ImGui::RadioButton("Select", &select_to_add,1);ImGui::SameLine();
            ImGui::RadioButton("Unselect", &select_to_add,0);
            ImGui::SliderInt("##seg_torlance", &seg_torlance,0,255,"Tor: %d");ImGui::SameLine();
            ImGui::ColorEdit4("##SEg_Background Colour", seg_col);
            ImGui::RadioButton("Keep selected", &segment_mod,0);ImGui::SameLine();
            ImGui::RadioButton("Discard selected", &segment_mod,1);ImGui::SameLine();
            ImGui::RadioButton("Keep them on seperate layers", &segment_mod,2);
            ImGui::Text("Mouse Left: Click to select/unselect; Mouse Right: drag to scroll");
            if (ImGui::Button("Save")){
                canvas_layer new_layer_1; canvas_layer new_layer_2;
                
                switch (segment_mod){
                    case 0:
                        canvas_layers.pop_back();
                        new_layer_1 = canvas_layer(to_segment.duplicate().select(select_segment,
                                                                {imaging::to_pixel(seg_col[0]*BRIGHT),
                                                                imaging::to_pixel(seg_col[1]*BRIGHT), 
                                                                imaging::to_pixel(seg_col[2]*BRIGHT)},seg_col[3]*BRIGHT));
                        canvas_layers.push_back(new_layer_1);
                        break;
                    case 1:
                        canvas_layers.pop_back();
                        new_layer_1 = canvas_layer(to_segment.duplicate().invert_select(select_segment,
                                                                {imaging::to_pixel(seg_col[0]*BRIGHT),
                                                                imaging::to_pixel(seg_col[1]*BRIGHT), 
                                                                imaging::to_pixel(seg_col[2]*BRIGHT)},seg_col[3]*BRIGHT));
                        canvas_layers.push_back(new_layer_1);
                        break;
                    case 2:
                        canvas_layers.pop_back();
                        new_layer_1 = canvas_layer(to_segment.duplicate().invert_select(select_segment,
                                                                {imaging::to_pixel(seg_col[0]*BRIGHT),
                                                                imaging::to_pixel(seg_col[1]*BRIGHT), 
                                                                imaging::to_pixel(seg_col[2]*BRIGHT)},seg_col[3]*BRIGHT));
                        new_layer_2 = canvas_layer(to_segment.duplicate().select(select_segment,
                                                                {imaging::to_pixel(seg_col[0]*BRIGHT),
                                                                imaging::to_pixel(seg_col[1]*BRIGHT), 
                                                                imaging::to_pixel(seg_col[2]*BRIGHT)},seg_col[3]*BRIGHT));                                                                
                        canvas_layers.push_back(new_layer_1);canvas_layers.push_back(new_layer_2);
                        break;
                    default:
                        break;
                }
                to_segment.free_image();
                seg_preview.free_image();
                std::vector<std::vector<bool>>().swap(select_segment);
                segmentating = false;
                updating = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Discard")){
                to_segment.free_image();
                seg_preview.free_image();
                std::vector<std::vector<bool>>().swap(select_segment);
                segmentating = false;
                updating = true;
            }
            static ImVec2 scrolling_seg(0.0f, 0.0f);
            if (first){
                seg_torlance = 0;
                canvas_seg_size[0] = canvas_layers[0].image_info.original.get_width();
                canvas_seg_size[1] = canvas_layers[0].image_info.original.get_height();
                scrolling_seg.x = 0.0f;
                scrolling_seg.y = 0.0f;
                
                to_segment.free_image();
                to_segment = canvas_layers[0].image_info.original.duplicate();
                bool ret = LoadTextureFromImaging(to_segment , &my_texture, &my_image_width, &my_image_height);
                IM_ASSERT(ret);
                std::vector<std::vector<bool>> temp_select_segment = to_segment.empty_segment();
                select_segment.swap(temp_select_segment);
                std::vector<std::vector<bool>>().swap(temp_select_segment);
                first = false;
            }
            ImVec2 canvas_seg_p0 = ImGui::GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
            ImVec2 canvas_seg_sz = ImGui::GetContentRegionAvail();   // Resize canvas to what's available
            ImVec2 canvas_seg_p1 = ImVec2(canvas_seg_p0.x + canvas_seg_sz.x, canvas_seg_p0.y + canvas_seg_sz.y);
            ImVec2 canvas_seg_p2 = ImVec2((scrolling_seg.x < 0)? canvas_seg_p0.x : canvas_seg_p0.x + scrolling_seg.x, 
                                        (scrolling_seg.y < 0)? canvas_seg_p0.y : canvas_seg_p0.y + scrolling_seg.y);
            ImVec2 canvas_seg_p3 = ImVec2((canvas_seg_p0.x + scrolling_seg.x + canvas_seg_size[0] >= canvas_seg_p1.x)? 
                                        canvas_seg_p1.x : canvas_seg_p0.x + scrolling_seg.x + canvas_seg_size[0],
                                        (canvas_seg_p0.y + scrolling_seg.y + canvas_seg_size[1] >= canvas_seg_p1.y)? 
                                        canvas_seg_p1.y : canvas_seg_p0.y + scrolling_seg.y + canvas_seg_size[1]);
            ImVec2 pic_seg_p0 = ImVec2((scrolling_seg.x < 0)? -scrolling_seg.x/canvas_seg_size[0] : 0.0f,
                                        (scrolling_seg.y < 0)? -scrolling_seg.y/canvas_seg_size[1] : 0.0f);
            ImVec2 pic_seg_p1 = ImVec2((canvas_seg_p0.x + scrolling_seg.x + canvas_seg_size[0] >= canvas_seg_p1.x)?
                                        (canvas_seg_sz.x - scrolling_seg.x)/canvas_seg_size[0] : 1.0f,
                                        (canvas_seg_p0.y + scrolling_seg.y + canvas_seg_size[1] >= canvas_seg_p1.y)?
                                        (canvas_seg_sz.y - scrolling_seg.y)/canvas_seg_size[1] : 1.0f);
            // Draw background color
            ImGuiIO& io = ImGui::GetIO();
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(canvas_seg_p0, canvas_seg_p1, IM_COL32(50, 50, 50, 255));

            ImGui::InvisibleButton("canvas_seg", canvas_seg_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
            const bool is_seg_hovered = ImGui::IsItemHovered(); // Hovered
            const bool is_seg_active = ImGui::IsItemActive();   // Held
            if (is_seg_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
                int delta_x = io.MousePos.x - canvas_seg_p0.x - scrolling_seg.x;
                int delta_y = io.MousePos.y - canvas_seg_p0.y - scrolling_seg.y;
                if (select_to_add == 1){
                    std::vector<std::vector<bool>> temp_select_segment = to_segment.segment_include(seg_torlance,{delta_x,delta_y},select_segment);
                    select_segment.swap(temp_select_segment);
                    std::vector<std::vector<bool>>().swap(temp_select_segment);
                }
                else {
                    std::vector<std::vector<bool>> temp_select_segment = to_segment.segment_exclude(seg_torlance,{delta_x,delta_y},select_segment);
                    select_segment.swap(temp_select_segment);
                    std::vector<std::vector<bool>>().swap(temp_select_segment);
                }
                seg_preview.free_image();
                seg_preview = to_segment.duplicate().invert_select(select_segment,
                                                {imaging::to_pixel(seg_col[0]*BRIGHT),
                                                 imaging::to_pixel(seg_col[1]*BRIGHT), 
                                                 imaging::to_pixel(seg_col[2]*BRIGHT)},seg_col[3]*BRIGHT);
                bool ret = LoadTextureFromImaging(seg_preview , &my_texture, &my_image_width, &my_image_height);
                IM_ASSERT(ret);
            }
            if (is_seg_active && ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f)){
                scrolling_seg.x += io.MouseDelta.x;
                scrolling_seg.y += io.MouseDelta.y;
            }
            //Draw grid
            const float GRID_seg_STEP = 64.0f;
            for (float x = fmodf(scrolling_seg.x, GRID_seg_STEP); x < canvas_seg_sz.x; x += GRID_seg_STEP)
                draw_list->AddLine(ImVec2(canvas_seg_p0.x + x, canvas_seg_p0.y), ImVec2(canvas_seg_p0.x + x, canvas_seg_p1.y), IM_COL32(200, 200, 200, 40));
            for (float y = fmodf(scrolling_seg.y, GRID_seg_STEP); y < canvas_seg_sz.y; y += GRID_seg_STEP)
                draw_list->AddLine(ImVec2(canvas_seg_p0.x, canvas_seg_p0.y + y), ImVec2(canvas_seg_p1.x, canvas_seg_p0.y + y), IM_COL32(200, 200, 200, 40));

            draw_list->AddImage((void*)my_texture,canvas_seg_p2,canvas_seg_p3, pic_seg_p0, pic_seg_p1);
            // Draw border
            draw_list->AddRect(canvas_seg_p2, canvas_seg_p3, IM_COL32(127, 127, 127, 127));
            draw_list->AddRect(canvas_seg_p0, canvas_seg_p1, IM_COL32(255, 255, 255, 255));
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    
    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_WARP, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}


// Simple helper function to load an image into a DX11 texture with common settings
bool LoadTextureFromImaging(imaging::image img, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height){

    imaging::pixel * image_data = img.get_all_pixels();
    int image_width = img.get_width();
    int image_height = img.get_height();
    if (image_data == NULL)
        return false;D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D *pTexture = NULL;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
    pTexture->Release();

    *out_width = image_width;
    *out_height = image_height;
    free(image_data);
    return true;
}


static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) 
{ 
    return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); 
}
static inline ImVec2 ImRotate(const ImVec2& v, float cos_a, float sin_a) 
{ 
    return ImVec2(v.x * cos_a - v.y * sin_a, v.x * sin_a + v.y * cos_a);
}
void ImageRotated(ImTextureID tex_id, ImVec2 center, ImVec2 size, float angle)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    float cos_a = cosf(angle);
    float sin_a = sinf(angle);
    ImVec2 pos[4] =
    {
        center + ImRotate(ImVec2(-size.x * 0.5f, -size.y * 0.5f), cos_a, sin_a),
        center + ImRotate(ImVec2(+size.x * 0.5f, -size.y * 0.5f), cos_a, sin_a),
        center + ImRotate(ImVec2(+size.x * 0.5f, +size.y * 0.5f), cos_a, sin_a),
        center + ImRotate(ImVec2(-size.x * 0.5f, +size.y * 0.5f), cos_a, sin_a)
    };
    ImVec2 uvs[4] = 
    { 
        ImVec2(0.0f, 0.0f), 
        ImVec2(1.0f, 0.0f), 
        ImVec2(1.0f, 1.0f), 
        ImVec2(0.0f, 1.0f) 
    };

    draw_list->AddImageQuad(tex_id, pos[0], pos[1], pos[2], pos[3], uvs[0], uvs[1], uvs[2], uvs[3], IM_COL32_WHITE);
}
static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}