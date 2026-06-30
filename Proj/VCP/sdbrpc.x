/* $Header:   /pvcs/data/arc/pegasus/gqam/sw/ppc/app/sdbrpc.x_v   1.1   13 Sep 2005 16:30:26   buchen  $ */
/**********************************************************************
 FILE:        sdbrpc.x

 DESCRIPTION: GQAM SDB Server RPC interface descriptions

 AUTHORS:     GQAM development team
 
 REVISION:    09/13/2005 

 Copyright (c) 2005, 2006 by Scientific-Atlanta, Inc. All rights reserved.
 
 COMMENTS:  This the RPC Language (RPCL) protocol definition file that defines the RPC
            interface for SDB_Server<>GQAM communications. The SDB_Server only needs 
	    this file and not the main file (rpc.x).   
 **********************************************************************/
 

/* Common Type Definitions */ 
typedef unsigned long       qXactionIdEdge; 		/* RPC transaction number */
typedef unsigned long       qOperationResultEdge;	/* Result of the operation */
typedef opaque              qSessionIdEdge<10>;	   	/* Identifies a session */
typedef unsigned long       qResponsePgmNumberEdge;  	/* RPC program number */

/* RPC Transaction Information. Always first element included in an RPC request to a server */
typedef struct XactionInfoEdge  qXactionInfoEdge;
struct XactionInfoEdge
{
        qXactionIdEdge		qxActionId; 		/* Unique nmbr assigned by client */		
        qResponsePgmNumberEdge 	qrpcPgmNumber;	 	/* Client's server program number
							   which awaits the async response */
};

/* RPC Transaction Information. Always first element included in an RPC response to a client */
typedef struct XactionResponseEdge qXactionResponseEdge;
struct XactionResponseEdge 
{
        qOperationResultEdge	qresultCode; 		/* Result of the operation -- see
							   result codes defined below */
        qXactionInfoEdge	qxactionInfo;		/* Copy of qXactionInfoEdge sent by the
							   client in the request phase of
							   the async call */  
};


/* Client Procedures: The Edge is the server and the SDB-Server/RM is the client. */
program EdgeSdbRequestControlProgram
{
  version EdgeSdbRequestControlProgram_ver
  {
     void qbindSessionEdge( qBindSessionEdge_param ) = 1;
     void qunbindSessionEdge( qUnbindSessionEdge_param ) = 2;
     void qqueryBindingInfoEdge( qQueryBindingInfo_param ) = 3;
     void qqueryBindingsEdge( qQueryBindings_param ) = 4;
  } = 1;
}= 0x20000051;

/* Server Procedures: The Edge is the client and the SDB-Server/RM is the server. */
program EdgeSdbResponseControlProgram
{
  version EdgeSdbResponseControlProgram_ver
  {
     void qbindSessionEdgeResponse( qBindSessionEdgeResponse_param ) = 1;
     void qunbindSessionEdgeResponse( qUnbindSessionEdgeResponse_param ) = 2;
     void qqueryBindingInfoEdgeResponse( qQueryBindingInfoResponse_param ) = 3;
     void qqueryBindingsEdgeResponse( qQueryBindingsResponse_param ) = 4;
  } = 1;
} = 0x30000052;

/* Result Codes:  To be encoded in qOperationResultEdge for responses */
const EDGE_NO_ERROR   	  			=	0x90020000;
const EDGE_ERROR_RPC_OUT_OF_MEMORY   	  	=	0x90020001;
const EDGE_ERROR_RPC_HARDWARE_FAILURE   	=	0x90020002;
const EDGE_ERROR_RPC_SESSION_NOT_FOUND         	= 	0x90020003;
const EDGE_ERROR_RPC_INSUFFICIENT_MEMORY       	=	0x90020006;
const EDGE_ERROR_RPC_INSUFFICIENT_CAPACITY     	=	0x90020007;
const EDGE_ERROR_RPC_PROGRAM_NUMBER_CONFLICT   	=	0x90020009;
const EDGE_ERROR_RPC_BANDWIDTH_UNAVAILABLE     	=	0x9002000A;
const EDGE_ERROR_RPC_BIND_FAILURE     	  	=	0x9002000B;
const EDGE_ERROR_RPC_SESSION_UNBOUND     	=	0x9002000C;
const EDGE_ERROR_RPC_SESSION_UNAVAILABLE	=	0x9002000D;
const EDGE_ERROR_RPC_UNBIND_SESSION_NOT_BOUND	=	0x9002000E;


/****************************************************************************

     void qbindSessionEdge( qBindSessionEdge_param ) -- SDB_Server is client
     void qbindSessionEdgeResponse( qBindSessionEdgeResponse_param ) -- GQAM is client
     
         This RPC sequence allows the binding of an SDB Server/RM session to a program stream.

****************************************************************************/

struct qBindSessionEdge_param
{
	qXactionInfoEdge	qxactionInfo;		/* Asynchronous RPC header */
	qSessionIdEdge		qsessionId; 		/* Session number */
	unsigned long           qoutputPgmNumber;	/* Outgoing program number */
	unsigned long           qinputPgmNumber; 	/* Incoming program number */
	bool                    qoverrideBindingFlag; 	/* If set, the session is unbound/rebound */
	unsigned long           qpgmBandwidth;		/* Program bandwidth */
	unsigned long           qdestIpAddr;  		/* Destination IP address (could be multicast) */
	unsigned long           qdestUdpPort;		/* Destination UDP port (variable for unicast, fixed for multicast) */
	unsigned long           qsourceIpAddr<3>;	/* Source IP addresses */
	bool			qnoPidRemapping;    	/* If set, do not remap PMT or elementary PIDs */
};

struct qBindSessionEdgeResponse_param
{
	qXactionResponseEdge	qresponseInfo;   	/* Asynchronous RPC header */
};

/****************************************************************************

     void qunbindSessionEdge( qUnbindSessionEdge_param ) -- SDB_Server is client
     void qunbindSessionEdgeResponse( qUnbindSessionEdgeResponse_param ) -- GQAM is client
     
         This RPC sequence is required when the SDB Server needs to free up 
	 bandwidth on the Edge device for other programs.

****************************************************************************/

struct qUnbindSessionEdge_param
{
	qXactionInfoEdge	qxactionInfo;		/* Asynchronous RPC header */
	qSessionIdEdge		qsessionId;   		/* Session number */       
};

struct qUnbindSessionEdgeResponse_param
{
	qXactionResponseEdge	qresponseInfo;		/* Asynchronous RPC header */
};


/****************************************************************************

     void qqueryBindingInfoEdge( qQueryBindingInfo_param ) -- SDB_Server is client
     void qqueryBindingInfoEdgeResponse( qQueryBindingInfoResponse_param ) -- GQAM is client
     
         This RPC sequence allows the SDB Server to retrieve a binding that it 
	 previously set on the Edge device.

****************************************************************************/

struct qQueryBindingInfo_param
{
	qXactionInfoEdge	qxactionInfo;		/* Asynchronous RPC header */	
	qSessionIdEdge       	qsessionId;   		/* Session number */       
};

struct qQueryBindingInfoResponse_param
{
	qXactionResponseEdge   	qresponseInfo; 		/* Asynchronous RPC header */
	qSessionIdEdge       	qsessionId;  		/* Session number */
	unsigned long           qoutputPgmNumber;	/* Outgoing program number */
	unsigned long           qinputPgmNumber;   	/* Incoming program number */
	unsigned long           qpgmBandwidth; 		/* Program bandwidth */
	unsigned long           qdestIpAddr;		/* Destination IP address (could be multicast) */
	unsigned long           qsourceIpAddr<3>;	/* Source IP addresses */
	bool			qnoPidRemapping;  	/* If set, do not remap PMT or elementary PIDs */
};


/****************************************************************************

     void qqueryBindingsEdge( qQueryBindings_param ) -- SDB_Server is client
     void qqueryBindingsEdgeResponse( qQueryBindingsResponse_param ) -- GQAM is client
     
         This RPC sequence allows the SDB Server to query the Edge device for all
	 bound sessions controlled by the SDB Server/Resource Manager.

****************************************************************************/

struct qQueryBindings_param
{
	qXactionInfoEdge	qxactionInfo;		/* Asynchronous RPC header */
};

struct qQueryBindingsResponse_param
{
	qXactionResponseEdge   	qresponseInfo;		/* Asynchronous RPC header */
	qSessionIdEdge    	qsessionId<1024>;	/* List of session numbers of bound sessions */
};

/*
 * $Log:   /pvcs/data/arc/pegasus/gqam/sw/ppc/app/sdbrpc.x_v  $
 *
 *   Rev 1.1   13 Sep 2005 16:30:26   buchen
 *add support for qnoPidRemapping bool for bind session and query binding info RPCs
 *add comments, header, and log 
 *
 */
