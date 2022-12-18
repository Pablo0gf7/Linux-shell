/*------------------------------------------------------------------------------
Proyecto Shell de UNIX. Sistemas Operativos
Grados I. Inform�tica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Algunas secciones est�n inspiradas en ejercicios publicados en el libro
"Fundamentos de Sistemas Operativos", Silberschatz et al.

Para compilar este programa: gcc ProyectoShell.c ApoyoTareas.c -o MiShell
Para ejecutar este programa: ./MiShell
Para salir del programa en ejecuci�n, pulsar Control+D
------------------------------------------------------------------------------*/

#include "ApoyoTareas.h" // Cabecera del m�dulo de apoyo ApoyoTareas.c
 
#define MAX_LINE 256 // 256 caracteres por l�nea para cada comando es suficiente
#include <string.h>  // Para comparar cadenas de cars. (a partir de la tarea 2)

// --------------------------------------------
//                     MAIN          
// --------------------------------------------
job * lista;

void manejador(int sig){
	int status, info, pid_wait;	
	enum status estado;		
	job* trabajo = lista;
	job *aux = NULL;
	block_SIGCHLD();
	trabajo = trabajo->next;
	
	while (trabajo != NULL) {
		
		pid_wait = waitpid(trabajo->pgid, &status, WUNTRACED | WNOHANG);
		estado = analyze_status(status, &info);

		if (pid_wait == trabajo->pgid) {
			printf("\nwait realizado a proceso en background: %s, pid: %i\n",
					trabajo->command, trabajo->pgid);

			if (estado == FINALIZADO  || estado==REANUDADO ) {
				printf("Comando");
        aux = trabajo->next;
				delete_job(lista,trabajo);
				free(trabajo);
				trabajo = aux;

			} else if (estado == SUSPENDIDO) {
				trabajo->ground = DETENIDO;
        printf("Comando");
				trabajo = trabajo->next;

			}

		}else{
			trabajo=trabajo->next;
		}


	}

	unblock_SIGCHLD();
}
int main(void)
{
      char inputBuffer[MAX_LINE]; // B�fer que alberga el comando introducido
      int background;         // Vale 1 si el comando introducido finaliza con '&'
      char *args[MAX_LINE/2]; // La l�nea de comandos (de 256 cars.) tiene 128 argumentos como m�x
                              // Variables de utilidad:
      int pid_fork, pid_wait; // pid para el proceso creado y esperado
      int status;             // Estado que devuelve la funci�n wait
      enum status status_res; // Estado procesado por analyze_status()
      int info;
    	int estado;	
    	char directory[1024];
    	getcwd(directory,1024);	      // Informaci�n procesada por analyze_status()
    	lista = new_list("lista_trabajo");
      int primerplano=0;
      job*item;
      signal(SIGCHLD,manejador);
      ignore_terminal_signals();
      
      
      
      while (1) // El programa termina cuando se pulsa Control+D dentro de get_command()
      {   		
        printf("COMANDO->");
        fflush(stdout);
        get_command(inputBuffer, MAX_LINE, args, &background); // Obtener el pr�ximo comando
        if (args[0]==NULL) continue; // Si se introduce un comando vac�o, no hacemos nada
        
        
        if(strcmp(args[0],"cd")==0){
          if(args[1]==NULL){
            chdir(getenv("HOME"));
            getcwd(directory,1024);
          }else{
            chdir(getenv(args[1]));
            getcwd(directory,1024);
          }                           
          status_res = analyze_status(status,&info);
		    	printf("Comando: %s, ejecutado en primer plano con pid: %i, Estado: %s, Info: %i \n",
          args[0], getpid(), status_strings[status_res], info); 
          continue;
          }
        
        if(strcmp(args[0],"logout")==0){
          status_res = analyze_status(status,&info);
		    	printf("Comando: %s, ejecutado en primer plano con pid: %i, Estado: %s, Info: %i \n",
          args[0], getpid(), status_strings[status_res], info); 
	        exit(0);
          continue;
        }
        if(strcmp(args[0], "jobs") == 0){          
			    if(list_size(lista)==0){
			    	printf("Empty list \n");
			     }else{
				      print_job_list(lista);
		      	}
		    	continue;
		    }
        if(strcmp(args[0], "bg")==0) {
	      	block_SIGCHLD();
    	    int pos=1;
        	if(args[1]!=NULL){
    		    pos=atoi(args[1]);
	      	}
	      	item=get_item_bypos(lista,pos);
	      	if((item!=NULL)&&(item->ground==DETENIDO)){
		      	item->ground==SEGUNDOPLANO;
		      	killpg(item->pgid,SIGCONT);
			
	      	}
	      	unblock_SIGCHLD();
		      continue;
      	}
	      if(strcmp(args[0], "fg")==0) {
		      block_SIGCHLD();
        	int pos=1;
        	primerplano=1;
    	
    	  if(args[1]!=NULL){
    	  	pos=atoi(args[1]);
	    	}
	    	item=get_item_bypos(lista,pos);
		    if(item!=NULL){
		    	set_terminal(item->pgid);
		  	  if(item->ground==DETENIDO){
			  	  killpg(item->pgid,SIGCONT);
		  	  }
		  	  pid_fork=item->pgid;
		  	  delete_job(lista,item);
		    }
		    unblock_SIGCHLD();
	    }
      
    // Los pasos a seguir a partir de aqu�, son:
    // (1) Genera un proceso hijo con fork()
    pid_fork=fork();
    //Si da error
    if(pid_fork == -1){	
				printf ("Error al crear un proceso");
				
			}
    // (2) El proceso hijo invocar� a execvp()
    if(pid_fork == 0){	// PROCESO HIJO
			
				new_process_group(getpid());
				restore_terminal_signals();
				execvp(args[0],args);
				printf("ERROR: command not found %s\n",args[0]);
				exit(EXIT_FAILURE);
   
     
    // (3) El proceso padre esperar� si background es 0; de lo contrario, "continue" 
    }else{
      new_process_group(pid_fork);
      if(background==0){
        set_terminal(pid_fork);
        pid_wait = waitpid(pid_fork,&status,WUNTRACED);
        set_terminal(getpid());
        status_res = analyze_status(status, &info);
        if(status_res==SUSPENDIDO){  
						job *auxi2 = new_job(pid_fork,args[0],DETENIDO);
						block_SIGCHLD();
						add_job(lista,auxi2);
						unblock_SIGCHLD();
				}
        if (info!=1){   //3
						printf("Comando: %s, ejecutado en primer plano con pid: %i, Estado: %s, Info: %i \n",
						args[0],pid_fork, status_strings[status_res], info);
					}
   
     
      }else{
          job * aux = new_job(pid_fork,args[0],SEGUNDOPLANO);   
					block_SIGCHLD(); 
					add_job(lista,aux); 
					unblock_SIGCHLD();  

          if (info!=1){
						printf("Comando: %s, ejecutando en segundo plano con pid: %i \n",
						args[0], getpid()); 

					}
         
      }
    }
    // (4) El Shell muestra el mensaje de estado del comando procesado 
    //  (5) El bucle regresa a la funci�n get_command()
    
      
   
  } // end while
}



