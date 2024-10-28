/* debugger.cpp */

//Jonathan Kong

//Implements the Debugger class, including the constructor, destructor, and the run function

//Functionality:
// - Responsible for managing the execution of a nuPython program. Includes:
//   - Setting, removing, and clearing breakpoints
//   - Running the program either until completion or until a breakpoint is hit
//   - Providing commands to list breakpoints, show memory, and print variables
//   - Managing the program state (Loaded, Running, Completed) and navigating the program graph

//Log for self: 
//Time spent: 32 hours (Saturday-Sunday)

#include <iostream>

#include "debugger.h"

using namespace std;

Debugger::Debugger(struct STMT* program) //Constructor: init state to "Loaded" and the currentStmt and head nodes to program (first node) and memory as ram_init, map and set empty on init
  : state("Loaded"), currentStmt(program), head(program), memory(ram_init()) 
{
}

Debugger::~Debugger() //Called automatically when debugger object goes out of scope (end of main() function)
{
    //clearAllBreakpoints();  //Call helper function to get all programgraph connections back to original so that programgraph_destory can clear all ndoes

    // programgraph_destroy(head);  // Frees the program graph memory (do NOT call again in main.cpp)

    ram_destroy(memory);  //Frees the RAM memory

    //Note, I "clear breakpoints" right after execute so I don't need to repair programgraph here, also programgraph_destroy called in main, NO need
    //to do so here, thus only need to call ram_destroy 
}

void Debugger::run()
{

  //Gather cmd from user input 
  string cmd;  
  while (true) {
    //List out all of the commands to the user 
    cout << "Enter a command, type h for help. Type r to run. > "; 
    cin >> cmd; 

    if (cmd=="h") {
      cout << "Available commands:"<<endl;
      cout << "r -> Run the program / continue from a breakpoint"<<endl; 
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
      if (breakpoints.size()>0) {
        clearAllBreakpoints(); 
      }
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
        //Did most of the work already in setting breakpoints. Here, we are dealing with execute as well as the results of execute
        //We need a reference to a CURRENT statement as that's execute parameter (starting point). Let execute run until completion 
        //(stops automatically when it has no next_stmt to process, which represnts a breakpoint hence why prev -> current connections were severed)
        //After execute runs, we have access to the last statement that was ran. Using MapNodes that was also filled by setting breakpoints, we 
        //can move current statement to the value of MapNodes[last statement]. 

        //State is completed: no need to go further
        if (state == "Completed") {
            cout << "program has completed" << endl;
            continue;
        }

        //Otherwise, perform execute
        state = "Running"; 
        ExecuteResult result = execute(currentStmt, memory); 

        if (result.Success==false) {
            state="Completed"; 
            clearAllBreakpoints(); //SUPER SNEAKY edge case, in program can "completed" in this case where a semantic error is caused, causing the breakpoint
                                   //repair in the next if case to NOT run -> causes memory leak, thus in this if loop, repair immediately using clearAllBreakpoints
            continue; 
        }

        //Get last executed statement , 1. succesfully completed execution, 2. stopped execution early due to program error, 3. stopped at breakpoint
        STMT* lastStmt = result.LastStmt; 


        //If lastStmt found in map, we've hit a breakpoint, move currenStmt to next using map 
        if (MapNodes.find(lastStmt)!=MapNodes.end()) { 
            //cout << "Hit a breakpoint at line #" <<MapNodes.find(lastStmt)->second->line <<endl; 
            auto iterator = MapNodes.find(lastStmt); 
            currentStmt = iterator -> second; 
            bool f = false; 
            removeBreakpointAuto(iterator->second->line); 
        } else {
            state="Completed"; 
            continue; 
        } //Otherwise, we're done 
    } 
        
    else if (cmd=="b") {
        //Keep a prev and current pointer, we want to iterate until we 
        //find the line number n (current) and then SEVER the connection 
        //between prev and current, to keep track of these broken connections
        //so that we can restore it and move on to the next breakpoint after 
        //an execute, store these connections in a map
        // stmt : stmt

        //In all, two main operations: 
        //1. Sever node connection 
        //2. Store that connection in map 

        int n; 
        cin >> n; 

        //Case where breakpoint is already set 
        if (breakpoints.find(n)!=breakpoints.end()) {
            cout << "breakpoint already set" <<endl; 
            continue; 
        }

        //Two STMT* references: prev and current 
        STMT* current = head; 
        STMT* prev = nullptr; 
        

        //First search for line n 
        while (current != nullptr) {
            

            if (current->line == n) {
                break;  //Found target line number n, break out of the loop
            }
            prev = current;  //prev always follows one behind current

            if (current->stmt_type == STMT_ASSIGNMENT) {
                if (current->types.assignment->next_stmt == nullptr) {
                    // Check if the current statement is in MapNodes
                    if (MapNodes.find(current) == MapNodes.end()) {
                        //This means the connection was severed and there's no mapping to follow, no such line case 
                        break;  
                    } else {
                        //Restore the severed connection from MapNodes
                        current = MapNodes[current];
                    }
                } else {
                    //Continue traversing normally if next_stmt is not nullptr
                    current = current->types.assignment->next_stmt;
                }
            }
            else if (current->stmt_type == STMT_FUNCTION_CALL) {
                if (current->types.function_call->next_stmt == nullptr) {
                    if (MapNodes.find(current) == MapNodes.end()) {
                        break;
                    } else {
                        current = MapNodes[current];
                    }
                } else {
                    current = current->types.function_call->next_stmt;
                }
            }
            else if (current->stmt_type == STMT_PASS) {
                if (current->types.pass->next_stmt == nullptr) {
                    if (MapNodes.find(current) == MapNodes.end()) {
                        break;
                    } else {
                        current = MapNodes[current];
                    }
                } else {
                    current = current->types.pass->next_stmt;
                }
            }
        }

        //After the loop, check if we actually found the target line
        if (current == nullptr || current->line != n) {
            //If current is nullptr or we didn't find the correct line number, exit the command
            cout << "No such line" << endl;
            continue;
        }

        //Proceed with setting the breakpoint: Sever prev -> current and fill in MapNodes
        if (prev != nullptr) {
            if (prev->stmt_type == STMT_ASSIGNMENT) {
                MapNodes[prev] = prev->types.assignment->next_stmt;
                prev->types.assignment->next_stmt = nullptr;  
            } else if (prev->stmt_type == STMT_FUNCTION_CALL) {
                MapNodes[prev] = prev->types.function_call->next_stmt;
                prev->types.function_call->next_stmt = nullptr;
            } else if (prev->stmt_type == STMT_PASS) {
                MapNodes[prev] = prev->types.pass->next_stmt;
                prev->types.pass->next_stmt = nullptr;
            }
    

            breakpoints.insert(n); //Insert n into breakpoint line numbers set 
            //cout << "Breakpoint set at line #" << n << endl;
        } 
    }

    else if (cmd == "rb") {
        int n;
        cin >> n;
        //Only print "breakpoint removed" if the removal was successful
        bool flag = false; 
        removeBreakpoint(n, flag); 
        if (flag) {
            cout << "breakpoint removed" << endl; 
        } 
    }
    else if (cmd == "cb") {
        //Call clearAllBreakpoints(), I put it in a helper as the destructor also needs to clear all breakpoints 
        clearAllBreakpoints(); 
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
                if (iterator != breakpoints.begin()) {
                    cout << ", ";
                }
                cout << *iterator; 
            }
            cout << endl; 
        }
    }

    else if (cmd == "w") {
        if (state == "Completed") {
            cout << "completed execution" << endl;
        } 
        else if (state == "Loaded") {
            cout << "line 1" << endl;
        } 
        //The line that's going to run next is the line that's at our currentStmt right now 
        else if (state == "Running") {
            cout << "line " << currentStmt->line << endl;
        }
    }
  }
}

void Debugger::clearAllBreakpoints() {
    //To clear all breakpoints, we relink everything that is severed which is represented by MapNodes pairs 
    if (MapNodes.size()>0) { 
        //Iterate over all the entries in MapNodes
        for (const auto& entry : MapNodes) {
            STMT* stmt = entry.first;  // The statement before the breakpoint
            STMT* nextStmt = entry.second;  // The statement at the breakpoint


            // Restore the severed connection 
            if (stmt->stmt_type == STMT_ASSIGNMENT) {
                stmt->types.assignment->next_stmt = nextStmt;
            } else if (stmt->stmt_type == STMT_FUNCTION_CALL) {
                stmt->types.function_call->next_stmt = nextStmt;
            } else if (stmt->stmt_type == STMT_PASS) {
                stmt->types.pass->next_stmt = nextStmt;
            }
        }

        // Clear the breakpoints set and MapNodes
        breakpoints.clear();
        MapNodes.clear(); 
    }
}

void Debugger::removeBreakpoint(int n, bool &flag) {
    STMT* targetStmt = nullptr;  // Line n statement
    STMT* next = nullptr;

    
    // Check if the breakpoint exists
    if (breakpoints.find(n) == breakpoints.end()) {
        cout << "no such breakpoint" << endl;
        flag=false; 
        return; 
    }

    // Remove from the breakpoints set
    breakpoints.erase(n);
    flag=true; 

    // Locate the target statement and its next statement in MapNodes
    for (const auto& entry : MapNodes) {
        next = entry.second;
        if (next != nullptr && next->line == n) {
            targetStmt = entry.first;
            break;
        }
    }

    // Restore the connection based on the statement type
    if (targetStmt != nullptr) {  // Ensure targetStmt is valid
        if (targetStmt->stmt_type == STMT_ASSIGNMENT) {
            targetStmt->types.assignment->next_stmt = next;
        } else if (targetStmt->stmt_type == STMT_FUNCTION_CALL) {
            targetStmt->types.function_call->next_stmt = next;
        } else if (targetStmt->stmt_type == STMT_PASS) {
            targetStmt->types.pass->next_stmt = next;
        }
    }

    // Remove the mapping pair from MapNodes
    MapNodes.erase(targetStmt);
}

void Debugger::removeBreakpointAuto(int n) {
    STMT* targetStmt = nullptr;  // Line n statement
    STMT* next = nullptr;

    
    // Check if the breakpoint exists
    if (breakpoints.find(n) == breakpoints.end()) {
        cout << "no such breakpoint" << endl;
        return; 
    }

    // Remove from the breakpoints set
    breakpoints.erase(n);

    // Locate the target statement and its next statement in MapNodes
    for (const auto& entry : MapNodes) {
        next = entry.second;
        if (next != nullptr && next->line == n) {
            targetStmt = entry.first;
            break;
        }
    }

    // Restore the connection based on the statement type
    if (targetStmt != nullptr) {  // Ensure targetStmt is valid
        if (targetStmt->stmt_type == STMT_ASSIGNMENT) {
            targetStmt->types.assignment->next_stmt = next;
        } else if (targetStmt->stmt_type == STMT_FUNCTION_CALL) {
            targetStmt->types.function_call->next_stmt = next;
        } else if (targetStmt->stmt_type == STMT_PASS) {
            targetStmt->types.pass->next_stmt = next;
        }
    }

    // Remove the mapping pair from MapNodes
    MapNodes.erase(targetStmt);
}

