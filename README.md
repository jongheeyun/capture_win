# Capture Win

Capture Win is a screen capturing application that utilizes GDI+ to capture screenshots and save them as PNG images. The application runs in a loop, capturing the screen at regular intervals and storing the images in a specified directory.

## Features

- Captures the screen at a resolution of 1280x720.
- Saves captured images in PNG format.
- Configurable capture interval (default is every 3 minutes).
- Automatically creates a directory for storing screenshots if it does not exist.

## Files

- `src/main.cpp`: Contains the main functionality of the application, including screen capturing and saving images.
- `CMakeLists.txt`: Configuration file for CMake, specifying project details and dependencies.

## Building the Project

To build the project, follow these steps:

1. Ensure you have CMake installed on your system.
2. Open a terminal and navigate to the project directory.
3. Create a build directory:
   ```
   mkdir build
   cd build
   ```
4. Run CMake to configure the project:
   ```
   cmake ..
   ```
5. Build the project:
   ```
   cmake --build .
   ```

## Running the Application

After building the project, you can run the application from the build directory. The application will start capturing the screen and saving images to the `C:\CapturedScreenshots` directory by default.

## License

This project is licensed under the MIT License. See the LICENSE file for more details.