/* debugger.cpp */

//Jonathan Kong

//Implements the Debugger class, including the constructor, destructor, and the run function

//Functionality:
// -Breakpoints: Setting, removing, and clearing (remove all) breakpoints
// -Run: Running the program either until completion or until a breakpoint is hit
// -Step: Stepping through statements one line at a time 
// -Extra: Provides commands to show commands, list breakpoints, show memory, print variables, show next execution line, and show program state 


#include <iostream>

#include "debugger.h"

using namespace std;

Debugger::Debugger(struct STMT* program) 
  : state("Loaded"), head(program), currentStmt(program), memory(ram_init()), second_time_breakpoint(false) //initialize data members 
{   
    //Responsible for: init next to next node of head, filling up the lines set with programgraph lines, and breaking connection between head and next node
    next = getNextNode(head); 

    while (head!=nullptr) {
        lines.insert(head->line); 
        head = next; 
        next = getNextNode(next);
    } //fill up lines set

    head = currentStmt; //reset head
    next = getNextNode(head); //reset next (next node of head)
    breakGraph(head); 
}

Debugger::~Debugger() //Called automatically when debugger object goes out of scope (end of main() function)
{
    ram_destroy(memory);  //Frees the RAM memory (programgraph cleared in main.cpp)
}

void Debugger::run()
{

  //Gather cmd from user input 
  string cmd;  
  while (true) {
    //List out all of the commands to the user 
    cout << endl; 
    cout << "Enter a command, type h for help. Type r to run. > " <<endl; 
    cin >> cmd; 

    if (cmd=="h") {
      cout << "Available commands:"<<endl;
      cout << "r -> Run the program / continue from a breakpoint"<<endl; 
      cout << "s -> Step to next stmt by executing current stmt" <<endl; 
      cout << "b n -> Breakpoint at line n"<<endl; 
      cout << "rb n -> Remove breakpoint at line n"<< endl; 
      cout << "lb -> List all breakpoints" << endl; 
      cout << "cb -> Clear all breakpoints" <<endl; 
      cout << "p varname -> Print variable"<<endl; 
      cout << "sm -> Show memory contents" <<endl; 
      cout << "ss -> Show state of debugger"<<endl; 
      cout << "w -> What line are we on?"<<endl; 
      cout << "q -> Quit the debugger"<<endl; 
    }

    else if (cmd=="q") {
      //return -> breaks out of the entire user input loop 
      repairGraph(currentStmt, next); 
      break; 
    }

    else if (cmd=="ss") {
      //State is kept in "state" data member, print that out 
      cout << state <<endl; 
    }

    else if (cmd=="sm") {
      //memory is init using ram_init() in the constructor, here we just need to pass it to ram_print()
      ram_print(memory); 
    }

    else if (cmd=="p") {
      //Get the varname (second user input)
      string varname_str; 
      cin>>varname_str; 
      const char* varname = varname_str.c_str(); 

      //Call ram_read_cell_by_name
      struct RAM_VALUE* cell = ram_read_cell_by_name(memory, (char*)varname); 

      //Print "varname (type): value" according to ram type, handle case where cell==NULL (no such variable) 
      if (cell==NULL) {
        cout << "no such variable" <<endl; 
      } else {
        int value_type = cell->value_type; 
        cout << varname << " ("; 
        if (value_type==RAM_TYPE_REAL) {
          cout << "real): " << cell->types.d << " " <<endl; 
        } else if (value_type==RAM_TYPE_STR) {
          cout << "str): " << cell->types.s << " " <<endl; 
        } else if (value_type == RAM_TYPE_INT) {
          cout << "int): " << cell->types.i << " " <<endl; 
        } else if (value_type == RAM_TYPE_PTR) {
          cout << "ptr): " << cell->types.i << " " <<endl; 
        } else if (value_type == RAM_TYPE_BOOLEAN) {
          cout << "bool): " << cell->types.i << " " <<endl; 
        } else {
          cout << "none): " << "null" << " " <<endl; 
        }
        ram_free_value(cell); //Free the RAM_VALUE* 
      }
    }

    else if (cmd == "r") {
        //Run command: perform step operation UNTIL currentStmt is either null or a breakpoint is reached
        if (state=="Completed") {
            cout << "program has completed" <<endl; 
            continue; 
        }
        int start = currentStmt->line; 
        step(); //Always begin with a step (moves currentStmt to the next of last, lets below loop run)
        while (currentStmt!=nullptr && breakpoints.find(currentStmt->line)==breakpoints.end()) {
            step(); 
            if (state=="Completed") {
                break; 
            }
        }
        //Note: If we were stopped by a breakpoint, we have to still perform step on that breakpoint line (this is the "first time breakpoint reached" case)
        //This will print out that a breakpoint was hit on {line} and then change the second_time_breakpoint to true so that the next execution actually
        //runs that breakpoint line
        if (currentStmt==nullptr) {
            state="Completed"; 
            continue; 
        }
        if (breakpoints.find(currentStmt->line)!=breakpoints.end() && (currentStmt->line!=start)) {
            step(); //Perform one more step to load state into second time hitting breakpoint 
        }
    } 

    else if (cmd=="s") {
        //Step command: call helper function step() defined below
        if (state=="Completed") {
            cout << "program has completed" <<endl; 
        }
        step(); 
    }
        
    else if (cmd=="b") {
        int n; 
        cin >> n; 
        //Case: line isn't found in the programgraph (utilize lines set)
        if (lines.find(n)==lines.end()) {
            cout << "no such line" <<endl; 
            continue; 
        }
        //Case: breakpoint already exists
        if (breakpoints.find(n)!=breakpoints.end()) {
            cout << "breakpoint already set" <<endl; 
            continue; 
        }

        //Breakpoint management revolves around inserting and deleting from breakpoints set
        breakpoints.insert(n); 
        cout << "breakpoint set" << endl;
    } 
    
    else if (cmd == "rb") {
        int n;
        cin >> n;
        //Removing means to just remove from the breakpoint set
        if (breakpoints.find(n)!=breakpoints.end()) {
            breakpoints.erase(n); 
            cout << "breakpoint removed" << endl; 
        } else {
            cout << "no such breakpoint" << endl; 
        } //Case: no such breakpoint
    }
    else if (cmd == "cb") {
        // Clear the breakpoints set 
        breakpoints.clear(); 
        cout << "breakpoints cleared" << endl;
    }

    else if (cmd == "lb") {
        //breakpoints set already has the line numbers in sorted order, so we take advantage of that 
        if (breakpoints.empty()) {
            cout << "no breakpoints" <<endl; 
        } else {
            //loop through breakpoints (using iterator which is also native to sets) and print line numbers 
            cout << "breakpoints on lines: "; 
            for (auto iterator = breakpoints.begin(); iterator != breakpoints.end(); ++iterator) { //++ operator for iterator to move one to right
                cout << *iterator << " "; 
            }
            cout << endl; 
        }
    }

    else if (cmd == "w") {
        if (state == "Completed") {
            cout << "completed execution" << endl;
        } 
        else if (state == "Loaded") {
            cout << "line "<<head->line << endl;
            programgraph_print(head); //This will always be the head line (head always ref first node, unchanged)
        } 
        //The line that's going to run next is the line that's at our currentStmt right now 
        else if (state == "Running") {
            cout << "line " << currentStmt->line << endl;
            programgraph_print(currentStmt); 
        }
    }
    else { //Case: unknown user command 
        cout << "unknown command" <<endl; 
    }
  }
}


void Debugger::step() {
    if (state=="Loaded") {
        state="Running"; 
    } //state management 

    if (breakpoints.find(currentStmt->line)!=breakpoints.end()) { //CURRENT STATEMENT IS A BREAKPOINT
        if (!second_time_breakpoint) { //FIRST TIME HITTING BREAKPOINT
            cout << "hit breakpoint at line " << currentStmt->line << endl; 
            programgraph_print(currentStmt); 
            second_time_breakpoint = true; //flip the breakpoint status flag
        } else { //SECOND TIME HITTING BREAKPOINT, EXECUTE ONE LINE
            second_time_breakpoint = false; //flip the breakpoint status flag
            executeOneLine(); 
        }
    } else { //NOT A BREAKPOINT, EXECUTE ONE LINE
        executeOneLine(); 
    }
}

void Debugger::executeOneLine() {
    ExecuteResult result = execute(currentStmt, memory); 
    //currentStmt and next is severed right now, so execute will only execute currentStmt as desired

    //Handle consequences: repairGraph and then move current to next and next to its next and then break graph from that state (each prepares next step command)
    if (result.Success==false) {
        state="Completed"; 
        repairGraph(currentStmt, next); 
    } 
    repairGraph(currentStmt, next); 
    currentStmt = next; 
    next = getNextNode(next); 
    if (currentStmt == nullptr) {
        state="Completed"; 
    } else {
        breakGraph(currentStmt); 
    }
}


void Debugger::breakGraph(STMT* node) {
    if (node!=nullptr) {
        if (node->stmt_type == STMT_ASSIGNMENT) {
            node->types.assignment->next_stmt = nullptr; 
        } else if (node->stmt_type == STMT_FUNCTION_CALL) {
            node->types.function_call->next_stmt = nullptr; 
        } else if (node->stmt_type == STMT_PASS) {
            node->types.pass->next_stmt = nullptr; 
        }
    } //Helper function: break graph at node -> node's next
}

void Debugger::repairGraph(STMT* node, STMT* next) {
    if (node != nullptr) {  // Ensure targetStmt is valid
        if (node->stmt_type == STMT_ASSIGNMENT) {
            node->types.assignment->next_stmt = next;
        } else if (node->stmt_type == STMT_FUNCTION_CALL) {
            node->types.function_call->next_stmt = next;
        } else if (node->stmt_type == STMT_PASS) {
            node->types.pass->next_stmt = next;
        }
    } //Helper function: set node's next to input next node
}

STMT* Debugger::getNextNode(STMT* node) {
    STMT* next = nullptr; 
    if (node != nullptr) {  // Ensure targetStmt is valid
        if (node->stmt_type == STMT_ASSIGNMENT) {
            next = node->types.assignment->next_stmt; 
        } else if (node->stmt_type == STMT_FUNCTION_CALL) {
            next= node->types.function_call->next_stmt; 
        } else if (node->stmt_type == STMT_PASS) {
            next= node->types.pass->next_stmt; 
        }
    } 
    return next; //Helper function: get input node's next node (may be nullptr!)
}


