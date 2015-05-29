#include<stdio.h>
#include <stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include <sys/wait.h>
#include<unistd.h>
#include<errno.h>


/**********************************FUNCTION DEFINITIONS*******************************************/

//makes the fork call and handles instruction execution
int processor();
//initialises the CPU registers and other configuration values
int initCPU();
//Prints the message needed for errorCode and exits the program.
void exitParent(char *errorMessage);
//Loads the program from the txt file into an array
int loadProgramFromFile(int* memory, int length);
//This function sends the address on the pipe and retreives the corresponding data from the memory process.
int readDataFromMemory(int address);
//This function sends data on the pipe which is written by the memory into the process.   
void writeDataIntoMemory(int address, int data);



//The file descriptors of the pipes to be used for communication.
int toMemory[2], toProcessor[2];

//The object sent over the pipe for communication between processes
struct data
{
    char controlCharacter;
    int data;
    int address;
};


int main()
{
    int result;
    processor();
    return result;
}


int processor()
{
    pid_t pid;
    int result;
    //Creating the two pipes: toMemory and toProcessor.
    result = pipe(toMemory);
	if (result == -1)
		exit(1);
    result = pipe(toProcessor);
	if (result == -1)     
		exit(1);
    //Creating a child process
    pid = fork();
    
    switch(pid)
    {
        case -1:
        printf("The fork call was not completed. Check system configuration\n");
        exit(1);
        break;
        
                
        /*******************************************THIS IS THE MEMORY CODE**********************************/      
        case 0:
        result = -1;
        struct data obj;
        int memory[1000];
        //Load the program from the file
        result = loadProgramFromFile(memory, 1000);
        if (result == -1){     
            printf("\nError while reading the program from the file. Exiting Program.\n");
            fflush(stdout);
            exit(1);
        }
        read(toMemory[0], &obj , sizeof(obj));
		
        while(obj.controlCharacter!=' ')
        {
            switch(obj.controlCharacter)
            {
                //Read Operation. Data is written into data bus and sent to the parent/processor
                case 'r':
                obj.data = memory[obj.address];
                //to write the data into Data Bus
                write(toProcessor[1], &obj, sizeof(obj));                
                break;
                
                //Write Operation. Data from the data bus is written into the address from the Data Bus.
                case 'w':
                memory[obj.address] = obj.data;
                break;
                
                //Exit signal is sent to child process/memory. 
                case 'e':
                _exit(0);
                break;
                
                //Control Signal could not be identified. Erroneous control signal.
                default:
                printf("\n\nControl Signal could not be indentified\n\n");
                fflush(stdout);
                break;
            }
            read(toMemory[0], &obj, sizeof(obj));
        }
        _exit(0);
        break;
        
              
        /**********************************THIS IS THE PROCESSOR CODE******************************/
        default:
        
		result = -1;
        //Registers
		int PC, IR, SP, AC, X, Y, TEMP;
        char eop = '0';
        
        //Initialise the CPU registers
        initCPU(&PC,&SP,&IR,&AC,&X,&Y,&TEMP);
                
        while(eop == '0')
        {
            //Get the Instruction from the memory process by pipe and store it in IR
            IR = readDataFromMemory(PC);
            PC++;
            
                //printf("\nAC=%d ", AC);
                //printf("PC=%d ", PC-1);
            	//printf("IR=%d ", IR);
                //printf("SP=%d ", SP);
            	//printf("X=%d ", X);
                //printf("Y=%d\n", Y);
            
            
            //Code that will interpret instructions and execute them
            switch(IR)
            {        
                case 0:
                //No Operation instruction
                break;
                
                case 1:
                AC = readDataFromMemory(PC);
                PC++;
                break;
                
                case 2:
                TEMP = readDataFromMemory(PC);
                PC++;
                AC = readDataFromMemory(TEMP);
                break;
                
                case 3:
                TEMP = readDataFromMemory(PC);
                PC++;
                writeDataIntoMemory(TEMP,AC);                
                break;
                
                case 4:
                AC = AC + X;
                break;
                
                case 5:
                AC = AC + Y;
                break;
                
                case 6:
                AC = AC - X;
                break;
                
                case 7:
                AC = AC - Y;
                break;
                
                case 8:
                TEMP = readDataFromMemory(PC);
                PC++;
                fflush(stdout);
                char c;
                switch(TEMP)
                {
                    case 1:
                    scanf("%d",&AC);
                    break;
                    case 2:
                    scanf("%c",&c);
                    AC = c;
                    break;
                    default:
                    exitParent("Invalid operand for Get Port instruction");
                }
				break;
                
				//Put port instruction
                case 9:
                TEMP = readDataFromMemory(PC);
                PC++;
                switch(TEMP)
                {
                    case 1:
                    printf("%d",AC);
                    break;
                    
                    case 2:
                    printf("%c",AC);
                    break;
                    
                    default:
                    exitParent("Invalid operand for Put Port instruction.");
                }
                fflush(stdout);
                break;
                
                case 10:
                X=AC;
                break;
                
                case 11:
                Y=AC;
                break;
                
                case 12:
                AC=X;
                break;
                
                case 13:
                AC=Y;
                break;

                //Jump
                case 14:
                PC = readDataFromMemory(PC);
                break;
                
				//Jump if AC=0
                case 15:
                if(AC == 0)
                	PC = readDataFromMemory(PC);
                else
                    PC++;
                break;
                
				//Jump if AC!=0
                case 16:
                if(AC != 0)
                	PC = readDataFromMemory(PC);
                else
                    PC++;
                break;

                //Call subroutine
                case 17:
                TEMP=readDataFromMemory(PC);
                PC++;
                writeDataIntoMemory(SP,PC);
                PC = TEMP;
                SP = SP - 1;
                break;
                
				//Return
                case 18:
                SP = SP + 1;
                PC=readDataFromMemory(SP);
                break;
                
				//Increment X
                case 19:
                X = X + 1;
                break;
                //Decrement X
                case 20:
                X = X - 1;
                break;
                
                case 30:
                exitParent("");
                break;
                
                default:
                fflush(stdout);
                exitParent("Invalid Instruction encountered");
            }
        }
    }
    
    return 0;
}


void exitParent(char *errorMessage)
{	
    //Prints the error message
    printf("\n\%s\n", errorMessage);
    fflush(stdout);
    
    //shuts down the child process
    struct data obj;
    obj.controlCharacter = 'e';
    write(toMemory[1], &obj, sizeof(obj));
    waitpid(-1, NULL, 0);
    
    //Close the pipes.
    int i;
    for(i=0;i<2;i++)
    {
        close(toMemory[i]);
        close(toProcessor[i]);
    }
    //Shuts down the parent process
    _exit(0);
}


int initCPU(int *PC,int *SP,int *IR,int *AC,int *X,int *Y,int *TEMP)
{
    *PC=0;
    *SP=999;
    *IR=0;
    *AC=0;
    *X=0;
    *Y=0;
    *TEMP=0;
	return 0;
}


int readDataFromMemory(int address)
{
    struct data obj;
    obj.controlCharacter = 'r';
    obj.address = address;
    
    write(toMemory[1], &obj, sizeof(obj));
    read(toProcessor[0], &obj, sizeof(obj));
	return obj.data;
}


void writeDataIntoMemory(int address, int data)
{
    struct data obj;
    obj.controlCharacter = 'w';
    obj.address = address;
    obj.data = data;
    write(toMemory[1], &obj, sizeof(obj));
}


int loadProgramFromFile(int* memory, int length)
{
    FILE *fr;
    int n;
    char line[5];
    fr = fopen ("program.txt", "rt");
    if(fr==NULL)
        return -1;
    int i=0;
    while(fgets(line, 5, fr) != NULL)
    {
        //This lines converts the string into an int and stores it in memory
        sscanf (line, "%d", memory);
        memory++;
    }
    fclose(fr);
    return 0;
}
