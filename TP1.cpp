//==============================================================================
// Title:       TP1.cpp
// 
// Description: This simple application template contains simple simulated tests using DPS12, DD48 and RF APIs.

// 
// Copyright (c) 2015 Test Evolution Inc.
// All Rights Reserved
//  MRL	 1	08/01/2015	Initial issue using MLog 


#include "stdafx.h"
#include "TP1.h"
#include "MLog.h"

#include "AXIeSys.h"
#include "AXIAPI.h"
#include "DDClass.h"
#include "TevPlot.h"
#include "TevShmoo.h"

// Global Defines ==============================================================
#define MAX_PATHNAME_LEN 260
#define TEST_FUNCT __declspec(dllexport)

#define MAX_SITES 2		

#define APPLICATION_NAME "TP1" 

// Global Variables ============================================================

BOOL gSimulator;

void MLog_UserDisableSite(UINT siteNumber) // Must be defined in user code
{	
	// User should disable hardware for site here ( 1 based)
	// When a site is Disabled the DPS12 channels are gated off automatically. See VISiteDisableCallback()
	// User must ensure DD48 channels are disconnected.

	
}

static void OnAPIOrSystemError (int errorNumber, const char* message)
{   // Break here to stop on API error.
}

static void VISiteDisableCallback (const char* mvp)
{	// mvp (pin) for disabled site is passed in. Example "VBAT@2"
    TevVI_Gate (mvp, "Off");
}


bool MLog_CustomizeDlogFileName (char*	dlogFileName,  char* ext) // Must be defined in user code
{
	// User callback from AxiDutStart to set a customized dlog file name
	// pass in the file extenstion for the filename. (examples: "dlg", "stdf", "txt")
	// should be modified for each customer preference
	SYSTEMTIME	sysTime;
	char timeStamp[64];
	const char *dlogPath;

	
	dlogPath = TevExec_GetDlogPath();
	
	GetLocalTime(&sysTime);
	sprintf_s(timeStamp, sizeof (timeStamp), 
		"%0.2u-%0.2u-%0.4u_%0.2u%0.2u%0.2u",
		sysTime.wMonth, sysTime.wDay, sysTime.wYear,\
		sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
	TevTester_SetRunInfoString("LotTimestamp", timeStamp);

	const char* lotId = TevTester_GetRunInfoString  ("LotId");
	const char* testerID = TevTester_GetRunInfoString  ("Tester ID");
	const char* subLotNo = TevTester_GetRunInfoString  ("SublotNo");		
	const char* testCode = TevTester_GetRunInfoString  ("Test Code");
	
	sprintf_s(dlogFileName,  sizeof (char)*256, "%s%s_%s_%s_%s_%s_%s.%s", dlogPath, testerID, APPLICATION_NAME, lotId,subLotNo,testCode, timeStamp, ext);
	return true;
	
}


void MLog_SetUserProductionVariables(void) // Must be defined in user code
{	
	// User callback from AxiDutStart to set User Production Variables for datalogging
		MLog_SetOperatorName(TevTester_GetRunInfoString  ("Operator"));
		MLog_SetSubLotId	(TevTester_GetRunInfoString  ("SublotNo"));
		MLog_SetTestCode	(TevTester_GetRunInfoString  ("Test Code"));
		MLog_SetTesterSerialNumber(TevTester_GetRunInfoString  ("Tester ID"));
		MLog_SetLoadboardId (TevTester_GetRunInfoString  ("LoadBoardID"));
		MLog_SetPartType    (TevTester_GetRunInfoString  ("Device Name"));
		MLog_SetModeCode    (TevTester_GetRunInfoString  ("Mode Code"));
		MLog_SetFabProcessId(TevTester_GetRunInfoString  ("FabricationID"));
		MLog_SetJobRev ("1.0");	
}


// User Generic Functions =========================================================


DDPattern DD;

double GetRandomResult(double minLimit, double maxLimit)
{

	double delta, error, maxResult ,minResult;

	delta = maxLimit - minLimit;

	error = delta * 0.005;

	maxResult = maxLimit + error;
	minResult = minLimit - error;
	
	double result = (double)rand() / (RAND_MAX) * (maxResult - minResult)+ minResult;

	return result;
}


#define NUM_SITES 1
#define USE_FOCUS_CAL 1
#define DO_FOCUS_CAL 0

// System Generic Functions ======================================================

// ApplicationLoad 
// This routine gets called from Application Load sequence step
TEST_FUNCT void ApplicationLoad(void)
{
	int rc;	
	char dllDirPath[MAX_PATHNAME_LEN];

	TEV_STATUS	status = AXI_SUCCESS, apiStatus = AXI_SUCCESS;

 	char FocusCalFileName[1000];
	char MVPFileName[1000];


	gSimulator = !TevTester_IsPresent();
	GetTestFunctionDllPath (dllDirPath, sizeof (dllDirPath));	// Gets working directory of DLL  

	if (!TevTester_IsMVPDataLoaded())
    {   // When not using OpenExec (TestStand stand alone).
      
        if (NUM_SITES == 2)
		{
			sprintf_s(MVPFileName,"%sTP1_DualSite.mvp",dllDirPath);
		
		}
		else
		{			
			sprintf_s(MVPFileName,"%sTP1.mvp",dllDirPath);			
		}
        rc = TevExec_LoadPinMap (MVPFileName);    // Default MVP for direct TestStand usage
        if (rc < 0)
        { 
			char str[1000];
			sprintf_s(str,"MVP File: %s not found!",MVPFileName);
            TevUtil_TraceMessage(str);
        }
		else
		{
			char str[1000];
			sprintf_s(str,"MVP File: %s",MVPFileName);
			TevUtil_TraceMessage(str);
		}

    }
	
	int	numSites = TevTester_NumberOfSites();	// Total active and inactive
		
	if (USE_FOCUS_CAL && !gSimulator)
	{
		sprintf_s(FocusCalFileName,"%sPatterns\\FocusedCalibrationFile.tdr",dllDirPath);
  
		tdrHandle focusHandle = TevDD_TDRCreateNewHandle ();

		if (DO_FOCUS_CAL)
		{

		int focusCalSts  = TevDD_TDRMeasure ("ALL_PINS", focusHandle );
		if (focusCalSts == 0)
			TevDD_TDRSave     (focusHandle ,FocusCalFileName ); 
		else
			MessageBox (NULL, "Focus Calibration Measure Error", APPLICATION_NAME,   MB_OK + MB_SYSTEMMODAL + MB_TOPMOST);

		 rc = TevExec_LoadPinMap (MVPFileName);   // reload MVP file 
		}

		int focusCalSts = TevDD_TDRLoad (FocusCalFileName, focusHandle);
		if (focusCalSts != 0)
			MessageBox (NULL, "Focus Calibration Load Error", APPLICATION_NAME,   MB_OK + MB_SYSTEMMODAL + MB_TOPMOST);
		else
			TevDD_TDREnable ("ALL_PINS", focusHandle);
	}

	TevUtil_SetTraceMode ("IMMEDIATE", MESSAGE_TRACE_DEBUG);	

	TevUtil_TraceMessage ("    Loading Program...");

	// Setup Datalogging Streams
	TevTester_RunInfoAdd       ("Immediate Mode Datalog", "Boolean", NULL);
	TevTester_RunInfoAdd       ("STDF Mode Datalog", "Boolean", NULL);
	TevTester_RunInfoAdd       ("TEXT Mode Datalog", "Boolean", NULL);

	TevTester_SetRunInfoBoolean("Immediate Mode Datalog", TRUE);	
    TevTester_SetRunInfoBoolean("STDF Mode Datalog", TRUE);	
    TevTester_SetRunInfoBoolean("TEXT Mode Datalog", TRUE);	


    // Initialized Datalog Engine
    MLog_OnProgramLoad(APPLICATION_NAME); 

    rc = TevError_NotificationCallback (OnAPIOrSystemError);

	char patternPath[256];
	status = AXI_SUCCESS;

	// Load Digital Patterns	
	sprintf_s(patternPath,"%sPatterns\\",dllDirPath);
		
	DD.Init();
	DD.LoadPatterns(patternPath, "PATS.txt");

	
	// Check if DUT Site Power is on. Turn on if Not if is not.
	TevDiag_DiagnosticStatement (0, "Checking for DUT Site Power");
    int dutPower = 0;
	while(!dutPower)
    {
        dutPower = TevTester_IsDutSitePoweredOn ();
        if (! dutPower)
		{
           TevExec_EnableDutSitePower(TRUE);
		}
	}
    
	if (!TevTester_IsDutSitePoweredOn())	// abort!
	{
		rc = ERROR_DUTPowerNotEnabled;
		return;
	}

	
	// *********************************************************************
	// "Variables" tab in OpenExec. These are disabled for "Operator" mode.
	// Must be one of the following:
	//	"Enum", " Double ", "Single", "Int32", "Int16", "Boolean", "Char", "String"
	// *********************************************************************
	// Always declare these
	TevTester_VariableAdd       ("FastBinning", "Boolean", NULL);	
	TevTester_SetVariableBoolean("FastBinning", FALSE);
	TevTester_VariableAdd       ("StopOnFirstFail", "Boolean", NULL);
	TevTester_SetVariableBoolean("StopOnFirstFail", FALSE);

	TevTester_VariableAdd       ("1 of N Filter", "Boolean", NULL);
	TevTester_SetVariableBoolean("1 of N Filter", FALSE);
	TevTester_VariableAdd       ("1 of N 'N'", "Int16", NULL);
	TevTester_SetVariableInt16	("1 of N 'N'", 3);

	
	// Production Variables for Test Program; GUI <=> test program
	// LotId Production Variable required by Open Exec for Lot management

	// Setup Production Variables for Operator Interface
    TevTester_AddProductionVariable  ("LotId",			 "String", "");
    TevTester_AddProductionVariable  ("SublotNo",        "String", "");
    TevTester_AddProductionVariable  ("Operator",        "String", "");
	TevTester_AddProductionVariable  ("Test Code",       "String", "");
	TevTester_AddProductionVariable  ("Tester ID",       "String", "");
	TevTester_AddProductionVariable  ("LoadBoardID",     "String", "");
	TevTester_AddProductionVariable  ("Device Name",     "String", "");
	TevTester_AddProductionVariable  ("Program Name",    "String", "");
	TevTester_AddProductionVariable  ("Mode Code",       "String", "");
	TevTester_AddProductionVariable  ("FabricationID",   "String", "");

	//Initial Settings of Production Variables
	TevTester_SetProductionVariable  ("LotId", "LOT88"); 
 	TevTester_SetProductionVariable  ("Program Name", "SimpleLimits");
 	TevTester_SetProductionVariable  ("Tester ID", "ASG0136");
 	TevTester_SetProductionVariable  ("SublotNo", "SUB123");
 	TevTester_SetProductionVariable  ("Test Code", "MRL");	
    TevTester_SetProductionVariable  ("Operator",     "Operator");	
	TevTester_SetProductionVariable  ("LoadBoardID",  "LB5");
	TevTester_SetProductionVariable  ("Device Name",  "PA99");
	TevTester_SetProductionVariable  ("Program Name", "SimpleLimitsTemplate");
	TevTester_SetProductionVariable  ("Mode Code",    "S");
	TevTester_SetProductionVariable  ("FabricationID","AAA");
   

	TevTester_RunInfoAdd        ("Prompt_SN", "Boolean", NULL);
	TevTester_SetRunInfoBoolean ("Prompt_SN", FALSE);
	TevTester_RunInfoAddHidden  ("LotTimestamp",    "String", "");

	// To avoid Prober/Handler "Variable Name 'X & Y', not found."
	TevTester_RunInfoAddHidden  ("X",                 "Int32",  "");
	TevTester_RunInfoAddHidden  ("Y",                 "Int32",  "");
	TevTester_SetRunInfoInt32   ("X", 1);
	TevTester_SetRunInfoInt32   ("X", 2);

	// Binning
	TevTester_SetNumberOfTestBins (32);				// 2-256 bins

	//			soft, hard, bin
	//			 bin, bin,  name,  bin_type
	MLog_SetupBin( 1,  1, "Bin1",  MLOG_BIN_PASS);	
	MLog_SetupBin( 2,  5, "Bin2",  MLOG_BIN_FAIL);	
	MLog_SetupBin( 3,  5, "Bin3",  MLOG_BIN_FAIL);	
	MLog_SetupBin( 4,  5, "Bin4",  MLOG_BIN_FAIL);	
	MLog_SetupBin( 5,  5, "Bin5",  MLOG_BIN_FAIL);	
	MLog_SetupBin( 6,  5, "Bin6",  MLOG_BIN_FAIL);	
	MLog_SetupBin( 7,  5, "Bin7",  MLOG_BIN_FAIL);	
	MLog_SetupBin( 8,  5, "Bin8",  MLOG_BIN_FAIL);	
	MLog_SetupBin( 9,  5, "Bin9",  MLOG_BIN_FAIL);	
	MLog_SetupBin(10,  5, "Bin10", MLOG_BIN_FAIL);	
	MLog_SetupBin(11,  5, "Bin11", MLOG_BIN_FAIL);	
	MLog_SetupBin(12,  5, "Bin12", MLOG_BIN_FAIL);	
	MLog_SetupBin(13,  5, "Bin13", MLOG_BIN_FAIL);	
	MLog_SetupBin(14,  5, "Bin14", MLOG_BIN_FAIL);	
	MLog_SetupBin(15,  5, "Bin15", MLOG_BIN_FAIL);	
	MLog_SetupBin(16,  5, "Bin16", MLOG_BIN_FAIL);	
	MLog_SetupBin(17,  5, "Bin17", MLOG_BIN_FAIL);	
	MLog_SetupBin(18,  5, "Bin18", MLOG_BIN_FAIL);	
	MLog_SetupBin(19,  5, "Bin19", MLOG_BIN_FAIL);	
	MLog_SetupBin(20,  5, "Bin20", MLOG_BIN_FAIL);	
	MLog_SetupBin(21,  5, "Bin21", MLOG_BIN_FAIL);	
	MLog_SetupBin(22,  5, "Bin22", MLOG_BIN_FAIL);	
	MLog_SetupBin(23,  5, "Bin23", MLOG_BIN_FAIL);	
	MLog_SetupBin(24,  5, "Bin24", MLOG_BIN_FAIL);	
	MLog_SetupBin(25,  5, "Bin25", MLOG_BIN_FAIL);	

	MLog_SetupBin(26,  5, "Bin26", MLOG_BIN_FAIL);	
	MLog_SetupBin(27,  5, "Bin27", MLOG_BIN_FAIL);	
	MLog_SetupBin(28,  5, "Bin28", MLOG_BIN_FAIL);	
	MLog_SetupBin(29,  5, "Bin29", MLOG_BIN_FAIL);	
	MLog_SetupBin(30,  5, "Bin30", MLOG_BIN_FAIL);	
	MLog_SetupBin(31,  5, "Bin31", MLOG_BIN_FAIL);	
	MLog_SetupBin(32,  5, "Bin32", MLOG_BIN_FAIL);	


	MLog_Set1ofNFilter(false , 3);	
	// Use PIN_MAJOR so DD48 and DPS12 use same order.
	MVP_SetDataOrder(PIN_MAJOR);	// Order of data returned in measurement arrays for MVP pins and groups.

	TevVI_SetDisableSiteCallback(VISiteDisableCallback);
}


// ApplicationUnload 
// This routine gets called from Application Unload sequence step
// It is called once when the sequence is first executed
TEST_FUNCT void ApplicationUnload(void)
{
	// Add Test Program Code Here 
    TEV_STATUS apiErr = TevError_NotificationCallback (NULL);
    MLog_OnProgramUnload();
}


// ApplicationStart 
// This routine gets called from Application Start sequence step
TEST_FUNCT void ApplicationStart(void)
{
   
	BOOL fastBinning = TevTester_GetVariableBoolean("FastBinning");
    MLog_SetFastBinning (fastBinning);

	BOOL stopFirstFail = TevTester_GetVariableBoolean("StopOnFirstFail");
    MLog_SetStopOnFirstFail (stopFirstFail);
  
	BOOL filter = TevTester_GetVariableBoolean("1 of N Filter");
	UINT N = TevTester_GetVariableInt16("1 of N 'N'");
	MLog_Set1ofNFilter(filter , N);
	

// Start Device Dlog
	MLog_OnStartDut();

}

// ApplicationEnd 
// This routine gets called from Application End sequence step
TEST_FUNCT void ApplicationEnd(void)
{

	// POWER OFF Device
	TevDD_DriverMode("ALL_PINS", "DriveSense");
	TevDD_StaticDrive ("ALL_PINS", "Z");
	TevDD_DataMode ("ALL_PINS", "Dynamic");
	TevDD_Disconnect("ALL_PINS");
	TevVI_ForceVoltage("VI_1P8_SUPPLY", 1.8 V, 5.0);
	TevVI_ForceVoltage("VI_1P2_SUPPLY", 1.2 V, 5.0);
	TevVI_SetVoltage("ALL_VI",  0, 25 mA ,  5 V, 25 mA,  -25 mA);	
	TevVI_Gate        ("ALL_VI", TEVVI_VAL_OFF);
	TevVI_Disconnect  ("ALL_VI");	
}

TEST_FUNCT int PowerOn(int testNumber)
{
	
	char str[128];

	sprintf_s(str, "PowerOn : %4d : %4d", testNumber);
	TevUtil_TraceMessage(str); 

	double expectedVoltages[4];

   	MLOG_CheckFastBinning

	UINT passMask = 0xFFFF; // 1 bit per site
 
	TevVI_SetVoltage  ("ALL_VI",  0, 1 A ,  5 V, 1 A,  -1 A);	
	TevVI_ConnectDUT  ("ALL_VI", TEVVI_VAL_REMOTE_SENSE);
	TevVI_Gate        ("ALL_VI", TEVVI_VAL_ON);
		

	TevVI_ForceVoltage("VI_1P2_SUPPLY", 1.2 V, 5.0);
	TevWait_Now(10 ms);
	TevVI_ForceVoltage("VI_1P8_SUPPLY", 1.8 V, 5.0);
	TevWait_Now(10 ms);
	TevVI_ForceVoltage("VI_3P3_SUPPLY", 3.3 V, 5.0);
	TevWait_Now(10 ms);

	// *** Must set an active pattern before *any* DD192 commands ***
	DD.SetActivePattern(0); 

	TevDD_DriverMode("ALL_PINS", "DriveSense");
	TevDD_StaticDrive ("ALL_PINS", "0");
	TevDD_DataMode ("ALL_PINS", "Dynamic");
	TevDD_Connect("ALL_PINS", "DUT");


	TevWait_Now(100 ms);
	MLog_Header (testNumber, "Power Up PMU", "");
	expectedVoltages[0] = 1.2; //VDD1p2
	expectedVoltages[1] = 1.8; //VDD1p8
	expectedVoltages[2] = 3.3; //VDD3p3
	expectedVoltages[3] = 3.3; //VBatt

	double minLimitV = 0;
	double maxLimitV = 0;

	int channels = TevTester_NumberOfPins("ALL_VI");	// Per site!
	double results_V[4*MAX_SITES]; 
	double results_I[4*MAX_SITES];
	double minLimitI = -100 mA;
	double maxLimitI = 500 mA;

	
	TevVI_MeasureCurrent("ALL_VI" , results_I, sizeof (results_I));
	TevVI_MeasureVoltage("ALL_VI" , results_V, sizeof (results_V));
	
	if (gSimulator)
	{
		int i = 0;
		for (int v = 0; v < 4; v ++)
		{
			MLOG_ForEachEnabledSite(siteIndex)
			{
				results_V[i] = GetRandomResult(expectedVoltages[v]- 100 mV,expectedVoltages[v] + 100 mV);
				results_I[i++] = GetRandomResult(minLimitI, maxLimitI);
			}
		}
		
	}
	
	for (int subTestIndex = 0; subTestIndex < channels; subTestIndex++)
	{
		char pinName[128];
		double channelResults[2];
		strcpy_s(pinName, TevTester_GetPinName("ALL_VI",subTestIndex));
		minLimitV = expectedVoltages[subTestIndex] - 100 mV;
		maxLimitV = expectedVoltages[subTestIndex] + 100 mV;
		MLOG_ForEachEnabledSite(siteIndex)
			channelResults[siteIndex] = MVP_GetResult("ALL_VI",results_V, siteIndex, subTestIndex);
		passMask  &= MLog_Datalog  (pinName, AUTONUM, "VOLTAGE", channelResults, "V", minLimitV, maxLimitV, "", "Bin2");
	}
	passMask  &= MLog_Datalog  ("ALL_VI", AUTONUM, "CURRENT", results_I, "mA", minLimitI, maxLimitI, "", "Bin2");
	
	MLOG_DisableFailedSites

    return MLOG_SITES_PASS(passMask);
}


TEST_FUNCT int PatternTests( int startingTestNumber, int testNumberIncrement)
{
   	MLOG_CheckFastBinning

	bool RunPat=false;
	char str[1024];
	int testNumber;

	UINT passMask = 0xFFFF; // 1 bit per site

	for (int patternIndex = 0; patternIndex < DD.GetNumPatterns(); patternIndex++)
	{
	     testNumber = startingTestNumber + patternIndex * testNumberIncrement;

		sprintf_s(str, "PatternTest : %4d %12s : %d", testNumber, DD.GetPatternName(patternIndex), patternIndex);
		TevUtil_TraceMessage(str); 
   

		if (! DD.IsPatternLoaded(patternIndex))
		{
				char buffer[200];
				double results[MAX_SITES] = {0.0};
				sprintf_s(buffer, "%s Pattern Tests",DD.GetPatternName(patternIndex));
				int subTestNumber = 1;
				MLog_Header (testNumber, buffer, "");
				MLog_LogState( DD.GetPatternName(patternIndex), 1, "Pattern Not Loaded", results, "", "Pattern Skipped", "Bin3");
				continue;
		}
		
		DD.SetActivePattern(patternIndex);
	
		int apiErr = 0;
			
		apiErr |= TevDD_HramSetup("StoreFirst", "StoreVectors", "StoreFailures", "FailContinue");
		apiErr |= TevDD_HramTrigger("TriggerCycle", 0);
	
		if (strcmp(DD.GetPatternName(patternIndex),"JTAG.DO") == 0) 
		{
			DD.ConnectPatternTrigger("DD_DStar_A", "PFI0", FALSE);
			DD.ConnectPinTrigger("GPIO_6","DD_DStar_B","PFI1",FALSE);
			DD.ConnectPinTrigger("GPIO_4","DD_DStar_C","PFI2",FALSE);
			DD.ConnectPinTrigger("GPIO_0","DD_DStar_D","PFI3",FALSE);
		}
	
	
		DD.DisableFailMask(patternIndex);
	
		DD.Run(patternIndex, "START");

		passMask &= DD.Log(testNumber+1, patternIndex, "Bin3");
		passMask &=	DD.LogPinFailures(testNumber+2, patternIndex, "Bin3", false);
				
	}

	TevDD_TriggerDisableAll();
	DD.DisconnectPatternTrigger("DD_DStar_A", "PFI0");
	DD.DisconnectPinTrigger("DD_DStar_B","PFI1");
	DD.DisconnectPinTrigger("DD_DStar_C", "PFI2");
	DD.DisconnectPinTrigger("DD_DStar_D", "PFI3");
		
	TevTrigger_DisconnectAllTerminals();

	

	MLOG_DisableFailedSites
		
    return MLOG_SITES_PASS(passMask);
}

#define SHMOO
#ifdef SHMOO


#define FUNC_PERIOD_2 10 ns

int ShmooStrobeLevel(int xstepIndex, int yStepIndex, double xStep, double yStep, double* result)
{

	int pass = 1;

	int patternIndex = DD.GetPatternIndex("FUNCTIONAL.DO");

	//xStep = xStep/100  * FUNC_PERIOD_2; 
	xStep = xStep * 1 ns; 

	TevDD_EdgeSetModify ("EDGE_FUNC_2A","Strobe",0.0,xStep);
	TevDD_SetLevel ("GPIO_8","VOH", yStep);   
	TevDD_SetLevel ("GPIO_8","VOL", yStep);   
	DD.Run(patternIndex, "START");

    int numFails = DD.GetVectorFails(1);

    if (numFails)
		pass = 0;
	else
		pass = 1;

	*result = numFails;

	return pass;

}


TEST_FUNCT int ShmooFunctionalPattern(void)
{
   	MLOG_CheckFastBinning

	bool RunPat=false;
	char str[1024];
	int testNumber;

	int patternIndex = DD.GetPatternIndex("FUNCTIONAL.DO");

	sprintf_s(str, "Shmoo : %12s : %d", DD.GetPatternName(patternIndex), patternIndex);
	TevUtil_TraceMessage(str); 
   

	if (! DD.IsPatternLoaded(patternIndex))
	{
			sprintf_s(str, "Shmoo : %12s Pattern Not Loaded Shmoo Aborted", DD.GetPatternName(patternIndex));
			TevUtil_TraceMessage(str); 
			return 0;
	}
		
	DD.SetActivePattern(patternIndex);
	
	int apiErr = 0;
			
	apiErr |= TevDD_HramSetup("StoreFirst", "StoreVectors", "StoreFailures", "FailContinue");
	apiErr |= TevDD_HramTrigger("TriggerCycle", 0);
	
	//DD.DisableFailMask(patternIndex);
	apiErr |= TevDD_FailMask ("ALL_PINS","Enabled");
	apiErr |= TevDD_FailMask ("GPIO_8","Disabled");
	
	DD.Run(patternIndex, "START");

	double period = 10; //ns
	TevShmoo_SetTitle("GPIO_8 Data Eye");
	TevShmoo_SetAxisLabel(SHMOO_X_AXIS, "EDGE_FUNC_2A Strobe (ns)");
	TevShmoo_SetAxisLabel(SHMOO_Y_AXIS, "GPIO_8 VOH (V)");
	TevShmoo_SetStartValue(SHMOO_X_AXIS, 0.0);
	TevShmoo_SetStopValue(SHMOO_X_AXIS, period ); // ns
	TevShmoo_SetNumberOfSteps(SHMOO_X_AXIS, 25);
	TevShmoo_SetStartValue(SHMOO_Y_AXIS, 0.0);
	TevShmoo_SetStopValue(SHMOO_Y_AXIS, 1.8 );
	TevShmoo_SetNumberOfSteps(SHMOO_Y_AXIS, 25	);
	TevShmoo_SetTestCallback(ShmooStrobeLevel);
	TevShmoo_Display(); 

    // Set things back up to original state
	TevDD_EdgeSetModify ("EDGE_FUNC_2A","Strobe",0 ns,0.75*FUNC_PERIOD_2);
	TevDD_SetLevel ("GPIO_8","VOH", 0.9);   
	TevDD_SetLevel ("GPIO_8","VOL", 0.9);
	
	
    return 1;
}
#endif

// DLL Requirements ========================================================
#ifdef _MANAGED
#pragma managed(push, off)
#endif

BOOL APIENTRY  DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
    return TRUE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif