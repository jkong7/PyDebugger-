/*debugger.h*/

//Jonathan Kong

//Header file for the debugger class, declares private data members that are necessary for the run method to keep track of the 
//states as well as the constructor, destructor, and main run function (also includes helper clearAllBreakpoints function)

#pragma once

#include <string> 
#include <set> 
#include <map> 
#include <vector> 
#include <algorithm>

#include "execute.h"
#include "programgraph.h"
#include "ram.h"

using namespace std;

class Debugger {
private: 
  string state; //Holds state string ("Loaded", "Running", "Completed")
  STMT* currentStmt; //Holds where we're at currently in the programgraph, this is the context (starting position) that execute() needs
  STMT* head; //Points to first programgraph node (which is program parameter to )
  RAM* memory; //RAM memory 
  map <STMT*, STMT*> MapNodes; //Map of prev -> current nodes so that severences can be restored and also to move execute forward
  set<int> breakpoints; //set of breakpoint line numbers 

public:
  //Constructor 
  Debugger(struct STMT* program);

  //Destructor
  ~Debugger();

  //Main function: run
  void run();

  //Helper function to clear all breakpoints 
  void clearAllBreakpoints(); 

  //Helper function to remove breakpoint (auto "clean up" after run command)
  void removeBreakpoint(int n, bool &flag); 

  void removeBreakpointAuto(int n); 

};

