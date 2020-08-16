#include "imgui\imgui.h"
#include "imgui\imgui_impl_glfw.h"
#include "imgui\imgui_impl_opengl3.h"

#define GLEW_STATIC
#include <GL\glew.h>
#include <GLFW\glfw3.h>
#include <glm\glm.hpp>
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\matrix_transform.hpp>

#include <string>
#include <iostream>
#include <fstream>

#include "Utils.hpp"
#include "CornerTable.h"
#include "min_path.h"

#define numVAOs 1
#define numVBOs 1 
#define numEBOs 2

#define PI 3.14
double normalize = 1;

using namespace std;

GLuint renderingProgram;
GLuint vao[numVAOs];
GLuint vbo[numVBOs];
GLuint ebo[numEBOs];

// float cameraX = 0.0f, cameraY = 0.0f, cameraZ = 0.8f; // grid cam 2D
float cameraX = 0.0f, cameraY = 0.0f, cameraZ = 1.0f; // sphere cam 3D
// float cameraX = 0.0f, cameraY = 0.0f, cameraZ = 9.0f; // torus cam 3D
float objLocX = 0.0f, objLocY = 0.0f, objLocZ = 0.0f; // first person cam

int width, heigth;
float aspect;
glm::mat4 pMat, vMat, mMat, mvMat;

CornerTable *CT;
string offFilePath = "offFiles/torus.off";
vector<unsigned int> path_indices;
string descFile = "";
int o, d;

/*------------------------------CONTROLES---------------------------*/
bool dragging = false;
double offsetX = 0.0;
double offsetY = 0.0;
double oldX = 0, oldY = 0;
double zoom = 1;
glm::mat4 transf = glm::mat4(1.0);

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods){
    if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS){
        glfwGetCursorPos(window, &oldX, &oldY);
        dragging = true;
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE){
        dragging = false;
    }

    // if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS){
    //     transf = glm::mat4(1.0f);
    // }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos){
    if (dragging){
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        offsetX = (xpos - oldX)/zoom;
        offsetY = (ypos - oldY)/zoom;

        oldX = xpos;
        oldY = ypos;

        float angleRotX = (PI/800) * offsetY;
        float angleRotY = (PI/800) * offsetX;

        transf = glm::rotate(transf, angleRotX, glm::vec3(1.0, 0.0, 0.0));
        transf = glm::rotate(transf, angleRotY, glm::vec3(0.0, 1.0, 0.0));
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
    double sensibilidad = 1.09;
    if(yoffset != 0){
        zoom = yoffset>0? sensibilidad:1/sensibilidad;
        transf = glm::scale(transf, glm::vec3(zoom, zoom, zoom));
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
        transf = glm::mat4(1.0f);
}
/*------------------------------------------------------------------*/

/*------------------------------IMGUI-------------------------------*/
    bool
    ALL_TRIANG_BORDER = false,
    PRINT_PATH = true,
    PRINT_PATH_TRIANG_BORDER = true,
    PRINT_VERT = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImVec4 surf_color = ImVec4(0.7f, 0.75f, 0.4f, 1.00f);
    ImVec4 path_color = ImVec4(0.0f, 0.0f, 0.00f, 1.00f);

/*---------- -------------------------------------------------------*/


void readMeshFiles() {
    // string filePath = "offFiles/torus.off";
    // string filePath = "offFiles/sphere.off";
    // string filePath = "offFiles/grid.off";

    ifstream fin(offFilePath);
    cout<<"[LOG]\tFichero OFF cargado"<<endl;
    //First Line OFF
    string name;
    //Second Line
    int npoint, ntriangle, var3;
    // npoint +1 Line edges that form triangle
    int var0;
    fin>>name;
    fin>>npoint>>ntriangle>>var3;
    cout<<"[LOG]\tCantidades leidas"<<endl;

    /* Llenamos la lista de puntos */
    double* vertexList = new double[3*npoint];
    for (int i = 0; i < npoint; ++i){
        fin>>vertexList[3*i]>>vertexList[3*i+1]>>vertexList[3*i+2];
        normalize = max(normalize, abs(vertexList[3*i]));
        normalize = max(normalize, abs(vertexList[3*i+1]));
        normalize = max(normalize, abs(vertexList[3*i+2]));
    }
    cout<<"normalize: "<<normalize;
    cout<<"[LOG]\tVertices leidos"<<endl;

    const CornerType numberTriangles = ntriangle;
    const CornerType numberVertices = npoint;
    
    /* Fill the list with all the points */
    CornerType* triangleList = new CornerType[3*ntriangle];
    for (int i = 0; i <ntriangle; ++i)
        fin>>var0>>triangleList[i*3]>>triangleList[i*3+1]>>triangleList[i*3+2];
    cout<<"[LOG]\tIndices triangulos leidos"<<endl;
    
    int numberCoordinatesByVertex = 3;
    
    CornerTable *ct = new CornerTable(triangleList, vertexList,
        numberTriangles, numberVertices, numberCoordinatesByVertex);
    cout<<"[LOG]\tCorner Table creada"<<endl<<endl;
    
    /* Especificaciones */
    descFile +="_________________________[OFF]_________________________ \n";
    descFile +="\t\t (*) File: " + offFilePath + "\n";
    descFile +="\t\t (*) Num vertices: " + to_string(npoint) + "\n";
    descFile +="\t\t (*) Num triangulos: " + to_string(ntriangle) + "\n";
    descFile +="\t\t (*) Num colores: " + to_string(var3) + "\n";
    descFile +="_________________________[OFF]_________________________ \n";

    cout<<descFile;
    descFile = "";
    descFile +="______________[OFF]______________ \n";
    descFile +="\t (*) File: " + offFilePath + "\n";
    descFile +="\t (*) Num vertices: " + to_string(npoint) + "\n";
    descFile +="\t (*) Num triangulos: " + to_string(ntriangle) + "\n";
    descFile +="\t (*) Num colores: " + to_string(var3) + "\n";
    descFile +="______________[OFF]______________ \n";

    CT = ct;
    fin.close();
}

void buildPath(int o, int d){
    vector<unsigned int> path;
    const CornerType *trianglesPositions = CT->getTriangleList();

    min_path(path, CT, o, d);
    
    PrintDijkstraPath(path, o ,d);

    int aux = path[d];
    path_indices.push_back(trianglesPositions[d*3]);
    path_indices.push_back(trianglesPositions[d*3 + 1]);
    path_indices.push_back(trianglesPositions[d*3 + 2]);

    while(aux>-1){
        path_indices.push_back(trianglesPositions[aux*3]);
        path_indices.push_back(trianglesPositions[aux*3 + 1]);
        path_indices.push_back(trianglesPositions[aux*3 + 2]);
        aux = path[aux];
    }

}

void setupVertices(void) {

    double* vertexPositions = CT->getAttributes();
    const CornerType *trianglesPositions = CT->getTriangleList();
    
    for (unsigned int i=0; i<CT->getNumberVertices()*3; i++) // normalizar coordenadas
        vertexPositions[i] *= 1/(normalize*2);

    glGenVertexArrays(1, vao);
    glBindVertexArray(vao[0]);

    glGenBuffers(numVBOs, vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, CT->getNumberVertices()*3*sizeof(double), &vertexPositions[0], GL_STATIC_DRAW);

    glGenBuffers(numEBOs, ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, CT->getNumTriangles()*3*sizeof(CornerType), &trianglesPositions[0], GL_STATIC_DRAW);
 
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, path_indices.size()*3*sizeof(CornerType), &path_indices[0], GL_STATIC_DRAW);
}

void init(GLFWwindow *window) {

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    renderingProgram = Utils::createShaderProgram("vertShader.glsl", "fragShader.glsl");
    glUseProgram(renderingProgram);
    
    buildPath(o, d);
    setupVertices();

    glfwGetFramebufferSize(window, &width, &heigth);
    aspect = (float)width / (float)heigth;
    pMat = glm::perspective(1.0472f, aspect, 0.1f, 1000.0f);
    vMat = glm::translate(glm::mat4(1.0f), glm::vec3(-cameraX, -cameraY, -cameraZ));
}

void display(GLFWwindow *window, double currentTime) {

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Camino mas corto en CornerTable - GUI");
    ImGui::Text("Pintar:");
    ImGui::Checkbox("Camino minimo encontrado", &PRINT_PATH);
    ImGui::Checkbox("Bordes de triangulos en el Camino minimo", &PRINT_PATH_TRIANG_BORDER);
    ImGui::Checkbox("Todos los vertices", &PRINT_VERT);
    ImGui::Checkbox("Bordes de todos los triangulos", &ALL_TRIANG_BORDER);
    ImGui::Separator();
    ImGui::ColorEdit3("Color de superficie", (float*)&surf_color);
    ImGui::ColorEdit3("Color de camino", (float*)&path_color);
    ImGui::ColorEdit3("Color de fondo", (float*)&clear_color);


    if (ImGui::CollapsingHeader("Controles")){
        ImGui::Text("Transformaciones:");
        ImGui::Indent();
        ImGui::Text("Rotaciones:");
        string t;
        t  = "Click derecho y arrastre con el\n"+
        t += "mouse en forma vertical para girar\n"+
        t += "en el eje X";
        ImGui::BulletText(t.c_str());

        t = "";
        t += "Click derecho y arrastre con el\n"+
        t += "mouse en forma horizontal para\n"+
        t += "girar en el eje Y";
        ImGui::BulletText(t.c_str());

        ImGui::BulletText("Presione 'D' para deshacer todas\nlas rotaciones");
        ImGui::Unindent();
        ImGui::Indent();
        ImGui::Text("Zoom in/out:");
        ImGui::BulletText("Haga zoom con el scroll\nde su mouse");
        ImGui::Unindent();
        ImGui::Separator();
    }

    if (ImGui::CollapsingHeader("Informacion (OFF file)")){
        ImGui::Text(descFile.c_str());
        ImGui::Separator();
    }

    ImGui::Separator();
    ImGui::Text("Promedio de aplicación %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Separator();
    ImGui::End();

    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(renderingProgram);
	glUniformMatrix4fv(glGetUniformLocation(renderingProgram, "proj_matrix"), 1, GL_FALSE, glm::value_ptr(pMat));

    mMat = glm::translate(glm::mat4(1.0f), glm::vec3(objLocX, objLocY, objLocZ));
    mMat *= transf;
    mvMat = vMat * mMat;
	glUniformMatrix4fv(glGetUniformLocation(renderingProgram, "mv_matrix"), 1, GL_FALSE, glm::value_ptr(mvMat));

    // ------------------ VBO VERTICES - Bind vertices ---------------------------------
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glVertexAttribPointer(0, 3, GL_DOUBLE, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glm::vec3 color1;

    // ------------------ EBO ALL Triangles - Bind EBO and PAINT MESH ------------------
    color1 = glm::vec3(surf_color.x, surf_color.y, surf_color.z);
    glUniform3fv(glGetUniformLocation(renderingProgram, "u_color"), 1, value_ptr(color1));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo[0]);
    glDrawElements(
        GL_TRIANGLES,
        CT->getNumTriangles()*3,
        GL_UNSIGNED_INT,
        0
    );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    if (ALL_TRIANG_BORDER){
        // ------------------ PAINT LINES (ALL TRIANGLES BORDER) ----------------------
        color1 = glm::vec3(1.0, 1.0, 1.0);
        glUniform3fv(glGetUniformLocation(renderingProgram, "u_color"), 1, value_ptr(color1));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo[0]);

        for(int i=0; i<CT->getNumTriangles() ; i++){
            glDrawElements(
                GL_LINE_LOOP,
                3,
                GL_UNSIGNED_INT,
                (void*)(i*3*sizeof(GL_UNSIGNED_INT))
            );
        }
    }

    if (PRINT_PATH){
        // ------------------ EBO PATH Triangles - Bind EBO and PAINT PATH ------------
        color1 = glm::vec3(path_color.x, path_color.y, path_color.z);
        glUniform3fv(glGetUniformLocation(renderingProgram, "u_color"), 1, value_ptr(color1));

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo[1]);
        glDrawElements(
            GL_TRIANGLES,
            path_indices.size(),
            GL_UNSIGNED_INT,
            0
        );
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    if (PRINT_PATH_TRIANG_BORDER){
        // ------------------ PAINT LINES (PATH TRIANGLES BORDER) --------------------
        color1 = glm::vec3(1.0, 0.0, 0.0);
        glUniform3fv(glGetUniformLocation(renderingProgram, "u_color"), 1, value_ptr(color1));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo[1]);

        for(int i=0; (unsigned int)i<path_indices.size()/3 ; i++){
            glDrawElements(
                GL_LINE_LOOP,
                3,
                GL_UNSIGNED_INT,
                (void*)(i*3*sizeof(GL_UNSIGNED_INT))
            );
        }
    }

    if (PRINT_VERT){
        // ------------------ PAINT POINTS (TRIANGLES VERTICES) -----------------------
        color1 = glm::vec3(0.0, 0.0, 0.0);
        glUniform3fv(glGetUniformLocation(renderingProgram, "u_color"), 1, value_ptr(color1));
        glPointSize(4.0);
        glDrawArrays(GL_POINTS, 0, CT->getNumberVertices());
    }


    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int main() {

    system("cls");
    int op_model;
    cout<<"_________________[Modelos disponibles]_________________"<<endl;
    cout<<"\t 1. [2D]GRID "<<endl;
    cout<<"\t 2. [3D]SPHERE"<<endl;
    cout<<"\t 3. [3D]TORUS"<<endl;
    cout<<"_______________________________________________________"<<endl;
    cout<<"Opcion: "; cin>>op_model;
    if (op_model == 1){
        offFilePath = "offFiles\\grid.off";
        normalize = 0.5*0.5*0.25;
    }
    if (op_model == 2) offFilePath = "offFiles\\sphere.off";
    if (op_model == 3) offFilePath = "offFiles\\torus.off";

    readMeshFiles();
    cout<<endl;
    cout<<"\t\t..:: CAMINO MINIMO ENTRE 2 TRIANGULOS ::.."<<endl;
    cout<<endl;
    cout<<"\t(*) Ingrese el triangulo de origen (0-"<<CT->getNumTriangles()-1<<"): ";
    cin>>o;
    cout<<"\t(*) Ingrese el triangulo de destino (0-"<<CT->getNumTriangles()-1<<"): ";
    cin>>d;

    if (!glfwInit()) exit(EXIT_FAILURE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    GLFWwindow * window = glfwCreateWindow(800, 800, "CG-T07-CornerTable", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    if (glewInit() != GLEW_OK) exit(EXIT_FAILURE);
    cout<<"GL_VERSION: "<<glGetString(GL_VERSION)<<endl;
    glfwSwapInterval(0);

    init(window);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        display(window, glfwGetTime());
        glfwSwapBuffers(window);
    }

    glDeleteBuffers(numVBOs, vbo);
    glDeleteBuffers(numEBOs, ebo);
    glDeleteShader(renderingProgram);
    glDeleteVertexArrays(numVAOs, vao);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
/*
g++ -o2 -Wall main.cpp -o main -L..\..\dependencies -lglew32s -lglu32 -lopengl32 -lglfw3 -mwindows -mconsole
*/
