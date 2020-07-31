#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#ifdef DEBUG
    #define GLCall(x) GLClearError(); x; ASSERT(GLLogCall())
#else
    #define GLCall(x) x
#endif

static void GLClearError() 
{
    while (glGetError() != GL_NO_ERROR); //This resets the flag of the error when it returns it.
}

static void GLCheckError() 
{
    while (GLenum error = glGetError())
        std::cout << "[OpenGL Error] (0x" << std::hex << error << ")" << std::endl;
}

struct ShaderProgramSource 
{
    std::string VertexSource;
    std::string FragmentSource;
};

/* Parses in the shaders from a file */
static ShaderProgramSource ParseShader(const std::string& filePath) 
{
    std::ifstream stream(filePath);

    enum class ShaderType 
    {
        NONE = -1, VERTEX = 0, FRAGMENT = 1
    };

    ShaderType type = ShaderType::NONE; //By default it is none
    std::string line;

    /* One for vertex shader and one for fragment shader */
    std::stringstream ss[2];
    while (getline(stream, line)) 
    {
        if (line.find("#shader") != std::string::npos)
        {
            if (line.find("vertex") != std::string::npos)
                type = ShaderType::VERTEX;
            else if (line.find("fragment") != std::string::npos)
                type = ShaderType::FRAGMENT;
        }
        else if (type != ShaderType::NONE)
        {
            /* Clever way to add code to right index */
            ss[(int)type] << line << std::endl;
        }
    }
    return { ss[0].str(), ss[1].str() };
}

/* Code to compile and get a shader */
static unsigned int CompileShader(unsigned int type, const std::string& source) 
{
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);

    //If there's an error in the compile status, then result is 0
    if (!result) 
    {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);         //Put length of error message into length
        char* message = (char*)_malloca(length * sizeof(char)); //_malloca dynamically allocates on stack - GOES TO HEAP above 1MB (1024 bytes)
        glGetShaderInfoLog(id, length, &length, message);
        std::cout << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader." << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(id);
        return 0;
    }
    return id;
}

/* Note: Static in this sense means that it is bound to the translation unit */
static unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader) 
{ 
    unsigned int program = glCreateProgram();
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    //TODO: error handling for validate program

    glValidateProgram(program);

    /* Delete the intermediate shaders, as they are in program now */
    glDeleteShader(vs);
    glDeleteShader(fs);
    
    return program;
}

int main(void)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;


    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    /* Initialize GLEW */
    if (glewInit() != GLEW_OK)
        std::cout << "GLEW INIT NOT OK" << std::endl;

    /* Get the version and build of OpenGL printed to stdout */
    std::cout << glGetString(GL_VERSION) << std::endl;

    float positions[] =
    { 
       -0.5f, -0.5f,
        0.5f, -0.5f,
        0.5f, 0.5f,
        -0.5f, 0.5f
    };

    /* We could have 'char' type instead of 'unsigned int' to save space */
    unsigned int indices[] = 
    { 
        0,1,2,
        2,3,0
    };

    unsigned int buffer;                   //Buffer object to hold data
    glGenBuffers(1, &buffer);              //Assign the buffer an id (within OpenGL)
    glBindBuffer(GL_ARRAY_BUFFER, buffer); //This is how you select a buffer in OpenGL - next step is to put data into the buffer
    glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(float), positions, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0); //Enable attribute (with regards to the currently bound/selected attribute)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);

    unsigned int ibo;                            //Index buffer object to hold data
    glGenBuffers(1, &ibo);                       //Assign the index buffer an id (within OpenGL)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);  //This is how you select a buffer in OpenGL - next step is to put data into the buffer
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), indices, GL_STATIC_DRAW); //Remember that the last parameter is just a hint to OpenGL about the usage of the data

    /* This path is relative to the project directory, check Properties->Debugging of project */
    ShaderProgramSource source = ParseShader("res/shaders/Basic.shader");

    /* Error checking */
    if (source.VertexSource.empty() || source.FragmentSource.empty())
        return -1;

    unsigned int shader = CreateShader(source.VertexSource, source.FragmentSource);
    glUseProgram(shader);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);

        /* Tell GL what to do with the data, how to draw it and what to draw */
        //glDrawArrays(GL_TRIANGLES, 0, 6); //This is a "draw" call
        GLClearError();
        glDrawElements(GL_TRIANGLES, 6, GL_INT, nullptr); //This is a "draw" call, 6 is the amount of INDICES we are drawing
        GLCheckError();
        
        /* Following code for error checking
        unsigned int gl = glGetError();
        std::cout << gl << std::endl;
        */

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glDeleteProgram(shader);
    glfwTerminate();
    return 0;
}