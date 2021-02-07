#ifndef LANG_HPP
#define LANG_HPP

enum
{
  MOk,
  MCancel,

  MOverwrite,
  MWrite,
  MAll,
  MSkip,
  MSkipAll,

  MConfig,
  MDetectFormats,
  MShowExt,
  MAutoMove,
  MUseDS,
  MHoBeta,
  MSCL,
  MAutoSelect,
  MEnterExec,
  MCtrlEnterExec,
  MTypes,
  MCmdLine,
  MFullScr,
  MIniPath,
  MSave,

  MName,
  MSize,
  MStart,
  MSectorSize,
  MTrack,
  MSector,
  MFormat,
  MComment,

  MDiskInfo,
  MImageType,
  MDiskTitle,
  MDiskType,
  M2SDD,
  M2SSD,
  M1SDD,
  M1SSD,
  MDiskUnknown,
  MWriteProtection,
  MOn,
  MOff,
  MFilesInfo,
  MFiles,
  MFilesDel,
  MDirInfo,
  MDirSys,
  MDisabled,
  MAbsent,
  MDS100,
  MFreeAreaInfo,
  M1Trk,
  M1Sec,
  MFreeSecs,

  MError,
  MWarning,
  MWriteProtected,
  MAlreadyExists,
  MTooManyFiles,
  MTooManyFolders,
  MDiskFull,
  MCanNotEdit,
  MCanNotProtect,
  MCanNotReadSec,
  MCanNotWriteSec,
  MCanNotDel,
  MCanNotMakeDir,
  MCanNotMove,
  MCanNotCopy,
  MCanNotCreate,
  MCanNotOpen,
  MCanNotExec,
  MTrkSec,

  MRenameFolder,
  MFolderName,
  MEditFileInfo,
  MFileName,
  MFileType,
  MStartAddress,

  MDiskMenu,
  MCheckDisk,
  MRemoveProtection,
  MProtectDisk,
  MEditDiskTitle,
  MMoveDisk,
  MCleanFreeSpace,
  MCheckDisk2,
  MTestFiles,
  MTestDS,
  MNoErrors,
  MErrors,
  MEditTitle,
  MChangeTitle,

  MDSNotInstalled,
  MInstallDS,
  MMakeFolder,
  MCreateFolder,

  MLine,//         "                            Size SSz",
  MSource,//       "Source                     %5d %3d"
  MDestination,//  "Destination                %5d %3d"
  MDestination1,// "Destination        %d"
  MDestination2,// "Destination        %d %02d.%02d.%04d %02d:%02d:%02d"

  MCopy,
  MMove,
  MCopying,
  MMoving,
  MCopyButton,
  MMoveButton,
  MFileTo	,
  MFilesTo,
  MSkipHeaders,
  MCopyWithoutPathnames,

  MWaiting,
  MWritingSCL,
  MCopyingFile,// "Copy file"
  MCopyTheFile,// "         Copy the file        "

  MFileSize,//"File size         %5d sec(s)"
  MFree,//    "Free              %5d sec(s)"

  MDelete,
  MDeleteFolder,
  MDeleteFiles,
  MDeleteAsk,
  MDelFiles,// "%s files"
  MDelFolder,//" The following folder will be deleted: "
  MDeleteButton,

  MxTRDWarning,
  MINIError,
  MCanNotOpenINI
};

#endif
