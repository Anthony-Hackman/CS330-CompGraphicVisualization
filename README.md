# CS330-CompGraphicVisualization

Anthony Hackman

## OpenGL, C++

2025 C-5 (Sep - Oct)

Portfolio item from CS330 Comp. Graphic and Visualization. This project is a rendered 3D Scene, built with OpenGL, and utilizing C++

### How do I approach designing software?

- *What new design skills has your work on the project helped you to craft?*

Being an art student in the past surely helped me through this project. I was able to think more about the initial design of the scene using basic 3D shapes given our contraints. This project helped me understand more about the structure of C++ applications, along with the many uses of computational graphics, and has provided knowledge on how to apply more advanced graphics for a project.

- *What design process did you follow for your project work?*

To design the scene, I broke down the pieces of the reference image (house) and thought about how to represent those pieces using a basic 3D shape. We were required to use a handful of basic shapes that I wanted to reuse as many times as I possible for efficiency. The design of the project's code followed the original structure used in the assignments of the course. The controls and view controls are located in `ViewManager.cpp` and `ViewManager.h`; the scene's objects, materials, and lighting are in the `SceneManager.cpp`, and the `SceneManager.h` files. This choice has made the scene manager file larger than I would like for the final project, however this can be improved by adding more a more modular way to draw the shapes in the scene. This can be done by adding another file, function, or table for object definitions.

- *How could tactics from your design approach be applied in future work?*

For future work this project has helped me learn how to study and understand a previously built system, along with how to build on top of a pre-existing systems more effectively. It also helped me reflect more on my understanding of the subject, since I had troubles in lighting.

### How do I approach developing programs?

- *What new development strategies did you use while working on your 3D scene?*

While working on the project I had to think more about placement of the objects in 3D space. This required me to understand more about positional values using x,y, and z. I also had to learn how to implement PHONG lighting, which initally (as I was learning) implemented backwards seeing only specular highlights. This tested my abilities to troubleshoot an application, having to indentify where my mistake was.

- *How did iteration factor into your development?*

In order to create the scene we have to iterate through the redndering process to make the application interactive. This allows us to process the lighting relative to the viewing perspective (camera). I also had to draw shapes over and over, to do this I utilized the already existing basic shapes and made blocks of code snippets to edit how those shapes are defined in the scene. Also we had to continuously source textures using royalty free assets, this project taught me how to find those more effectively.

- *How has your approach to developing code evolved throughout the milestones, which led you to the project’s completion?*

Initially I had to understand what I was doing, reading through the provided resources did not align exactly with the provided course material. To get around this I saw what was included with OpenGL, and read more of the documentation. After sucessfully linking both libraries we needed to use the existing shape meshes defined in the project, and place them in the scene efficiently while still being human readable. This eventually evolved into a snippet for each shape in `SceneManager.cpp` in order to edit the scene while maintaining a readable, easily scalable code base.

### How can computer science help me in reaching my goals?

- *How do computational graphics and visualizations give you new knowledge and skills that can be applied in - your future educational pathway?*

Computational graphics give a visual to what you are working on, this helps you see how the changes you are making effect the scene. If something is wrong with the implementation, you should quickly be able to tell what is incorrect when it comes to visuals. With this I have gained some troubleshooting skills, learned more about utilizing available libraries, and more about how to implement efficient solutions using C++.

- *How do computational graphics and visualizations give you new knowledge and skills that can be applied in your future professional pathway?*

Professionally, graphics are used for effective visuals of data, we can use graphics for a variety of projects. Whether in a creative profession or more of a buisness use case, graphics can enhance a project's experience making it more enjoyable to interact with. Some creative projects are entirely graphics and visualization, the skills gained in this course have given me the tools to bring a thought to an interactive scene through the use of a programming language.
