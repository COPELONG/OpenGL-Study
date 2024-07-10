# 绘制YUV420P



得到解码一帧的AVframe数据后，使用opengl渲染

## 流程：

需要创建三个纹理对象，因为frame->data[0  、 1    、2]；所以对应的需要创建三个Y、U、V采样器，

纹理对象 -----纹理单元 ----- 采样器 结构。



### 初始化 OpenGL 上下文（使用 GLFW）

```c
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <libavutil/frame.h>

// 窗口尺寸
const GLint WIDTH = 800, HEIGHT = 600;

// 键盘回调函数
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

int main() {
    // 初始化 GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    // 设置 GLFW 选项
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // 创建 GLFW 窗口对象
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "YUV420P Renderer", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    // 设置当前上下文
    glfwMakeContextCurrent(window);

    // 设置键盘回调函数
    glfwSetKeyCallback(window, key_callback);

    // 初始化 GLEW 以设置 OpenGL 函数指针
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }

    // 定义视口尺寸
    int screenWidth, screenHeight;
    glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
    glViewport(0, 0, screenWidth, screenHeight);

    // ... (在下面创建着色器、纹理和渲染循环)
    
    return 0;
}
```

### 着色器代码

编写顶点着色器和片段着色器，用于处理 YUV420P 数据。

**顶点着色器（vertex_shader.glsl）**：

```glsl
#version 330 core
layout(location = 0) in vec3 position; // 顶点位置属性
layout(location = 1) in vec2 texCoords; // 纹理坐标属性

out vec2 TexCoords; // 输出给片段着色器的纹理坐标

void main() {
    gl_Position = vec4(position, 1.0); // 计算顶点位置
    TexCoords = texCoords; // 将纹理坐标传递给片段着色器
}
```

**片段着色器（fragment_shader.glsl）**：

```glsl
#version 330 core
in vec2 TexCoords; // 从顶点着色器传递来的纹理坐标

out vec4 color; // 输出颜色

uniform sampler2D texY; // Y 纹理采样器
uniform sampler2D texU; // U 纹理采样器
uniform sampler2D texV; // V 纹理采样器

void main() {
    float y = texture(texY, TexCoords).r; // 从 Y 纹理中获取亮度值
    float u = texture(texU, TexCoords).r - 0.5; // 从 U 纹理中获取色度 U 值，并减去 0.5
    float v = texture(texV, TexCoords).r - 0.5; // 从 V 纹理中获取色度 V 值，并减去 0.5
    
    // YUV 转 RGB 转换
    float r = y + 1.402 * v;
    float g = y - 0.344 * u - 0.714 * v;
    float b = y + 1.772 * u;

    color = vec4(r, g, b, 1.0); // 输出颜色值
}



在 YUV420P 格式中，U 和 V 分量的范围通常是 [0, 1]，中心点是 0.5。为了转换到 YUV 颜色空间的中心值为 0 的表示，我们需要将 U 和 V 值减去 0.5。因此，减去 0.5 是为了将 U 和 V 的范围从 [0, 1] 转换为 [-0.5, 0.5]，从而在颜色转换过程中使用中心为 0 的 U 和 V 值。

至于 texture(texV, TexCoords).g 和 texture(texV, TexCoords).b 的问题，在 YUV420P 格式中，Y、U 和 V 数据是存储在单独的纹理中的，并且每个纹理只有一个通道（通常是红色通道），即 GL_RED 格式。所以你只能访问 .r 通道。
```

### 创建和编译着色器

```c
// 编译着色器
GLuint compileShader(const char* source, GLenum shaderType) {
    GLuint shader = glCreateShader(shaderType); // 创建着色器对象
    glShaderSource(shader, 1, &source, NULL); // 设定着色器源码
    glCompileShader(shader); // 编译着色器

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success); // 获取编译状态
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog); // 获取编译错误信息
        fprintf(stderr, "Error compiling shader: %s\n", infoLog); // 输出错误信息
    }

    return shader; // 返回着色器对象
}

// 创建着色器程序
GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER); // 编译顶点着色器
    GLuint fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER); // 编译片段着色器

    GLuint shaderProgram = glCreateProgram(); // 创建着色器程序对象
    glAttachShader(shaderProgram, vertexShader); // 附加顶点着色器
    glAttachShader(shaderProgram, fragmentShader); // 附加片段着色器
    glLinkProgram(shaderProgram); // 链接着色器程序

    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success); // 获取链接状态
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog); // 获取链接错误信息
        fprintf(stderr, "Error linking shader program: %s\n", infoLog); // 输出错误信息
    }

    glDeleteShader(vertexShader); // 删除顶点着色器
    glDeleteShader(fragmentShader); // 删除片段着色器

    return shaderProgram; // 返回着色器程序对象
}
```

### 上传 YUV 数据到纹理

```c
GLuint texY, texU, texV; // 纹理对象

// 创建纹理
void createTextures(int width, int height) {
    glGenTextures(1, &texY); // 生成 Y 纹理
    glBindTexture(GL_TEXTURE_2D, texY); // 绑定 Y 纹理
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL); // 分配 Y 纹理内存
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // 设置纹理过滤参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // 设置纹理过滤参数
    glBindTexture(GL_TEXTURE_2D, 0); // 解绑纹理

    glGenTextures(1, &texU); // 生成 U 纹理
    glBindTexture(GL_TEXTURE_2D, texU); // 绑定 U 纹理
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width / 2, height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL); // 分配 U 纹理内存
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // 设置纹理过滤参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // 设置纹理过滤参数
    glBindTexture(GL_TEXTURE_2D, 0); // 解绑纹理

    glGenTextures(1, &texV); // 生成 V 纹理
    glBindTexture(GL_TEXTURE_2D, texV); // 绑定 V 纹理
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width / 2, height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL); // 分配 V 纹理内存
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // 设置纹理过滤参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // 设置纹理过滤参数
    glBindTexture(GL_TEXTURE_2D, 0); // 解绑纹理
}

// 上传 AVFrame 数据到纹理
void uploadFrameData(AVFrame* frame) {
    glBindTexture(GL_TEXTURE_2D, texY); // 绑定 Y 纹理
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->width, frame->height, GL_RED, GL_UNSIGNED_BYTE, frame->data[0]); // 更新 Y 纹理数据

    glBindTexture(GL_TEXTURE_2D, texU); // 绑定 U 纹理
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->width / 2, frame->height / 2, GL_RED, GL_UNSIGNED_BYTE, frame->data[1]); // 更新 U 纹理数据

    glBindTexture(GL_TEXTURE_2D, texV); // 绑定 V 纹理
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->width / 2, frame->height / 2, GL_RED, GL_UNSIGNED_BYTE, frame->data[2]); // 更新 V 纹理数据

    glBindTexture(GL_TEXTURE_2D, 0); // 解绑纹理
}
```

### 渲染 YUV 纹理

```c
// 顶点数据
GLfloat vertices[] = {
    // 位置               // UV    纹理坐标
    -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
     1.0f, -1.0f, 0.0f,  1.0f, 0.0f,

    -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
     1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
     1.0f,  1.0f, 0.0f,  1.0f, 1.0f
};

// 顶点缓冲对象和顶点数组对象
GLuint VBO, VAO;
glGenVertexArrays(1, &VAO); // 生成顶点数组对象
glGenBuffers(1, &VBO); // 生成顶点缓冲对象

glBindVertexArray(VAO); // 绑定顶点数组对象

glBindBuffer(GL_ARRAY_BUFFER, VBO); // 绑定顶点缓冲对象
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // 传递顶点数据

glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0); // 设置顶点属性指针
glEnableVertexAttribArray(0); // 启用顶点属性

glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat))); // 设置纹理坐标属性指针
glEnableVertexAttribArray(1); // 启用UV  纹理坐标属性

glBindBuffer(GL_ARRAY_BUFFER, 0); // 解绑顶点缓冲对象
glBindVertexArray(0); // 解绑顶点数组对象

// 主渲染循环
while (!glfwWindowShouldClose(window)) {
    // 检查事件
    glfwPollEvents();

    // 清空颜色缓冲
    glClear(GL_COLOR_BUFFER_BIT);

    // 使用着色器程序
    glUseProgram(shaderProgram);

    // 绑定纹理
    glActiveTexture(GL_TEXTURE0); // 激活纹理单元 0
    glBindTexture(GL_TEXTURE_2D, texY); // 绑定 Y 纹理
    glUniform1i(glGetUniformLocation(shaderProgram, "texY"), 0); // 设置纹理采样器

    glActiveTexture(GL_TEXTURE1); // 激活纹理单元 1
    glBindTexture(GL_TEXTURE_2D, texU); // 绑定 U 纹理
    glUniform1i(glGetUniformLocation(shaderProgram, "texU"), 1); // 设置纹理采样器

    glActiveTexture(GL_TEXTURE2); // 激活纹理单元 2
    glBindTexture(GL_TEXTURE_2D, texV); // 绑定 V 纹理
    glUniform1i(glGetUniformLocation(shaderProgram, "texV"), 2); // 设置纹理采样器

    // 渲染
    glBindVertexArray(VAO); // 绑定顶点数组对象
    glDrawArrays(GL_TRIANGLES, 0, 6); // 绘制三角形
    glBindVertexArray(0); // 解绑顶点数组对象

    // 交换屏幕缓冲
    glfwSwapBuffers(window);
}

// 释放所有资源
glDeleteVertexArrays(1, &VAO);
glDeleteBuffers(1, &VBO);

// 终止 GLFW 并释放所有分配的资源
glfwTerminate();
```