//////////
//
//	File:		QTActionTargets.c
//
//	Contains:	QuickTime wired sprites target support for QuickTime movies.
//
//	Written by:	Tim Monroe
//
//	Copyright:	� 2001 by Apple Computer, Inc., all rights reserved.
//
//	Change History (most recent first):
//
//	   <2>	 	04/07/01	rtm		added theMovieHasID parameter to QTUtils_GetMovieTargetID
//	   <1>	 	02/28/01	rtm		first file; based on existing QTWiredSpritesJr sample code
//	   
//
//////////

//////////
//
// header files
//
//////////

#include "QTActionTargets.h"


//////////
//
// global variables
//
//////////

extern short 				gAppResFile;						// file reference number for this application's resource file
extern ModalFilterUPP		gModalFilterUPP;					// UPP to our custom dialog event filter


//////////
//
// QTTarg_GetStringFromUser
// Display a dialog box to elicit a string from the user; return a C string that contains the text in the
// dialog box when the user clicks the OK button; otherwise, return NULL.
//
// The caller is responsible for disposing of the pointer returned by this function (by calling free).
//
//////////

char *QTTarg_GetStringFromUser (short thePromptStringIndex)
{
	short 			myItem;
	short 			mySavedResFile;
	GrafPtr			mySavedPort;
	DialogPtr		myDialog = NULL;
	short			myItemKind;
	Handle			myItemHandle;
	Rect			myItemRect;
	Str255			myString;
	char			*myCString = NULL;
	OSErr			myErr = noErr;

	//////////
	//
	// save the current resource file and graphics port
	//
	//////////

	mySavedResFile = CurResFile();
	GetPort(&mySavedPort);

	// set the application's resource file
	UseResFile(gAppResFile);

	//////////
	//
	// create the dialog box in which the user will enter a URL
	//
	//////////

	myDialog = GetNewDialog(kGetStr_DLOGID, NULL, (WindowPtr)-1L);
	if (myDialog == NULL)
		goto bail;

	QTFrame_ActivateController(QTFrame_GetFrontMovieWindow(), false);
	
	MacSetPort(GetDialogPort(myDialog));
	
	SetDialogDefaultItem(myDialog, kGetStr_OKButton);
	SetDialogCancelItem(myDialog, kGetStr_CancelButton);
	
	// set the prompt string	
	GetIndString(myString, kTextKindsResourceID, thePromptStringIndex);

	GetDialogItem(myDialog, kGetStr_StrLabelItem, &myItemKind, &myItemHandle, &myItemRect);
	SetDialogItemText(myItemHandle, myString);
	
	MacShowWindow(GetDialogWindow(myDialog));
	
	//////////
	//
	// display and handle events in the dialog box until the user clicks OK or Cancel
	//
	//////////
	
	do {
		ModalDialog(gModalFilterUPP, &myItem);
	} while ((myItem != kGetStr_OKButton) && (myItem != kGetStr_CancelButton));
	
	//////////
	//
	// handle the selected button
	//
	//////////
	
	if (myItem != kGetStr_OKButton) {
		myErr = userCanceledErr;
		goto bail;
	}
	
	// retrieve the edited text
	GetDialogItem(myDialog, kGetStr_StrTextItem, &myItemKind, &myItemHandle, &myItemRect);
	GetDialogItemText(myItemHandle, myString);
	myCString = QTUtils_ConvertPascalToCString(myString);
	
bail:
	// restore the previous resource file and graphics port
	MacSetPort(mySavedPort);
	UseResFile(mySavedResFile);
	
	if (myDialog != NULL)
		DisposeDialog(myDialog);

	return(myCString);
}


//////////
//
// QTTarg_ShowStringToUser 
// Display a string in an alert box.
//
//////////

void QTTarg_ShowStringToUser (StringPtr theString)
{
	short 		mySavedResFile;

	// get the current resource file and set the application's resource file
	mySavedResFile = CurResFile();
	UseResFile(gAppResFile);

	ParamText(theString, NULL, NULL, NULL);
	Alert(kQTTargAlertID, NULL);
	
	// restore the original resource file
	UseResFile(mySavedResFile);
}


//////////
//
// QTTarg_CreateTwinSpritesMovie
// Create a movie with two sprites that target each other.
//
//////////

OSErr QTTarg_CreateTwinSpritesMovie (void)
{
	Movie					myMovie = NULL;
	Track					myTrack = NULL;
	Media					myMedia = NULL;
	FSSpec					myFile;
	Boolean					myIsSelected = false;
	Boolean					myIsReplacing = false;	
	Fixed					myHeight = 0;
	Fixed					myWidth = 0;
	StringPtr 				myPrompt = QTUtils_ConvertCToPascalString(kSpriteSavePrompt);
	StringPtr 				myFileName = QTUtils_ConvertCToPascalString(kSpriteSaveMovieFileName);
	long					myFlags = createMovieFileDeleteCurFile | createMovieFileDontCreateResFile;
	OSType					myType = FOUR_CHAR_CODE('none');
	short					myResRefNum = 0;
	short					myResID = movieInDataForkResID;
	OSErr					myErr = noErr;

	//////////
	//
	// create a new movie file
	//
	//////////

	// prompt the user for the destination file name
	QTFrame_PutFile(myPrompt, myFileName, &myFile, &myIsSelected, &myIsReplacing);
	myErr = myIsSelected ? noErr : userCanceledErr;
	if (!myIsSelected)
		goto bail;

	// create a movie file for the destination movie
	myErr = CreateMovieFile(&myFile, FOUR_CHAR_CODE('TVOD'), smSystemScript, myFlags, &myResRefNum, &myMovie);
	if (myErr != noErr)
		goto bail;
	
	// select the "no-interface" movie controller
	myType = EndianU32_NtoB(myType);
	SetUserDataItem(GetMovieUserData(myMovie), &myType, sizeof(myType), kUserDataMovieControllerType, 1);
	
	//////////
	//
	// create the sprite track and media
	//
	//////////
	
	myWidth = Long2Fix(kIconSpriteTrackWidth);
	myHeight = Long2Fix(kIconSpriteTrackHeight);

	myTrack = NewMovieTrack(myMovie, myWidth, myHeight, kNoVolume);
	myMedia = NewTrackMedia(myTrack, SpriteMediaType, kSpriteMediaTimeScale, NULL, 0);

	myErr = BeginMediaEdits(myMedia);
	if (myErr != noErr)
		goto bail;

	//////////
	//
	// add the appropriate samples to the sprite media
	//
	//////////
	
	myErr = QTTarg_AddIconMovieSamplesToMedia(myMedia);
	if (myErr != noErr)
		goto bail;
	
	myErr = EndMediaEdits(myMedia);
	if (myErr != noErr)
		goto bail;
	
	// add the media to the track
	InsertMediaIntoTrack(myTrack, 0, 0, GetMediaDuration(myMedia), fixed1);
		
	//////////
	//
	// set the sprite track properties
	//
	//////////
	
	QTTarg_SetTrackProperties(myMedia, 1);
	
	//////////
	//
	// add the movie resource to the movie file
	//
	//////////
	
	myErr = AddMovieResource(myMovie, myResRefNum, &myResID, myFile.name);
		
bail:
	if (myResRefNum != 0)
		CloseMovieFile(myResRefNum);

	if (myMovie != NULL)
		DisposeMovie(myMovie);
		
	free(myPrompt);
	free(myFileName);

	return(myErr);
}


//////////
//
// QTTarg_AddIconMovieSamplesToMedia
// Build the key frame for the icon sprites movie.
//
//////////

OSErr QTTarg_AddIconMovieSamplesToMedia (Media theMedia)
{
	QTAtomContainer			mySample = NULL;
	QTAtomContainer			mySpriteData = NULL;
	QTAtom					myActionAtom = 0;
	QTAtom					myTargetAtom = 0;
	RGBColor				myKeyColor;
	Point					myLocation;
	short					isVisible, myIndex;
	Fixed					myDegrees;
	FixedPoint				myPoint;
	OSErr					myErr = noErr;
	
	//////////
	//
	// create a key frame sample containing the sprite images
	//
	//////////

	// create a new, empty key frame sample
	myErr = QTNewAtomContainer(&mySample);
	if (myErr != noErr)
		goto bail;

	myKeyColor.red = myKeyColor.green = myKeyColor.blue = 0xffff;		// white
	
	myPoint.x = Long2Fix(kIconDimension / 2);
	myPoint.y = Long2Fix(kIconDimension / 2);

	// add images to the key frame sample
	SpriteUtils_AddPICTImageToKeyFrameSample(mySample, kOldQTIconID, &myKeyColor, kOldQTIconSpriteAtomID, &myPoint, NULL);
	SpriteUtils_AddPICTImageToKeyFrameSample(mySample, kNewQTIconID, &myKeyColor, kNewQTIconSpriteAtomID, &myPoint, NULL);

	//////////
	//
	// add the initial sprite properties and actions to the key frame sample
	//
	//////////
	
	// the old QT icon sprite
	myErr = QTNewAtomContainer(&mySpriteData);
	if (myErr != noErr)
		goto bail;

	myLocation.h 	= kIconDimension + (kIconDimension / 2);
	myLocation.v	= ((kIconSpriteTrackHeight - kIconDimension) / 2) + (kIconDimension / 2);
	isVisible		= true;
	myIndex			= kOldQTIconImageIndex;
	
	SpriteUtils_SetSpriteData(mySpriteData, &myLocation, &isVisible, NULL, &myIndex, NULL, NULL, NULL);
	WiredUtils_AddQTEventAndActionAtoms(mySpriteData, kParentAtomIsContainer, kQTEventMouseClickEndTriggerButton, kActionSpriteRotate, &myActionAtom);
	myDegrees = EndianS32_NtoB(FixRatio(90, 1));
	WiredUtils_AddActionParameterAtom(mySpriteData, myActionAtom, kFirstParam, sizeof(myDegrees), &myDegrees, NULL);
	WiredUtils_AddSpriteIDActionTargetAtom(mySpriteData, myActionAtom, 2, NULL);
	SpriteUtils_AddSpriteToSample(mySample, mySpriteData, kOldQTIconSpriteAtomID);
	
	QTDisposeAtomContainer(mySpriteData);
	
	// the new QT icon sprite
	myErr = QTNewAtomContainer(&mySpriteData);
	if (myErr != noErr)
		goto bail;

	myLocation.h 	= kIconSpriteTrackWidth - (kIconDimension + (kIconDimension / 2));
	myLocation.v	= ((kIconSpriteTrackHeight - kIconDimension) / 2) + (kIconDimension / 2);
	isVisible		= true;
	myIndex			= kNewQTIconImageIndex;
	
	SpriteUtils_SetSpriteData(mySpriteData, &myLocation, &isVisible, NULL, &myIndex, NULL, NULL, NULL);
	WiredUtils_AddQTEventAndActionAtoms(mySpriteData, kParentAtomIsContainer, kQTEventMouseClickEndTriggerButton, kActionSpriteRotate, &myActionAtom);
	myDegrees = EndianS32_NtoB(FixRatio(90, 1));
	WiredUtils_AddActionParameterAtom(mySpriteData, myActionAtom, kFirstParam, sizeof(myDegrees), &myDegrees, NULL);
	WiredUtils_AddSpriteIDActionTargetAtom(mySpriteData, myActionAtom, 1, NULL);
	SpriteUtils_AddSpriteToSample(mySample, mySpriteData, kNewQTIconSpriteAtomID);
	
	SpriteUtils_AddSpriteSampleToMedia(theMedia, mySample, kSpriteMediaFrameDurationIcon, true, NULL);	

bail:	
	if (mySample != NULL)
		QTDisposeAtomContainer(mySample);

	if (mySpriteData != NULL)
		QTDisposeAtomContainer(mySpriteData);
	
	return(myErr);
}


//////////
//
// QTTarg_AddVRControllerButtonSamplesToMedia
// Build the key frame for the VR controller sprite track.
//
//////////

void QTTarg_AddVRControllerButtonSamplesToMedia (Media theMedia, long theTrackWidth, long theTrackHeight, TimeValue theDuration)
{
#pragma unused(theTrackHeight)
	QTAtomContainer			mySample = NULL;
	QTAtomContainer			myActions = NULL;
	QTAtomContainer			myTiltUpButton, myPanRightButton, myTiltDownButton, myZoomInButton, myPanLeftButton, myZoomOutButton;
	RGBColor				myKeyColor;
	Point					myLocation;
	short					isVisible, myIndex, myLayer;
	short					myCount;
	long					mySixth;
	QTAtom					myActionAtom = 0;
	QTAtom					myParamAtom = 0;
	float					myDeltaAngle = 0;
	StringPtr				myMovieNames[2];
	OSErr					myErr = noErr;
	
	//////////
	//
	// set up the two target movie names
	//
	//////////

	myMovieNames[0] = QTUtils_ConvertCToPascalString(kTarget1);
	myMovieNames[1] = QTUtils_ConvertCToPascalString(kTarget2);

	//////////
	//
	// create a key frame sample containing the sprite images
	//
	//////////

	// create a new, empty key frame sample
	myErr = QTNewAtomContainer(&mySample);
	if (myErr != noErr)
		goto bail;

	myKeyColor.red = myKeyColor.green = myKeyColor.blue = 0xffff;		// white
	
	// add images to the key frame sample
	for (myCount = 0; myCount < kNumControllerImages; myCount++)
		SpriteUtils_AddPICTImageToKeyFrameSample(mySample, kFirstControllerImageID + myCount, &myKeyColor, myCount + 1, NULL, NULL);

	// assign group IDs to the images
	SpriteUtils_AssignImageGroupIDsToKeyFrame(mySample);
	
	//////////
	//
	// add the initial sprite properties and actions to the key frame sample
	//
	//////////
	
	mySixth = theTrackWidth / kNumControllerButtons;

	// the Tilt-Up button sprite
	myErr = QTNewAtomContainer(&myTiltUpButton);
	if (myErr != noErr)
		goto bail;

	myLocation.h 	= (kTiltPosSpritePosition * mySixth) + ((mySixth - kButtonWidth) / 2);
	myLocation.v	= 0;
	isVisible		= true;
	myIndex			= kTiltPosUpIndex;
	myLayer			= 1;
	myDeltaAngle	= 1;
	EndianUtils_Float_NtoB(&myDeltaAngle);
	
	for (myCount = 0; myCount <= 1; myCount++) {
		SpriteUtils_SetSpriteData(myTiltUpButton, &myLocation, &isVisible, &myLayer, &myIndex, NULL, NULL, NULL);
		WiredUtils_AddSpriteSetImageIndexAction(myTiltUpButton, kParentAtomIsContainer, kQTEventMouseEnter, 0, NULL, 0, 0, NULL, kTiltPosDownIndex, NULL);
		WiredUtils_AddSpriteSetImageIndexAction(myTiltUpButton, kParentAtomIsContainer, kQTEventMouseExit, 0, NULL, 0, 0, NULL, kTiltPosUpIndex, NULL);
		WiredUtils_AddSpriteTrackSetVariableAction(myTiltUpButton, kParentAtomIsContainer, kQTEventMouseEnter, kMouseOverTiltUpVariableID, 1, 0, NULL, 0);
		WiredUtils_AddSpriteTrackSetVariableAction(myTiltUpButton, kParentAtomIsContainer, kQTEventMouseExit, kMouseOverTiltUpVariableID, 0, 0, NULL, 0);
		QTTarg_AddIdleEventVarTestAction(myTiltUpButton, kParentAtomIsContainer, kMouseOverTiltUpVariableID, 1, kActionQTVRSetTiltAngle, &myActionAtom);
		WiredUtils_AddActionParameterAtom(myTiltUpButton, myActionAtom, kFirstParam, sizeof(myDeltaAngle), &myDeltaAngle, NULL);
		WiredUtils_AddActionParameterOptions(myTiltUpButton, myActionAtom, 1, kActionFlagActionIsDelta, 0, NULL, 0, NULL);
		WiredUtils_AddMovieNameActionTargetAtom(myTiltUpButton, myActionAtom, myMovieNames[myCount], NULL);
	}
	
	SpriteUtils_AddSpriteToSample(mySample, myTiltUpButton, kTiltPosSpriteAtomID);	

	// the Pan-Right button sprite
	myErr = QTNewAtomContainer(&myPanRightButton);
	if (myErr != noErr)
		goto bail;

	myLocation.h 	= (kPanRightSpritePosition * mySixth) + ((mySixth - kButtonWidth) / 2);
	myLocation.v	= 0;
	isVisible		= true;
	myIndex			= kPanRightUpIndex;
	myLayer			= 1;
	myDeltaAngle	= -3;
	EndianUtils_Float_NtoB(&myDeltaAngle);
	
	SpriteUtils_SetSpriteData(myPanRightButton, &myLocation, &isVisible, &myLayer, &myIndex, NULL, NULL, NULL);

	for (myCount = 0; myCount <= 1; myCount++) {
		WiredUtils_AddSpriteSetImageIndexAction(myPanRightButton, kParentAtomIsContainer, kQTEventMouseEnter, 0, NULL, 0, 0, NULL, kPanRightDownIndex, NULL);
		WiredUtils_AddSpriteSetImageIndexAction(myPanRightButton, kParentAtomIsContainer, kQTEventMouseExit, 0, NULL, 0, 0, NULL, kPanRightUpIndex, NULL);
		WiredUtils_AddSpriteTrackSetVariableAction(myPanRightButton, kParentAtomIsContainer, kQTEventMouseEnter, kMouseOverPanRightVariableID, 1, 0, NULL, 0);
		WiredUtils_AddSpriteTrackSetVariableAction(myPanRightButton, kParentAtomIsContainer, kQTEventMouseExit, kMouseOverPanRightVariableID, 0, 0, NULL, 0);
		QTTarg_AddIdleEventVarTestAction(myPanRightButton, kParentAtomIsContainer, kMouseOverPanRightVariableID, 1, kActionQTVRSetPanAngle, &myActionAtom);
		WiredUtils_AddActionParameterAtom(myPanRightButton, myActionAtom, kFirstParam, sizeof(myDeltaAngle), &myDeltaAngle, NULL);
		WiredUtils_AddActionParameterOptions(myPanRightButton, myActionAtom, 1, kActionFlagActionIsDelta | kActionFlagParameterWrapsAround, 0, NULL, 0, NULL);
		WiredUtils_AddMovieNameActionTargetAtom(myPanRightButton, myActionAtom, myMovieNames[myCount], NULL);
	}
	
	SpriteUtils_AddSpriteToSample(mySample, myPanRightButton, kPanRightSpriteAtomID);	

	// the Pan-Left button sprite
	myErr = QTNewAtomContainer(&myPanLeftButton);
	if (myErr != noErr)
		goto bail;

	myLocation.h 	= (kPanLeftSpritePosition * mySixth) + ((mySixth - kButtonWidth) / 2);
	myLocation.v	= 0;
	isVisible		= true;
	myIndex			= kPanLeftUpIndex;
	myLayer			= 1;
	myDeltaAngle	= 3;
	EndianUtils_Float_NtoB(&myDeltaAngle);
	
	SpriteUtils_SetSpriteData(myPanLeftButton, &myLocation, &isVisible, &myLayer, &myIndex, NULL, NULL, NULL);
	
	for (myCount = 0; myCount <= 1; myCount++) {
		WiredUtils_AddSpriteSetImageIndexAction(myPanLeftButton, kParentAtomIsContainer, kQTEventMouseEnter, 0, NULL, 0, 0, NULL, kPanLeftDownIndex, NULL);
		WiredUtils_AddSpriteSetImageIndexAction(myPanLeftButton, kParentAtomIsContainer, kQTEventMouseExit, 0, NULL, 0, 0, NULL, kPanLeftUpIndex, NULL);
		WiredUtils_AddSpriteTrackSetVariableAction(myPanLeftButton, kParentAtomIsContainer, kQTEventMouseEnter, kMouseOverPanLeftVariableID, 1, 0, NULL, 0);
		WiredUtils_AddSpriteTrackSetVariableAction(myPanLeftButton, kParentAtomIsContainer, kQTEventMouseExit, kMouseOverPanLeftVariableID, 0, 0, NULL, 0);
		QTTarg_AddIdleEventVarTestAction(myPanLeftButton, kParentAtomIsContainer, kMouseOverPanLeftVariableID, 1, kActionQTVRSetPanAngle, &myActionAtom);
		WiredUtils_AddActionParameterAtom(myPanLeftButton, myActionAtom, kFirstParam, sizeof(myDeltaAngle), &myDeltaAngle, NULL);
		WiredUtils_AddActionParameterOptions(myPanLeftButton, myActionAtom, 1, kActionFlagActionIsDelta | kActionFlagParameterWrapsAround, 0, NULL, 0, NULL);
		WiredUtils_AddMovieNameActionTargetAtom(myPanLeftButton, myActionAtom, myMovieNames[myCount], NULL);
	}

	SpriteUtils_AddSpriteToSample(mySample, myPanLeftButton, kPanLeftSpriteAtomID);	

	// the Zoom-Out button sprite
	myErr = QTNewAtomContainer(&myZoomOutButton);
	if (myErr != noErr)
		goto bail;

	myLocation.h 	= (kZoomOutSpritePosition * mySixth) + ((mySixth - kButtonWidth) / 2);
	myLocation.v	= 0;
	isVisible		= true;
	myIndex			= kZoomOutUpIndex;
	myLayer			= 1;
	myDeltaAngle	= 1;
	EndianUtils_Float_NtoB(&myDeltaAngle);
	
	SpriteUtils_SetSpriteData(myZoomOutButton, &myLocation, &isVisible, &myLayer, &myIndex, NULL, NULL, NULL);
	
	for (myCount = 0; myCount <= 1; myCount++) {
		WiredUtils_AddSpriteSetImageIndexAction(myZoomOutButton, kParentAtomIsContainer, kQTEventMouseEnter, 0, NULL, 0, 0, NULL, kZoomOutDownIndex, NULL);
		WiredUtils_AddSpriteSetImageIndexAction(myZoomOutButton, kParentAtomIsContainer, kQTEventMouseExit, 0, NULL, 0, 0, NULL, kZoomOutUpIndex, NULL);
		WiredUtils_AddSpriteTrackSetVariableAction(myZoomOutButton, kParentAtomIsContainer, kQTEventMouseEnter, kMouseOverZoomOutVariableID, 1, 0, NULL, 0);
		WiredUtils_AddSpriteTrackSetVariableAction(myZoomOutButton, kParentAtomIsContainer, kQTEventMouseExit, kMouseOverZoomOutVariableID, 0, 0, NULL, 0);
		QTTarg_AddIdleEventVarTestAction(myZoomOutButton, kParentAtomIsContainer, kMouseOverZoomOutVariableID, 1, kActionQTVRSetFieldOfView, &myActionAtom);
		WiredUtils_AddActionParameterAtom(myZoomOutButton, myActionAtom, kFirstParam, sizeof(myDeltaAngle), &myDeltaAngle, NULL);
		WiredUtils_AddActionParameterOptions(myZoomOutButton, myActionAtom, 1, kActionFlagActionIsDelta, 0, NULL, 0, NULL);
		WiredUtils_AddMovieNameActionTargetAtom(myZoomOutButton, myActionAtom, myMovieNames[myCount], NULL);
	}

	SpriteUtils_AddSpriteToSample(mySample, myZoomOutButton, kZoomOutSpriteAtomID);	

	// the Tilt-Down button sprite
	myErr = QTNewAtomContainer(&myTiltDownButton);
	if (myErr != noErr)
		goto bail;

	myLocation.h 	= (kTiltNegSpritePosition * mySixth) + ((mySixth - kButtonWidth) / 2);
	myLocation.v	= 0;
	isVisible		= true;
	myIndex			= kTiltNegUpIndex;
	myLayer			= 1;
	myDeltaAngle	= -1;
	EndianUtils_Float_NtoB(&myDeltaAngle);
	
	SpriteUtils_SetSpriteData(myTiltDownButton, &myLocation, &isVisible, &myLayer, &myIndex, NULL, NULL, NULL);

	for (myCount = 0; myCount <= 1; myCount++) {
		WiredUtils_AddSpriteSetImageIndexAction(myTiltDownButton, kParentAtomIsContainer, kQTEventMouseEnter, 0, NULL, 0, 0, NULL, kTiltNegDownIndex, NULL);
		WiredUtils_AddSpriteSetImageIndexAction(myTiltDownButton, kParentAtomIsContainer, kQTEventMouseExit, 0, NULL, 0, 0, NULL, kTiltNegUpIndex, NULL);
		WiredUtils_AddSpriteTrackSetVariableAction(myTiltDownButton, kParentAtomIsContainer, kQTEventMouseEnter, kMouseOverTiltDownVariableID, 1, 0, NULL, 0);
		WiredUtils_AddSpriteTrackSetVariableAction(myTiltDownButton, kParentAtomIsContainer, kQTEventMouseExit, kMouseOverTiltDownVariableID, 0, 0, NULL, 0);
		QTTarg_AddIdleEventVarTestAction(myTiltDownButton, kParentAtomIsContainer, kMouseOverTiltDownVariableID, 1, kActionQTVRSetTiltAngle, &myActionAtom);
		WiredUtils_AddActionParameterAtom(myTiltDownButton, myActionAtom, kFirstParam, sizeof(myDeltaAngle), &myDeltaAngle, NULL);
		WiredUtils_AddActionParameterOptions(myTiltDownButton, myActionAtom, 1, kActionFlagActionIsDelta, 0, NULL, 0, NULL);
		WiredUtils_AddMovieNameActionTargetAtom(myTiltDownButton, myActionAtom, myMovieNames[myCount], NULL);
	}

	SpriteUtils_AddSpriteToSample(mySample, myTiltDownButton, kTiltNegSpriteAtomID);	

	// the Zoom-In button sprite
	myErr = QTNewAtomContainer(&myZoomInButton);
	if (myErr != noErr)
		goto bail;

	myLocation.h 	= (kZoomInSpritePosition * mySixth) + ((mySixth - kButtonWidth) / 2);
	myLocation.v	= 0;
	isVisible		= true;
	myIndex			= kZoomInUpIndex;
	myLayer			= 1;
	myDeltaAngle	= -1;
	EndianUtils_Float_NtoB(&myDeltaAngle);
	
	SpriteUtils_SetSpriteData(myZoomInButton, &myLocation, &isVisible, &myLayer, &myIndex, NULL, NULL, NULL);
	
	for (myCount = 0; myCount <= 1; myCount++) {
		WiredUtils_AddSpriteSetImageIndexAction(myZoomInButton, kParentAtomIsContainer, kQTEventMouseEnter, 0, NULL, 0, 0, NULL, kZoomInDownIndex, NULL);
		WiredUtils_AddSpriteSetImageIndexAction(myZoomInButton, kParentAtomIsContainer, kQTEventMouseExit, 0, NULL, 0, 0, NULL, kZoomInUpIndex, NULL);
		WiredUtils_AddSpriteTrackSetVariableAction(myZoomInButton, kParentAtomIsContainer, kQTEventMouseEnter, kMouseOverZoomInVariableID, 1, 0, NULL, 0);
		WiredUtils_AddSpriteTrackSetVariableAction(myZoomInButton, kParentAtomIsContainer, kQTEventMouseExit, kMouseOverZoomInVariableID, 0, 0, NULL, 0);
		QTTarg_AddIdleEventVarTestAction(myZoomInButton, kParentAtomIsContainer, kMouseOverZoomInVariableID, 1, kActionQTVRSetFieldOfView, &myActionAtom);
		WiredUtils_AddActionParameterAtom(myZoomInButton, myActionAtom, kFirstParam, sizeof(myDeltaAngle), &myDeltaAngle, NULL);
		WiredUtils_AddActionParameterOptions(myZoomInButton, myActionAtom, 1, kActionFlagActionIsDelta, 0, NULL, 0, NULL);
		WiredUtils_AddMovieNameActionTargetAtom(myZoomInButton, myActionAtom, myMovieNames[myCount], NULL);
	}

	SpriteUtils_AddSpriteToSample(mySample, myZoomInButton, kZoomInSpriteAtomID);	

	SpriteUtils_AddSpriteSampleToMedia(theMedia, mySample, theDuration, true, NULL);	

bail:
	free(myMovieNames[0]);
	free(myMovieNames[1]);

	if (mySample != NULL)
		QTDisposeAtomContainer(mySample);
	if (myTiltUpButton != NULL)
		QTDisposeAtomContainer(myTiltUpButton);	
	if (myPanRightButton != NULL)
		QTDisposeAtomContainer(myPanRightButton);	
	if (myTiltDownButton != NULL)
		QTDisposeAtomContainer(myTiltDownButton);	
	if (myZoomInButton != NULL)
		QTDisposeAtomContainer(myZoomInButton);	
	if (myPanLeftButton != NULL)
		QTDisposeAtomContainer(myPanLeftButton);	
	if (myZoomOutButton != NULL)
		QTDisposeAtomContainer(myZoomOutButton);	
}


//////////
//
// QTTarg_AddTextButtonSamplesToMedia
// Build the key frame for the text button sprite track.
//
//////////

void QTTarg_AddTextButtonSamplesToMedia (Media theMedia, long theTrackWidth, long theTrackHeight, TimeValue theDuration)
{
#pragma unused(theTrackWidth, theTrackHeight)
	QTAtomContainer			mySample = NULL;
	QTAtomContainer			myTextButton;
	QTAtom					myActionAtom = 0;
	RGBColor				myKeyColor;
	Point					myLocation;
	short					isVisible, myIndex, myLayer;
	short					myCount;
	OSErr					myErr = noErr;
	
	//////////
	//
	// create a key frame sample containing the sprite images
	//
	//////////

	// create a new, empty key frame sample
	myErr = QTNewAtomContainer(&mySample);
	if (myErr != noErr)
		goto bail;

	myKeyColor.red = myKeyColor.green = myKeyColor.blue = 0xffff;		// white
	
	// add images to the key frame sample
	for (myCount = 0; myCount < kNumTextImages; myCount++)
		SpriteUtils_AddPICTImageToKeyFrameSample(mySample, kFirstTextImageID + myCount, &myKeyColor, myCount + 1, NULL, NULL);

	// assign group IDs to the images
	SpriteUtils_AssignImageGroupIDsToKeyFrame(mySample);
	
	//////////
	//
	// add the initial sprite properties and actions to the key frame sample
	//
	//////////
	
	// the Text button sprite
	myErr = QTNewAtomContainer(&myTextButton);
	if (myErr != noErr)
		goto bail;

	myLocation.h 	= kButtonWidth / 2;
	myLocation.v	= kButtonHeight / 2;
	isVisible		= true;
	myIndex			= kTextUpIndex;
	myLayer			= 1;
	
	SpriteUtils_SetSpriteData(myTextButton, &myLocation, &isVisible, &myLayer, &myIndex, NULL, NULL, NULL);
	
	WiredUtils_AddSpriteSetImageIndexAction(myTextButton, kParentAtomIsContainer, kQTEventMouseClick, 0, NULL, 0, 0, NULL, kTextDownIndex, NULL);
	WiredUtils_AddSpriteSetImageIndexAction(myTextButton, kParentAtomIsContainer, kQTEventMouseClickEnd, 0, NULL, 0, 0, NULL, kTextUpIndex, NULL);
	WiredUtils_AddQTEventAndActionAtoms(myTextButton, kParentAtomIsContainer, kQTEventMouseClickEndTriggerButton, kActionTrackSetEnabled, &myActionAtom);
	WiredUtils_AddActionParameterOptions(myTextButton, myActionAtom, 1, kActionFlagActionIsToggle, 0, NULL, 0, NULL);
	WiredUtils_AddTrackTargetAtom(myTextButton, myActionAtom, kTargetTrackType, (void *)TextMediaType, 1);
	
	SpriteUtils_AddSpriteToSample(mySample, myTextButton, kTextSpriteAtomID);	
	SpriteUtils_AddSpriteSampleToMedia(theMedia, mySample, theDuration, true, NULL);	

bail:	
	if (mySample != NULL)
		QTDisposeAtomContainer(mySample);
	if (myTextButton != NULL)
		QTDisposeAtomContainer(myTextButton);	
}


//////////
//
// QTTarg_AddVRSpriteControllerTrack
// Add a track to the specified VR movie that contains wired sprites for controlling the movie.
//
// We add six sprites to the track, which will function like buttons. On mouse over, the image
// of the sprite is changed to the "clicked" image; on mouse exit, the image is returned to the
// normal "unclicked" image; on idle events we execute an action (for instance kActionMovieGoToBeginning).
// No other logic and no sprite track variables are required here.
//
//////////

OSErr QTTarg_MakeDualVRControllerMovie (void)
{
	Movie					myMovie = NULL;
	Track					myTrack = NULL;
	Media					myMedia = NULL;
	RGBColor				myKeyColor;
	Fixed					myWidth, myHeight;
	FSSpec					myFile;
	Boolean					myIsSelected = false;
	Boolean					myIsReplacing = false;	
	StringPtr 				myPrompt = QTUtils_ConvertCToPascalString(kSpriteSavePrompt);
	StringPtr 				myFileName = QTUtils_ConvertCToPascalString(kSpriteSaveMovieFileName);
	long					myFlags = createMovieFileDeleteCurFile | createMovieFileDontCreateResFile;
	short					myResRefNum = 0;
	short					myResID = movieInDataForkResID;
	OSType					myType = FOUR_CHAR_CODE('none');
	OSErr					myErr = noErr;

	//////////
	//
	// create a new movie file
	//
	//////////

	// prompt the user for the destination file name
	QTFrame_PutFile(myPrompt, myFileName, &myFile, &myIsSelected, &myIsReplacing);
	myErr = myIsSelected ? noErr : userCanceledErr;
	if (!myIsSelected)
		goto bail;

	// create a movie file for the destination movie
	myErr = CreateMovieFile(&myFile, FOUR_CHAR_CODE('TVOD'), smSystemScript, myFlags, &myResRefNum, &myMovie);
	if (myErr != noErr)
		goto bail;
	
	// select the "no-interface" movie controller
	myType = EndianU32_NtoB(myType);
	SetUserDataItem(GetMovieUserData(myMovie), &myType, sizeof(myType), kUserDataMovieControllerType, 1);

	//////////
	//
	// get some information about the target movie
	//
	//////////

	myWidth = Long2Fix(kVRControlMovieWidth);
	myHeight = Long2Fix(kVRControlMovieHeight);
	
	//////////
	//
	// create a new sprite track in the target movie
	//
	//////////
	
	myTrack = NewMovieTrack(myMovie, myWidth, myHeight, kNoVolume);
	myMedia = NewTrackMedia(myTrack, SpriteMediaType, kVRControlMovieDuration, NULL, 0);

	myErr = BeginMediaEdits(myMedia);
	if (myErr != noErr)
		goto bail;
	
	//////////
	//
	// add sprite images and sprites to the sprite track; add actions to the sprites
	//
	//////////
	
	QTTarg_AddVRControllerButtonSamplesToMedia(myMedia, kVRControlMovieWidth, kVRControlMovieHeight, kVRControlMovieDuration);
	
	//////////
	//
	// insert media into track
	//
	//////////
	
	myErr = EndMediaEdits(myMedia);
	if (myErr != noErr)
		goto bail;
	
	// add the media to the track
	InsertMediaIntoTrack(myTrack, 0, 0, GetMediaDuration(myMedia), fixed1);
		
	//////////
	//
	// set the sprite track properties
	//
	//////////
	
	QTTarg_SetTrackProperties(myMedia, 0);								// idle as fast as possible
	
	myKeyColor.red = myKeyColor.green = myKeyColor.blue = 0xffff;		// white
	MediaSetGraphicsMode(GetMediaHandler(myMedia), transparent, &myKeyColor);
	
	//////////
	//
	// add the movie resource to the movie file
	//
	//////////
	
	myErr = AddMovieResource(myMovie, myResRefNum, &myResID, myFile.name);
		
bail:
	if (myResRefNum != 0)
		CloseMovieFile(myResRefNum);

	if (myMovie != NULL)
		DisposeMovie(myMovie);
		
	free(myPrompt);
	free(myFileName);

	return(myErr);
}


//////////
//
// QTTarg_AddTextToggleButtonTrack
// Add a track to the specified movie that contains a wired sprite for toggling the visibility
// state of the first text track in the movie.
//
//////////

OSErr QTTarg_AddTextToggleButtonTrack (Movie theMovie)
{
	Track					myTrack = NULL;
	Media					myMedia = NULL;
	MatrixRecord			myMatrix;
	RGBColor				myKeyColor;
	Fixed					myWidth, myHeight;
	TimeValue				myDuration = 0L;
	TimeValue				myTimeScale = 0L;
	OSErr					myErr = noErr;

	//////////
	//
	// get some information about the target movie
	//
	//////////

	if (theMovie == NULL) {
		myErr = paramErr;
		goto bail;
	}

	myWidth = Long2Fix(2 * kButtonWidth);
	myHeight = Long2Fix(2 * kButtonHeight);
	myDuration = GetMovieDuration(theMovie);
	myTimeScale = GetMovieTimeScale(theMovie);
	
	//////////
	//
	// create a new sprite track in the target movie
	//
	//////////
	
	myTrack = NewMovieTrack(theMovie, myWidth, myHeight, kNoVolume);
	myMedia = NewTrackMedia(myTrack, SpriteMediaType, myTimeScale, NULL, 0);

	// set the track matrix to compensate for any existing movie matrix
	GetMovieMatrix(theMovie, &myMatrix);
	if (InverseMatrix(&myMatrix, &myMatrix))
		SetTrackMatrix(myTrack, &myMatrix);

	myErr = BeginMediaEdits(myMedia);
	if (myErr != noErr)
		goto bail;
	
	//////////
	//
	// add sprite images and sprites to the sprite track; add actions to the sprites
	//
	//////////
	
	QTTarg_AddTextButtonSamplesToMedia(myMedia, 2 * kButtonWidth, 2 * kButtonHeight, myDuration);
	
	//////////
	//
	// insert media into track
	//
	//////////
	
	myErr = EndMediaEdits(myMedia);
	if (myErr != noErr)
		goto bail;
	
	// add the media to the track
	InsertMediaIntoTrack(myTrack, 0, 0, GetMediaDuration(myMedia), fixed1);
		
	//////////
	//
	// set the sprite track properties
	//
	//////////
	
	QTTarg_SetTrackProperties(myMedia, kNoQTIdleEvents);				// no idle events
	
	myKeyColor.red = myKeyColor.green = myKeyColor.blue = 0xffff;		// white
	MediaSetGraphicsMode(GetMediaHandler(myMedia), transparent, &myKeyColor);
	
	// make sure that the sprite track is in the frontmost layer
	SetTrackLayer(myTrack, kMaxLayerNumber);
	SetTrackLayer(myTrack, QTTarg_GetLowestLayerInMovie(theMovie) - 1);
		
bail:
	return(myErr);
}


//////////
//
// QTTarg_SetTrackProperties
// Set the track properties for the specified sample sprite movie.
//
//////////

void QTTarg_SetTrackProperties (Media theMedia, UInt32 theIdleFrequency)
{
	QTAtomContainer		myTrackProperties;
	RGBColor			myBackgroundColor;
	Boolean				hasActions;
	UInt32				myFrequency;
	OSErr				myErr = noErr;
		
	// add a background color to the sprite track
	myBackgroundColor.red = EndianU16_NtoB(0xffff);
	myBackgroundColor.green = EndianU16_NtoB(0xffff);
	myBackgroundColor.blue = EndianU16_NtoB(0xffff);
	
	myErr = QTNewAtomContainer(&myTrackProperties);
	if (myErr == noErr) {
		QTInsertChild(myTrackProperties, 0, kSpriteTrackPropertyBackgroundColor, 1, 1, sizeof(myBackgroundColor), &myBackgroundColor, NULL);

		// tell the movie controller that this sprite track has actions
		hasActions = true;
		QTInsertChild(myTrackProperties, 0, kSpriteTrackPropertyHasActions, 1, 1, sizeof(hasActions), &hasActions, NULL);
	
		// tell the sprite track to generate QTIdleEvents
		myFrequency = EndianU32_NtoB(theIdleFrequency);
		QTInsertChild(myTrackProperties, 0, kSpriteTrackPropertyQTIdleEventsFrequency, 1, 1, sizeof(myFrequency), &myFrequency, NULL);

		SetMediaPropertyAtom(theMedia, myTrackProperties);
		
		QTDisposeAtomContainer(myTrackProperties);
	}
}
	

//////////
//
// QTTarg_GetLowestLayerInMovie
// Return the layer of the frontmost layer in the specified movie.
//
//////////

short QTTarg_GetLowestLayerInMovie (Movie theMovie)
{
	long		myCount = 0;
	long		myIndex;
	short		myLayer = 0;
	short		myMinLayer = kMaxLayerNumber;
	
	myCount = GetMovieTrackCount(theMovie);
	
	for (myIndex = 1; myIndex <= myCount; myIndex++) {
		myLayer = GetTrackLayer(GetMovieIndTrack(theMovie, myIndex));
		if (myLayer < myMinLayer)
			myMinLayer = myLayer;
	}
	
	return(myMinLayer);
}


//////////
//
// QTTarg_AddIdleEventVarTestAction
// Add, to the specified atom, a conditional atom that executes the specified action if theVariableID = theTrueValue.
//
//////////

OSErr QTTarg_AddIdleEventVarTestAction (QTAtomContainer theContainer, QTAtom theAtom, QTAtomID theVariableID, UInt32 theTrueValue, long theActionConstant, QTAtom *theNewActionAtom)
{
	QTAtom		myActionAtom = 0;
	QTAtom		myParamAtom = 0;
	QTAtom		myConditionalAtom = 0;
	QTAtom		myExpressionAtom = 0;
	QTAtom		myOperatorAtom = 0;
	QTAtom		myActionListAtom = 0;
	OSErr		myErr = noErr;
	
	myErr = WiredUtils_AddQTEventAndActionAtoms(theContainer, theAtom, kQTEventIdle, kActionCase, &myActionAtom);
	if (myErr != noErr)
		goto bail;

	// add a parameter atom to the kActionCase action atom; this will serve as a parent to hold the expression and action atoms
	myErr = WiredUtils_AddActionParameterAtom(theContainer, myActionAtom, kFirstParam, 0, NULL, &myParamAtom);
	if (myErr != noErr)
		goto bail;
	
	myErr = WiredUtils_AddConditionalAtom(theContainer, myParamAtom, 1, &myConditionalAtom);
	if (myErr != noErr)
		goto bail;
		
	myErr = WiredUtils_AddExpressionContainerAtomType(theContainer, myConditionalAtom, &myExpressionAtom);
	if (myErr != noErr)
		goto bail;
		
	myErr = WiredUtils_AddOperatorAtom(theContainer, myExpressionAtom, kOperatorEqualTo, &myOperatorAtom);
	if (myErr != noErr)
		goto bail;

	myErr = WiredUtils_AddOperandAtom(theContainer, myOperatorAtom, kOperandConstant, 1, NULL, theTrueValue);
	if (myErr != noErr)
		goto bail;

	myErr = WiredUtils_AddVariableOperandAtom(theContainer, myOperatorAtom, 2, 0, NULL, 0, theVariableID);
	if (myErr != noErr)
		goto bail;
		
	myErr = WiredUtils_AddActionListAtom(theContainer, myConditionalAtom, &myActionListAtom);
	if (myErr != noErr)
		goto bail;

	myErr = WiredUtils_AddActionAtom(theContainer, myActionListAtom, theActionConstant, theNewActionAtom);
	if (myErr != noErr)
		goto bail;

bail:
	return(myErr);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Target utilities
//
// Use these functions to get, set, and find wired action movie targets.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////
//
// QTUtils_GetMovieTargetName
// Get the movie target name stored in the specified movie.
//
// The caller is responsible for disposing of the pointer returned by this function (by calling free).
//
//////////

char *QTUtils_GetMovieTargetName (Movie theMovie)
{
	UserData		myUserData = NULL;
	char 			*myString = NULL;
	
	// make sure we've got a movie
	if (theMovie == NULL)
		goto bail;
	
	// get the movie's user data list
	myUserData = GetMovieUserData(theMovie);
	if (myUserData == NULL)
		goto bail;

	// find the "value" of the user data item of type 'plug' that begins with the string "moviename="
	myString = QTUtils_GetUserDataPrefixedValue(myUserData, FOUR_CHAR_CODE('plug'), kMovieNamePrefix);

bail:
	return(myString);
}


//////////
//
// QTUtils_SetMovieTargetName
// Set the movie target name stored in the specified movie.
//
//////////

OSErr QTUtils_SetMovieTargetName (Movie theMovie, char *theTargetName)
{
	UserData		myUserData = NULL;
	char 			*myString = NULL;
	Handle			myHandle = NULL;
	OSErr			myErr = noErr;

	// make sure we've got a movie and a name
	if ((theMovie == NULL) || (theTargetName == NULL))
		return(paramErr);
		
	// get the movie's user data list
	myUserData = GetMovieUserData(theMovie);
	if (myUserData == NULL)
		return(paramErr);

	// remove any existing movie target name
	while (QTUtils_FindUserDataItemWithPrefix(myUserData, FOUR_CHAR_CODE('plug'), kMovieNamePrefix) != 0)
		RemoveUserData(myUserData, FOUR_CHAR_CODE('plug'), QTUtils_FindUserDataItemWithPrefix(myUserData, FOUR_CHAR_CODE('plug'), kMovieNamePrefix));

	// create the user data item data
	myString = malloc(strlen(kMovieNamePrefix) + strlen(theTargetName) + 2 + 1);	// 2 + 1 == '\"' + '\"' + '\0'
	if (myString != NULL) {
		myString[0] = '\0';
		strcat(myString, kMovieNamePrefix);
		strcat(myString, "\"");
		strcat(myString, theTargetName);
		strcat(myString, "\"");

		// add in a new user data item
		PtrToHand(myString, &myHandle, strlen(myString));
		if (myHandle != NULL)
			myErr = AddUserData(myUserData, myHandle, FOUR_CHAR_CODE('plug'));
	} else {
		myErr = memFullErr;
	}
	
	free(myString);
	
	if (myHandle != NULL)
		DisposeHandle(myHandle);

	return(myErr);
}


//////////
//
// QTUtils_GetMovieTargetID
// Get the movie target ID stored in the specified movie.
//
//////////

long QTUtils_GetMovieTargetID (Movie theMovie, Boolean *theMovieHasID)
{
	UserData		myUserData = NULL;
	long			myID = 0;
	char 			*myString = NULL;
	StringPtr 		myPString = NULL;
	Boolean			myMovieHasID = false;
	OSErr			myErr = noErr;
	
	// make sure we've got a movie
	if (theMovie == NULL)
		goto bail;
	
	// get the movie's user data list
	myUserData = GetMovieUserData(theMovie);
	if (myUserData == NULL)
		goto bail;

	// find the "value" of the user data item of type 'plug' that begins with the string "movieid="
	myString = QTUtils_GetUserDataPrefixedValue(myUserData, FOUR_CHAR_CODE('plug'), kMovieIDPrefix);
	
	// convert the string into a number
	if (myString != NULL) {
		myPString = QTUtils_ConvertCToPascalString(myString);
		StringToNum(myPString, &myID);
		myMovieHasID = true;
	}

bail:
	free(myString);
	free(myPString);

	if (theMovieHasID != NULL)
		*theMovieHasID = myMovieHasID;
		
	return(myID);
}


//////////
//
// QTUtils_SetMovieTargetID
// Set the movie target ID stored in the specified movie.
//
//////////

OSErr QTUtils_SetMovieTargetID (Movie theMovie, long theTargetID)
{
	UserData		myUserData = NULL;
	long			myID = 0;
	char 			*myString = NULL;
	Str255	 		myPString;
	char 			*myCString = NULL;
	Handle			myHandle = NULL;
	OSErr			myErr = noErr;

	// make sure we've got a movie
	if (theMovie == NULL)
		return(paramErr);
		
	// get the movie's user data list
	myUserData = GetMovieUserData(theMovie);
	if (myUserData == NULL)
		return(paramErr);

	// remove any existing movie target ID
	while (QTUtils_FindUserDataItemWithPrefix(myUserData, FOUR_CHAR_CODE('plug'), kMovieIDPrefix) != 0)
		RemoveUserData(myUserData, FOUR_CHAR_CODE('plug'), QTUtils_FindUserDataItemWithPrefix(myUserData, FOUR_CHAR_CODE('plug'), kMovieIDPrefix));

	// convert the ID into a string
	NumToString(theTargetID, myPString);
	myCString = QTUtils_ConvertPascalToCString(myPString);
	if (myCString == NULL)
		return(paramErr);
	
	// create the user data item data
	myString = malloc(strlen(kMovieIDPrefix) + strlen(myCString) + 2 + 1);	// 2 + 1 == '\"' + '\"' + '\0'
	if (myString != NULL) {
		myString[0] = '\0';
		strcat(myString, kMovieIDPrefix);
		strcat(myString, "\"");
		strcat(myString, myCString);
		strcat(myString, "\"");

		// add in a new user data item
		PtrToHand(myString, &myHandle, strlen(myString));
		if (myHandle != NULL)
			myErr = AddUserData(myUserData, myHandle, FOUR_CHAR_CODE('plug'));

	} else {
		myErr = memFullErr;
	}
	
	free(myString);
	free(myCString);
	
	if (myHandle != NULL)
		DisposeHandle(myHandle);
	
	return(myErr);
}


//////////
//
// QTUtils_FindUserDataItemWithPrefix
// Return the index of the user data item of the specified type whose data begins with the specified string of characters.
//
//////////

static long QTUtils_FindUserDataItemWithPrefix (UserData theUserData, OSType theType, char *thePrefix)
{
	Handle			myData = NULL;
	long			myCount = 0;
	long			myIndex = 0;
	long			myItemIndex = 0;
	OSErr			myErr = noErr;
	
	// make sure we've got some valid user data
	if (theUserData == NULL)
		goto bail;
	
	// allocate a handle for GetUserData
	myData = NewHandle(0);
	if (myData == NULL)
		goto bail;

	myCount = CountUserDataType(theUserData, theType);
	for (myIndex = 1; myIndex <= myCount; myIndex++) {
		myErr = GetUserData(theUserData, myData, theType, myIndex);
		if (myErr == noErr) {
			if (GetHandleSize(myData) < strlen(thePrefix))
				continue;

			// see if the user data begins with the specified prefix (IdenticalText is case-insensitive)
			if (IdenticalText(*myData, thePrefix, strlen(thePrefix), strlen(thePrefix), NULL) == 0) {
				myItemIndex = myIndex;
				goto bail;
			}
		}
	}

bail:
	if (myData != NULL)
		DisposeHandle(myData);
		
	return(myItemIndex);
}


//////////
//
// QTUtils_GetUserDataPrefixedValue
// Return the string value associated with the specified prefix. The string value may or may not be
// enclosed in double quotes, so we need to handle both cases. (With double quotes is preferred.)
//
// The caller is responsible for disposing of the pointer returned by this function (by calling free).
//
//////////

static char *QTUtils_GetUserDataPrefixedValue (UserData theUserData, OSType theType, char *thePrefix)
{
	long			myIndex = 0;
	Handle			myData = NULL;
	long			myLength = 0;
	long			myOffset = 0;
	char 			*myString = NULL;
	OSErr			myErr = noErr;
	
	if (theUserData == NULL)
		goto bail;

	// allocate a handle for GetUserData
	myData = NewHandle(0);
	if (myData == NULL)
		goto bail;
	
	myIndex = QTUtils_FindUserDataItemWithPrefix(theUserData, theType, thePrefix);
	if (myIndex > 0) {
		myErr = GetUserData(theUserData, myData, theType, myIndex);
		if (myErr == noErr) {
			if ((*myData)[strlen(thePrefix)] == '"') {
				myLength = GetHandleSize(myData) - strlen(thePrefix) - 2;
				myOffset = 1;
			} else {
				myLength = GetHandleSize(myData) - strlen(thePrefix);
				myOffset = 0;
			}
		
			myString = malloc(myLength + 1);
			if (myString != NULL) {
				memcpy(myString, *myData + strlen(thePrefix) + myOffset, myLength);
				myString[myLength] = '\0';
			}
		}
	}

bail:
	if (myData != NULL)
		DisposeHandle(myData);

	return(myString);
}


//////////
//
// QTFrame_FindExternalMovieTarget
// Find the external movie targeted by theEMRecPtr.
//
//////////

void QTFrame_FindExternalMovieTarget (MovieController theMC, QTGetExternalMoviePtr theEMRecPtr)
{
#if ALLOW_SELF_TARGETING
#pragma unused(theMC)
#endif
	WindowReference			myWindow = NULL;
	Movie					myTargetMovie = NULL;
	MovieController			myTargetMC = NULL;
	Boolean					myFoundIt = false;
	
	if (theEMRecPtr == NULL)
		return;
	
	// loop through all open movies until we find the one requested
	myWindow = QTFrame_GetFrontMovieWindow();
	while (myWindow != NULL) {
		Movie				myMovie = NULL;
		MovieController		myMC = NULL;
	
		myMC = QTFrame_GetMCFromWindow(myWindow);
		
#if ALLOW_SELF_TARGETING
		if (myMC != NULL) {
#else
		if ((myMC != NULL) && (myMC != theMC)) {
#endif		
			myMovie = MCGetMovie(myMC);
			
			if (theEMRecPtr->targetType == kTargetMovieName) {
				char 		*myStr = NULL;
				
				myStr = QTUtils_GetMovieTargetName(myMovie);
				if (myStr != NULL) {
					if (IdenticalText(&theEMRecPtr->movieName[1], myStr, theEMRecPtr->movieName[0], strlen(myStr), NULL) == 0)
						myFoundIt = true;
					free(myStr);
				}
			}
			
			if (theEMRecPtr->targetType == kTargetMovieID) {
				long		myID = 0;
				Boolean		myMovieHasID = false;
				
				myID = QTUtils_GetMovieTargetID(myMovie, &myMovieHasID);
				if ((theEMRecPtr->movieID == myID) && myMovieHasID)
					myFoundIt = true;
			}
			
			if (myFoundIt) {
				myTargetMovie = myMovie;
				myTargetMC = myMC;
				break;		// break out of while loop
			}
		}
		
		myWindow = QTFrame_GetNextMovieWindow(myWindow);
	}	// while
	
	*theEMRecPtr->theMovie = myTargetMovie;
	*theEMRecPtr->theController = myTargetMC;
}
