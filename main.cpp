/*
Written by: Alexander Yue
First Assignment
Cougarnet: AZYue
PID: 2079436
COSC 3360
Instructor: Professor Paris
No special measures are necessary to run and compile, the code uses input redirection as directed by the assignment
*/



#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <list>

using namespace std;


struct Operation { //struct used to store data for the inputtable
    string action;
    int value;
};
struct slot{ // struct used to represent the process table
    int PID; //process ID
    int state = 0; //utilizes array processStates to tell the state of the process
};
struct Processes { //struct used to store data for the priority queue
    int PID; //Process ID
    double TIME = 0; //Process time
    string PROC; //operation
    int LogReads; //nummber of logical reads
    int PhyWrites; //number of physical writes
    int PhyReads; //number of physical reads
    int Buffer = 0; //I/O buffer size
    bool BEmpty = true; //flag for empty buffer
    int CPUwait = 0; //cpu wait time in ready queue
    
    Processes(int pid, long time, const string& proc, int logReads, int phyWrites, int phyReads, int buffer)
    : PID(pid), TIME(time), PROC(proc), LogReads(logReads), PhyWrites(phyWrites), PhyReads(phyReads), Buffer(buffer) {} //constructor


    // Overloading the less than operator to decine priority order
    bool operator<(const Processes& other) const {
        // Higher TIME means lower priority
        return TIME > other.TIME;
    }
};


vector<list<Operation>> InputTable;
vector<slot> ProcessTable;
priority_queue<Processes> MainQ;
queue<Processes> CPUQ, SSDQ;
const string processStates[] = {"NULL", "READY", "RUNNING", "BLOCKED", "TERMINATED"}; 
bool CPUEmpty = true; //bool to tell if CPU is busy
bool SSDEmpty = true; // boool to tell if SSD is busy
int ProcessCNT = 0; // counts how many processes
int BSIZE; //global BSIZE integer
string line; // string to be read from input file
int VAL; // int value read corresponding to the process tring
double SYSTime = 0;

void ArrivalFunction(Processes process);//function that handles arrival events
void CoreComp(Processes process);//funciton that handles Core-associated events
int PIDT(Processes process); // function that correctly adjusts the PID number
void CoreReq(Processes process); //function that handles core requests
void SSDReq(Processes process); //function that hands SSD requests
void SSDComp(Processes process); //function that handles SSD Completion



void outputMainQueue(priority_queue<Processes> mainQueue) { // Function used during testing to output contents of mainq
    priority_queue<Processes> tempQueue = mainQueue; // Create a copy of the main queue
    while (!tempQueue.empty()) {
        Processes process = tempQueue.top(); // Get the top element
      //cout << "PID: " << process.PID << ", TIME: " << fixed << setprecision(1) << process.TIME << ", PROC: " << process.PROC << endl;
        tempQueue.pop(); // Remove the top element
    }
   //cout<<endl;
}

int PIDT(Processes process) // Correctly returns the PID
{
    return process.PID-1;
}

int main() {
    ifstream cin("input12.txt");
    cin>>line>>VAL;
    BSIZE = VAL;
    slot Proc;
    while (cin >> line >> VAL) { // while loop that reads file into input table and process table
        Operation op;
        op.action = line; 
        op.value = VAL;
        if (line == "START") {
            ProcessCNT++;//increments plus one because new process
            Proc.PID = ProcessCNT;
            ProcessTable.push_back(Proc); //pushes process info to the back of ProcessTable
            InputTable.push_back(list<Operation>()); // adds a new list to the back of the input table
            InputTable.back().push_back(op); // Save the start node
        } 
        else {
            InputTable.back().push_back(op); //adds the operation and time to the input table
        }
    }


    for(int i = 0; i<ProcessTable.size();i++) // For loop that pushes data from ProcessTable into the MainQ. The data from Struct "slot" gets transferred to a new struct called "Processes"
    {
        MainQ.push(Processes(ProcessTable[i].PID, InputTable[i].front().value, "", 0, 0, 0, BSIZE)); // pushes data into main loop
    }
    while (!MainQ.empty()) { //Main while loop
        Processes process = MainQ.top();
        //outputMainQueue(MainQ); unused function to output contents of the main q
        MainQ.pop();
        SYSTime = process.TIME; //updates the system time
        string OPERATION = InputTable[PIDT(process)].front().action; // gets Operation from Inputatble
        process.PROC = OPERATION; //sets Processes operation to operation at the front of the list in input table
        if (OPERATION == "START") //If Arrival
        {
            ArrivalFunction(process);
        }
        else if (OPERATION == "CORE")//Core associated processes
        {
            CoreComp(process);
        }
        else if (OPERATION=="WRITE"|| OPERATION == "READ")
        {
            SSDComp(process);
        }
       else if(process.PROC == "INPUT" || process.PROC == "DISPLAY")
        {
            process.TIME += InputTable[PIDT(process)].front().value;
            InputTable[PIDT(process)].pop_front();
            CoreReq(process);
        } 
    }
    ProcessTable.clear();
    InputTable.clear(); //clears the vectors to avoid memory issues
    return 0;
}

void ArrivalFunction(Processes process) //function that handles arrivals (start)
{
    InputTable[PIDT(process)].pop_front(); // removes the head of the process list  
    CoreReq(process);
}

void CoreComp(Processes process) // core completion function
{
    CPUEmpty = true;
    if (!CPUQ.empty()) //checks if CPU que is empty, if not, runs the head of the q in the request function
    {
        Processes top = CPUQ.front();
        CPUQ.pop();
        CoreReq(top);
    }
    if (ProcessCNT==1) // if statement to set the process time
    {
        SYSTime += InputTable[PIDT(process)].front().value;
        process.TIME = SYSTime;
    }
    InputTable[PIDT(process)].pop_front(); //removes completed process from input table
    if (InputTable[PIDT(process)].empty()) //if input table is empty, i.e if all inputs have been executed
    {
      ProcessTable[PIDT(process)].state = 4; //sets state of the process to termianted
      cout<<endl;
      cout<< "Process "<< PIDT(process) << " terminates at time " << fixed << setprecision(1) <<process.TIME<< " ms."<<endl;
      cout<< "it performed " << process.PhyReads<<" physical read(s), "<< process.LogReads<<" in-memory read(s), and " << process.PhyWrites<<" physical write(s)."<<endl;
      cout<<"Process Table:"<<endl;
        for(int i = 0; i<ProcessTable.size(); i++) // for loop to output process table
        {
            if (ProcessTable[i].state!=4 || i == PIDT(process))
            {
          cout <<"Process " << i << " is " << processStates[ProcessTable[i].state] <<"."<<endl;
            }
        }
    }
    else
    {
        process.PROC = InputTable[PIDT(process)].front().action; //if Input table is not empty, set process to the next input
    }
    if(process.PROC == "READ" || process.PROC == "WRITE")
    {
        SSDReq(process);
    }
    if(process.PROC == "INPUT" || process.PROC == "DISPLAY")
    {
        MainQ.push(process);
    }
}


void CoreReq(Processes process) //Function to handle core requests
{
    if (CPUEmpty)
    {
    CPUEmpty = false;
    ProcessTable[PIDT(process)].state = 2; // sets process state to running
    process.PROC = "CORE"; // sets process operation to core
        if (ProcessCNT>1)    // if statement to set the process time
        {
            SYSTime += InputTable[PIDT(process)].front().value;
            process.TIME = SYSTime;
        }
    MainQ.push(process);
    }
    else
    { 
        ProcessTable[process.PID].state = 1;
        process.PROC = "CORE";
        process.CPUwait = InputTable[PIDT(process)].front().value;
        CPUQ.push(process);
    }
}

void SSDComp(Processes process)
{
    SSDEmpty = true;
    if (!SSDQ.empty()) // checks if SSD queue is empty
    {
        Processes top = SSDQ.front();
        SSDQ.pop();
        SSDReq(top); //if not empty, runs top element of queue through request function
    }
    else
    {
        InputTable[PIDT(process)].pop_front(); //removes completed process from input table
        process.PROC = InputTable[PIDT(process)].front().action; //sets next operation
        CoreReq(process); //core request because core immediately follows SSD operations
    }
}
void SSDReq(Processes process)
{
    int request = InputTable[PIDT(process)].front().value;//gets request size from input table
    if (process.PROC == "WRITE")
    {
        ProcessTable[PIDT(process)].state = 2; // sets state to running
        process.TIME+=.1; //increments the processes time by .1
        process.PhyWrites+=1; //increments the total Physical Writes
        if (SSDEmpty)//checks if SSD is empty
        {
            SSDEmpty = false; //set SSD to full
            MainQ.push(process);
        }
        else
        {
            SSDQ.push(process);
        }
    }
    else if (process.BEmpty)//before starting a read operation, checks if the buffer is empty or not
    {
        process.TIME+=.1;
        process.PhyReads+=1;
        process.Buffer = BSIZE-request; //sets the buffer size
        process.BEmpty = false;
        if (SSDEmpty)
        {
            SSDEmpty = false;
            MainQ.push(process);
        }
        else
        {
            SSDQ.push(process);
        }
    }
    else if (request>process.Buffer) //physical read
    {
        process.TIME+=.1;
        process.PhyReads+=1;
        process.Buffer = BSIZE+process.Buffer-request;
        process.BEmpty = false;
        if (SSDEmpty)
        {
            SSDEmpty = false;
            MainQ.push(process);
        }
        else
        {
            SSDQ.push(process);
        }
    }
    else // logical read
    {
        process.LogReads+=1;
        process.Buffer = process.Buffer - request;
        MainQ.push(process);
    }
}


    // unused functions for Displaying the contents of the vector of lists
    /*
    for (size_t i = 0; i < InputTable.size(); ++i) {
       //cout << "List " << i << ":" << endl;
        for (const auto& op : InputTable[i]) {
           //cout << op.action << " " << op.value << endl;
        }
       //cout << endl;
    }
        // Iterate over each ProcessSlot in the current list
        for (const auto& slot : ProcessTable) {
           //cout << "PID: " << slot.PID << endl;
           //cout << "Start Line: " << slot.start_line << endl;
           //cout << "End Line: " << slot.end_line << endl;
           //cout << "Current Line: " << slot.current_line << endl;
           //cout << "State: " << slot.state << endl;
           //cout << "TOTAL: " << slot.TOTAL << endl;
           //cout << endl;
        }

     while (!MainQ.empty()) {
        const Processes& process = MainQ.top();
       //cout << "PID: " << process.PID << ", TIME: " << process.TIME << ", PROC: " << process.PROC << endl;
        MainQ.pop();
    }




*/
