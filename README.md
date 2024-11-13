To compile and run my project, complete the following steps:

1. Ensure you have the GCC compiler installed.

2. Open a terminal in the ./ViewTransformsShading/bin directory.

3. On Windows, run the command "mingw32-make.exe" to build the project, if you have MinGW.
   Alternatively, if you are not on Windows or use another command to run makefiles,
   such as "make", then do that instead. All you need to do is run the makefile that
   is located in the ./bin directory.

4. Now that the program has started, enter the object file you would like to render
   into the terminal. Alternatively, you can enter "DEFAULT" (all caps) to run
   "porsche.obj". This command will read and load the contents of the corresponding
   .obj file located in the ./ViewTransformsShading/data directory.

5. Next, enter the number for the lighting model you would like to use.
   '1' - Flat Shading
   '2' - Gouraud Shading
   '3' - Phong Shading
   '4' - Depth Test
   Once entered, the object will automatically render using the selected lighting model.

6. Now the object is rendered on your screen. Follow the control scheme listed in the
   terminal to transform and interact with the object. That's it!