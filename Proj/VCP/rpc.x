/* $Header:   /pvcs/data/arc/pegasus/gqam/sw/ppc/app/rpc.x_v   1.14   14 Sep 2005 17:16:14   buchen  $ */
/**********************************************************************
 FILE:        rpc.x

 DESCRIPTION: GQAM RPC interface descriptions

 AUTHORS:     GQAM development team
 
 REVISION:    07/29/2005 

 Copyright (c) 2005, 2006 by Scientific-Atlanta, Inc. All rights reserved.
 
 COMMENTS:  This the RPC Language (RPCL) protocol definition file that defines the RPC
            interface for SRM<>GQAM communications.  It also includes the RPCL protocol
            definition file for SDB_Server<>GQAM communications.  The GQAM must provide
            interfaces to both management entities in order to support Switch Digital 
            Broadcast (SDB) operation.   
 
 **********************************************************************/
 
/* Include the SDB_Server<>GQAM RPCL protocol definition file. The SRM (DNCS) need not    */ 
/* include this file.  The SDB_Server only needs this file and not the main file (rpc.x). */
#include "sdbrpc.x"

#ifndef _NO_qGqamClientRequest_

/*GQAM is server, element manager is client */
program qGqamClientRequestControlProgram
{
  version qGqamClientRequestControlProgram_ver
  {
     void qprovisionGqam( qProvisionGqam_param ) = 1;
     void qresetGqam( qResetGqam_param ) = 2;
     void qprovisionRequestGqamResponse( qXactionResponseGqam) = 3;
     void qprovisionQueryGqam( qXactionInfoGqam ) = 4;

     void qcreateSessionGqam( qCreateSessionGqam_param ) = 5;
     void qdeleteSessionGqam( qDeleteSessionGqam_param ) = 6;
     void qsessionQueryGqam( qSessionQueryGqam_param ) = 7;
     void qquerySessionsGqam( qQuerySessionsGqam_param ) = 8;

     void qcreateSubprogramGqam( qCreateSubprogramGqam_param ) = 9;
     void qdeleteSubprogramGqam( qDeleteSubprogramGqam_param ) = 10;
     void qprogramQueryGqam( qProgramQueryGqam_param ) = 11;
     void qqueryProgramsGqam( qQueryProgramsGqam_param ) = 12;

     void qdeliverMskGqam( qDeliverMskGqam_param ) = 13;
     void qdeliverSessionKeyGqam( qDeliverSkGqam_param ) =14;
     void qdeliverEcmGqam( qDeliverECMGqam_param ) = 15;
     void qrequestMskGqamResponse( qXactionInfoGqam ) = 16;
     void qdeliverCADescGqam( qDeliverCADescGqam_param ) = 17;
     void qqueryCADescGqam( qQueryCADescGqam_param ) = 18;

     void qinsertPacketGqam( qInsertPacketGqam_param ) = 19;
     void qcancelPacketGqam( qCancelPacketGqam_param ) = 20;
     void qqueryInsertPacketGqam( qQueryInsertPacketGqam_param ) = 21;     

     void qsetTimeGqam( qSetTimeGqam_param ) = 22;
     void qgetTimeGqam( qXactionInfoGqam ) = 23;
     
     void qgetPublicKeyGqam( qXactionInfoGqam ) = 24;
     
     void qqueryPerfStatsGqam(qQueryPerfStatsGqam_param) = 25;
     
     void qdeliverIskGqam(qDeliverIskGqam_param) = 26;
     
     void qsetGigeIpGqam( qsetGigeIpGqam_param ) = 27;
     
  } = 1;
  
  
  version qGqamClientRequestControlProgram_ver_2
  {
     void qprovisionGqam( qProvisionGqam_param_2 ) = 1;
     void qresetGqam( qResetGqam_param ) = 2;
     void qprovisionRequestGqamResponse( qXactionResponseGqam) = 3;
     void qprovisionQueryGqam( qXactionInfoGqam ) = 4;

     void qcreateSessionGqam( qCreateSessionGqam_param_2 ) = 5;
     void qdeleteSessionGqam( qDeleteSessionGqam_param_2 ) = 6;
     void qsessionQueryGqam( qSessionQueryGqam_param ) = 7;
     void qquerySessionsGqam( qQuerySessionsGqam_param ) = 8;

     void qprogramQueryGqam( qProgramQueryGqam_param ) = 11;
     void qqueryProgramsGqam( qQueryProgramsGqam_param ) = 12;

     void qdeliverMskGqam( qDeliverMskGqam_param ) = 13;
     void qdeliverEcmGqam( qDeliverECMGqam_param_2 ) = 15;
     void qrequestMskGqamResponse( qXactionInfoGqam ) = 16;

     void qinsertPacketGqam( qInsertPacketGqam_param_2 ) = 19;
     void qcancelPacketGqam( qCancelPacketGqam_param_2 ) = 20;
     void qqueryInsertPacketGqam( qQueryInsertPacketGqam_param ) = 21;     

     void qsetTimeGqam( qSetTimeGqam_param ) = 22;
     void qgetTimeGqam( qXactionInfoGqam ) = 23;
     
     void qgetPublicKeyGqam( qXactionInfoGqam ) = 24;
     
     void qqueryPerfStatsGqam(qQueryPerfStatsGqam_param) = 25;
     
     void qdeliverIskGqam(qDeliverIskGqam_param_2) = 26;
          
     void qcreateSessionGroupSdbGqam( qCreateSessionGroupSdbGqam_param ) = 28;
     
     void qdeleteSessionGroupSdbGqam( qDeleteSessionGroupSdbGqam_param ) = 29;


  } = 2;
} = 0x20000061;

#endif

#ifndef _NO_qGqamClientResponse_
 
           /*GQAM is client, element manager is server */
program qGqamClientResponseControlProgram
{
  version qGqamClientResponseControlProgram_ver
  {
    void qprovisionGqamResponse( qXactionResponseGqam ) = 1;
    void qresetGqamResponse( qXactionResponseGqam ) = 2;
    void qprovisionRequestGqam( qprovisionRequestGqam_param) = 3;
    void qprovisionQueryGqamResponse( qProvisionQueryGqamResponse_param ) = 4;
    
    void qcreateSessionGqamResponse( qCreateSessionGqamResponse_param ) = 5;
    void qdeleteSessionGqamResponse( qXactionResponseGqam ) = 6;
    void qsessionQueryGqamResponse( qSessionQueryGqamResponse_param ) = 7;
    void qquerySessionsGqamResponse( qQuerySessionsGqamResponse_param ) = 8;

    void qcreateSubprogramGqamResponse( qCreateSubprogramGqamResponse_param ) = 9;
    void qdeleteSubprogramGqamResponse( qXactionResponseGqam ) = 10;
    void qprogramQueryGqamResponse( qProgramQueryGqamResponse_param ) = 11;
    void qqueryProgramsGqamResponse( qQueryProgramsGqamResponse_param ) = 12;

    void qdeliverMskGqamResponse( qXactionResponseGqam ) = 13;
    void qdeliverSessionKeyGqamResponse( qXactionResponseGqam ) =14;
    void qdeliverEcmGqamResponse( qXactionResponseGqam ) = 15;
    void qrequestMskGqam( qRequestMskGqam_param ) = 16;
    void qdeliverCADescGqamResponse( qXactionResponseGqam ) = 17;
    void qqueryCADescGqamResponse( qQueryCADescGqamResponse_param ) = 18;

    void qInsertPacketGqamResponse( qInsertPacketGqamResponse_param ) = 19;
    void qcancelPacketGqamResponse( qXactionResponseGqam ) = 20;
    void qqueryInsertPacketGqamResponse( qQueryInsertPacketGqamResponse_param ) = 21;

    void qsetTimeGqamResponse( qXactionResponseGqam ) = 22;
    void qgetTimeGqamResponse( qGetTimeGqamResponse_param ) = 23;
    
    void qgetPublicKeyGqamResponse(qGetPublicKeyGqamResponse_param ) = 24;
    
    void qqueryPerfStatsGqamResponse(qQueryPerfStatsGqamResponse_param) = 25;
    
    void qdeliverIskGqamResponse(qXactionResponseGqam) = 26;

    void qsetGigeIpGqamResponse( qXactionResponseGqam ) = 27;
        
  } = 1;
  
  version qGqamClientResponseControlProgram_ver_2
  {
    void qprovisionGqamResponse( qXactionResponseGqam ) = 1;
    void qresetGqamResponse( qXactionResponseGqam ) = 2;
    void qprovisionRequestGqam( qprovisionRequestGqam_param) = 3;
    void qprovisionQueryGqamResponse( qProvisionQueryGqamResponse_param_2 ) = 4;
    
    void qcreateSessionGqamResponse( qCreateSessionGqamResponse_param_2 ) = 5;
    void qdeleteSessionGqamResponse( qXactionResponseGqam ) = 6;
    void qsessionQueryGqamResponse( qSessionQueryGqamResponse_param_2 ) = 7;
    void qquerySessionsGqamResponse( qQuerySessionsGqamResponse_param ) = 8;

    void qprogramQueryGqamResponse( qProgramQueryGqamResponse_param_2 ) = 11;
    void qqueryProgramsGqamResponse( qQueryProgramsGqamResponse_param ) = 12;

    void qdeliverMskGqamResponse( qXactionResponseGqam ) = 13;
    void qdeliverEcmGqamResponse( qXactionResponseGqam ) = 15;
    void qrequestMskGqam( qRequestMskGqam_param ) = 16;

    void qInsertPacketGqamResponse( qInsertPacketGqamResponse_param ) = 19;
    void qcancelPacketGqamResponse( qXactionResponseGqam ) = 20;
    void qqueryInsertPacketGqamResponse( qQueryInsertPacketGqamResponse_param_2 ) = 21;

    void qsetTimeGqamResponse( qXactionResponseGqam ) = 22;
    void qgetTimeGqamResponse( qGetTimeGqamResponse_param ) = 23;
    
    void qgetPublicKeyGqamResponse(qGetPublicKeyGqamResponse_param ) = 24;
    
    void qqueryPerfStatsGqamResponse(qQueryPerfStatsGqamResponse_param) = 25;
    
    void qdeliverIskGqamResponse(qXactionResponseGqam) = 26;
    
    void qcreateSessionGroupSdbGqamResponse( qCreateSessionGroupSdbGqamResponse_param  ) = 29;
    
    void qdeleteSessionGroupSdbGqamResponse( qXactionResponseGqam ) = 30;

  } = 2;
} = 0x30000062;

#endif

#ifndef _NO_qGqamClientAlarm_

/* Calls to send alarm and configuration responses to control computer
   GQAM  is client, element manager is server */

program qGqamClientAlarmProgram
{
  version qGqamClientAlarmProgram_ver
  {
   void qregisterAlarmGqam( qRegisterAlarmGqam_param ) = 1;  
   void qqueryAlarmsGqamResponse( qQueryAlarmsGqamResponse_param) = 2; 
   void qprovisionAlarmGqamResponse( qXactionResponseGqam ) = 3;
  } = 1;
} = 0x30000063; 

#endif

#ifndef _NO_qGqamServerAlarm_

/* Calls to configure alarms and return responses by control computer
   GQAM  is server, element manager is clientr */

program qGqamServerAlarmProgram
{
  version qGqamServerAlarmProgram_ver
  {
  void qprovisionAlarmGqam( qProvisionAlarmGqam_param ) = 1; 
  void qqueryAlarmsGqam( qQueryAlarmsGqam_param ) = 2;  
  void qregisterAlarmGqamResponse( qXactionResponseGqam ) = 3;
  } = 1;
} = 0x20000064;

#endif

#ifndef _NO_GQAM_PARAM_DEFS_

typedef opaque         	qEcmCaMessageGqam<80>;  /* ECM in CA_message form */
typedef opaque         	qEmmCaMessageGqam<>;    /* EMM in CA_message form */
typedef opaque         	qPmtGqam<1024>;         /* PMT in raw section form */
typedef opaque         	qPatGqam<1024>;         /* PAT in raw section form */
typedef qPatGqam       	qPatListGqam<>;         /* List of PAT sections */
typedef unsigned long  	qPidGqam;               /* An MPEG PID value, 0 < PID < 2^13 */
typedef opaque        	qSessionIDGqam<10>;     /* Identifies a session */
typedef unsigned long  	qXactionIdGqam;         /* RPC transaction number */
typedef unsigned long  	qResponsePgmNumberGqam; /* RPC program number */

typedef unsigned long  	qMskSelectGqam;         /* MSK number */
typedef opaque         	qMPEGPacketGqam<188>;   /* An MPEG Transport packet */
typedef unsigned long  	qPacketDelayGqam;       /* # of msecs before packet sent */
typedef unsigned long  	qPacketRepeatGqam;      /* repeat interval in millisecs. */
typedef unsigned long  	qPacketListIDGqam;      /* IDs group of packets to insert */
typedef unsigned long  	qUnixEpochTimeGqam;     /* system time */
typedef unsigned long   qECMUpdatePeriodGqam;   /* Milliseconds per ECM update */
typedef qPidGqam        qPidListGqam<>;         /* list of PIDs */ 
typedef qMPEGPacketGqam	qMPEGPacketListGqam<>;  /* List of MPEG packets */
typedef qMskSelectGqam 	qMSKListGqam<>;         /* list of MSKs */
typedef unsigned long  	qPrgmNmbrGqam;          /* MPEG Program number */
typedef opaque         	qPublicKeyGqam<>;       /* Public Key storage */



/* as yet undefined types - change later... */


typedef unsigned long  qOperationResultGqam;
typedef unsigned long  qMskSerialNumberGqam;
       
/*  Format of codes:     
                            0xWWXXYYZZ

    where WW = PRODUCT ID    90 = GQAM
          XX = TYPE          01 = ALARMS
                             02 = ERRORS
          YY = Major category  unique to TYPE
          ZZ = Minor category  unique to TYPE and Major category

*/ 

/* Define the result codes for all RPC calls to the GQAM */

const GQAM_NO_ERROR                            		=  0x90020000;
const GQAM_ERROR_RPC_OUT_OF_MEMORY             		=  0x90020001;
const GQAM_ERROR_RPC_HARDWARE_FAILURE          		=  0x90020002;
const GQAM_ERROR_RPC_SESSION_NOT_FOUND         		=  0x90020003;
const GQAM_ERROR_RPC_MISSING_MSK               		=  0x90020004;
const GQAM_ERROR_RPC_SESSION_ALREADY_EXISTS    		=  0x90020005;
const GQAM_ERROR_RPC_INSUFFICIENT_MEMORY       		=  0x90020006;
const GQAM_ERROR_RPC_INSUFFICIENT_CAPACITY     		=  0x90020007;
const GQAM_ERROR_RPC_PROVISION_FAILURE	        	=  0x90020008;
const GQAM_ERROR_RPC_PROGRAM_NUMBER_CONFLICT   		=  0x90020009;
const GQAM_ERROR_RPC_BANDWIDTH_UNAVAILABLE     		=  0x9002000A;
const GQAM_ERROR_RPC_SAME_GIGAIP               		=  0x9002000B;
const GQAM_ERROR_RPC_GIGAIP_INVALID            		=  0x9002000C;
const GQAM_ERROR_RPC_GIGAIP_FAILURE            		=  0x9002000D;
const GQAM_ERROR_RPC_GROUP_SDB_SESSION_FAILURE 		=  0x9002000E;
const GQAM_ERROR_RPC_INSUFFICIENT_OUTPUT_CAPACITY 	=  0x9002000F;

/* Common structure definitions used by many all products and many procedures */

struct qXactionInfoGqam{
   qXactionIdGqam          qxactionId;     /* unique nmbr assigned by client */
   qResponsePgmNumberGqam  qpgmNumber;     /* client's server program number
                                              which awaits the async response */
};

struct qXactionResponseGqam{
   qOperationResultGqam    qresultCode;    /* result of the operation -- see
                                              result codes defined above */
   qXactionInfoGqam        qxactionInfo;   /* copy of qXactionInfoGqam sent by the
                                              client in the request phase of
                                              the async call */  
};

struct qExecutionTimeGqam
{
   bool                qimmediateExecution;/* The execution of the function
                                              starts immediately when true */
   qUnixEpochTimeGqam      qstartTime;         /* Start time when
                                              qimmediateExecution is false */
};

struct qEcmEntryGqam  /* Element of ECM list */
{ 
  qEcmCaMessageGqam        qecm;               /* Single ECM */
  qMskSerialNumberGqam     qmskSerialNumber;   /* Serial number of the MSK to be used
                                              with the above ECM */
};

struct qEmmEntryGqam  /* Element of EMM list */
{ 
  qEmmCaMessageGqam        qemm;               /* Single EMM */
  qMskSerialNumberGqam     qmskSerialNumber;   /* Serial number of the MSK embedded
                                              in the above EMM */
};
struct qEcmsPerSessionGqam  /* Contains all  ECMs associated with a session */
{ 
  qEcmEntryGqam        qecmList<>;         /* list of all ECMs for the session */
  bool                 qisEncrypted;       /* True if session is to be encrypted*/
};


/****************************************************************************

     void qresetGqam( qResetGqam_param ) -- DNCS is client
     void qresetGqamResponse( qXactionInfoGqam ) -- GQAM is client
         This functions allows the GQAM to be reset by the element manager.
    Upon receiving this command, the GQAM returns a normal response and 
    then performs a "cold" boot.

****************************************************************************/

struct qResetGqam_param
{
  qXactionInfoGqam     qxactionInfo;         /* asynchronous RPC header */  
  unsigned char        qserialNumber<6>;     /* MAC address of the GQAM */
};

/****************************************************************************/
/****************************************************************************/



/****************************************************************************

     void qprovisionRequestGqam( qProvisionRequestGqam_param ) -- GQAM is client
     void qprovisionRequestGqamResponse( qXactionInfoGqam ) -- DNCS is client
         This function allows the GQAM, after boot, to request provisioning.
    Prior to executing this function however, the GQAM first executes a
    Bootp to obtain its IP address along with the IP address of the server
    responsible for its configuration and its configuration file name.
    The GQAM then reads the configuration file from the server using TFTP.  
    After processing the configuration file, the GQAM finally executes the
    qprovisionRequest function.    

****************************************************************************/

struct qprovisionRequestGqam_param
{
  qXactionInfoGqam     qxactionInfo;         /* asynchronous RPC header */
  unsigned char        qserialNumber<6>;     /* MAC address of the GQAM */
};


/****************************************************************************/
/****************************************************************************/


/****************************************************************************

     void qprovisionGqam( qProvisionGqam_param ) -- DNCS is client
     void qprovisionGqamResponse( qXactionResponseGqam ) -- GQAM is client
         This function delivers provisioning information to the GQAM. 

****************************************************************************/

struct qModulatorSettingsGqam
{
  bool                qmodCtrlPresent;       /* This bool, when true, indicates
                                                that the following 6 parameters
                                                contain valid data for the referenced
						                              modulator */
						
  unsigned long       qoutputFrequency;      /* Output frequency in Hz (constraints need to be applied) */ 
        				
  unsigned char       qoutputModulation;     /* Set DAVIC 1.0, DVB, or ITUB Mode */
  
  unsigned long       qoutputBandwidth;      /* Output bandwidth */
  
  bool                qoutputSquelch;        /* Enable/diable squelch (mute) */
  
  bool                qcwMode;               /* Enable/disable continuous wave mode */
  
  unsigned char       qitubInterleaverDepth; /* Sets ITUB interleaver depth */
};


struct qPsiOutputSettingsGqam
{

  bool                qpsiOutputCtrlPresent;	/* This bool, when true, indicates
						                           that the following 6 parameters
						                           contain valid data for the referenced
						                           stream */   
						
  unsigned long       qtransportStreamID;  	/* This is the value placed in the
						                           output PAT to identify the stream.
						                           0 < qtransportStreamID < 0xffff */
						
  unsigned long       qnetworkPID;         	/* This is the PID identified in the
						                           output PAT as the network PID 
						                           0 < qnetworkPID < 2^13 and as
						                           additionally restricted by MPEG
						                           specification */
						
  unsigned long       qemmPID;              	/* This is the PID identified in the
                           			         output CAT as the CA PID. 
                           			         0 < qnetworkPID < 2^13 and as
                           			         additionally restricted by MPEG
                           			         specification */
						
  unsigned long       qcatInsrtRate;       	/* These three values control the
                           			         insertion rate for the internally
                           			         generated psi tables for that stream.
                           			         The unit is in bits/second (bps) */
  unsigned long       qpatInsrtRate;
  unsigned long       qpmtInsrtRate;
};


struct qProvisionGqam_param
{
  qXactionInfoGqam         qxactionInfo;              /* asynchronous RPC header */
  
  qUnixEpochTimeGqam       qtime;             	      /* Time to use for setting GQAM clock */
  
  bool                     qlocalControlLockout;  	   /* Enable/disable front panel control */
  
  bool		               qdisallowOverflowSessionDrop;  /* Disallow/allow overflow protection algorithm */			
  
  bool		               qdisableAutomux;		      /* Disable Automux mode if set, no effect if zero */			
  
  qModulatorSettingsGqam   qmodulatorSettings[16]; 	/* Modulation/RF section provisioning */
							                                 /* (16 modulated outputs) */
					     
  qPsiOutputSettingsGqam   qpsiOutputSettings[16]; 	/* Psi output Control parameters */
							                                 /* (16 independent output streams) */
						
  bool		               qdisallowSessionRejection;	/* Disallow/allow session rejection if no available bandwidth */  
};


struct qProvisionGqam_param_2
{
  qXactionInfoGqam         qxactionInfo;           /* asynchronous RPC header */
  
  qUnixEpochTimeGqam       qtime;             	   /* Time to use for setting GQAM clock */
  
  bool                     qlocalControlLockout;  	/* Enable/disable front panel control */
    
  qModulatorSettingsGqam   qmodulatorSettings[16]; 	/* Modulation/RF section provisioning */
							                                 /* (16 modulated outputs) */
					     
  qPsiOutputSettingsGqam   qpsiOutputSettings[16]; 	/* Psi output Control parameters */
							                                 /* (16 independent output streams) */
						
  bool		               qdisallowSessionRejection;	/* Disallow/allow session rejection if no available bandwidth */ 
   
  unsigned long 	         qgigaEtherIPaddr;		      /* IP address of the GbE port */
};



/****************************************************************************

     void qsetGigeIpGqam( qsetGigeIpGqam_param ) -- DNCS is client
     This RPC is issued by the DNCS following the qProvisionGqam RPC. 
     void qsetGigeIpGqamResponse( qXactionResponseGqam )
	         
****************************************************************************/

struct qsetGigeIpGqam_param
{
  qXactionInfoGqam         qxactionInfo;        /* asynchronous RPC header */
  unsigned long 	         qgigaEtherIPaddr;		/* IP address of the GbE port */
};

/****************************************************************************

     void qprovisionQueryGqam( qXactionInfoGqam ) -- DNCS is client
     void qprovisionQueryGqamResponse( qProvisionQueryGqamResponse_parm ) -- GQAM is client
         This function queries provisioning and version information of the GQAM. 

****************************************************************************/



struct qModQSettingsGqam
{       					
  unsigned long       qoutputFrequency;      /* Output frequency in Hz */ 
  
  unsigned long       qoutputLevel;	         /* RF output level (0.1 dBmV steps) */
  
  unsigned char       qoutputModulation;     /* Set DAVIC 1.0, DVB, or ITUB Mode */
  
  unsigned long       qoutputBandwidth;      /* Output bandwidth */
  
  bool                qoutputSquelch;        /* Enable/diable squelch (mute) */
  
  bool                qcwMode;               /* Enable/disable continuous wave mode */
  
  unsigned char       qitubInterleaverDepth; /* Sets ITUB interleaver depth */
};


struct qModBoardQSettingsGqam
{
  unsigned char	      qmcSoftwareRevision;    /* Microcontroller software version */
						
  qModQSettingsGqam	   qmodQSettings[4];	      /* Four sets of modulator query settings 
						                              per modulator board */
};


struct qPsiQSettingsGqam
{      					
  unsigned long       qtransportStreamID;  	/* This is the value placed in the
                           			         output PAT to identify the stream.
                           			         0 < qtransportStreamID < 0xffff */
						
  unsigned long       qnetworkPID;         	/* This is the PID identified in the
                           			         output PAT as the network PID 
                           			         0 < qnetworkPID < 2^13 and as
                           			         additionally restricted by MPEG
                           			         specification */
						
  unsigned long       qemmPID;              	/* This is the PID identified in the
                           			         output CAT as the CA PID. 
                           			         0 < qnetworkPID < 2^13 and as
                           			         additionally restricted by MPEG
                           			         specification */
						
  unsigned long       qcatInsrtRate;       	/* These three values control the
                           			         insertion rate for the internally
                           			         generated psi tables for that stream.
                           			         The unit is in bits/second (bps) */
  unsigned long       qpatInsrtRate;
  unsigned long       qpmtInsrtRate;
};


struct qProvisionQueryGqamResponse_param
{
  qXactionResponseGqam    qresponseInfo;        /* asynchronous RPC header */
    
  /* Management Control Parameters */
  unsigned long       qalarmManagerProgram;	   /* program number of alarm server */
  unsigned long       qalarmManagerIP;     	   /* IP address of the alarm server */
  unsigned long       qGqamManagerProgram;      /* program number of GQAM server */
  unsigned long       qGqamManagerIP;           /* IP address of the GQAM server */
  
  /* Time Management Parameter */
  qUnixEpochTimeGqam      qtime; 		     	   /* Current time */

  /* Front Panel Lockout */
  bool               qlocalControlLockout;      /* Enable/disable front panel control */
  
  /* Bandwidth Overflow Protection */
  bool		     qdisallowOverflowSessionDrop;  /* Disallowed/allowed overflow protection */			
  
  /* Automux mode */
  bool		     qautomuxMode;  		/* Automux mode enabled/disabled */			
  
  /* Revision Information */
  char      	     qhardwareRevision[32];     /* Hardware revision string */
  
  unsigned long      qappSoftwareRevision;          /* Host Application (MPC8265) Code Software Revision (BCD) */
  unsigned long      qbootSoftwareRevision;         /* Host Boot (MPC8265) Code Software Revision (BCD) */ 
  
  unsigned long      qinputAppSoftwareRevision;     /* Input Application (IXP1200) Code Software Revision (BCD) */
  unsigned long      qinputBootSoftwareRevision;    /* Input Boot (IXP1200) Code Software Revision (BCD) */ 
  
  unsigned long      qoutputAppSoftwareRevision;    /* Output Application (IXP1200) Code Software Revision (BCD) */
  unsigned long      qoutputBootSoftwareRevision;   /* Output Boot (IXP1200) Code Software Revision (BCD) */ 
  
  /* Model Information */
  unsigned char qmodelType;                     	/* Model type (i.e. 0xXX = D9479) */
  
  /* Modulation/RF section provisioning */
  qModBoardQSettingsGqam qmodBoardQSettings[4];     	/* Modulator query structures */
							                                 /* 4 output converter boards with 4 outputs each */
							                                 /* 16 modulated outputs total */ 

  /* PSI output Control parameters */
  qPsiQSettingsGqam qpsiQSettings[16];               /* PSI query structures (16) */
  
  /* Session Rejection Based On Bandwidth Available */
  bool		qdisallowSessionRejection;	      /* Disallowed/allowed session rejection */
  
  /* Giga Ethernet Port IP address */
  unsigned long	 qgigaEtherIPaddr;         /* IP address for the GIGE port */     
};


struct qProvisionQueryGqamResponse_param_2
{
  qXactionResponseGqam    qresponseInfo;        /* asynchronous RPC header */
    
  /* Management Control Parameters */
  unsigned long       qalarmManagerProgram;	   /* program number of alarm server */
  unsigned long       qalarmManagerIP;     	   /* IP address of the alarm server */
  unsigned long       qGqamManagerProgram;      /* program number of GQAM server */
  unsigned long       qGqamManagerIP;           /* IP address of the GQAM server */
  
  /* Time Management Parameter */
  qUnixEpochTimeGqam      qtime; 		     	   /* Current time */

  /* Front Panel Lockout */
  bool               qlocalControlLockout;      /* Enable/disable front panel control */
    
  /* Revision Information */
  char      	      qhardwareRevision[32];     /* Hardware revision string */
  
  unsigned long      qappSoftwareRevision;          /* Host Application (MPC8265) Code Software Revision (BCD) */
  unsigned long      qbootSoftwareRevision;         /* Host Boot (MPC8265) Code Software Revision (BCD) */ 
  
  unsigned long      qinputAppSoftwareRevision;     /* Input Application (IXP1200) Code Software Revision (BCD) */
  unsigned long      qinputBootSoftwareRevision;    /* Input Boot (IXP1200) Code Software Revision (BCD) */ 
  
  unsigned long      qoutputAppSoftwareRevision;    /* Output Application (IXP1200) Code Software Revision (BCD) */
  unsigned long      qoutputBootSoftwareRevision;   /* Output Boot (IXP1200) Code Software Revision (BCD) */ 
  
  /* Model Information */
  unsigned char qmodelType;                     	/* Model type (i.e. 0xXX = D9479) */
  
  /* Modulation/RF section provisioning */
  qModBoardQSettingsGqam qmodBoardQSettings[4];     	/* Modulator query structures */
							                                 /* 4 output converter boards with 4 outputs each */
							                                 /* 16 modulated outputs total */ 
  /* PSI output Control parameters */
  qPsiQSettingsGqam qpsiQSettings[16];               /* PSI query structures (16) */
  
  /* Session Rejection Based On Bandwidth Available */
  bool		qdisallowSessionRejection;	      /* Disallowed/allowed session rejection */
  
  /* Giga Ethernet Port IP address */
  unsigned long	 qgigaEtherIPaddr;         /* IP address for the GIGE port */     
};



/****************************************************************************/
/****************************************************************************/



/****************************************************************************
     void qcreateSessionGqam( qCreateSessionGqam_param )
     void qcreateSessionGqamResponse( qCreateSessionGqamResponse_param  )
         This call delivers the parameters to the GQAM to initiate the creation
         of a session.  This call specifically ties a session to an MPEG program
         identified in the incoming PSI of the selected input port and to the
         selected output port.  
****************************************************************************/

/* Indicates the type of GIGE encapsulation */
enum qGigaEtherTypesGqam
{
  qMPLS_label_gqam,			         /* MPLS label (unimplemented) */
  qUDP_socket_gqam,	 	            /* UDP socket for MPTS */
  qUDP_socket_no_program_num_gqam  	/* UDP socket for SPTS */
};

struct qCreateSessionGqam_param
{ 
  qXactionInfoGqam    qxactionInfo;       /* asynchronous RPC header */
  qSessionIDGqam      qsessionId;         /* session number */
  qExecutionTimeGqam  qexecutionTime;     /* Function execution time */
  unsigned long       qincommingPrgmNmbr; /* program number associated with session */
  unsigned long       qoutgoingPrgmNmbr;  /* program number associated with session */
  unsigned char       qinputPortNmbr;     /* input port for session (4 ASI inputs (0-3), 1 Giga Ether input (4)) */
  unsigned char       qoutputPortNmbr;    /* output port for session (16 outputs (0-15)) */
  unsigned long       qsessionRate;       /* Session rate ( bits/sec) 0 = unknown */
  unsigned long	    qblockPIDList<8>;      /* Block these PIDs from being in the output transport stream */
					                              /* PID values range from 0x0 - 0x1FFF */
  unsigned char	    qblockElemStreamList<8>;	/* Block the PIDs that are identified by elementary stream types */
						                              /* elementary stream types range from 0x0 - 0xFF */
  unsigned char	    qgigaEtherType;	      /* Type of Giga Ethernet if that input is selected (see enum above) */
  unsigned long	    qgigaEtherTypeValue;   /* Value associated with above Giga Ethernet */ 
  unsigned char       qsessionPriority;	   /* Priority of this session wrt other sessions for the selected output */
};

struct qCreateSessionGqamResponse_param
{ 
  qXactionResponseGqam	qresponseInfo;      /* asynchronous RPC header */
  qSessionIDGqam        qsessionId;         /* session number */
  unsigned long       	qincommingPrgmNmbr; /* program associated with session */
  unsigned long       	qoutgoingPrgmNmbr;  /* program associated with session */
  unsigned char       	qinputPortNmbr;     /* input port for session */
  unsigned char       	qoutputPortNmbr;    /* output port for session */
};

/* Indicates the encryption to be applied to elementary streams (ESs) 
   as they pass through the GQAM, and the handling of incoming ECMs */
enum qIsEncryptedGqam
{
  qUnEncrypted = 0,                   /* Unencrypted mode. No encryption applied to ESs, but any
                                         existing encryption is left in place.  Incoming PowerKEY 
                                         ECMs are passed, all other ECMs are dropped */                                       
  qPowerKeyEncrypted,                 /* PowerKEY encryption applied to ESs. All incoming ECMs are dropped. */
  qSharedKeyEncrypted,                /* Shared Key encryption applied to ESs.  Incoming Shared Key ECMs are
                                         passed, all other ECMs dropped. */
  qPreviouslyEncrypted,               /* Pass through mode.  No encryption applied to ESs, but any existing
                                         encryption is left in place.  All incoming ECMs are passed. */
  qFixedKeyEncrypted		              /* PowerKEY encryption with Fixed Key Leader.  Same as qPowerKeyEncrypted,
                                         except that during the time between session setup and application of
                                         normal PowerKEY encryption, the ESs are encrypted with a fixed
                                         key rather than passed in the clear. */ 
};

/* Indicates the type of GIGE encapsulation */
enum qGigaEtherTypesGqam_2
{
  qUDP_socket_gqam_2,	 		/* UDP socket */
  qIP_multicast_gqam_2          	/* IP multicast */
};

/* Indicates the type of session */
enum qSessionTypesGqam
{
  qCFB_session_gqam,			/* Continous feed broadcast */
  qVOD_session_gqam,			/* Video on demand */
  qSDB_shell_session_gqam  		/* Switched digital broadcast shell */
};

struct qCreateSessionGqam_param_2
{ 
  qXactionInfoGqam    	qxactionInfo;		/* asynchronous RPC header */
  qSessionIDGqam      	qsessionId;            	/* session number */
  unsigned long       	qincomingPrgmNmbr;     	/* program number associated with session */
  unsigned long       	qoutgoingPrgmNmbr;     	/* program number associated with session */
  unsigned char       	qinputPortNmbr;        	/* input port for session (4 ASI inputs (0-3), 1 Giga Ether input (4)) */
  unsigned char       	qoutputPortNmbr;       	/* output port for session (16 outputs (0-15)) */
  unsigned long       	qsessionRate;          	/* Session rate ( bits/sec) 0 = unknown */
  unsigned char	    	qgigaEtherType;	      	/* Type of Giga Ethernet if that input is selected (see qGigaEtherTypesGqam_2 enum above) */
  unsigned long		qgigaEtherTypeValue<>; 	/* Value(s) associated with above Giga Ethernet type */ 
  qIsEncryptedGqam    	qisEncrypted;          	/* denotes whether the session is encrypted or not */
  unsigned int        	qsourceIpAddress<3>;   	/* Redundant Source IP addresses for SSM used when Giga Ethernet type IP multicast */
  unsigned char       	qsessionPriority;	/* Priority of this session wrt other sessions for the selected output */
  bool			qnoPidRemapping;    	/* If set, do not remap PMT or elementary PIDs */
  unsigned char		qsessionType;		/* Type of session (see qSessionTypesGqam enum above) */
};


struct qCreateSessionGqamResponse_param_2
{ 
  qXactionResponseGqam	qresponseInfo;      /* asynchronous RPC header */
  qSessionIDGqam        qsessionId;         /* session number */
  unsigned long       	qincomingPrgmNmbr;  /* program associated with session */
  unsigned long       	qoutgoingPrgmNmbr;  /* program associated with session */
  unsigned char       	qinputPortNmbr;     /* input port for session */
  unsigned char       	qoutputPortNmbr;    /* output port for session */
};

/****************************************************************************/


/****************************************************************************
     void qcreateSessionGroupSdbGqam( qCreateSessionGroupSdbGqam_param )
     void qcreateSessionGroupSdbGqamResponse( qCreateSessionGroupSdbGqamResponse_param  )
         This call delivers the output port and a list of session ids to the GQAM 
	 to initiate the group creation of multiple SDB shell sessions.  This call 
	 specifically ties multiple SDB shell sessions to the selected output port.  The
	 input port is assumed to be the GIGE port of the GQAM.
****************************************************************************/

struct qCreateSessionGroupSdbGqam_param
{ 
  qXactionInfoGqam    qxactionInfo;     /* asynchronous RPC header */
  unsigned char       qoutputPortNmbr;  /* output port for group of SDB shell sessions */
  qSessionIDGqam      qsessionID<32>;   /* 32 possible SDB shell sessions to create */
  unsigned long	    qgroupRate;       /* total bandwidth in bps for the entire group */
};


struct qGroupSdbSessionStatus
{ 
  qSessionIDGqam        qsessionId;         /* session number */
  qOperationResultGqam  qresultCode;        /* result of the each session create operation */
};


struct qCreateSessionGroupSdbGqamResponse_param
{ 
  qXactionResponseGqam		qresponseInfo;	 		            /* asynchronous RPC header */      			
  qGroupSdbSessionStatus 	qgroupSdbSessionStatus<32>;   	/* 32 possible sessions created */
  unsigned long			   qgroupRate;			               /* total bandwidth in bps for the entire group */
};

/****************************************************************************/

/****************************************************************************/



/****************************************************************************

     void qdeleteSessionGqam( qDeleteSessionGqam_param )
     void qdeleteSessionGqamResponse( qXactionResponseGqam )
         This call deletes a session, and everything about it with the exception
         of common elements used by other sessions such as MSK's. 
           
****************************************************************************/

struct qDeleteSessionGqam_param
{
  qXactionInfoGqam      qxactionInfo;      /* asynchronous RPC header */
  unsigned char        	qoutputPortNmbr;   /* Output port for session */
  qSessionIDGqam        qsessionId;        /* Session to delete */
  qExecutionTimeGqam    qexecutionTime;    /* Function execution time */
};


struct qDeleteSessionGqam_param_2
{
  qXactionInfoGqam      qxactionInfo;      /* asynchronous RPC header */
  unsigned char        	qoutputPortNmbr;   /* Output port for session */
  qSessionIDGqam        qsessionId;        /* Session to delete */
};


/****************************************************************************/
/****************************************************************************/


/****************************************************************************

     void qdeleteSessionGroupSdbGqam( qDeleteSessionGroupSdbGqam_param )
     void qdeleteSessionGroupSdbGqamResponse( qXactionResponseGqam )
         This call deletes group SDB shell sessions and everything about them with the exception
         of common elements used by other sessions such as MSK's. 
           
****************************************************************************/

struct qDeleteSessionGroupSdbGqam_param
{
  qXactionInfoGqam      qxactionInfo;     /* asynchronous RPC header */
  unsigned char        	qoutputPortNmbr;  /* Output port for SDB shell sessions */
  qSessionIDGqam        qsessionID<32>;   /* 32 possible SDB shell sessions to delete */
};


/****************************************************************************/
/****************************************************************************/


/****************************************************************************
     void qsessionQueryGqam( qSessionQueryGqam_param )
     void qsessionQueryGqamResponse( qSessionQueryGqamResponse_param )
              This call returns specific information for a session. 

****************************************************************************/

struct qSessionAlarmInfoGqam
{
  unsigned long       qalarmID;            /* alarm identification */
  bool                qactive;             /* alarm is currently active when true */
  unsigned long       qalarmThreshold;     /* alarm threshold setting */
  qUnixEpochTimeGqam  qalarmTime;          /* alarm timestamp (could differ from timestamp 
                                              provided by alarm queries by one second) */
  opaque              qalarmCustom[16];    /* 16-byte array for alarm specifics */
};


struct qSessionQueryGqam_param
{
  qXactionInfoGqam      qxactionInfo;       /* asynchronous RPC header */
  unsigned char         qoutputPortNmbr;    /* output port for session */
  qSessionIDGqam        qsessionId;         /* session number */
};


struct qSessionInfoGqam    /* current settings for a session */
{
  qSessionIDGqam      qsessionId;         /* session number */
  qExecutionTimeGqam  qexecutionTime;     /* session execution time */
  unsigned long       qincommingPrgmNmbr; /* program associated with session */
  unsigned long       qoutgoingPrgmNmbr;  /* program associated with session */
  unsigned char       qinputPortNmbr;     /* input port for session */
  unsigned char       qoutputPortNmbr;    /* output port for session */
  unsigned long       qsessionRate;       /* Data rate (bits/second) */
  bool                qsessionSleeping;   /* Start time has not arrived */
  bool                qinputPsiDetected;  /* Incomming PSI has been detected */
  bool                qkeysDelivered;     /* All keys have been delivered */
  bool                qecmsDelivered;     /* All ecms have been delivered */
  bool                qresourcesAllocated;/* Resources have been allocated */
  bool                qpacketsDetected;   /* Session data has been detected */ 
  bool                qsessionStarted;    /* i.e. data is flowing */
  qPmtGqam            qprogramPmt;        /* Outgoing  PMT for the program */
  qSessionAlarmInfoGqam   qsessionDataAlarm; /* Session data alarm */        
  qSessionAlarmInfoGqam   qsessionProgAlarm; /* Session program alarm */        
  qSessionAlarmInfoGqam   qsessionCaAlarm;   /* Session CA alarm */
  unsigned char	    qgigaEtherType;	      /* Type of Giga Ethernet format (see enum above) */
  unsigned long	    qgigaEtherTypeValue;   /* Value associated with above Giga Ethernet */ 
  unsigned char       qsessionPriority;	   /* Priority of this session wrt other sessions */      
};


struct qSessionQueryGqamResponse_param
{
  qXactionResponseGqam  qresponseInfo;    /* asynchronous RPC header */
  qSessionInfoGqam      qsessionInfo;     /* session information */
}; 


struct qSessionInfoGqam_2    /* current settings for a session */
{
  qSessionIDGqam      qsessionId;         /* session number */
  unsigned long       qincomingPrgmNmbr;  /* program associated with session */
  unsigned long       qoutgoingPrgmNmbr;  /* program associated with session */
  unsigned char       qinputPortNmbr;     /* input port for session */
  unsigned char       qoutputPortNmbr;    /* output port for session */
  unsigned long       qsessionRate;       /* Data rate (bits/second) */
  bool                qsessionSleeping;   /* Start time has not arrived */
  bool                qinputPsiDetected;  /* Incomming PSI has been detected */
  bool                qkeysDelivered;     /* All keys have been delivered */
  bool                qecmsDelivered;     /* All ecms have been delivered */
  bool                qresourcesAllocated;/* Resources have been allocated */
  bool                qpacketsDetected;   /* Session data has been detected */ 
  bool                qsessionStarted;    /* i.e. data is flowing */
  qPmtGqam            qprogramPmt;        /* Outgoing  PMT for the program */
  qSessionAlarmInfoGqam   qsessionDataAlarm; /* Session data alarm */        
  qSessionAlarmInfoGqam   qsessionProgAlarm; /* Session program alarm */        
  qSessionAlarmInfoGqam   qsessionCaAlarm;   /* Session CA alarm */
  unsigned char	      qgigaEtherType;	     /* Type of Giga Ethernet format (see qGigaEtherTypesGqam_2 enum above) */
  unsigned long	      qgigaEtherTypeValue<>; /* Value(s) associated with above Giga Ethernet type */ 
  unsigned char       qsessionPriority;	     /* Priority of this session wrt other sessions for the selected output */
  qIsEncryptedGqam    qisEncrypted;          /* denotes whether the session is encrypted or not */
  unsigned int        qsourceIpAddress<3>;   /* Redundant Source IP addresses for SSM used when Giga Ethernet type IP multicast */
  bool		      qnoPidRemapping;       /* If set, do not remap PMT or elementary PIDs */
  unsigned char	      qsessionType;	     /* Type of session (see qSessionTypesGqam enum above) */
};


struct qSessionQueryGqamResponse_param_2
{
  qXactionResponseGqam    qresponseInfo;     /* asynchronous RPC header */
  qSessionInfoGqam_2      qsessionInfo;      /* session information */ 
}; 

/****************************************************************************/
/****************************************************************************/

 
/****************************************************************************
     void qquerySessionsGqam( qQuerySessions_param )
     void qquerySessionsGqamResponse( qQuerySessionsGqamResponse_param )
              This call returns a list of non-free sessions 

****************************************************************************/
 
struct qQuerySessionsGqam_param
{
  qXactionInfoGqam      qxactionInfo;       /* asynchronous RPC header */
  unsigned char         qoutputPortNmbr;    /* output port for session */
};


struct qQuerySessionsGqamResponse_param
{
  qXactionResponseGqam qresponseInfo;      /* asynchronous RPC header */
  unsigned char        qoutputPortNmbr;    /* output port for session */
  qSessionIDGqam       qsessionID<>;       /* array of session ids */      
};

/****************************************************************************/
/****************************************************************************/



/****************************************************************************
 
    void qcreateSubprogramGqam( qCreateSubprogramGqam_param )
     void qcreateSubprogramGqamResponse( qCreateSubprogramGqamResponse_param  )
         This call delivers the parameters to the GQAM to initiate the
         creation of a subprogram.  A subprogram is defined to consist of
         elements of other programs in the incoming stream.  The subprogram
         number is returned at successful completion of the qcreateSubprogram
         call.  The equivalent PMT delivered to the GQAM identifies the elements
         of the subprogram.  The GQAM uses this information to create the
         outgoing PMT when an actual session is created for the subprogram.
         The GQAM chooses the outgoing parameters such as PID's for all
         programs and subprograms.
            
****************************************************************************/

struct qCreateSubprogramGqam_param
{
  qXactionInfoGqam     qxactionInfo;       /* asynchronous RPC header */ 
  unsigned char        qoutputPortNmbr;    /* output port for session */
  qPmtGqam             qsubProgramPmt;     /* equiv incoming subprogram PMT*/  
  qExecutionTimeGqam   qexecutionTime;     /* Function execution time */
};


struct qCreateSubprogramGqamResponse_param
{
  qXactionResponseGqam qresponseInfo;      /* asynchronous RPC header */
  int                  subProgramNumber;   /* subProgramNumber created */
  unsigned char        qoutputPortNmbr;    /* output port for session */
};


/****************************************************************************/
/****************************************************************************/



/****************************************************************************

     void qdeleteSubprogramGqam( qDeleteSubprogramGqam_param )
     void qdeleteSubprogramGqamResponse( qXactionResponseGqam  )
              Deletes a subprogram, and everything about it 

****************************************************************************/

struct qDeleteSubprogramGqam_param
{
  qXactionInfoGqam     qxactionInfo;       /* asynchronous RPC header */
  unsigned char        qoutputPortNmbr;    /* output port for session */
  int                  subProgramNumber;   /* subProgramNumber to delete */
  qExecutionTimeGqam   qexecutionTime;     /* Function execution time */
};

/****************************************************************************/
/****************************************************************************/


/****************************************************************************
     void qprogramQueryGqam( qProgramQueryGqam_param )
     void qprogramQueryGqamResponse( qProgramQueryGqamResponse_param )
              This call returns specific information for a  program. 

****************************************************************************/
struct qProgramQueryGqam_param
{
  qXactionInfoGqam     qxactionInfo;      /* asynchronous RPC header */
  unsigned char        qoutputPortNmbr;   /* output port for session */
  qPrgmNmbrGqam        qprgmNmbr;         /* Program number */
};


struct qPrgmInfoGqam                      /* current settings for a program */
{
  qPrgmNmbrGqam        qprgmNmbr;         /* program or subprogram number ... 
                                              use subPrgmFlag as additional
                                              qualifier .. number not guaranteed
                                              unique between program and
                                              subprogram */
  bool                 qsubPrgmFlag;      /* When true indicates subprogram */
  qExecutionTimeGqam   qexecutionTime;    /* Subprogram execution time....
                                              not Function!!!!.. for program
                                              it is always immediate */
  qPrgmNmbrGqam        qoutgoingPrgmNmbr; /* Outgoing program number  */
  bool                 qsubprgmSleeping;  /* Start time has not arrived only
                                              valid for subprogram */
  qPmtGqam             qinPrgmPmt;        /* Incoming Pmt for (sub)program */
  qPmtGqam             qoutPrgmPmt;       /* Outgoing  PMT for (sub)program */
};


struct qPrgmInfoGqam_2                    /* current settings for a program (restricted) */
{
  qPrgmNmbrGqam        qprgmNmbr;         /* Incoming program number */ 
  qPrgmNmbrGqam        qoutgoingPrgmNmbr; /* Outgoing program number  */
  qPmtGqam             qinPrgmPmt;        /* Incoming PMT for program */
  qPmtGqam             qoutPrgmPmt;       /* Outgoing  PMT for program */
};


struct qProgramQueryGqamResponse_param
{
  qXactionResponseGqam qresponseInfo;     /* asynchronous RPC header */
  unsigned char        qoutputPortNmbr;   /* output port for session */
  qPrgmInfoGqam        qprgmInfo;         /* program information structure */
}; 


struct qProgramQueryGqamResponse_param_2
{
  qXactionResponseGqam qresponseInfo;     /* asynchronous RPC header */
  unsigned char        qoutputPortNmbr;   /* output port for session */
  qPrgmInfoGqam_2      qprgmInfo;         /* program information structure */
}; 

/****************************************************************************/
/****************************************************************************/

 

/****************************************************************************
     void qqueryProgramsGqam( qQueryPrograms_param )
     void qqueryProgramsGqamResponse( qQueryProgramsResponse_param )
              This call returns a list of incoming program numbers including 
              subprogram numbers. 


****************************************************************************/
 
struct qQueryProgramsGqam_param
{
  qXactionInfoGqam     qxactionInfo;       /* asynchronous RPC header */
  unsigned char        qoutputPortNmbr;    /* output port for session */
};

struct qQueryProgramsGqamResponse_param
{
  qXactionResponseGqam qresponseInfo;      /* asynchronous RPC header */
  unsigned char        qoutputPortNmbr;    /* output port for session */
  qPrgmNmbrGqam        qprgmNmbrs<>;       /* array of program numbers */
};

/****************************************************************************/
/****************************************************************************/



/****************************************************************************

     void qdeliverMskGqam( qDeliverMskGqam_param )
     void qdeliverMskGqamResponse( qXactionResponseGqam  )
           This call delivers one MSK via an EMM to the GQAM - 
           i.e. delivers a global MSK  

****************************************************************************/

struct qDeliverMskGqam_param
{
  qXactionInfoGqam         qxactionInfo;       /* asynchronous RPC header */
  qEmmEntryGqam            qemmList<>;         /* list of MSK's delivered in EMM */
};

/****************************************************************************/
/****************************************************************************/




/****************************************************************************

     void qdeliverSessionKeyGqam( qDeliverSkGqam_param )
     void qdeliverSessionKeyGqamResponse( qXactionResponseGqam )
              This call delivers a session key for the specified session 

****************************************************************************/

struct qDeliverSkGqam_param
{
  qXactionInfoGqam     qxactionInfo;       /* asynchronous RPC header */
  unsigned char        qoutputPortNmbr;    /* output port for session */
  qSessionIDGqam       qsessionId;         /* which session */
  qEmmCaMessageGqam    qemmCaMessage;      /* SK delivered in an EMM */
};

/****************************************************************************/
/****************************************************************************/



/****************************************************************************

     void qdeliverEcmGqam( qDeliverECMGqam_param )
     void qdeliverEcmGqamResponse( qXactionResponseGqam )
              Delivers ECMs for a specified session 

****************************************************************************/

struct qDeliverECMGqam_param
{
  qXactionInfoGqam     qxactionInfo;       /* asynchronous RPC header */  
  unsigned char        qoutputPortNmbr;    /* output port for session */
  qSessionIDGqam       qsessionId;         /* which session */
  qExecutionTimeGqam   qexecutionTime;     /* Function execution time */
  unsigned long        qecmUpdatePeriod;   /* milliseconds per ECM  update */
  unsigned long        qcwUpdatePeriod;    /* milliseconds per CW update */
  unsigned long        qcwSetupPeriod;     /* milliseconds cw deliverd to use */
  qEcmsPerSessionGqam  qecmsPerSession;    /* list of ECM info for all pids */
};


struct qDeliverECMGqam_param_2
{
  qXactionInfoGqam     qxactionInfo;       /* asynchronous RPC header */  
  unsigned char        qoutputPortNmbr;    /* output port for session */
  qSessionIDGqam       qsessionId;         /* which session */
  unsigned long        qecmUpdatePeriod;   /* milliseconds per ECM  update */
  unsigned long        qcwUpdatePeriod;    /* milliseconds per CW update */
  unsigned long        qcwSetupPeriod;     /* milliseconds cw deliverd to use */
  qEcmsPerSessionGqam  qecmsPerSession;    /* list of ECM info for all pids */
};

/****************************************************************************/
/****************************************************************************/


/****************************************************************************

     void qdeliverIskGqam( qDeliverIskGqam_param )
     void qdeliverIskGqamResponse( qXactionResponseGqam )
              Delivers ISK and ECM(s) for a specified interactive session 

****************************************************************************/

struct qDeliverIskGqam_param
{
  qXactionInfoGqam     qxactionInfo;       /* asynchronous RPC header */  
  unsigned char        qoutputPortNmbr;    /* output port for session */
  qSessionIDGqam       qsessionId;         /* which session */
  qExecutionTimeGqam   qexecutionTime;     /* Function execution time */
  unsigned long        qecmUpdatePeriod;   /* milliseconds per ECM  update */
  unsigned long        qcwUpdatePeriod;    /* milliseconds per CW update */
  unsigned long        qcwSetupPeriod;     /* milliseconds cw deliverd to use */
  qEcmsPerSessionGqam  qecmsPerSession;    /* list of ECM info for all pids */
  qEmmCaMessageGqam    qemmCaMessage;      /* ISK delivered in an EMM */
};


struct qDeliverIskGqam_param_2
{
  qXactionInfoGqam     qxactionInfo;       /* asynchronous RPC header */  
  unsigned char        qoutputPortNmbr;    /* output port for session */
  qSessionIDGqam       qsessionId;         /* which session */
  unsigned long        qecmUpdatePeriod;   /* milliseconds per ECM  update */
  unsigned long        qcwUpdatePeriod;    /* milliseconds per CW update */
  unsigned long        qcwSetupPeriod;     /* milliseconds cw deliverd to use */
  qEcmsPerSessionGqam  qecmsPerSession;    /* list of ECM info for all pids */
  qEmmCaMessageGqam    qemmCaMessage;      /* ISK delivered in an EMM */
};

/****************************************************************************/
/****************************************************************************/


/****************************************************************************
  
     void qqueryInsertPacketGqam( qQueryInsertPacketGqam_param )
     void qqueryInsertPacketGqamResponse( qQueryInsertPacketGqamResponse_param )
          This call provides the ability to read the status of messages
          being inserted into the stream.              

****************************************************************************/

struct qQueryInsertPacketGqam_param
{
  qXactionInfoGqam     qxactionInfo;       /* asynchronous RPC header */  
  unsigned char        qoutputPortNmbr;    /* output port for session */
};


struct qInsertPacketInfoGqam
{
  qPacketListIDGqam    qpacketlistid;        /* The reference number */
  bool                 qactive;              /* True if currently being inserted*/
  qExecutionTimeGqam   qexecutionTime;       /* Function execution time */
  qPacketRepeatGqam    qpacketRepeat;        /* Times the packets will be sent */
  bool                 qcontinuousFlag;      /* If true, the packets are (will be)
                                              sent continuously */
  unsigned long        qpacketRate;          /* Rate the packets are (will be)
                                              sent (bits/second)  */
};


struct qInsertPacketInfoGqam_2
{
  qPacketListIDGqam    qpacketlistid;        /* The reference number */
  bool                 qactive;              /* True if currently being inserted*/
  qPacketRepeatGqam    qpacketRepeat;        /* Times the packets will be sent */
  bool                 qcontinuousFlag;      /* If true, the packets are (will be)
                                              sent continuously */
  unsigned long        qpacketRate;          /* Rate the packets are (will be)
                                              sent (bits/second)  */
  unsigned short       qversionNum;          /* Store and return for query */                                            
};

struct qQueryInsertPacketGqamResponse_param
{
  qXactionResponseGqam  qresponseInfo;          /* asynchronous RPC header */
  unsigned char         qoutputPortNmbr;        /* output port for session */
  qInsertPacketInfoGqam qinsertPacketInfo<>;    /* array of insert packet information */
};

  
struct qQueryInsertPacketGqamResponse_param_2
{
  qXactionResponseGqam     qresponseInfo;          /* asynchronous RPC header */
  unsigned char            qoutputPortNmbr;        /* output port for session */
  qInsertPacketInfoGqam_2  qinsertPacketInfo<>;    /* array of insert packet information */
};

/****************************************************************************/
/****************************************************************************/



/****************************************************************************

     void qinsertPacketGqam( qInsertPacketGqam_param )
     void qInsertPacketGqamResponse( qInsertPacketGqamResponse_param  )
         This call provides the ability to insert MPEG packets into the output
         stream(s).  The PIDs of these packets are restricted to the Network PID 
         and to a finite set of PIDs reserved specifically for insertion.   
         These PIDs are: 0x1FF0 - 0x1FFE.   The qinsertPacket procedure can 
         be used to queue up data with a qInsertPacketId identical to one that is 
         currently being inserted. Any attempt at queuing an insertion with the
         same qInsertPacketId and same qexecutionTime results in the deletion of 
         the previous insert in the queue. The qInsertPacketId<> is assigned and 
         maintained by the GQAM element manager.
               
****************************************************************************/

struct qInsertPacketIdGqam
{
  unsigned char        qoutputPortNmbr;	   /* Output port to use */
  qPacketListIDGqam    qpacketListId;        /* The reference number if not new */
};


struct qInsertPacketIdGqam_2
{
  unsigned char        qoutputPortNmbr;	   /* Output port to use */
  qPacketListIDGqam    qpacketListId;        /* The reference number if not new */
  
  unsigned short       qversionNum;          /* Store and return for query */                                            
};

struct qInsertPacketGqam_param
{
  qXactionInfoGqam     qxactionInfo;         /* asynchronous RPC header */
  qInsertPacketIdGqam  qinsertPacketId<>;    /* could have up to 16 elements in array */
					                              /* to support broadcast to up to 16 outputs */
					                              /* with possibly differing qpacketListIds */
  qMPEGPacketListGqam  qmpegpacketlist;      /* MPEG transport packets to insert*/
  qExecutionTimeGqam   qexecutionTime;       /* Function execution time */
  qPacketRepeatGqam    qpacketRepeat;        /* Times the packets are be sent */
  bool                 qcontinuousFlag;      /* If true, the packets should be
                                              sent continuously starting at the
                                              qexecutionTime  */
  unsigned long        qpacketRate;          /* Rate the packets should be sent
                                              (bits/second)  */
};


struct qInsertPacketGqam_param_2
{
  qXactionInfoGqam       qxactionInfo;       /* asynchronous RPC header */
  qInsertPacketIdGqam_2  qinsertPacketId<>;  /* could have up to 16 elements in array */
					                              /* to support broadcast to up to 16 outputs */
					                              /* with possibly differing qpacketListIds */
  qMPEGPacketListGqam  qmpegpacketlist;      /* MPEG transport packets to insert*/
  qPacketRepeatGqam    qpacketRepeat;        /* Times the packets are be sent */
  bool                 qcontinuousFlag;      /* If true, the packets should be
                                              sent continuously starting at the
                                              qexecutionTime  */
  unsigned long        qpacketRate;          /* Rate the packets should be sent
                                              (bits/second)  */
};


struct qInsertPacketGqamResponse_param
{
  qXactionResponseGqam     qresponseInfo;    /* asynchronous RPC header */
};

/****************************************************************************/
/****************************************************************************/


/****************************************************************************

     void qcancelPacketGqam( qCancelPacketGqam_param )
     void qcancelPacketGqamResponse( qXactionResponseGqam )
            The qcancelPacket call removes insertion data either currently 
            being inserted or queued.  Both output port number and packet
	         list id are used to specify the inserted packets for cancelation.         

****************************************************************************/

struct qCancelPacketGqam_param
{
  qXactionInfoGqam     qxactionInfo;         /* asynchronous RPC header */ 
  qInsertPacketIdGqam  qinsertPacketId<>;    /* could have up to 16 elements in array */
					                              /* to support cancellation of broadcast to up to */
					                              /* 16 outputs with possibly differing qpacketListIds */
  bool                 qcancelAll;           /* if true, all inserts with matching
                                              qInsertPacketId's are deleted.
                                              otherwise only the ones matching
                                              executionTime. */ 
  qExecutionTimeGqam   qexecutionTime;       /* Function execution time */
};


struct qCancelPacketGqam_param_2
{
  qXactionInfoGqam     qxactionInfo;         /* asynchronous RPC header */ 
  qInsertPacketIdGqam  qinsertPacketId<>;    /* could have up to 16 elements in array */
					                              /* to support cancellation of broadcast to up to */
					                              /* 16 outputs with possibly differing qpacketListIds */
  bool                 qcancelAll;           /* if true, ALL inserts for all 16 outputs will be deleted 
                                                therefore qinsertPacketId<> will not be used */ 
};

/****************************************************************************/
/****************************************************************************/



/****************************************************************************

     void qsetTimeGqam( qSetTimeGqam_param )
     void qsetTimeGqamResponse( qXactionResponseGqam )
              Sets the time on the GQAM 

****************************************************************************/

struct qSetTimeGqam_param 
{
   qXactionInfoGqam         qxactionInfo;       /* asynchronous RPC header */ 
   qUnixEpochTimeGqam       qtime;              /* epoch time for set */
};

/****************************************************************************/
/****************************************************************************/



/****************************************************************************

     void qgetTimeGqam( qXactionInfoGqam )
     void qgetTimeGqamResponse( qGetTimeGqamResponse_param )
              Gets the time from the GQAM 

****************************************************************************/

struct qGetTimeGqamResponse_param 
{
   qXactionResponseGqam     qresponseInfo;   /* asynchronous RPC header */
   qUnixEpochTimeGqam       qtime;           /* epoch time returned */
};

/****************************************************************************/
/****************************************************************************/



/****************************************************************************

     void qdeliverCADescGqam( qDeliverCADescGqam_param )
     void qdeliverCADescGqamResponse( qXactionResponseGqam )
              This function delivers the CA descriptor to the GQAM

****************************************************************************/

struct qDeliverCADescGqam_param 
{
   qXactionInfoGqam     qxactionInfo;        /* asynchronous RPC header */ 
   unsigned char        qoutputPortNmbr;     /* output port for session */
   opaque               qcaDesc<>;           /* CA descriptor to use */          
};

/****************************************************************************/
/****************************************************************************/



/****************************************************************************

     void qqueryCADescGqam( qQueryCADescGqam_param )
     void qqueryCADescGqamResponse( qQueryCADescGqamResponse_param )
              This function returns the CA descriptor to the element manager

****************************************************************************/

struct qQueryCADescGqam_param 
{
   qXactionInfoGqam     qxactionInfo;       /* asynchronous RPC header */ 
   unsigned char        qoutputPortNmbr;    /* output port for session */
};

struct qQueryCADescGqamResponse_param 
{
   qXactionResponseGqam qresponseInfo;       /* asynchronous RPC header */
   unsigned char        qoutputPortNmbr;     /* output port for session */
   opaque               qcaDesc<>;           /* CA descriptor returned */
};

/****************************************************************************/
/****************************************************************************/


/****************************************************************************

     void qrequestMskGqamResponse( qXactionInfoGqam )
     void qrequestMskGqam( qRequestMskGqam_param )        
            This call allows the GQAM to notify the conditional access manager
            of the lack of a necessary MSK.  The MSK is delivered through
            the normal calls defined previously.

****************************************************************************/

struct qRequestMskGqam_param
{
  qXactionInfoGqam        qxactionInfo;       /* asynchronous RPC header */   
  qMskSelectGqam          qmskSelect;         /* missing MSK */ 
};

/****************************************************************************/
/****************************************************************************/



/****************************************************************************

     void qgetPublicKeyGqam( qXactionInfoGqam )
     void qgetPublicKeyGqamResponse(qGetPublicKeyGqamResponse_param )        
            This call allows the conditional access manager to retrieve the
            public key from the GQAM.
            
****************************************************************************/

struct qGetPublicKeyGqamResponse_param
{
  qXactionResponseGqam    qresponseInfo;     /* asynchronous RPC header */
  qPublicKeyGqam          qpublicKey;        /* requested public key */ 
};

/****************************************************************************/
/****************************************************************************/



/****************************************************************************

  void qregisterAlarmGqam( qRegisterAlarmGqam_param )    
  void qregisterAlarmGqamResponse( qXactionResponseGqam )   
          This call allows the GQAM to report the activation or clearing of
          any alarm condition to the alarm manager.

****************************************************************************/

enum qAlarmLevelGqam                      /* alarms levels for below */
{
  disabled_gqam = 0,
  critical_gqam = 1,
  major_gqam = 2,
  minor_gqam = 3,
  status_gqam  = 4
};

struct qRegisterAlarmGqam_param
{
  qXactionInfoGqam    qxactionInfo;       /* asynchronous RPC header */   
  unsigned long       qalarmID;           /* alarm identification */
  qAlarmLevelGqam     qalarmLevel;        /* alarm level (see enum above) */
  bool                qactive;            /* alarm activated if true, otherwise
                                             alarm cleared */
  qUnixEpochTimeGqam  qalarmTime;         /* time when alarm condition occured */ 
  opaque              qalarmCustom[16];   /* 16-byte array for alarm specifics */
};

/****************************************************************************/
/****************************************************************************/



/****************************************************************************

  void qprovisionAlarmGqam( qProvisionAlarmGqam_param )  
  void qprovisionAlarmGqamResponse( qXactionResponseGqam )  
        This call allows the alarm manager to provision alarms.

****************************************************************************/

struct qAlarmControlGqam
{
  unsigned long       qalarmID;            /* alarm identification */
  qAlarmLevelGqam     qalarmLevel;         /* classifies the alarm level */
  unsigned long       qalarmThreshold;     /* alarm threshold setting */
};

struct qProvisionAlarmGqam_param
{
  qXactionInfoGqam        qxactionInfo;   /* asynchronous RPC header */   
  qAlarmControlGqam       qalarms<>;      /* array of alarm controls */        
};

/****************************************************************************/
/****************************************************************************/



/****************************************************************************

  void qqueryAlarmsGqamResponse( qQueryAlarmsGqamResponse_param) 
  void qqueryAlarmsGqam( qQueryAlarmsGqam_param )   
         This call allows the alarm manager to query the enabled
         alarms on the GQAM.
 
****************************************************************************/

struct qQueryAlarmsGqam_param
{
  qXactionInfoGqam    qxactionInfo;        /* asynchronous RPC header */ 
  bool                qalarmAll;           /* query all if true */
  unsigned long       qalarmIDstart;       /* starting ID for query */
  unsigned long       qalarmNumIDs;        /* number of alarm IDs to query */          
};


struct qAlarmControlQueryGqam
{
  unsigned long       qalarmID;            /* alarm identification */
  qAlarmLevelGqam     qalarmLevel;         /* classifies the alarm level */
  bool                qactive;             /* alarm is currently active when true */
  unsigned long       qalarmThreshold;     /* alarm threshold setting */
  qUnixEpochTimeGqam  qalarmTime;          /* alarm timestamp */
  opaque              qalarmCustom[16];    /* 16-byte array for alarm specifics */
};

struct qQueryAlarmsGqamResponse_param
{
  qXactionResponseGqam qresponseInfo;      /* asynchronous RPC header */   
  qAlarmControlQueryGqam   qalarms<>;      /* array of alarm query information */       
};


/****************************************************************************/

/* Alarm definitions */
/* space allocated for 256 non-session alarms */
/* 4096 unique defined session alarms */
/* 1024 total sessions * 4 alarm sources */

enum qalarm_id_gqam
{

   GQAM_MOD_SYSALARM_ID = 0,  	     	/*  0 Modulator manager runtime error */
   
   GQAM_OC0_COMMALARM_ID,           	/*  1 Output Converter 0 communication failure */    	  
   GQAM_OC0_TEMPALARM_ID,	     	      /*  2 Output Converter 0 exceeded max temperature */          
   GQAM_OC0_PS_ALARM_ID,   	     	   /*  3 Output Converter 0 power supply failure */      
   GQAM_OC0_DCLOCKDETECTALARM_ID,   	/*  4 Output Converter 0 downconverter lock detect logic */  
   GQAM_OC0_DCPLLUNLOCKALARM_ID,    	/*  5 Output Converter 0 downconverter PLL unlocked */  
   GQAM_OC0_INITALARM_ID,  	     	   /*  6 Output Converter 0 ASIC initialization failure */      
   GQAM_OC0_LEVELNOCAL_ID,   	     	   /*  7 Output Converter 0 level not calibrated */    
   GQAM_OC0_UCLOCKDETECTALARM_ID,   	/*  8 Output Converter 0 upconverter Lock detect logic */  
   GQAM_OC0_UCPLLUNLOCKALARM_ID,    	/*  9 Output Converter 0 upconverter PLL unlocked */  
   GQAM_OC0_EEPROMALARM_ID,	     	   /* 10 Output Converter 0 EEPROM failure */
   
   GQAM_OC1_COMMALARM_ID,           	/* 11 Output Converter 1 communication failure */    	  
   GQAM_OC1_TEMPALARM_ID,	     	      /* 12 Output Converter 1 exceeded max temperature */          
   GQAM_OC1_PS_ALARM_ID,   	     	   /* 13 Output Converter 1 power supply failure */      
   GQAM_OC1_DCLOCKDETECTALARM_ID,   	/* 14 Output Converter 1 downconverter Lock detect logic */  
   GQAM_OC1_DCPLLUNLOCKALARM_ID,    	/* 15 Output Converter 1 downconverter PLL unlocked */  
   GQAM_OC1_INITALARM_ID,  	     	   /* 16 Output Converter 1 ASIC initialization failure */      
   GQAM_OC1_LEVELNOCAL_ID,   	     	   /* 17 Output Converter 1 level not calibrated */    
   GQAM_OC1_UCLOCKDETECTALARM_ID,   	/* 18 Output Converter 1 upconverter Lock detect logic */  
   GQAM_OC1_UCPLLUNLOCKALARM_ID,    	/* 19 Output Converter 1 upconverter PLL unlocked */  
   GQAM_OC1_EEPROMALARM_ID,	     	   /* 20 Output Converter 1 EEPROM failure */
   
   GQAM_OC2_COMMALARM_ID,           	/* 21 Output Converter 2 communication failure */    	  
   GQAM_OC2_TEMPALARM_ID,	     	      /* 22 Output Converter 2 exceeded max temperature */          
   GQAM_OC2_PS_ALARM_ID,   	     	   /* 23 Output Converter 2 power supply failure */      
   GQAM_OC2_DCLOCKDETECTALARM_ID,   	/* 24 Output Converter 2 downconverter Lock detect logic */  
   GQAM_OC2_DCPLLUNLOCKALARM_ID,    	/* 25 Output Converter 2 downconverter PLL unlocked */  
   GQAM_OC2_INITALARM_ID,  	     	   /* 26 Output Converter 2 ASIC initialization failure */      
   GQAM_OC2_LEVELNOCAL_ID,   	     	   /* 27 Output Converter 2 level not calibrated */    
   GQAM_OC2_UCLOCKDETECTALARM_ID,   	/* 28 Output Converter 2 upconverter Lock detect logic */  
   GQAM_OC2_UCPLLUNLOCKALARM_ID,    	/* 29 Output Converter 2 upconverter PLL unlocked */  
   GQAM_OC2_EEPROMALARM_ID,	     	   /* 30 Output Converter 2 EEPROM failure */
    
   GQAM_OC3_COMMALARM_ID,           	/* 31 Output Converter 3 communication failure */    	  
   GQAM_OC3_TEMPALARM_ID,	     	      /* 32 Output Converter 3 exceeded max temperature */          
   GQAM_OC3_PS_ALARM_ID,   	     	   /* 33 Output Converter 3 power supply failure */      
   GQAM_OC3_DCLOCKDETECTALARM_ID,   	/* 34 Output Converter 3 downconverter Lock detect logic */  
   GQAM_OC3_DCPLLUNLOCKALARM_ID,    	/* 35 Output Converter 3 downconverter PLL unlocked */  
   GQAM_OC3_INITALARM_ID,  	     	   /* 36 Output Converter 3 ASIC initialization failure */      
   GQAM_OC3_LEVELNOCAL_ID,   	    	   /* 37 Output Converter 3 level not calibrated */    
   GQAM_OC3_UCLOCKDETECTALARM_ID,   	/* 38 Output Converter 3 upconverter Lock detect logic */  
   GQAM_OC3_UCPLLUNLOCKALARM_ID,    	/* 39 Output Converter 3 upconverter PLL unlocked */  
   GQAM_OC3_EEPROMALARM_ID,	     	   /* 40 Output Converter 3 EEPROM failure */
   
   GQAM_PM_RESET_DETECTED_ID,        	/* 41 cold reset occurred */
    
   GQAM_PM_IN0_CONT_ERROR_ID,	     	   /* 42 MPEG continuity error counter */ 
   GQAM_PM_IN0_TEI_ERROR_ID,	     	   /* 43 MPEG transport error indicator counter */ 
   
   GQAM_PM_IN1_CONT_ERROR_ID,	     	   /* 44 MPEG continuity error counter */ 
   GQAM_PM_IN1_TEI_ERROR_ID,	     	   /* 45 MPEG transport error indicator counter */ 
   
   GQAM_PM_IN2_CONT_ERROR_ID,	     	   /* 46 MPEG continuity error counter */ 
   GQAM_PM_IN2_TEI_ERROR_ID,	     	   /* 47 MPEG transport error indicator counter */ 
   
   GQAM_PM_IN3_CONT_ERROR_ID,	     	   /* 48 MPEG continuity error counter */ 
   GQAM_PM_IN3_TEI_ERROR_ID,	     	   /* 49 MPEG transport error indicator counter */ 
   
   GQAM_PM_IN4_CONT_ERROR_ID,	     	   /* 50 MPEG continuity error counter */ 
   GQAM_PM_IN4_TEI_ERROR_ID,	     	   /* 51 MPEG transport error indicator counter */ 
   
   GQAM_PM_IN0_LOSS_OF_SIGNAL_ID,       /* 52 Input 0 loss of input signal */       
   GQAM_PM_IN0_ERRORED_MPEG_PACKETS_ID, /* 53 Input 0 global error of MPEG packet */
   GQAM_PM_IN0_FIFO_OVERFLOW_ID,        /* 54 Input 0 FIFO Overflow bit  */
   GQAM_PM_IN0_EXCESS_DUMPED_PACKETS_ID,/* 55 Input 0 packets were dumped */
   
   GQAM_PM_IN1_LOSS_OF_SIGNAL_ID,       /* 56 Input 1 loss of input signal */       
   GQAM_PM_IN1_ERRORED_MPEG_PACKETS_ID, /* 57 Input 1 global error of MPEG packet */
   GQAM_PM_IN1_FIFO_OVERFLOW_ID,        /* 58 Input 1 FIFO Overflow bit  */
   GQAM_PM_IN1_EXCESS_DUMPED_PACKETS_ID,/* 59 Input 1 packets were dumped */
   
   GQAM_PM_IN2_LOSS_OF_SIGNAL_ID,       /* 60 Input 1 loss of input signal */       
   GQAM_PM_IN2_ERRORED_MPEG_PACKETS_ID, /* 61 Input 1 global error of MPEG packet */
   GQAM_PM_IN2_FIFO_OVERFLOW_ID,        /* 62 Input 1 FIFO Overflow bit  */
   GQAM_PM_IN2_EXCESS_DUMPED_PACKETS_ID,/* 63 Input 1 packets were dumped */
   
   GQAM_PM_IN3_LOSS_OF_SIGNAL_ID,       /* 64 Input 1 loss of input signal */       
   GQAM_PM_IN3_ERRORED_MPEG_PACKETS_ID, /* 65 Input 1 global error of MPEG packet */
   GQAM_PM_IN3_FIFO_OVERFLOW_ID,        /* 66 Input 1 FIFO Overflow bit  */
   GQAM_PM_IN3_EXCESS_DUMPED_PACKETS_ID,/* 67 Input 1 packets were dumped */
   
   GQAM_PM_IN4_LOSS_OF_SIGNAL_ID,       /* 68 Input 1 loss of input signal */       
   GQAM_PM_IN4_ERRORED_MPEG_PACKETS_ID, /* 69 Input 1 global error of MPEG packet */
   GQAM_PM_IN4_FIFO_OVERFLOW_ID,        /* 70 Input 1 FIFO Overflow bit  */
   GQAM_PM_IN4_EXCESS_DUMPED_PACKETS_ID,/* 71 Input 1 packets were dumped */
   
   GQAM_PM_OUT0_DROPPED_PACKETS_ID,	   /* 72 Output 0 low priority packets are being dropped */
   GQAM_PM_OUT1_DROPPED_PACKETS_ID, 	/* 73 Output 1 low priority packets are being dropped */
   GQAM_PM_OUT2_DROPPED_PACKETS_ID,	   /* 74 Output 2 low priority packets are being dropped */
   GQAM_PM_OUT3_DROPPED_PACKETS_ID,	   /* 75 Output 3 low priority packets are being dropped */
   GQAM_PM_OUT4_DROPPED_PACKETS_ID,	   /* 76 Output 4 low priority packets are being dropped */
   GQAM_PM_OUT5_DROPPED_PACKETS_ID,	   /* 77 Output 5 low priority packets are being dropped */
   GQAM_PM_OUT6_DROPPED_PACKETS_ID,	   /* 78 Output 6 low priority packets are being dropped */
   GQAM_PM_OUT7_DROPPED_PACKETS_ID,	   /* 79 Output 7 low priority packets are being dropped */
   GQAM_PM_OUT8_DROPPED_PACKETS_ID,	   /* 80 Output 8 low priority packets are being dropped */
   GQAM_PM_OUT9_DROPPED_PACKETS_ID,	   /* 81 Output 9 low priority packets are being dropped */
   GQAM_PM_OUT10_DROPPED_PACKETS_ID,	/* 82 Output 10 low priority packets are being dropped */
   GQAM_PM_OUT11_DROPPED_PACKETS_ID,	/* 83 Output 11 low priority packets are being dropped */
   GQAM_PM_OUT12_DROPPED_PACKETS_ID,	/* 84 Output 12 low priority packets are being dropped */
   GQAM_PM_OUT13_DROPPED_PACKETS_ID,	/* 85 Output 13 low priority packets are being dropped */
   GQAM_PM_OUT14_DROPPED_PACKETS_ID,	/* 86 Output 14 low priority packets are being dropped */
   GQAM_PM_OUT15_DROPPED_PACKETS_ID,	/* 87 Output 15 low priority packets are being dropped */
   
   GQAM_HARDWARE_ERROR_ID,           	/* 88 general purpose hardware error */
   GQAM_RUNTIME_ERROR_ID,            	/* 89 general purpose software error */

   GQAM_CFT_EVENT_CHANGE_ID,         	/* 90 craft port event change */
   GQAM_FP_EVENT_CHANGE_ID,          	/* 91 front panel event change */
   
   GQAM_PM_OUT0_FIFO_OVERFLOW_ID,	   /* 92 Output 0 FIFO overflow */
   GQAM_PM_OUT1_FIFO_OVERFLOW_ID,	   /* 93 Output 1 FIFO overflow */
   GQAM_PM_OUT2_FIFO_OVERFLOW_ID,	   /* 94 Output 2 FIFO overflow */
   GQAM_PM_OUT3_FIFO_OVERFLOW_ID,	   /* 95 Output 3 FIFO overflow */
   GQAM_PM_OUT4_FIFO_OVERFLOW_ID,	   /* 96 Output 4 FIFO overflow */
   GQAM_PM_OUT5_FIFO_OVERFLOW_ID,	   /* 97 Output 5 FIFO overflow */
   GQAM_PM_OUT6_FIFO_OVERFLOW_ID,	   /* 98 Output 6 FIFO overflow */
   GQAM_PM_OUT7_FIFO_OVERFLOW_ID,	   /* 99 Output 7 FIFO overflow */
   GQAM_PM_OUT8_FIFO_OVERFLOW_ID,	   /* 100 Output 8 FIFO overflow */
   GQAM_PM_OUT9_FIFO_OVERFLOW_ID,	   /* 101 Output 9 FIFO overflow */
   GQAM_PM_OUT10_FIFO_OVERFLOW_ID,	   /* 102 Output 10 FIFO overflow */
   GQAM_PM_OUT11_FIFO_OVERFLOW_ID,	   /* 103 Output 11 FIFO overflow */
   GQAM_PM_OUT12_FIFO_OVERFLOW_ID,	   /* 104 Output 12 FIFO overflow */
   GQAM_PM_OUT13_FIFO_OVERFLOW_ID,	   /* 105 Output 13 FIFO overflow */
   GQAM_PM_OUT14_FIFO_OVERFLOW_ID,	   /* 106 Output 14 FIFO overflow */
   GQAM_PM_OUT15_FIFO_OVERFLOW_ID,	   /* 107 Output 15 FIFO overflow */
   
   GQAM_RESERVED_ALARM_108_ID,		   /* Start adding additional alarms here */
   GQAM_RESERVED_ALARM_109_ID,
   GQAM_RESERVED_ALARM_110_ID,
   GQAM_RESERVED_ALARM_111_ID,
   GQAM_RESERVED_ALARM_112_ID,
   GQAM_RESERVED_ALARM_113_ID,
   GQAM_RESERVED_ALARM_114_ID,
   GQAM_RESERVED_ALARM_115_ID,
   GQAM_RESERVED_ALARM_116_ID,
   GQAM_RESERVED_ALARM_117_ID,
   GQAM_RESERVED_ALARM_118_ID,
   GQAM_RESERVED_ALARM_119_ID,
   GQAM_RESERVED_ALARM_120_ID,
   GQAM_RESERVED_ALARM_121_ID,
   GQAM_RESERVED_ALARM_122_ID,
   GQAM_RESERVED_ALARM_123_ID,
   GQAM_RESERVED_ALARM_124_ID,
   GQAM_RESERVED_ALARM_125_ID,
   GQAM_RESERVED_ALARM_126_ID,
   GQAM_RESERVED_ALARM_127_ID,		
   GQAM_RESERVED_ALARM_128_ID ,
   GQAM_RESERVED_ALARM_129_ID ,
   GQAM_RESERVED_ALARM_130_ID ,

   GQAM_RESERVED_ALARM_131_ID ,
   GQAM_RESERVED_ALARM_132_ID ,
   GQAM_RESERVED_ALARM_133_ID ,
   GQAM_RESERVED_ALARM_134_ID ,
   GQAM_RESERVED_ALARM_135_ID ,
   GQAM_RESERVED_ALARM_136_ID ,
   GQAM_RESERVED_ALARM_137_ID ,
   GQAM_RESERVED_ALARM_138_ID ,
   GQAM_RESERVED_ALARM_139_ID ,
   GQAM_RESERVED_ALARM_140_ID ,
   GQAM_RESERVED_ALARM_141_ID ,
   GQAM_RESERVED_ALARM_142_ID ,
   GQAM_RESERVED_ALARM_143_ID ,
   GQAM_RESERVED_ALARM_144_ID ,
   GQAM_RESERVED_ALARM_145_ID ,
   GQAM_RESERVED_ALARM_146_ID ,
   GQAM_RESERVED_ALARM_147_ID ,
   GQAM_RESERVED_ALARM_148_ID ,
   GQAM_RESERVED_ALARM_149_ID ,
   GQAM_RESERVED_ALARM_150_ID ,

   GQAM_RESERVED_ALARM_151_ID ,
   GQAM_RESERVED_ALARM_152_ID ,
   GQAM_RESERVED_ALARM_153_ID ,
   GQAM_RESERVED_ALARM_154_ID ,
   GQAM_RESERVED_ALARM_155_ID ,
   GQAM_RESERVED_ALARM_156_ID ,
   GQAM_RESERVED_ALARM_157_ID ,
   GQAM_RESERVED_ALARM_158_ID ,
   GQAM_RESERVED_ALARM_159_ID ,
   GQAM_RESERVED_ALARM_160_ID ,
   GQAM_RESERVED_ALARM_161_ID ,
   GQAM_RESERVED_ALARM_162_ID ,
   GQAM_RESERVED_ALARM_163_ID ,
   GQAM_RESERVED_ALARM_164_ID ,
   GQAM_RESERVED_ALARM_165_ID ,
   GQAM_RESERVED_ALARM_166_ID ,
   GQAM_RESERVED_ALARM_167_ID ,
   GQAM_RESERVED_ALARM_168_ID ,
   GQAM_RESERVED_ALARM_169_ID ,
   GQAM_RESERVED_ALARM_170_ID ,

   GQAM_RESERVED_ALARM_171_ID ,
   GQAM_RESERVED_ALARM_172_ID ,
   GQAM_RESERVED_ALARM_173_ID ,
   GQAM_RESERVED_ALARM_174_ID ,
   GQAM_RESERVED_ALARM_175_ID ,
   GQAM_RESERVED_ALARM_176_ID ,
   GQAM_RESERVED_ALARM_177_ID ,
   GQAM_RESERVED_ALARM_178_ID ,
   GQAM_RESERVED_ALARM_179_ID ,
   GQAM_RESERVED_ALARM_180_ID ,
   GQAM_RESERVED_ALARM_181_ID ,
   GQAM_RESERVED_ALARM_182_ID ,
   GQAM_RESERVED_ALARM_183_ID ,
   GQAM_RESERVED_ALARM_184_ID ,
   GQAM_RESERVED_ALARM_185_ID ,
   GQAM_RESERVED_ALARM_186_ID ,
   GQAM_RESERVED_ALARM_187_ID ,
   GQAM_RESERVED_ALARM_188_ID ,
   GQAM_RESERVED_ALARM_189_ID ,
   GQAM_RESERVED_ALARM_190_ID ,

   GQAM_RESERVED_ALARM_191_ID ,
   GQAM_RESERVED_ALARM_192_ID ,
   GQAM_RESERVED_ALARM_193_ID ,
   GQAM_RESERVED_ALARM_194_ID ,
   GQAM_RESERVED_ALARM_195_ID ,
   GQAM_RESERVED_ALARM_196_ID ,
   GQAM_RESERVED_ALARM_197_ID ,
   GQAM_RESERVED_ALARM_198_ID ,
   GQAM_RESERVED_ALARM_199_ID ,
   GQAM_RESERVED_ALARM_200_ID ,
   GQAM_RESERVED_ALARM_201_ID ,
   GQAM_RESERVED_ALARM_202_ID ,
   GQAM_RESERVED_ALARM_203_ID ,
   GQAM_RESERVED_ALARM_204_ID ,
   GQAM_RESERVED_ALARM_205_ID ,
   GQAM_RESERVED_ALARM_206_ID ,
   GQAM_RESERVED_ALARM_207_ID ,
   GQAM_RESERVED_ALARM_208_ID ,
   GQAM_RESERVED_ALARM_209_ID ,
   GQAM_RESERVED_ALARM_210_ID ,
   GQAM_RESERVED_ALARM_211_ID ,
   GQAM_RESERVED_ALARM_212_ID ,
   GQAM_RESERVED_ALARM_213_ID ,
   GQAM_RESERVED_ALARM_214_ID ,
   GQAM_RESERVED_ALARM_215_ID ,
   GQAM_RESERVED_ALARM_216_ID ,
   GQAM_RESERVED_ALARM_217_ID ,
   GQAM_RESERVED_ALARM_218_ID ,
   GQAM_RESERVED_ALARM_219_ID ,
   GQAM_RESERVED_ALARM_220_ID ,

   GQAM_RESERVED_ALARM_221_ID ,
   GQAM_RESERVED_ALARM_222_ID ,
   GQAM_RESERVED_ALARM_223_ID ,
   GQAM_RESERVED_ALARM_224_ID ,
   GQAM_RESERVED_ALARM_225_ID ,
   GQAM_RESERVED_ALARM_226_ID ,
   GQAM_RESERVED_ALARM_227_ID ,
   GQAM_RESERVED_ALARM_228_ID ,
   GQAM_RESERVED_ALARM_229_ID ,
   GQAM_RESERVED_ALARM_230_ID ,
   GQAM_RESERVED_ALARM_231_ID ,
   GQAM_RESERVED_ALARM_232_ID ,
   GQAM_RESERVED_ALARM_233_ID ,
   GQAM_RESERVED_ALARM_234_ID ,
   GQAM_RESERVED_ALARM_235_ID ,
   GQAM_RESERVED_ALARM_236_ID ,
   GQAM_RESERVED_ALARM_237_ID ,
   GQAM_RESERVED_ALARM_238_ID ,
   GQAM_RESERVED_ALARM_239_ID ,
   GQAM_RESERVED_ALARM_240_ID ,

   GQAM_RESERVED_ALARM_241_ID ,
   GQAM_RESERVED_ALARM_242_ID ,
   GQAM_RESERVED_ALARM_243_ID ,
   GQAM_RESERVED_ALARM_244_ID ,
   GQAM_RESERVED_ALARM_245_ID ,
   GQAM_RESERVED_ALARM_246_ID ,
   GQAM_RESERVED_ALARM_247_ID ,
   GQAM_RESERVED_ALARM_248_ID ,
   GQAM_RESERVED_ALARM_249_ID ,
   GQAM_RESERVED_ALARM_250_ID ,

   GQAM_RESERVED_ALARM_251_ID ,
   GQAM_RESERVED_ALARM_252_ID ,
   GQAM_RESERVED_ALARM_253_ID ,
   GQAM_RESERVED_ALARM_254_ID ,

   
   GQAM_RESERVED_ALARM_255_ID = 255,	/* Last non-session alarm */	
  
   /* The following alarm ID groups are session related (1024 IDs each) */ 
   GQAM_PM_SESSION_DATA_ERROR_ID =  256,  /* GQAM_RESERVED_ALARM_255_ID + 1 */ 
   GQAM_TM_SESSION_PROG_ERROR_ID = 1280,  /* GQAM_PM_SESSION_DATA_ERROR_ID + (MAX_SESSIONS_PER_UNIT) */ 
   GQAM_CA_SESSION_CA_ERROR_ID   = 2304,  /* GQAM_TM_SESSION_PROG_ERROR_ID + (MAX_SESSIONS_PER_UNIT) */ 
   GQAM_NP_SESSION_GE_ERROR_ID   = 3328,  /* GQAM_CA_SESSION_CA_ERROR_ID + (MAX_SESSIONS_PER_UNIT) */
    
   /* end of list identifier */
   qgqam_end_of_alarm_ids        = 4352   /* GQAM_NP_SESSION_GE_ERROR_ID + (MAX_SESSIONS_PER_UNIT) */ 
};

const  GQAM_FIRST_ALARM_ID               = GQAM_MOD_SYSALARM_ID ; 
const  GQAM_FIRST_PM_ALARM_ID            = GQAM_PM_LOSS_OF_SIGNAL_ID ; 
const  GQAM_LAST_PM_ALARM_ID             = GQAM_RUNTIME_ERROR_ID ; 
const  GQAM_FIRST_NON_SESSION_ALARM_ID   = GQAM_MOD_SYSALARM_ID ; 
const  GQAM_LAST_NON_SESSION_ALARM_ID    = GQAM_RESERVED_ALARM_127_ID ; 

/* Session alarms encode addition information in qalarmCustom[16] */
/* Bytes 0 - 9 contain the 10-byte external session number */
/* Byte 13 contains the output port number session resides on (0-3) */
/* Byte 14 contains the input port number session gets its data from (0-1) */
/* Byte 15 contains the cause code for active session alarms (described below) */

enum qSessionAlarmDataCauseCodesGqam
{
  data_underflow_gqam = 1,
  data_overflow_gqam,
  data_pid_enable_error_gqam,
  data_drop_scheduled_gqam,
  data_overflow_and_drop_scheduled_gqam,
  data_continuity_gqam,
  data_pll_unlocked_gqam,
  data_glue_events_gqam
};

enum qSessionAlarmProgCauseCodesGqam
{
  prog_pmt_crc_error_gqam = 1,
  prog_pmt_update_gqam,
  prog_session_create_error_gqam
};

enum qSessionAlarmCACauseCodesGqam
{
  ca_general_purpose_gqam = 0
};


enum qSessionAlarmGigaEtherCauseCodesGqam
{
  ge_excess_jitter_gqam = 1
};


/****************************************************************************

  void qqueryPerfStatsGqam( qQueryPerfStatsGqam_param) 
  void qqueryPerfStatsGqamResponse( qQueryPerfStatsGqamResponse_param )   
         This call allows the element manager to query the performance
    status of the GQAM.
 
****************************************************************************/

/* Performance identifers */

enum qPerfIDGqam
{
   goc0plus12Voltage = 0, 		/* Measured voltage levels */
   goc0plus5Voltage,
   goc0plus33Voltage,
   goc0minus5Voltage,
   goc0temperature,
   goc1plus12Voltage,
   goc1plus5Voltage,
   goc1plus33Voltage,
   goc1minus5Voltage,
   goc1temperature,
   goc2plus12Voltage,
   goc2plus5Voltage,
   goc2plus33Voltage,
   goc2minus5Voltage,
   goc2temperature,
   goc3plus12Voltage,
   goc3plus5Voltage,
   goc3plus33Voltage,
   goc3minus5Voltage,
   goc3temperature,
	
	gpmin0lossofsignal,		   /* Alarm counters */			
	gpmin0erroredmpegpackets,
	gpmin0fifooverflow,
	gpmin0dumpedpackets,
	gpmin1lossofsignal,
	gpmin1erroredmpegpackets,
	gpmin1fifooverflow,
	gpmin1dumpedpackets,
	gpmin2lossofsignal,
	gpmin2erroredmpegpackets,
	gpmin2fifooverflow,
	gpmin2dumpedpackets,
	gpmin3lossofsignal,
	gpmin3erroredmpegpackets,
	gpmin3fifooverflow,
	gpmin3dumpedpackets,
	gpmin4lossofsignal,
	gpmin4erroredmpegpackets,
	gpmin4fifooverflow,
	gpmin4dumpedpackets,
	
	gpmresetdetected,  		/* Alarm counters */
	gpmhardwareerror,
	gpmruntimeerror,
	
	gpmin0conterror, 		   /* Alarm counters */ 		
	gpmin0teierror,
	gpmin1conterror,
	gpmin1teierror,
	gpmin2conterror,
	gpmin2teierror,
	gpmin3conterror,
	gpmin3teierror,
	gpmin4conterror,
	gpmin4teierror,
	
	gpmin0inputflow,  		/* Total bit rate per input */
	gpmin1inputflow,
	gpmin2inputflow,
	gpmin3inputflow,
	gpmin4inputflow,
	
	gpmout0outputflow,  		/* Total bit rate per output */
	gpmout1outputflow,
	gpmout2outputflow,
	gpmout3outputflow,
	gpmout4outputflow,
	gpmout5outputflow,
	gpmout6outputflow,
	gpmout7outputflow,
	gpmout8outputflow,
	gpmout9outputflow,
	gpmout10outputflow,
	gpmout11outputflow,
	gpmout13outputflow,
	gpmout14outputflow,
	gpmout15outputflow,
	gpunused1,                                  
	
	gpmout0droppedpackets,		/* Alarm counters */	
	gpmout1droppedpackets,	
	gpmout2droppedpackets,	
	gpmout3droppedpackets,
	gpmout4droppedpackets,	
	gpmout5droppedpackets,	
	gpmout6droppedpackets,	
	gpmout7droppedpackets,
	gpmout8droppedpackets,	
	gpmout9droppedpackets,	
	gpmout10droppedpackets,	
	gpmout11droppedpackets,
	gpmout12droppedpackets,
	gpmout13droppedpackets,	
	gpmout14droppedpackets,	
	gpmout15droppedpackets,	
	
	gpmout0fifooverflow,  		/* Alarm counters */
	gpmout1fifooverflow,	
	gpmout2fifooverflow,	
	gpmout3fifooverflow, 
	gpmout4fifooverflow,	
	gpmout5fifooverflow,	
	gpmout6fifooverflow,	
	gpmout7fifooverflow, 
	gpmout8fifooverflow,	
	gpmout9fifooverflow,	
	gpmout10fifooverflow,	
	gpmout11fifooverflow, 
	gpmout12fifooverflow,	
	gpmout13fifooverflow,	
	gpmout14fifooverflow,	
	gpmout15fifooverflow, 
	   
	qEndPerfIdGqam
};


const  GQAM_FIRST_MOD_qPerfID   = goc0plus12Voltage ; 
const  GQAM_LAST_MOD_qPerfID    = goc3temperature ;     
const  GQAM_FIRST_PM_qPerfID    = gpmin0lossofsignal ; 
const  GQAM_LAST_PM_qPerfID     = gpmout3fifooverflow ; 
const  GQAM_FIRST_qPerfID       = goc0plus12Voltage ; 
const  GQAM_LAST_qPerfID        = gpmout3fifooverflow ; 


struct qQueryPerfStatsGqam_param
{
  qXactionInfoGqam    qxactionInfo;          /* asynchronous RPC header */           
  qPerfIDGqam         qperfID;               /* performance stat identifier */
  unsigned long       qcount;                /* how many to return */
  bool                qreset;                /* reset value after query */
};


struct qPerfStatsQueryGqam
{
  qPerfIDGqam         qperfID;               /* performance stat identifier */
  unsigned long       qperfValue;            /* value for performance stat */
};

struct qQueryPerfStatsGqamResponse_param
{
  qXactionResponseGqam    qresponseInfo;     /* asynchronous RPC header */   
  qPerfStatsQueryGqam     qstats<>;          /* array of perf ids and values */
};

    

/****************************************************************************/
/****************************************************************************/

#endif

/* $Log:   /pvcs/data/arc/pegasus/gqam/sw/ppc/app/rpc.x_v  $
 * 
 *    Rev 1.14   14 Sep 2005 17:16:14   buchen
 * rework giga Ethernet type enum
 * add no PID remap bool to session create and query session response
 * add session type uchar to session create and query session response
 * 
 *    Rev 1.13   03 Aug 2005 16:47:46   buchen
 * added GQAM_ERROR_RPC_INSUFFICIENT_OUTPUT_CAPACITY result code used when the maximum number of
 * sessions per output port has been violated and is returned in the create session response 
 * 
 *    Rev 1.12   01 Aug 2005 12:07:08   buchen
 * incorporate version approach
 * obsolete some procedures and parameters for version 2
 * distribute to the DNCS team
 * remove extended session create 
 * remove extended session query response
 * 
 *    Rev 1.11   22 Jul 2005 09:46:36   buchen
 * removed IP address from create session extended parameters
 * removed create session extended response proc, create session response will be used
 * added session query extended response to house new parameters
 * added create session group SDB procedure pair
 * added delete session group SDB procedure pair
 * compiles with rpcgen
 * 
 *    Rev 1.10   13 Jul 2005 12:56:16   menons
 * Extended Create Session support for mulitcast IP / Source Addresses and qisEncrypted enum ( continous feed sessions )
 * 
 *    Rev 1.9   01 Apr 2005 02:18:44   menons
 * GigE IP address provisioning from DNCS
 * 
 *    Rev 1.8   19 Mar 2003 19:59:58   baldwiw
 * Add enums for new cause codes for session data alarms
 * 
 *    Rev 1.7   09 Sep 2002 16:34:02   buchen
 * change qmqam_end_of_alarm_ids to qgqam_end_of_alarm_ids
 * 
 *    Rev 1.6   08 Aug 2002 08:55:24   buchen
 * add a few more GQAM specific names
 * 
 *    Rev 1.5   11 Jun 2002 09:02:34   buchen
 * fix typos
 * 
 *    Rev 1.4   06 Jun 2002 15:02:26   buchen
 * Fixed keyword expansion
 *
 *    Rev 1.3   06 Jun 2002 13:26:46   buchen
 * rename all first order definitions to be GQAM specific to assist DNCS builds
 * definitions will be redefined in the file rpc_map.h to reusable definitions
 *
 *    Rev 1.2   08 May 2002 12:49:34   buchen
 * made change to qGigaEtherTypes to remove qIP_address and add qUDP_socket_no_program_num
 * 
 *    Rev 1.1   02 Apr 2002 10:53:28   buchen
 * first cut at the GQAM x-file
 *
 *    Rev 1.0   11 Jan 2002 14:10:56   buchen
 * Initial revision.
 * 
*/
