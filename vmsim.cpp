/*

OS: Project 4
Author: Jackie Ye

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <vector>


using namespace std;

//defile variables for sizing
const int tlb_size = 16;
const int page_size = 256;
int tlb_hits = 0;

int main(int argc, char *argv[]){

    //struct of a tlb element
    struct tlb{
        int tag;
        int page;
    };

    struct pg{
        int f_num;
        int p_num;
    };

    //empty vector for lru part
    vector<pg> lru_pg_table;

    //initialize the tlb table
    int j;
    vector<tlb> tlb_table;
    for(j = 0; j < tlb_size; j++){
        tlb_table.push_back(tlb());
        tlb_table[j].tag = -1;
        tlb_table[j].page = -1;
    }

    //initialize the page table
    vector<pg> page_table;
    for(j = 0; j < page_size; j++){
        page_table.push_back(pg());
        page_table[j].f_num = -1;
        page_table[j].p_num = -1;
    }

    //define variables for output
    double page_fault_rate = 0.0;
    double tlb_hits_rate = 0.0;
    const int total = 1000;
    int page_fault = 0;
    int p_addr = 0;
    int value = 0;

    //declare varrables for input
    char readFile[20];
    char outFile[20];
    char output[100];
    char mode[10];
    char virtual_mem[10];
    char* line = NULL;
    char v_addr[8];
    char *page_num_str = (char*)malloc(2);
    int page_num = 0;
    char *offset_num_str = (char*)malloc(2);
    int offset_num = 0;
    char *frame_num_str = (char*)malloc(2);
    int frame_num = -1;
    char *p_addr_str = (char*)malloc(4);
    signed char buffer[page_size];

    //get all the command line arguments
    if(argc != 7){
        printf("ERROR: Incorrect Argument Format!\n");
        exit(1);
    }else{
        //identify which mode to use
        strcpy(mode, argv[6]);
        if(!(strcmp(mode, "DEMAND") == 0 || strcmp(mode, "FIFO") == 0 || strcmp(mode, "LRU") == 0)){
            printf("ERROR: Incorrect Mode!\n");
            exit(1);
        }
        strcpy(readFile, argv[4]);
        strcpy(outFile, argv[2]);
    }

    //open swapfile
    FILE *oFile;
    oFile = fopen(outFile, "rb");

    //open address file
    FILE *pFile;
    pFile = fopen(readFile, "r");

    //return error if oFile contains nothing
    if(oFile == NULL){
        printf("ERROR: swap file contains NOTHING!\n");
        exit(1);
    }

    if (fseek(oFile, 65536, SEEK_SET) != 0){
        printf("ERROR: unable to seek the swap file!\n");
        exit(1);
    } 

    ofstream wFile;
    wFile.open("output.txt");

    wFile << "MODE SELECTED: " << mode << "\n";

    //return error if pFile contains nothing
    if(pFile == NULL){
        printf("ERROR: input file contains NOTHING!\n");
        exit(1);
    }

    //lenth variable for getline()
    size_t len = 0;
    ssize_t read;
    int k = 0;
    int tlb_cursor = 0;
    int fifo_cursor = 0;

    //declare a new strct p1
    struct pg p1;
    struct pg p2;

    //translate memory addresses in different modes
    while((read = getline(&line, &len, pFile)) != -1){
        int i;
        //set the physical address string back to empty
        strcpy(p_addr_str, "");
        frame_num = -1;
        //get the decimal number from the file and convert it into hex
        sprintf(v_addr, "%04X", atoi(line));
        //save the first 2 digits in to page_num_str
        for(i = 0; i < 2; i++){
            page_num_str[i] = v_addr[i];
        }
        //convert the page number back to decimal
        page_num = strtol(page_num_str, NULL, 16);
        //save the last 2 digits in to page_num_str
        for(i = 0; i < 2; i++){
            offset_num_str[i] = v_addr[i + 2];
        }
        //convert the offset number back to decimal
        offset_num = strtol(offset_num_str, NULL, 16);

        for(i = 0; i < tlb_size; i++){
            if(tlb_table[i].tag == page_num){
                tlb_hits++;
                frame_num = tlb_table[i].page;
                break;
            }
        }
        //if it's a TLB hit
        if(frame_num != -1){
            sprintf(frame_num_str, "%02X", frame_num);
        }else{
            //then we are going to break the code into 3 different modes
            if(strcmp(mode, "LRU") == 0){
                int l;
                if(lru_pg_table.size() == 0){
                    //set the variables
                    p1.p_num = page_num;
                    p1.f_num = page_fault;
                    sprintf(frame_num_str, "%02X", page_fault);
                    //push it to the beginning of the vector
                    lru_pg_table.insert(lru_pg_table.begin(), p1);
                    //update page fault
                    page_fault++;
                    fifo_cursor++;
                }else{
                    for(l = 0; l < lru_pg_table.size(); l++){
                        //if page number found
                        if(lru_pg_table[l].p_num == page_num){
                            //get the frame number
                            frame_num = lru_pg_table[l].f_num;
                            sprintf(frame_num_str, "%02X", lru_pg_table[l].f_num);
                            p2.p_num = lru_pg_table[l].p_num;
                            p2.f_num = lru_pg_table[l].f_num;
                            //remove this element from the vector
                            lru_pg_table.erase(lru_pg_table.begin() + l);
                            //add it to the end of the vector again
                            lru_pg_table.insert(lru_pg_table.begin(), p2);
                            break;
                        }
                    }
                    //if page number not found
                    if(frame_num == -1){
                        //delete the last element of the group
                        if(lru_pg_table.size() == 128){
                            p1.f_num = lru_pg_table[127].f_num;
                            lru_pg_table.pop_back();
                        }else{
                            p1.f_num = page_fault;
                        }
                        //set the variables
                        p1.p_num = page_num;
                        frame_num = fifo_cursor;
                        sprintf(frame_num_str, "%02X", fifo_cursor);
                        //push it to the end of the vector
                        lru_pg_table.insert(lru_pg_table.begin(), p1);
                        //update page fault
                        page_fault++;
                        //update cursor
                        if(fifo_cursor == 127){
                            fifo_cursor = 0;
                        }else{
                            fifo_cursor++;
                        }
                    }
                }
            }
            //for fifo mode
            else if(strcmp(mode, "FIFO") == 0){
                int k;
                for(k = 0; k < 128; k++){
                    //if page number found
                    if(page_table[k].p_num == page_num){
                        //get the frame number
                        frame_num = page_table[k].f_num;
                        sprintf(frame_num_str, "%02X", page_table[k].f_num);
                        break;
                    }
                }
                //if page number not found
                if(frame_num == -1){
                    //update the page table
                    page_table[fifo_cursor].p_num = page_num;
                    page_table[fifo_cursor].f_num = fifo_cursor;
                    sprintf(frame_num_str, "%02X", page_table[fifo_cursor].f_num);
                    //update the page fault counter
                    page_fault++;
                    //update fifo_cursor
                    if(fifo_cursor == 127){
                        fifo_cursor = 0;
                    }else{
                        fifo_cursor++;
                    }
                }
            }
            //for demand mode
            else{
                //look for page fault
                if(page_table[page_num].f_num == -1){
                    //if the page table element is empty, directly update the frame number
                    page_table[page_num].f_num = page_fault;
                    page_fault++;
                }
                //get the frame number from the page table
                sprintf(frame_num_str, "%02X", page_table[page_num].f_num);
            }
            frame_num = strtol(frame_num_str, NULL, 16);
            //update tlb
            tlb_table[tlb_cursor].tag = page_num;
            tlb_table[tlb_cursor].page = frame_num;
        }
        strcat(p_addr_str, frame_num_str);
        strcat(p_addr_str, offset_num_str);
        p_addr = strtol(p_addr_str, NULL ,16);
        fread(buffer, sizeof(buffer), page_size, oFile);
        value = buffer[offset_num];

        sprintf(output, "Virtual Address: %d, Physical Address: %d, Value: %d", atoi(line), p_addr, value);

        wFile << output << "\n";
        //update tlb_cursor
        if(tlb_cursor == 15){
            tlb_cursor = 0;
        }else{
            tlb_cursor++;
        }
    }

    tlb_hits -= 2;
    page_fault_rate = (double)page_fault / (double)total * 100;
    tlb_hits_rate = (double)tlb_hits / (double)total * 100;

    cout << "MODE SELECTED: " << mode << endl;
    cout << "Number of translated addresses: " << total << endl;
    cout << "Page Faults: " << page_fault << endl;
    cout << "Page Fault Rate: " << page_fault_rate << "%" << endl;
    cout << "TLB Hits: " << tlb_hits << endl;
    cout << "TLB Hit Rate: " << tlb_hits_rate << "%" << endl;
    cout << "For exact translated physical addresses, please check \'output.txt\'." << endl;

    //free allocated variables
    free(page_num_str);
    free(offset_num_str);
    free(frame_num_str);
    free(p_addr_str);

    //close files
    fclose(pFile);
    wFile.close();
    fclose(oFile);

    return 0;
}