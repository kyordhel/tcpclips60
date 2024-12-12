   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*                  A Product Of The                   */
   /*             Software Technology Branch              */
   /*             NASA - Johnson Space Center             */
   /*                                                     */
   /*             CLIPS Version 6.00  05/12/93            */
   /*                                                     */
   /*                     MAIN MODULE                     */
   /*******************************************************/

/*************************************************************/
/* Purpose:                                                  */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*	Jesus Savage					     */
/*	Mauricio Matamoros				     */
/*                                                           */
/*             FI-UNAM  Version 6.00  05/1/2008              */
/*             FI-UNAM  Version 6.00  04/1/2013              */
/*             FI-UNAM  Version 6.00  04/18/2023             */
/*                                                           */
/*************************************************************/
/** @file main.cpp
 * Anchor point (main function) for the clipscontrol node
 */

/** @cond */
#include <cstdio>
#include <string>
#include <iostream>

#include <boost/algorithm/string.hpp>
/** @endcond */

#include "server.h"
#include "clipswrapper.h"

/* ** ********************************************************
* Global variables
* *** *******************************************************/
/**
 * The CLIPS TCP Server
 */
Server server;

/* ** ********************************************************
* Prototypes
* *** *******************************************************/
int main(int argc, char **argv);
inline int server_sendto_invoker(Server& server, const std::string& destPort, const std::string& message);
inline int server_broadcast_invoker(Server& server, const std::string& message);

/* ** ********************************************************
* C-compatible Prototypes
* *** *******************************************************/
extern "C" {
	void UserFunctions();
	int CLIPS_sendto_wrapper();
	int CLIPS_broadcast_wrapper();
}


/* ** ********************************************************
* Main (program anchor)
* *** *******************************************************/
/**
 * Program anchor
 * @param  argc The number of arguments to the program
 * @param  argv The arguments passed to the program
 * @return      The program exit code
 */
int main(int argc, char **argv){

	if( !server.init(argc, argv) )
		return -1;

	// server.runAsync();
	server.run();
	server.stop();
	std::cout << std::endl;
	return 0;
}


/* ** ********************************************************
* Function definitions
* *** *******************************************************/

/**
 * Defines and sets up userfunctions to use within clips scripts
 *
 * @remark Function invoked by CLIPS to setup user functions
 */
void UserFunctions(){
	// int clips::defineFunction(functionName, functionType, functionPointer, actualFunctionName);
	// char *functionName, functionType, *actualFunctionName;
	// int (*functionPointer)();

	// (sendto ?topic ?str)
	// DefineFunction("sendto", 'i', CLIPS_sendto_wrapper, "CLIPS_sendto_wrapper");
	clips::defineFunction("sendto", 'i', CLIPS_sendto_wrapper);
	// (broadcast ?topic ?fact)
	// DefineFunction("broadcast", 'i', CLIPS_broadcast_wrapper, "CLIPS_broadcast_wrapper");
	clips::defineFunction("broadcast", 'i', CLIPS_broadcast_wrapper);
}


/**
 * Sends the given message (second paramenter) to the specified client via its remote endpoint.
 * Wrapper for the CLIPS' sendto function. It calls Server::sendTo via friend-function server_sendto_invoker.
 * @return Zero if unwrapping was successful, -1 otherwise.
 */
int CLIPS_sendto_wrapper(){
	// (sendto ?port ?str)
	if(clips::argCountCheck("sendto", clips::ArgCountRestriction::Exactly, 2) == -1)
		return -1;

	/* Get the values for the 1st and 2rd arguments */
	std::string strEp = clips::returnLexeme(1);
	std::string message = clips::returnLexeme(2);
	boost::trim_right(message);

	/* It sends the data */
	return server_sendto_invoker(server, strEp, message + '\n');
}

inline
int server_sendto_invoker(Server& server, const std::string& cliEP, const std::string& message){
	return server.sendTo(cliEP, message) ? 0 : -1;
}

/**
 * Broadcasts the given message to all connected clients.
 * Wrapper for the CLIPS' broadcast function. It calls Server::broadcast via friend-function server_broadcast_invoker
 * @return Zero if unwrapping was successful, -1 otherwise.
 */
int CLIPS_broadcast_wrapper(){
	// (broadcast ?str)
	if(clips::argCountCheck("broadcast", clips::ArgCountRestriction::Exactly, 1) == -1)
		return -1;

	/* Get the values for the 1st argument */
	std::string message = clips::returnLexeme(1);
	boost::trim_right(message);

	/* It sends the data */
	return server_broadcast_invoker(server, message + '\n');
}

inline
int server_broadcast_invoker(Server& server, const std::string& message){
	return server.broadcast(message) ? 0 : -1;
}


// Sandalia casual flexi