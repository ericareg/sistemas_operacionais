#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/stat.h>






#define NUM_PROCESSOS 3
#define TIMESLICE 1 // Timeslice de 1 segundo






int cpu = 0;
int processos[NUM_PROCESSOS]; // PIDs dos processos
int processo_atual = NUM_PROCESSOS-1; // Índice do processo que está executando
// int espera[NUM_PROCESSOS] = {0}; //se processo block --> 1
int terminados[NUM_PROCESSOS] = { 0 }; //se processo terminado --> 1
int term = 0;
int* PC;






void irq0_handler(int sig) {
    
    
    printf("\n\n\nTempo CPU: %d\n", cpu);
    cpu++;
   if(cpu!=1 && !terminados[processo_atual] ){
        kill(processos[processo_atual], SIGSTOP); // Pausar o processo atual
        printf("Parando processo %d\n", processo_atual);
    } 


    
    // Avançar para o próximo processo
    processo_atual = (processo_atual + 1) % NUM_PROCESSOS;


    for (int i = 0; i < NUM_PROCESSOS; i++) {




        if (!terminados[processo_atual]) {
            //printf("\nespera proc atua%d\n", espera[processo_atual]);




            printf("Processo %d, PC=%d\n", processo_atual, PC[processo_atual]);//de fora
            
            
            kill(processos[processo_atual], SIGCONT);   // Continuar o próximo processo
  //          printf("p atual%d\n",processo_atual);






//            printf("pre:%d  ",PC[processo_atual]);


            PC[processo_atual]--;
//            printf("pos:%d/n",PC[processo_atual]);
            break;
        }
        
        if(term == NUM_PROCESSOS){
            printf("Todos processos terminaram!");
        }
        
    }
}




void processo_funcao(int id) {
    while (PC[id] > 0) {






        sleep(1); // Simula a execução
        
    }
    terminados[id] = 1;
    term++;
    exit(0); // Termina o processo 
}


void intercontroller_sim() {
    while (1) {
        sleep(TIMESLICE); // Emula IRQ0 a cada 1 segundo
        kill(getppid(), SIGUSR1); // Envia sinal para o KernelSim
    }
}




int main() {
    signal(SIGUSR1, irq0_handler); // Registrar o tratador de IRQ0


    int segmento = shmget(IPC_PRIVATE, (sizeof(int) * NUM_PROCESSOS), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    if (segmento == -1) {
        perror("Erro ao alocar memória compartilhada");
        exit(1);
    }






    // associa a memória compartilhada ao processo
    PC = (int*)shmat(segmento, NULL, 0); // comparar o retorno com -1
    if (PC == (void*)-1) {
        perror("Erro ao associar a memória compartilhada");
        exit(1);
    }






    for (int i = 0; i < NUM_PROCESSOS; i++) {
        PC[i] = 7;
    }








    // Criar processos de aplicação
    for (int i = 0; i < NUM_PROCESSOS; i++) {
        if ((processos[i] = fork()) == 0) {
            // Código do processo de aplicação
            processo_funcao(i);
            
        }
    }




    // Pausar todos os processos no início
    for (int i = 0; i < NUM_PROCESSOS; i++) {
        kill(processos[i], SIGSTOP);
    }
    // Criar o controlador de interrupções (InterControllerSim)
    int pidInter = fork();
    if (pidInter == 0) {
        intercontroller_sim();
        exit(0);
    }








    // Inicializar o primeiro processo
    kill(processos[processo_atual], SIGCONT);






    // KernelSim espera a finalização de todos os processos
    for (int i = 0; i < NUM_PROCESSOS; i++) {
        wait(NULL);
    }


    kill(pidInter, SIGKILL);




shmdt(PC);
    shmctl(segmento, IPC_RMID, NULL);






  return 0;
}
