# EdLighten


The purpose is to make a light photo editing application which is still more useful than the average "freewares".  
Do you know that MS Word/Powerpoint can segmentating a photo? But why can't Windows nor Andriod default photo editor do so? It seems that it is not very difficult to bulid such a project.  
This project utilities Dear ImGui, imgui-filebrowser and stb.  
To bulid the application from source code:  
1. Download all files from this depository.
2. Download all required files from Dear ImGui (https://github.com/ocornut/imgui). You need imgui.h,
imgui_draw.cpp,
imgui_internal.h,
imgui_tables.cpp,
imgui_widgets.cpp,
imstb_rectpack.h,
imstb_textedit.h,
imstb_truetype.h,
and four backend files (I used imgui_impl_dx11.h, imgui_impl_dx11.cpp, imgui_impl_win32.h and imgui_impl_win32.cpp).  
3. Download imfilebrowser.h from imgui-filebrowser (https://github.com/AirGuanZ/imgui-filebrowser)  
4. Download stb_image.h and stb_image_write.h from stb (https://github.com/nothings/stb)  
5. Put all files into a single folder.  
6. If you choose a different combination of back files, you need to move the relevant code in main.cpp to the main.cpp given on https://github.com/ocornut/imgui/tree/master/examples.  
7. If you are on Windows with g++, run the command "g++ *.cpp -o main -ld3d11 -ld3dcompiler -lgdi32 -ldwmapi -mwindows". To my understanding, the command is the same on Linux.
