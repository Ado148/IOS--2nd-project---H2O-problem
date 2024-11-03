#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <signal.h>

#define MMAP(pointer) {(pointer) = mmap(NULL, sizeof(*(pointer)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);}//namapovanie stranok pamate, na zaklade premennej pointeru sa urci pocet bajtov
#define UNMAP(pointer) {munmap((pointer), sizeof((pointer)));} //unmap stranky pamate 
 
//inicializacia zdielanych premennych / pamate
int *hydrogen_counter = NULL,
    *oxygen_counter = NULL,
    *num_of_line_file = NULL,//premenna pre pocet riadkov v subore
    *noM = NULL,//cislo molekuly
    *counter_3_atoms = NULL,//premenna na pocitanie poctu moelkul ak su tri vypis molecule x created
    *molecule_created = NULL;

int molecules_max = 0;//maximalny pocet molekul ktory vieme vytvorit

//inicializacia semaforov
sem_t *write_to_file_sem = NULL,
      *oxygen_sem = NULL,
      *hydrogen_sem = NULL,
      *mutex = NULL,
      *mutex_barrier_sem = NULL,
      *mutex_barrier_1_sem = NULL,
      *mutex_barrier_2_sem = NULL;

//vystupny subor
FILE *output_file;

int inicialization()
{   
    //otvorenie/vytvorenie suboru;
    if((output_file = fopen("proj2.out", "w")) == NULL)
    {
        fprintf(stderr, "Chyba vystupneho suboru\n");
        return 1;
    }
    //inicializacia semaforov
    write_to_file_sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if(write_to_file_sem == MAP_FAILED)//zapisovaci semafor
    {
        fprintf(stderr, "Chyba pri vytvarani zapisovacieho semaforu\n");
        return 1;
    }
    if (sem_init(write_to_file_sem, 1, 1) == -1)
    {
        fprintf(stderr,"Chyba pri vytvarani zapisovacieho semaforu\n");
        return 1;
    }

    oxygen_sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if(oxygen_sem == MAP_FAILED)//oxygen semafor
    {
        fprintf(stderr, "Chyba pri vytvarani oxygen semaforu\n");
        return 1;
    }
    if (sem_init(oxygen_sem, 1, 0) == -1)
    {
        fprintf(stderr,"Chyba pri vytvarani oxygen semaforu\n");
        return 1;
    }


    hydrogen_sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if(hydrogen_sem == MAP_FAILED)//hydrogen semafor
    {
        fprintf(stderr, "Chyba pri vytvarani hydrogen semaforu\n");
        return 1;
    }
    if (sem_init(hydrogen_sem, 1, 0) == -1)
    {
        fprintf(stderr,"Chyba pri vytvarani hydrogen semaforu\n");
        return 1;
    }

    mutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if(mutex == MAP_FAILED)//mutex semafor
    {
        fprintf(stderr, "Chyba pri vytvarani mutex semaforu\n");
        return 1;
    }
    if (sem_init(mutex, 1, 1) == -1)
    {
        fprintf(stderr,"Chyba pri vytvarani mutex semaforu\n");
        return 1;
    }

    mutex_barrier_sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if(mutex_barrier_sem == MAP_FAILED)//mutex_barrier_sem semafor
    {
        fprintf(stderr, "Chyba pri vytvarani mutex_barrier semaforu\n");
        return 1;
    }
    if (sem_init(mutex_barrier_sem, 1, 1) == -1)
    {
        fprintf(stderr,"Chyba pri vytvarani mutex_barrier semaforu\n");
        return 1;
    }

    mutex_barrier_1_sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if(mutex_barrier_1_sem == MAP_FAILED)//mutex_barrier_1_sem semafor
    {
        fprintf(stderr, "Chyba pri vytvarani mutex_barrier_1 semaforu\n");
        return 1;
    }
    if (sem_init(mutex_barrier_1_sem, 1, 0) == -1)
    {
        fprintf(stderr,"Chyba pri vytvarani mutex_barrier_1 semaforu\n");
        return 1;
    }

    mutex_barrier_2_sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if(mutex_barrier_2_sem == MAP_FAILED)//mutex_barrier_2_sem semafor
    {
        fprintf(stderr, "Chyba pri vytvarani mutex_barrier_2 semaforu\n");
        return 1;
    }
    if (sem_init(mutex_barrier_2_sem, 1, 1) == -1)
    {
        fprintf(stderr,"Chyba pri vytvarani mutex_barrier_2 semaforu\n");
        return 1;
    }

    //alokacia zdielanych premennych;
    MMAP(hydrogen_counter)
    MMAP(oxygen_counter);
    MMAP(num_of_line_file);
    MMAP(noM);
    MMAP(counter_3_atoms);
    MMAP(molecule_created);

    return 0;
}

void free_resources()
{
    //uzavretie vystupneho suboru
    if((fclose(output_file)) != 0)
    {
        fprintf(stderr, "Chyba vystupneho suboru\n");
        exit(1);
    }

    //uvolnenie semaforov
    if(sem_destroy(write_to_file_sem) == -1)
    {
        fprintf(stderr, "Chyba pri sem destroy\n");
        exit(1);
    }
    if(sem_destroy(oxygen_sem) == -1)
    {
        fprintf(stderr, "Chyba pri sem destroy\n");
        exit(1);
    }
    if(sem_destroy(hydrogen_sem) == -1)
    {
        fprintf(stderr, "Chyba pri sem destroy\n");
        exit(1);
    }
    if(sem_destroy(mutex) == -1)
    {
        fprintf(stderr, "Chyba pri sem destroy\n");
        exit(1);
    }
    if(sem_destroy(mutex_barrier_sem) == -1)
    {
        fprintf(stderr, "Chyba pri sem destroy\n");
        exit(1);
    }
    if(sem_destroy(mutex_barrier_1_sem) == -1)
    {
        fprintf(stderr, "Chyba pri sem destroy\n");
        exit(1);
    }
    if(sem_destroy(mutex_barrier_2_sem) == -1)
    {
        fprintf(stderr, "Chyba pri sem destroy\n");
        exit(1);
    }

    //uvolnenie zdielanej pamate
    UNMAP(hydrogen_counter);
    UNMAP(oxygen_counter);
    UNMAP(num_of_line_file);
    UNMAP(noM);
    UNMAP(counter_3_atoms);
    UNMAP(molecule_created);
}

void oxygen_proc(int counterO, int ti, int tb, int oxygen, int hydrogen)//proces kyslika
{   
    int time_to_wait = (rand()%(ti + 1));
    int time_to_bond = (rand()%(tb + 1));
    counterO++;
    (*noM) = 1;
    //====================Started====================
    sem_wait(write_to_file_sem);
    (*num_of_line_file)++;//pocet riadkov v subore
    fprintf(output_file,"%d: O %d: started\n",*num_of_line_file, counterO);//counterO je idO
    fflush(output_file);
    sem_post(write_to_file_sem);
    //====================Going to queue====================
    usleep(time_to_wait * 1000);
    sem_wait(write_to_file_sem);
    (*num_of_line_file)++;
    fprintf(output_file,"%d: O %d: going to queue\n",*num_of_line_file, counterO);
    fflush(output_file);//pre spravne poradie zapisu do output suboru
    sem_post(write_to_file_sem);

    sem_wait(mutex);
    (*oxygen_counter)++; 
    if(*hydrogen_counter >= 2)
    {
        sem_post(hydrogen_sem);
        sem_post(hydrogen_sem);
        (*hydrogen_counter)-=2;
        sem_post(oxygen_sem);
        (*oxygen_counter)--;
    }else
    {
        if(molecules_max == 0)//Kontrola ci moze dojst k vytvoreniu dalsej molekuly
        {
            sem_wait(write_to_file_sem);
            (*num_of_line_file)++;
            fprintf(output_file,"%d: O %d: not enough H\n",*num_of_line_file, counterO);
            fflush(output_file);
            sem_post(write_to_file_sem);
            sem_post(mutex);
            return;
        }
        sem_post(mutex);
    }
    sem_wait(oxygen_sem);
    
    if(molecules_max == *molecule_created)//Kontrola ci moze dojst k vytvoreniu dalsej molekuly
    {
        sem_wait(write_to_file_sem);
        (*num_of_line_file)++;
        fprintf(output_file,"%d: O %d: not enough H\n",*num_of_line_file, counterO);
        fflush(output_file);
        sem_post(write_to_file_sem);
        sem_post(mutex);
        return;
    }
    //====================Creating molecule====================
    sem_wait(write_to_file_sem);
    (*num_of_line_file)++;
    fprintf(output_file,"%d: O %d: creating molecule %d\n",*num_of_line_file, counterO, *noM);
    fflush(output_file);
    sem_post(write_to_file_sem);
    usleep(time_to_bond * 1000);//uspanie na dobu vytvarania molekuly vody
    //====================Molecule created====================
    sem_wait(mutex_barrier_sem);
    (*counter_3_atoms)++;
    if(*counter_3_atoms == 3)
    {
        sem_wait(mutex_barrier_2_sem);
        sem_post(mutex_barrier_1_sem);
    }
    sem_post(mutex_barrier_sem);

    sem_wait(mutex_barrier_1_sem);
    sem_post(mutex_barrier_1_sem);

    sem_wait(write_to_file_sem);
    (*num_of_line_file)++;
    fprintf(output_file,"%d: O %d: molecule %d created\n",*num_of_line_file, counterO, *noM);;
    fflush(output_file);
    sem_post(write_to_file_sem);
    
    sem_wait(mutex_barrier_sem);
    (*counter_3_atoms)--;
    if((*counter_3_atoms) == 0)
    {
        sem_wait(mutex_barrier_1_sem);
        sem_post(mutex_barrier_2_sem);
    }
    sem_post(mutex_barrier_sem);

    sem_wait(mutex_barrier_2_sem);
    sem_post(mutex_barrier_2_sem);
    (*noM)++;

    (*molecule_created)++;//zaznamenanie mvytvorenia molekuly
    if(*molecule_created == molecules_max)
    {
        int j = 0;
        int i = 0;
        while(j < oxygen-molecules_max)
        {   
            sem_post(oxygen_sem);
            j++;
        }
        while(i < hydrogen-(molecules_max*2))
        {
            sem_post(hydrogen_sem);
            i++;
        }    
    }
    sem_post(mutex);
    
    //uzavretie vystupneho suboru
    fclose(output_file);
}

void hydrogen_proc(int counterH, int ti)//proces vodika
{     
    int time_to_wait = (rand()%(ti + 1));
    //int time_to_bond = (rand()%(tb + 1));
    counterH++;
    //====================Started====================
    sem_wait(write_to_file_sem);
    (*num_of_line_file)++;
    fprintf(output_file,"%d: H %d: started\n",*num_of_line_file, counterH);
    fflush(output_file);
    sem_post(write_to_file_sem);
    
    //====================Going to queue====================
    usleep(time_to_wait * 1000);
    sem_wait(write_to_file_sem);
    (*num_of_line_file)++;
    fprintf(output_file,"%d: H %d: going to queue\n",*num_of_line_file, counterH);
    fflush(output_file);
    sem_post(write_to_file_sem);

    sem_wait(mutex);
    (*hydrogen_counter)++;
    if(*hydrogen_counter >= 2 && *oxygen_counter >= 1)
    {
        sem_post(hydrogen_sem);
        sem_post(hydrogen_sem);
        (*hydrogen_counter)-=2;
        sem_post(oxygen_sem);
        (*oxygen_counter)--;
    }else
    { 
        if(molecules_max == 0)
        {
            sem_wait(write_to_file_sem);
            (*num_of_line_file)++;
            fprintf(output_file,"%d: H %d: not enough O or H\n",*num_of_line_file, counterH);
            fflush(output_file);
            sem_post(write_to_file_sem);
            return;
        }
        sem_post(mutex);
    }
    sem_wait(hydrogen_sem);
    
    if(molecules_max == *molecule_created)
    {
        sem_wait(write_to_file_sem);
        (*num_of_line_file)++;
        fprintf(output_file,"%d: H %d: not enough O or H\n",*num_of_line_file, counterH);
        fflush(output_file);
        sem_post(write_to_file_sem);
        return;
    }
    //====================Creating molecule====================
    sem_wait(write_to_file_sem); 
    (*num_of_line_file)++;
    fprintf(output_file,"%d: H %d: creating molecule %d\n",*num_of_line_file, counterH, *noM);
    fflush(output_file);
    sem_post(write_to_file_sem);
    //====================Molecule created====================
    sem_wait(mutex_barrier_sem);
    (*counter_3_atoms)++;
    if(*counter_3_atoms == 3)
    {
        sem_wait(mutex_barrier_2_sem);
        sem_post(mutex_barrier_1_sem);
    }
    sem_post(mutex_barrier_sem);

    sem_wait(mutex_barrier_1_sem);
    sem_post(mutex_barrier_1_sem);

    sem_wait(write_to_file_sem);
    //usleep(time_to_bond * 1000);
    (*num_of_line_file)++;
    fprintf(output_file,"%d: H %d: molecule %d created\n",*num_of_line_file, counterH, *noM);
    fflush(output_file);
    sem_post(write_to_file_sem);
    
    sem_wait(mutex_barrier_sem);
    (*counter_3_atoms)--;
    if(*counter_3_atoms == 0)
    {
        sem_wait(mutex_barrier_1_sem);
        sem_post(mutex_barrier_2_sem);
    }
    sem_post(mutex_barrier_sem);

    sem_wait(mutex_barrier_2_sem);
    sem_post(mutex_barrier_2_sem);

    //uzavretie vystupneho suboru
    fclose(output_file);
}

int main(int argc, char *argv[])
{
    if(argc > 5)//osetrenie vstupov
    {
        fprintf(stderr, "Prilis vela argumentov!\n");
        return 1;
    }
    if (argc < 5)
    {
        fprintf(stderr, "Prilis malo argumentov!\n");
        return 1;
    }
 
    char *end_strO = NULL;//ostatkova cast parametru NO
    char *end_strH = NULL;//ostatkova cast parametru NH
    char *end_strTi = NULL;//ostatkova cast parametru Ti
    char *end_strTb = NULL;//ostatkova cast parametru Tb
    int oxygen = strtol(argv[1], &end_strO, 10);
    int hydrogen = strtol(argv[2], &end_strH, 10);
    int ti = strtol(argv[3], &end_strTi, 10);//cas ktory caka atom kysliku alebo vodiku na zaradenie do fronty
    int tb = strtol(argv[4], &end_strTb, 10);//cas potrebny pre vytvorenie molekuly

    if (ti > 1000 || ti < 0 || tb >1000 || tb < 0)//Kontorla rozsahu casov
    {
        fprintf(stderr, "Neplatne hodnoty casovych argumentov!\n");
        exit(1);
    }
    if (oxygen >= 1000 || oxygen < 0 || hydrogen >=1000 || hydrogen < 0)//Kontorla rozsahu atomov
    {
        fprintf(stderr, "Neplatne hodnoty argumentov!\n");
        exit(1);
    }
    if ((*end_strO >= 'a' && *end_strO <= 'z') || (*end_strO >= 'A' && *end_strO <= 'Z'))//Kontrola ciselneho znaku NO
    {
        fprintf(stderr, "Zadal si neciselny znak za kyslik!\n");
        exit(1);
    }
    if ((*end_strH >= 'a' && *end_strH <= 'z') || (*end_strH >= 'A' && *end_strH <= 'Z'))//Kontrola ciselneho znaku NH
    {
        fprintf(stderr, "Zadal si neciselny znak za vodik!\n");
        exit(1);
    }
    if ((*end_strTi >= 'a' && *end_strTi <= 'z') || (*end_strTi >= 'A' && *end_strTi <= 'Z'))//Kontrola ciselneho znaku Ti
    {
        fprintf(stderr, "Zadal si neciselny znak za cas Ti!\n");
        exit(1);
    }
    if ((*end_strTb >= 'a' && *end_strTb <= 'z') || (*end_strTb >= 'A' && *end_strTb <= 'Z'))//Kontrola ciselneho znaku Tb
    {
        fprintf(stderr, "Zadal si neciselny znak za cas Tb!\n");
        exit(1);
    }

    if(oxygen == 0 || hydrogen == 0)//Kontrola ci jeden z argumentov nechyba
    {
        fprintf(stderr, "Jeden z argumentov chyba!\n");
        exit(1);
    }

    //pocitanie poctu molekul ktore vieme vytvorit
    //molecules_max = hydrogen/2;
    int help1 = oxygen;
    int help2 = hydrogen/2;
    if(help1 < help2 )
    {
        molecules_max = help1;
    }else
    {
        molecules_max = help2;
    }
    
    //inicializacia, otvorenie suboru, tvorenie semaforov
    if(inicialization() == 1)
    {
        free_resources();
        fprintf(stderr, "Chyba pri inicializacii!\n");
        exit(1);
    }else
    {
        inicialization();
    }
    pid_t process_oxygen;
    pid_t process_hydrogen;
    //pid_t hydrogen_field[hydrogen];
    //pid_t oxygen_field[oxygen];

        for (int o = 0; o < oxygen; o++)
        {
            process_oxygen = fork();
            /*if(process_oxygen > 0)//rodic
            {
                oxygen_field[o] = oxygen;
                if(o == oxygen-1)
                {
                    for (int j = 0; j < o; j++)
                    {
                        waitpid(oxygen_field[j], NULL, 0);
                    }                
                }
            }*/if(process_oxygen == 0)//dieta
            {
                oxygen_proc(o,ti,tb, oxygen, hydrogen);
                exit(0);
            }else if(process_oxygen < 0)//chybny stav
            {
                fprintf(stderr, "Chyba pri generovani procesov kyslika!\n");
                free_resources();
                exit(1);
            } 
        }
        

        for (int h = 0; h < hydrogen; h++)
        {
            process_hydrogen = fork();           
            /*if(process_hydrogen > 0)//rodic
            {
                hydrogen_field[h] = hydrogen;
                if(h == hydrogen-1)
                {
                    for (int j = 0; j < h; j++)
                    {
                        waitpid(hydrogen_field[j], NULL, 0);
                    }                
                }
            }*/if(process_hydrogen == 0)//dieta
            {
                hydrogen_proc(h,ti);
                exit(0);
            }else if(process_hydrogen < 0)//chybny stav
            {
                fprintf(stderr, "Chyba pri generovani procesov vodika!\n");
                free_resources();
                exit(1);
            }
        }
        while (wait(NULL) > 0);             
    //uvolnenie semaforov, zdielanej pamate, zevretie vystupneho suboru
    free_resources();
    return 0;
}