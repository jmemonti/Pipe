#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>

#define MAXCOMMAND 100
#define READ 0
#define WRITE 1
#define COMMAND1 "ls"
#define COMMAND2 "grep"
#define EXTENSION ".txt"


pid_t pidHijos[2];//array para almacenar los pid's de los 2 procesos hijo

void ayuda(){
  fprintf(stderr, "Uso: buscaletra letra1 <letra2> <letraN> ruta");
  exit(EXIT_FAILURE);
}

void senyal(int sen){

  printf("\nSeñal SIGINT recivida, terminando procesoss...\n");
  
  //matar a los 2 procesos hijo vivos
  for (int j=0; j<2; j++) {
    kill(pidHijos[j], SIGTERM);
    printf("Fin hijo %d\n", pidHijos[j]);
  }
  exit(EXIT_SUCCESS);//finalizar programa y evitar doble printf terminando los procesos y haciendo los printf del wait
}

void create_file(const char*archive, char*root){
  //vars
  FILE *archivo; 
  char file[MAXCOMMAND];//para resetear las variables en cada iteración del bucle
  char path[MAXCOMMAND];


  strncpy(file, archive, sizeof(file));//copias el argumento actual en la variable file
  strcat(file, EXTENSION);//concatenas el file=file+EXTENSION

  strncpy(path, root, sizeof(path));//copias el último argumento en la variable path
  strcat(path, file);//concatenas el path=path+file
    
  archivo=fopen(path, "w");//creas el fichero

  if (archivo == NULL) {
    fprintf(stderr,"Error al crear el fichero: %s\n", file);
    exit(EXIT_FAILURE);
  }else {
    fclose(archivo);
  }
}

void createProcess(const char* file, char*path){
    //vars
    pid_t pid_ls;
    pid_t pid_grep;
    int status;
    int fd[2];
    int fd_fich;
    char element[MAXCOMMAND]="^";
    int pid;

  
    //crear tubería
    if(pipe(fd) !=0){
      fprintf(stderr, "Erro al crear la tubería\n");
      exit(EXIT_FAILURE);
    }
    
    //proceso 1
    pid_ls=fork(); 
    if ( pid_ls == 0 ) {//hijo proceso 1
      close(fd[READ]);
      dup2(fd[WRITE], STDOUT_FILENO);
      close(fd[WRITE]);
      sleep(3);//tener tiempo para poder mandar una señal de terminación
      execlp(COMMAND1, COMMAND1, NULL);
    }else {//padre proceso 1
      close(fd[WRITE]);
      pidHijos[0]=pid_ls;//guardar pid del hijo 1
      printf("Inicio hijo %d, proceso %d con letra inicial %s\n", pid_ls, 1, file);
    }

    //proceso 2
    pid_grep=fork();
    if ( pid_grep == 0 ) {//hijo proceso 2
      close(fd[WRITE]);
      dup2(fd[READ], STDIN_FILENO);
      close(fd[READ]);
      
      create_file(file, path); 
      
      strcat(path, file);

      fd_fich=open(strcat(path, EXTENSION), O_WRONLY);//abrir fichero para obtener el fd del fichero
      dup2(fd_fich, STDOUT_FILENO);//redireccionas el stdout al descriptor de archivo del fichero 
      
      execlp(COMMAND2, COMMAND2, strcat(element, file), NULL);//buscar solo ficheros con iniciales ^x
    }else {//padre proceso 2 
      close(fd[READ]); 
      pidHijos[1]=pid_grep;//guardar pid del hijo 2
      printf("Inicio hijo %d, proceso %d con letra inicial %s\n", pid_grep, 2, file);
    }
    
    //esperar a que los 2 procesos hijo terminen
    for (int j=0; j<2; j++) {
      if ( (pid=wait(&status)) != -1 ) {
        printf("Fin hijo %d\n", pid);
      }
    }
  }


int main(int argc, char*argcv[]){
  //vars
  char* files[MAXCOMMAND];
  int i;
  struct stat st;

  //sintaxis control
  if (argc<3) {
    ayuda();
  }
  
  //analizas el archivo
  if (stat(argcv[argc-1], &st) !=0) {
    fprintf(stderr, "Error al obtener información del archivo");
    return EXIT_FAILURE;
  }
  
  //si no es un directorio
  if (S_ISDIR(st.st_mode) ==0) {
    fprintf(stderr, "buscaletra: %s: No existe el directorio", argcv[argc-1]);
    return EXIT_FAILURE;
  }
 
  //lista de parámetros
  for (i=1; i<argc-1; i++) {
    files[i-1]=argcv[i];
  }
 
  signal(SIGINT, senyal);//captura señal SIGINT y llama al handler( senyal(int x), x= id de la señal capturada)
  i=0;
  while (i<argc-2) { 
    createProcess(files[i], argcv[argc-1]);
    i++;
  }
  return 0;  
}
