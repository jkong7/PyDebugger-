/*debugger.h*/

//Jonathan Kong

//Header file for the debugger class, declares private data members that are necessary for the run method to keep track of the 
//states as well as the constructor, destructor, and main run function. Includes all helper functions as well 

#pragma once

#include <string> 
#include <set> 
#include <algorithm>

#include "execute.h"
#include "programgraph.h"
#include "ram.h"

using namespace std;

class Debugger {
private: 
  string state; //Holds state string ("Loaded", "Running", "Completed")
  STMT* head; //Points to first programgraph node (same inititially as currentStmt, but head will remain unchanged)
  STMT* currentStmt; //Holds where we're at currently in the programgraph, this is the context (starting position) that execute() needs
  STMT* next; //Next node in relation to currentStmt
  RAM* memory; //RAM memory 
  set<int> breakpoints; //Set of breakpoint line numbers 
  bool second_time_breakpoint; //flag that determines if the current breakpoint line is being seen for the first or second time
  set<int> lines; //Set of program graph line numbers, makes it easy to see if a breakpoint line exists in the graph
  
public:
  //Constructor 
  Debugger(struct STMT* program);

  //Destructor
  ~Debugger();

  //Main function: run
  void run();

  //Helper function to perform the step operation
  void step(); 

  //Helper function to moduralize the "execute one line" operation 
  void executeOneLine(); 

  //Helper function to break the graph at an input node (node's next set to null)
  void breakGraph(STMT* node); 

  //Helper function to repair the graph at the node->next connection 
  void repairGraph(STMT* node, STMT* next); 

  //Helper function to get the next node relative to input node 
  STMT* getNextNode(STMT* node); 

};

