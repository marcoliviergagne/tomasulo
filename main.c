/****** main.c **************************************************************
*   Description
*       Entry point for Tomasulo's Out-of-order execution algorithm
*       simulator.
*
*       Contains initialization, parameters, main simulation loop
*       and display functions 
*
*   Author          : Simon Pichette
*   Creation date   : Thu Jul 16 09:45:09 2020
*****************************************************************************
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*****************************************************************************/

/*
 Elements ajoutes pour parametrisation du programe suivant (atelier 9) :

 1. Entree du nom de fichier a traiter en console.

 2. Nombre instructions calcule automatiquement : permet de generer une liste
 de taille exacte (pas d'espace non necessaire en trop)

 3. Option (g)oto dans chaque cycle : permet d'atteindre un cycle precise, sans devoir
 passer une a la fois par toute les etapes intermediaires.

 4. Option d'entree des parametres ou conservation des valeurs par defaut des durees
 en cycles des types d'operation.

 5. Ecriture du test dans un fichier de sortie TEST_RESULTS_HERE.txt : permet de preserver
 en fichier un test. De plus, permet d'eviter quelques bug d'affichage de la console CLion.

 */


#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "instruction.h"
#include "station.h"
#include "tomasulo.h"

void print_banner();
void print_scoreboard(struct ilist* program);
void print_stations(struct slist* stations);
void print_registers();
int load_program(const char* filename, struct ilist* prog, const int exec_cycles[]);
void print_state(struct state* s, char* reg_names[]);

// Global variable for output file
FILE *ptr_output_file;

int main(void) {

    ptr_output_file = fopen ("TEST_RESULTS_HERE.txt", "w+");
    // arrays of strings for register names and content
    char* reg_names[] = {"F0", "F2", "F4", "F6", "F8", "F10", "F12", "F14"};    
    char* reg_contents[] = {"", "", "", "", "", "", "", ""};
    char input;
    int nbr_inst = -9;
    int enter_parameters = -9;

    char filename[100];

    while(nbr_inst == -9)
    {
        printf("Enter the name of file (with extension) to compile with Tomasulo's algorithm :\n>");
        scanf("%s", filename);

        // Count nbr of instructions to create a list of appropriate number of elements
        nbr_inst = count_nbr_inst(filename);

        if(nbr_inst == -9)
            printf("No such file, please enter a valid file name.\n\n");
    }

    // program loading
    struct ilist* program = create_inst_list(nbr_inst);
    if (!program) {
        puts("list creation failed");
    }

    // "ld", "sw", "addd", "subd", "muld", "divd". Default values
    int exec_cycles[] = { 1,    1,    2    ,  2    ,  4     ,  8};

    while(enter_parameters == -9)
    {
        printf("Do you want to change default execute cycles number ? Enter 1 for yes, 0 for no. "
               "\n\nThe current numbers are :"
               "ld = 1"
               "sw = 1"
               "addd = 2"
               "subd = 2"
               "muld = 4"
               "divd = 8"
               "\n\n");

        scanf("%d", &enter_parameters);

        if(enter_parameters == 0)
        {
            enter_parameters = -1;
            break;
        }

        else if (enter_parameters == 1)
        {
            printf("Enter new value for ld : \n\n");
            scanf("%i", &exec_cycles[0]);

            printf("Enter new value for sw : \n\n");
            scanf("%i", &exec_cycles[1]);

            printf("Enter new value for addd : \n\n");
            scanf("%i", &exec_cycles[2]);

            printf("Enter new value for subd : \n\n");
            scanf("%i", &exec_cycles[3]);

            printf("Enter new value for muld : \n\n");
            scanf("%i", &exec_cycles[4]);

            printf("Enter new value for divd : \n\n");
            scanf("%i", &exec_cycles[5]);
        }

        else
            enter_parameters = -9;

    }

    load_program(filename, program, exec_cycles);

    // create reservation stations
    struct slist* stations = create_station_list(10);
    add_station(stations, "Add1", addsub);
    add_station(stations, "Add2", addsub);
    add_station(stations, "Add3", addsub);
    add_station(stations, "Mul1", muldiv);
    add_station(stations, "Mul2", muldiv);
    add_station(stations, "Load1", loadstore);
    add_station(stations, "Load2", loadstore);

    // init simulation state context
    struct state context;
    context.program = program;
    context.stations = stations;
    context.issue_width = 1;
    context.regfile_size = 8;
    int desired_cycle = 0;

    // run simulation
    for (context.cycle = 1; context.cycle < 100; context.cycle++) {
        retire(&context, reg_contents);
        issue(&context, reg_names, reg_contents);
        execute(&context);
        writeback(&context);

        print_state(&context, reg_contents);

        if(desired_cycle == context.cycle)
        {
            desired_cycle = 0;
        }

        if(desired_cycle == 0)
        {
        puts("(c)ontinue, (a)bort, (g)oto");
        scanf(" %c", &input);
            switch(input) {
                case 'c' :
                    break;
                case 'a':
                    fclose(ptr_output_file);
                    return 0;
                case 'g':

                    printf("Enter the cycle you want to reach :\n>");
                    scanf("%i", &desired_cycle);

                    break;

            }

        }
        //system("clear");    // UNIX
        system("cls");    // DOS
    }
}


void print_state(struct state* s, char* reg_names[]) {
    print_banner();
    fprintf(ptr_output_file,"Cycle : %d \n", s->cycle);
    printf("Cycle : %d \n", s->cycle);
    print_scoreboard(s->program);
    print_stations(s->stations);
    print_registers(reg_names, s->regfile_size);
}


int load_program(const char* filename, struct ilist* prog, const int exec_cycles[]) {
    char buffer[20];
    char* line; 

    FILE *source = fopen(filename, "rt");
    if (!source) {
        return -1;
    }

    while (fgets(buffer, sizeof(buffer), source)) {
        line = strtok(buffer, "\n");        // remove trailing newline
        int result = add_inst(prog, line, exec_cycles);
        if (!result) {
            //inst_details(&prog->data[prog->occupied-1]);
        } else {
            printf ("Error!!, code %d\n", result);
            return -1;
        }

    }
    return 0;
}


void print_banner() {
    static int count_cycle = 0;
    if(count_cycle != 0) {
        fputs("\n\n\n", ptr_output_file);
    }
    count_cycle++;
    fputs("***********************************************************************\n",ptr_output_file);
    fputs("*  ELE749 Demonstration of Tomasulo's algorithm      *\n",ptr_output_file);
    fputs("***********************************************************************\n",ptr_output_file);

    fputs("",ptr_output_file);

    puts("***********************************************************************");
    puts("*  ELE749 Demonstration of Tomasulo's algorithm      *");
    puts("***********************************************************************");

    puts("");
}


void print_scoreboard(struct ilist* program) {
    fputs("|---------------------------------------------------------------------|\n",ptr_output_file);
    fputs("| Instruction         | Issue     | Execute   | Writeback | Retired   |\n",ptr_output_file);
    fputs("|---------------------------------------------------------------------|\n",ptr_output_file);
    for (size_t i = 0; i < program->occupied; i++) {
        print_inst(&program->data[i], ptr_output_file);
    }
    fputs("|---------------------------------------------------------------------|\n",ptr_output_file);
    fputs("",ptr_output_file);

    puts("|---------------------------------------------------------------------|");
    puts("| Instruction         | Issue     | Execute   | Writeback | Retired   |");
    puts("|---------------------------------------------------------------------|");
    for (size_t i = 0; i < program->occupied; i++) {
        print_inst(&program->data[i], ptr_output_file);
    }
    puts("|---------------------------------------------------------------------|");
    puts("");
}


void print_stations(struct slist* stations) {
    fputs("|---------------------------------------------------------------------|\n",ptr_output_file);
    fputs("| Reservation stations                                                |\n",ptr_output_file);
    fputs("|---------------------------------------------------------------------|\n",ptr_output_file);
    fputs("| Name     |  Busy  |    Op   |   Vj    |    Vk   |    Qj   |    Qk   |\n",ptr_output_file);
    fputs("|---------------------------------------------------------------------|\n",ptr_output_file);
    for (size_t i = 0; i < stations->occupied; i++) {
        print_station(&stations->data[i],ptr_output_file);
    }
    fputs("|---------------------------------------------------------------------|\n",ptr_output_file);
    fputs("",ptr_output_file);

    puts("|---------------------------------------------------------------------|");
    puts("| Reservation stations                                                |");
    puts("|---------------------------------------------------------------------|");
    puts("| Name     |  Busy  |    Op   |   Vj    |    Vk   |    Qj   |    Qk   |");
    puts("|---------------------------------------------------------------------|");
    for (size_t i = 0; i < stations->occupied; i++) {
        print_station(&stations->data[i],ptr_output_file);
    }
    puts("|---------------------------------------------------------------------|");
    puts("");
}


void print_registers(char* reg_names[], size_t num) {
    fputs("|-----------------------------------------------------------------------|\n",ptr_output_file);
    fputs("| Register wait queues (Qi)                                             |\n",ptr_output_file);
    fputs("|-----------------------------------------------------------------------|\n",ptr_output_file);
    fputs("|   F0   |   F2   |   F4   |   F6   |   F8   |  F10   |  F12   |  F14   |\n",ptr_output_file);
    for (size_t i = 0; i < num; i++) {
        fprintf(ptr_output_file,"|%7s ", reg_names[i]);
    }
    fputs("|\n",ptr_output_file);
    fputs("|-----------------------------------------------------------------------|\n",ptr_output_file);

    puts("|-----------------------------------------------------------------------|");
    puts("| Register wait queues (Qi)                                             |");
    puts("|-----------------------------------------------------------------------|");
    puts("|   F0   |   F2   |   F4   |   F6   |   F8   |  F10   |  F12   |  F14   |");
    for (size_t i = 0; i < num; i++) {
        printf("|%7s ", reg_names[i]);
    }
    puts("|");
    puts("|-----------------------------------------------------------------------|");
}
