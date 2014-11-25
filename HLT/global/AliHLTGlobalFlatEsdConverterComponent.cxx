// $Id$

//**************************************************************************
//* This file is property of and copyright by the ALICE HLT Project        * 
//* ALICE Experiment at CERN, All rights reserved.                         *
//*                                                                        *
//* Primary Authors: Matthias Richter <Matthias.Richter@ift.uib.no>        *
//*                  for The ALICE HLT Project.                            *
//*                                                                        *
//* Permission to use, copy, modify and distribute this software and its   *
//* documentation strictly for non-commercial purposes is hereby granted   *
//* without fee, provided that the above copyright notice appears in all   *
//* copies and that both the copyright notice and this permission notice   *
//* appear in the supporting documentation. The authors make no claims     *
//* about the suitability of this software for any purpose. It is          *
//* provided "as is" without express or implied warranty.                  *
//**************************************************************************

//  @file   AliHLTGlobalFlatEsdConverterComponent.cxx
//  @author Matthias Richter
//  @date   
//  @brief  Global ESD converter component.
// 

// see header file for class documentation
// or
// refer to README to build package
// or
// visit http://web.ift.uib.no/~kjeks/doc/alice-hlt

#include <cassert>
#include "AliHLTGlobalFlatEsdConverterComponent.h"
#include "AliFlatESDEvent.h"
#include "AliFlatESDTrigger.h"
#include "AliFlatESDVertex.h"
#include "AliFlatESDV0.h"
#include "AliFlatESDTrack.h"
#include "AliFlatExternalTrackParam.h"
#include "AliExternalTrackParam.h"

#include "AliHLTGlobalBarrelTrack.h"
#include "AliHLTExternalTrackParam.h"
#include "AliHLTTrackMCLabel.h"
#include "AliHLTCTPData.h"
#include "AliHLTErrorGuard.h"
#include "AliESDEvent.h"
#include "AliESDtrack.h"
#include "AliESDMuonTrack.h"
#include "AliESDMuonCluster.h"
#include "AliCDBEntry.h"
#include "AliCDBManager.h"
#include "AliPID.h"
#include "TTree.h"
#include "TList.h"
#include "TClonesArray.h"
//#include "AliHLTESDCaloClusterMaker.h"
//#include "AliHLTCaloClusterDataStruct.h"
//#include "AliHLTCaloClusterReader.h"
//#include "AliESDCaloCluster.h"
//#include "AliESDVZERO.h"
#include "AliHLTGlobalVertexerComponent.h"
#include "AliHLTVertexFinderBase.h"
#include "AliHLTTPCSpacePointData.h"
#include "AliHLTTPCClusterDataFormat.h"
#include "AliHLTTPCDefinitions.h"
#include "AliHLTTPCClusterMCData.h"
#include "AliHLTTPCTransform.h"
#include "AliSysInfo.h"

/** ROOT macro for the implementation of ROOT specific class methods */
ClassImp(AliHLTGlobalFlatEsdConverterComponent)

AliHLTGlobalFlatEsdConverterComponent::AliHLTGlobalFlatEsdConverterComponent()
  : AliHLTProcessor()
  , fVerbosity(0)    
  , fBenchmark("FlatEsdConverter")
  , fProduceFriend(1)
{
  // see header file for class documentation
  // or
  // refer to README to build package
  // or
  // visit http://web.ift.uib.no/~kjeks/doc/alice-hlt
}

AliHLTGlobalFlatEsdConverterComponent::~AliHLTGlobalFlatEsdConverterComponent()
{
  // see header file for class documentation
}

int AliHLTGlobalFlatEsdConverterComponent::Configure(const char* arguments)
{
  // see header file for class documentation
  int iResult=0;
  if (!arguments) return iResult;

  TString allArgs=arguments;
  TString argument;
  int bMissingParam=0;

  TObjArray* pTokens=allArgs.Tokenize(" ");
  if (pTokens) {
    for (int i=0; i<pTokens->GetEntries() && iResult>=0; i++) {
      argument=((TObjString*)pTokens->At(i))->String();	
      if (argument.IsNull()) continue;      
      HLTError("unknown argument %s", argument.Data());
      iResult=-EINVAL;
      break;
    }  
    delete pTokens;
  }
  if (bMissingParam) {
    HLTError("missing parameter for argument %s", argument.Data());
    iResult=-EINVAL;
  }

  return iResult;
}

int AliHLTGlobalFlatEsdConverterComponent::Reconfigure(const char* cdbEntry, const char* chainId)
{
  // see header file for class documentation
  int iResult=0;
  const char* path=NULL;
  const char* defaultNotify="";
  if (cdbEntry) {
    path=cdbEntry;
    defaultNotify=" (default)";
  }
  if (path) {
    HLTInfo("reconfigure from entry %s%s, chain id %s", path, defaultNotify,(chainId!=NULL && chainId[0]!=0)?chainId:"<none>");
    AliCDBEntry *pEntry = AliCDBManager::Instance()->Get(path/*,GetRunNo()*/);
    if (pEntry) {
      TObjString* pString=dynamic_cast<TObjString*>(pEntry->GetObject());
      if (pString) {
	HLTInfo("received configuration object string: \'%s\'", pString->String().Data());
	iResult=Configure(pString->String().Data());
      } else {
	HLTError("configuration object \"%s\" has wrong type, required TObjString", path);
      }
    } else {
      HLTError("can not fetch object \"%s\" from CDB", path);
    }
  }
  
  return iResult;
}

void AliHLTGlobalFlatEsdConverterComponent::GetInputDataTypes(AliHLTComponentDataTypeList& list)
{
  // see header file for class documentation
  list.push_back(kAliHLTDataTypeTrack);
  list.push_back(kAliHLTDataTypeTrackMC);
  list.push_back(kAliHLTDataTypeCaloCluster);
  list.push_back(kAliHLTDataTypedEdx );
  list.push_back(kAliHLTDataTypeESDVertex );
  list.push_back(kAliHLTDataTypeESDObject);
  list.push_back(kAliHLTDataTypeTObject);
  list.push_back(kAliHLTDataTypeGlobalVertexer);
  list.push_back(kAliHLTDataTypeV0Finder); // array of track ids for V0s
  list.push_back(kAliHLTDataTypeKFVertex); // KFVertex object from vertexer
  list.push_back(kAliHLTDataTypePrimaryFinder); // array of track ids for prim vertex
  list.push_back(kAliHLTDataTypeESDContent);
  list.push_back(AliHLTTPCDefinitions::fgkClustersDataType| kAliHLTDataOriginTPC);
  list.push_back(AliHLTTPCDefinitions::fgkAliHLTDataTypeClusterMCInfo| kAliHLTDataOriginTPC);
}

AliHLTComponentDataType AliHLTGlobalFlatEsdConverterComponent::GetOutputDataType()
{
  // see header file for class documentation
  return kAliHLTDataTypeFlatESD|kAliHLTDataOriginOut;
}

void AliHLTGlobalFlatEsdConverterComponent::GetOutputDataSize(unsigned long& constBase, double& inputMultiplier)
{
  // see header file for class documentation
  constBase=2000000;
  inputMultiplier=10.0;
}

int AliHLTGlobalFlatEsdConverterComponent::DoInit(int argc, const char** argv)
{
  // see header file for class documentation
  int iResult=0;
  TString argument="";
  int bMissingParam=0;

  iResult=Reconfigure(NULL, NULL);
  TString allArgs = "";
  for ( int i = 0; i < argc; i++ ) {
    if ( !allArgs.IsNull() ) allArgs += " ";
    allArgs += argv[i];
  }

  TObjArray* pTokens=allArgs.Tokenize(" ");
  if (pTokens) {
    for (int i=0; i<pTokens->GetEntries() && iResult>=0; i++) {
      argument=((TObjString*)pTokens->At(i))->String();	
      if (argument.IsNull()) continue;

      // -noclusters
      if (argument.CompareTo("-nofriend")==0) {
	fProduceFriend=0;	
	// -clusters
      } else if (argument.CompareTo("-friend")==0) {
	fProduceFriend=1;
      } else {
	HLTError("unknown argument %s", argument.Data());
	iResult=-EINVAL;
	break;
      }
    }
  }
  if (bMissingParam) {
    HLTError("missing parameter for argument %s", argument.Data());
    iResult=-EINVAL;
  }

  if (iResult>=0) {
    SetupCTPData();
  }

  fBenchmark.SetTimer(0,"total");

  return iResult;
}

int AliHLTGlobalFlatEsdConverterComponent::DoDeinit()
{
  // see header file for class documentation

  return 0;
}

int AliHLTGlobalFlatEsdConverterComponent::DoEvent( const AliHLTComponentEventData& /*evtData*/,
						    const AliHLTComponentBlockData* /*blocks*/, 
						    AliHLTComponentTriggerData& trigData,
						    AliHLTUInt8_t* outputPtr, 
						    AliHLTUInt32_t& size,
						    AliHLTComponentBlockDataList& outputBlocks )
{
  // see header file for class documentation

  AliSysInfo::AddStamp("AliHLTGlobalFlatEsdConverterComponent::DoEvent.Start");
	
	
  int iResult=0;
	bool benchmark = true;

  if (!IsDataEvent()) return iResult;

  fBenchmark.StartNewEvent();
  fBenchmark.Start(0);

  size_t maxOutputSize = size;
  size = 0;

  // Read part of the input  data to local arrays
  
  // 1) first, read MC information if present

  std::map<int,int> mcLabelsTPC;

  for (const AliHLTComponentBlockData* pBlock=GetFirstInputBlock(kAliHLTDataTypeTrackMC|kAliHLTDataOriginTPC);
       pBlock!=NULL; pBlock=GetNextInputBlock()) {
    fBenchmark.AddInput(pBlock->fSize);
    AliHLTTrackMCData* dataPtr = reinterpret_cast<AliHLTTrackMCData*>( pBlock->fPtr );
    if (sizeof(AliHLTTrackMCData)+dataPtr->fCount*sizeof(AliHLTTrackMCLabel)==pBlock->fSize) {
      for( unsigned int il=0; il<dataPtr->fCount; il++ ){
	AliHLTTrackMCLabel &lab = dataPtr->fLabels[il];
	mcLabelsTPC[lab.fTrackID] = lab.fMCLabel;
      }
    } else {
      HLTWarning("data mismatch in block %s (0x%08x): count %d, size %d -> ignoring track MC information", 
		 DataType2Text(pBlock->fDataType).c_str(), pBlock->fSpecification, 
		 dataPtr->fCount, pBlock->fSize);
    }
  }
 
  // 2) read dEdx information (if present)

  AliHLTFloat32_t *dEdxTPC = 0; 
  Int_t ndEdxTPC = 0;
  for (const AliHLTComponentBlockData* pBlock=GetFirstInputBlock(kAliHLTDataTypedEdx|kAliHLTDataOriginTPC);
       pBlock!=NULL; pBlock=NULL/*GetNextInputBlock() there is only one block*/) {
    fBenchmark.AddInput(pBlock->fSize);
    dEdxTPC = reinterpret_cast<AliHLTFloat32_t*>( pBlock->fPtr );
    ndEdxTPC = pBlock->fSize / (3*sizeof(AliHLTFloat32_t));
  }

  // 3) read  TPC tracks, ITS refitted tracks, ITS OUT tracks

  vector<AliHLTGlobalBarrelTrack> tracksTPC;
  vector<AliHLTGlobalBarrelTrack> tracksTPCOut;
  vector<AliHLTGlobalBarrelTrack> tracksITS;
  vector<AliHLTGlobalBarrelTrack> tracksITSOut;

  if( iResult>=0 ){
    const AliHLTComponentBlockData* pBlock=GetFirstInputBlock(kAliHLTDataTypeTrack|kAliHLTDataOriginTPC);
    if( pBlock ){
      fBenchmark.AddInput(pBlock->fSize);
      iResult = AliHLTGlobalBarrelTrack::ConvertTrackDataArray(reinterpret_cast<const AliHLTTracksData*>(pBlock->fPtr), pBlock->fSize, tracksTPC);
    }  
  
    if( iResult>=0 ) { 
      pBlock=GetFirstInputBlock(kAliHLTDataTypeTrack|kAliHLTDataOriginITS);
      if( pBlock ){
	fBenchmark.AddInput(pBlock->fSize);
	iResult=AliHLTGlobalBarrelTrack::ConvertTrackDataArray(reinterpret_cast<const AliHLTTracksData*>(pBlock->fPtr), pBlock->fSize, tracksITS);
      }
    }
    if( iResult>=0 ) { 
      pBlock=GetFirstInputBlock(kAliHLTDataTypeTrack|kAliHLTDataOriginITSOut);
      if( pBlock ){
	fBenchmark.AddInput(pBlock->fSize);
	iResult=AliHLTGlobalBarrelTrack::ConvertTrackDataArray(reinterpret_cast<const AliHLTTracksData*>(pBlock->fPtr), pBlock->fSize, tracksITSOut);
      } 
    }   
    if( iResult<0 ){
      HLTError("can not extract tracks from data block of type %s (specification %08x) of size %d: error %d", 
	       DataType2Text(pBlock->fDataType).c_str(), pBlock->fSpecification, pBlock->fSize, iResult);     
    }
  }

  HLTWarning("converted %d TPC %d ITS %d ITSout track(s) to GlobalBarrelTrack", tracksTPC.size(), tracksITS.size(), tracksITSOut.size() );
  
  // Set TPC MC labels to tracks
  for( UInt_t itr=0; itr < tracksTPC.size(); itr++) {
    AliHLTGlobalBarrelTrack &track = tracksTPC[itr];
    std::map<int,int>::const_iterator lab = mcLabelsTPC.find( track.TrackID() );
    if( lab!=mcLabelsTPC.end() ) track.SetLabel( lab->second );
    else track.SetLabel( -1 );
  }

  // Create TPC Out tracks - just propagate to the outermost TPC cluster
  for( UInt_t itr=0; itr < tracksTPC.size(); itr++) {
    tracksTPCOut.push_back( tracksTPC[itr] );
    AliHLTGlobalBarrelTrack &track = tracksTPCOut.back();
    const Int_t N=10; // number of steps.
    const Float_t xRange = track.GetLastPointX() - track.GetX();
    const Float_t xStep = xRange / N ;
    for(int i = 1; i <= N; ++i) {
      if(!track.AliExternalTrackParam::PropagateTo(track.GetX() + xStep, GetBz() )) break;
    }
  }


  // ---------------------------------------------
  //
  // Start to fill the flat ESD structure
  //

  Int_t err = 0;
  AliFlatESDEvent *flatEsd = reinterpret_cast<AliFlatESDEvent*>(outputPtr); 

  do{ // single loop for easy break in case of output buffer overflow

    size_t freeSpace = maxOutputSize;

    err = ( freeSpace < sizeof( AliFlatESDEvent ) );    
    if( err ) break;

    new (flatEsd) AliFlatESDEvent;    
 
    freeSpace -= flatEsd->GetSize();
  
    // fill run info
    {
      flatEsd->SetMagneticField( GetBz() );
      flatEsd->SetPeriodNumber( GetPeriodNumber() );
      flatEsd->SetRunNumber( GetRunNo() );
      flatEsd->SetOrbitNumber( GetOrbitNumber() );
      flatEsd->SetBunchCrossNumber( GetBunchCrossNumber() );
      flatEsd->SetTimeStamp( GetTimeStamp() );
      //flatEsd->SetEventSpecie( GetEventSpecie() ); !!SG!! to do
    }

    // Fill trigger information  
    {
      const AliHLTCTPData* pCTPData=CTPData();
      if (pCTPData) {
	size_t triggerSize = 0;
	int nTriggers = 0;
	AliFlatESDTrigger *trigger = flatEsd->SetTriggersStart();
	AliHLTTriggerMask_t mask = pCTPData->ActiveTriggers(trigData);
	for (int index=0; index<gkNCTPTriggerClasses; index++) {
	  if ((mask&(AliHLTTriggerMask_t(0x1)<<index)) == 0) continue;
	  const char* name = pCTPData->Name(index);
	  if( name && name[0]!='\0' ){
	    err = trigger->SetTriggerClass( name, index, freeSpace );
	    if( err != 0 ) break;
	    nTriggers++;
	    freeSpace -= trigger->GetSize();
	    triggerSize += trigger->GetSize();
	    trigger = trigger->GetNextTriggerNonConst();
	  }
	}    
	flatEsd->SetTriggersEnd( nTriggers, triggerSize );
	//first 50 triggers
	AliHLTTriggerMask_t mask50;
	mask50.set(); // set all bits
	mask50 >>= 50; // shift 50 right
	flatEsd->SetTriggerMask((mask&mask50).to_ulong());
	//next 50
	flatEsd->SetTriggerMaskNext50((mask>>50).to_ulong());
      }
    }
 
    if( err ) break;
    
    const AliESDVertex *primaryVertex = 0;

    { // fill primary vertex Tracks

      const AliESDVertex *primaryVertexTracks = dynamic_cast<const AliESDVertex*>( GetFirstInputObject( kAliHLTDataTypeESDVertex|kAliHLTDataOriginOut ) );               
      primaryVertex = primaryVertexTracks;
      err = flatEsd->SetPrimaryVertexTracks( primaryVertexTracks, freeSpace );
      freeSpace = maxOutputSize - flatEsd->GetSize();
    }

    if( err ) break;

    { // fill primary vertex SPD
    
      const AliESDVertex *primaryVertexSPD = dynamic_cast<const AliESDVertex*>( GetFirstInputObject( kAliHLTDataTypeESDVertex|kAliHLTDataOriginITS ) );
      if( !primaryVertex ) primaryVertex = primaryVertexSPD;  
      err = flatEsd->SetPrimaryVertexSPD( primaryVertexSPD, freeSpace );
      freeSpace = maxOutputSize - flatEsd->GetSize();     
    }  
    
    if( err ) break;

    { // Fill track information to the flat ESD structure
     
      size_t trackSize = 0;
      int nTracks = 0;
      Long64_t *table = NULL;
      AliFlatESDTrack *flatTrack = NULL;
      err = flatEsd->SetTracksStart( flatTrack, table, tracksTPC.size(), freeSpace );
      freeSpace = maxOutputSize - flatEsd->GetSize();
    
      if( err ) break;

      for( UInt_t tpcIter=0, itsIter = 0, itsOutIter = 0; tpcIter < tracksTPC.size(); tpcIter++) {

	// ----------------------------
	// -- read track information

	// TPC track parameters
	
	AliHLTGlobalBarrelTrack *tpcTrack = &(tracksTPC[tpcIter]);
	AliHLTGlobalBarrelTrack *tpcOutTrack = &(tracksTPCOut[tpcIter]);

	// ITS track parameters
	
	AliHLTGlobalBarrelTrack *itsRefit=0;
	AliHLTGlobalBarrelTrack *itsOut=0;
	
	// ITS Refit track
	  
	for(; itsIter< tracksITS.size() && tracksITS[itsIter].TrackID()<(int) tpcIter; itsIter++ );
	
	if( itsIter< tracksITS.size() && tracksITS[itsIter].TrackID() == (int) tpcIter ){
	  itsRefit = &(tracksITS[itsIter]);
	  itsIter++;
	}
	
	// ITS Out track
	  
	for(; itsOutIter< tracksITSOut.size() && tracksITSOut[itsOutIter].TrackID()<(int) tpcIter; itsOutIter++ );
	
	if( itsOutIter< tracksITSOut.size() && tracksITSOut[itsOutIter].TrackID() == (int) tpcIter ){
	  itsOut = &(tracksITSOut[itsOutIter]);
	  itsOutIter++;
	}	
	
	// 

	if (fVerbosity>0) tpcTrack->Print();  

	Float_t tpcDeDx[3]={0,0,0};
	
	if( ndEdxTPC>0 ){ 	
	  if( tpcTrack->TrackID() < ndEdxTPC ){
	    AliHLTFloat32_t *val = &(dEdxTPC[3*tpcTrack->TrackID()]);
	    tpcDeDx[0] = val[0];
	    tpcDeDx[1] = val[1];
	    tpcDeDx[2] = val[2];
	  } else {
	    HLTWarning("Wrong number of dEdx TPC labels");
	  }
	}
    
	// vertex-constrained parameters for TPC tracks	

	const AliExternalTrackParam *tpcConstrained=0;       
	AliESDtrack esdTrack;
	if( primaryVertex ){
	  esdTrack.UpdateTrackParams(&(*tpcTrack),AliESDtrack::kTPCrefit);	  
	  esdTrack.RelateToVertex( primaryVertex, GetBz(), 1000 );	
	  tpcConstrained = esdTrack.GetConstrainedParam();	
	}

	UInt_t nClustersTPC = tpcTrack->GetNumberOfPoints();
	UInt_t nClustersITS = itsRefit ?itsRefit->GetNumberOfPoints() :0;

	
	// ----------------------------
	// -- fill flat track structure

	table[tpcIter] = trackSize;
	err = ( freeSpace < flatTrack->EstimateSize() );
	if( err ) break;
	
	new (flatTrack) AliFlatESDTrack;       
	
	flatTrack->SetExternalTrackParam( itsRefit, itsRefit, tpcTrack, tpcOutTrack, tpcConstrained, itsOut );
	flatTrack->SetNumberOfTPCClusters( nClustersTPC );
	flatTrack->SetNumberOfITSClusters( nClustersITS );
	trackSize += flatTrack->GetSize();
	freeSpace -= flatTrack->GetSize();
	nTracks++;
	flatTrack = flatTrack->GetNextTrackNonConst();    
      }
      flatEsd->SetTracksEnd( nTracks, trackSize );
    }       

    if( err ) break;

    // Fill v0's
  
    {    
      size_t v0size = 0;
      int nV0s = 0; 
      AliFlatESDV0 *flatV0 = flatEsd->SetV0sStart();

      const AliHLTComponentBlockData* pBlock = GetFirstInputBlock(kAliHLTDataTypeGlobalVertexer|kAliHLTDataOriginOut);
      if ( pBlock && pBlock->fSize && pBlock->fPtr) {
	fBenchmark.AddInput(pBlock->fSize);
 	const AliHLTGlobalVertexerComponent::AliHLTGlobalVertexerData *data = reinterpret_cast<AliHLTGlobalVertexerComponent::AliHLTGlobalVertexerData*>(pBlock->fPtr);
	const int* v0s = data->fTrackIndices + data->fNPrimTracks;
	for (int i = 0; i < data->fNV0s; ++i) {
	  if(  freeSpace < flatV0->GetSize() ) { err = -1; break; }
	  new (flatV0) AliFlatESDV0;
	  flatV0->SetNegTrackID( v0s[2 * i] );
	  flatV0->SetPosTrackID( v0s[2 * i + 1] );
	  nV0s++;
	  v0size += flatV0->GetSize();
	  freeSpace -= flatV0->GetSize(); 
	  flatV0 = flatV0->GetNextV0NonConst();	  
	}
      } else {
	HLTWarning("xxx No V0 data block");
      }

      flatEsd->SetV0sEnd( nV0s, v0size );
      cout<<"\nxxxx Found "<<nV0s<<" V0's\n"<<endl;
    }
    
    if( err ) break;

  }while(0);


  if( err ){
    HLTWarning( "Output buffer size %d exceeded, flat ESD event is not stored", maxOutputSize );
  } else {

    // set up the output block description
    
    AliHLTComponentBlockData outBlock;
    FillBlockData( outBlock );
    outBlock.fOffset = size;
    outBlock.fSize = flatEsd->GetSize();
    outBlock.fDataType = kAliHLTDataTypeFlatESD|kAliHLTDataOriginOut;
    outputBlocks.push_back( outBlock );
    fBenchmark.AddOutput(outBlock.fSize);
    size += outBlock.fSize;
  }
  
  // ---------------------------------------------
  //
  // Fill the flat ESD friend structure
  //
  
  while( !err && fProduceFriend ){ // single loop for easy break in case of output buffer overflow

    // ---------- Access to clusters --------------------

    const AliHLTTPCClusterData  *partitionClusters[fkNPartition];  //! arrays of cluster data for each TPC partition
    Int_t                        partitionNClusters[fkNPartition]; //! number of clusters for each TPC partition

    {
      for(Int_t i=0; i<fkNPartition; i++){
	partitionClusters[i]  = 0;
	partitionNClusters[i] = 0;    
      }
      for(const AliHLTComponentBlockData *iter = GetFirstInputBlock(AliHLTTPCDefinitions::fgkClustersDataType); iter != NULL; iter = GetNextInputBlock()){      
	if(iter->fDataType != AliHLTTPCDefinitions::fgkClustersDataType) continue;
	Int_t slice     = AliHLTTPCDefinitions::GetMinSliceNr(iter->fSpecification);
	Int_t partition = AliHLTTPCDefinitions::GetMinPatchNr(iter->fSpecification);    
	Int_t slicepartition = slice*6+partition;      
	if(slicepartition<0 || slicepartition > fkNPartition){
	  HLTWarning("Wrong header of TPC cluster data, slice %d, partition %d", slice, partition );
	  continue;
	}
	const AliHLTTPCClusterData * clusterData = ( AliHLTTPCClusterData* )( iter->fPtr );
	if( clusterData ) {
	  partitionClusters[slicepartition] = clusterData;
	  partitionNClusters[slicepartition] = clusterData->fSpacePointCnt;
	}
      } // end of loop over blocks of clusters    
    }


    AliFlatESDFriend *flatFriend = reinterpret_cast<AliFlatESDFriend*>(outputPtr + size);     

    size_t freeSpaceTotal = maxOutputSize - size;
    size_t freeSpace = freeSpaceTotal;

    err = ( freeSpace < sizeof( AliFlatESDEvent ) );    
    if( err ) break;

    new (flatFriend) AliFlatESDFriend;
      
    freeSpace = freeSpaceTotal - flatFriend->GetSize();
  
    // fill event info
    {
      //flatFriend->SetSkipBit( 0 ); // SG!!
      for( Int_t iSlice=0; iSlice<36; iSlice++ ){
	int iSector = iSlice;
	int nclu = 0;
	for( Int_t iPartition=0; iPartition<3; iPartition++){	    
	  int slicepartition = iSlice*6+iPartition;
	  nclu+= partitionNClusters[slicepartition];
	}
	flatFriend->SetNclustersTPC( iSector, nclu );
	iSector = 36+iSlice;
	nclu = 0;
	for( Int_t iPartition=3; iPartition<6; iPartition++){	    
	  int slicepartition = iSlice*6+iPartition;
	  nclu+= partitionNClusters[slicepartition];
	}
	flatFriend->SetNclustersTPC( iSector, nclu );
	//SG!!!flatFriend->SetNclustersTPCused( iSector, esdFriend->GetNclustersTPCused(iSector) );
      }
    }
      
    { // Fill track friends information to the flat ESD friend structure
     
      size_t trackSize = 0;
      int nTracks = 0;
      int nTrackEntries = 0;
      Long64_t *table = NULL;
      AliFlatESDFriendTrack *flatTrack = NULL;
      err = flatFriend->SetTracksStart( flatTrack, table, tracksTPC.size(), freeSpace );
      if( err ) break;
      freeSpace = freeSpaceTotal - flatFriend->GetSize();

      for( UInt_t tpcIter=0, itsIter = 0, itsOutIter = 0; tpcIter < tracksTPC.size(); tpcIter++) {       
	
	// TPC track parameters
	
	AliHLTGlobalBarrelTrack *tpcTrack = &(tracksTPC[tpcIter]);
	AliHLTGlobalBarrelTrack *tpcOutTrack = &(tracksTPCOut[tpcIter]);

	// ITS track parameters
	
	AliHLTGlobalBarrelTrack *itsRefit=0;
	AliHLTGlobalBarrelTrack *itsOut=0;
	
	// ITS Refit track
	  
	for(; itsIter< tracksITS.size() && tracksITS[itsIter].TrackID()<(int) tpcIter; itsIter++ );
	
	if( itsIter< tracksITS.size() && tracksITS[itsIter].TrackID() == (int) tpcIter ){
	  itsRefit = &(tracksITS[itsIter]);
	  itsIter++;
	}
	
	// ITS Out track
	  
	for(; itsOutIter< tracksITSOut.size() && tracksITSOut[itsOutIter].TrackID()<(int) tpcIter; itsOutIter++ );
	  
	if( itsOutIter< tracksITSOut.size() && tracksITSOut[itsOutIter].TrackID() == (int) tpcIter ){
	  itsOut = &(tracksITSOut[itsOutIter]);
	  itsOutIter++;
	}
	
	// fill track parameters
		  
	table[tpcIter] = trackSize;
	err = ( freeSpace < flatTrack->EstimateSize() );	  
	if( err ) break;
	new (flatTrack) AliFlatESDFriendTrack;
	  	  
	freeSpace = freeSpaceTotal - flatFriend->GetSize();

	flatTrack->SetSkipBit( 0 );
	flatTrack->SetTrackParamTPCOut( tpcOutTrack );
	flatTrack->SetTrackParamITSOut( itsOut );
	// flatTrack->SetTrackParamTRDIn( track->GetTRDIn() );

	// fill TPC seed

	AliFlatTPCseed* seed = flatTrack->SetTPCseedStart();
	new( seed ) AliFlatTPCseed;

	seed->SetLabel( tpcTrack->GetLabel() );
	seed->SetExternalTrackParam( tpcTrack );
	
	// clusters 

	UInt_t nClusters = tpcTrack->GetNumberOfPoints();	
	const UInt_t*clusterIDs = tpcTrack->GetPoints();
	for(UInt_t ic=0; ic<nClusters; ic++){	 
	  UInt_t id      = clusterIDs[ic];	     
	  int iSlice = AliHLTTPCSpacePointData::GetSlice(id);
	  int iPartition = AliHLTTPCSpacePointData::GetPatch(id);
	  int iCluster = AliHLTTPCSpacePointData::GetNumber(id);
	  
	  if(iSlice<0 || iSlice>36 || iPartition<0 || iPartition>5){
	    HLTError("Corrupted TPC cluster Id: slice %d, partition %d, cluster %d", iSlice, iPartition, iCluster);
	    continue;
	  }
	  
	  const AliHLTTPCClusterData * clusterData = partitionClusters[iSlice*6 + iPartition];
	  if(!clusterData ){
	    HLTError("Clusters are missed for slice %d, partition %d", iSlice, iPartition );
	    continue;
	  }
	    
	  if(iCluster >= partitionNClusters[iSlice*6 + iPartition]){
	    HLTError("TPC slice %d, partition %d: ClusterID==%d >= N Cluaters==%d ", iSlice, iPartition,iCluster, partitionNClusters[iSlice*6 + iPartition] );
	    continue;
	  }

	    
	  const AliHLTTPCSpacePointData *chlt = &( clusterData->fSpacePoints[iCluster] );
	  AliTPCclusterMI cl;
	  cl.SetX(chlt->fX);
	  cl.SetY(chlt->fY);
	  cl.SetZ(chlt->fZ);
	  cl.SetSigmaY2(chlt->fSigmaY2);
	  cl.SetSigmaYZ( 0 );
	  cl.SetSigmaZ2(chlt->fSigmaZ2);
	  cl.SetQ( chlt->fCharge );
	  cl.SetMax( chlt->fQMax );
	  Int_t sector, row;
	  AliHLTTPCTransform::Slice2Sector(iSlice,chlt->fPadRow, sector, row);
	  cl.SetDetector( sector );
	  cl.SetRow( row );
	  //Float_t padtime[3] = {0,chlt->fY,chlt->fZ};
	  //AliHLTTPCTransform::Local2Raw( padtime, sector, row);
	  //cl.SetPad( (Int_t) padtime[1] );
	  //cl.SetTimeBin( (Int_t) padtime[2] );
	  
	  	  
	  tpcTrack->Propagate( TMath::DegToRad()*(sector%18*20.+10.), cl.GetX(), GetBz() );
	  Double_t angle2 = tpcTrack->GetSnp()*tpcTrack->GetSnp();
	  angle2 = (angle2<1) ?TMath::Sqrt(angle2/(1-angle2)) :10.; 
	  AliTPCTrackerPoint point;
	  point.SetAngleY( angle2 );
	  point.SetAngleZ( tpcTrack->GetTgl() );

	  seed->AddCluster(&cl, &point ); 
	} // end of associated cluster loop
	        
	
	flatTrack->SetTPCseedEnd( seed->GetSize() );	
	  
	trackSize += flatTrack->GetSize();
	freeSpace -= flatTrack->GetSize();
	nTrackEntries++;
	nTracks++;
 	flatTrack = flatTrack->GetNextTrackNonConst();	
	
      } // fill tracks
      

      flatFriend->SetTracksEnd( nTracks, nTrackEntries, trackSize );
      
    }      

    if( err ) break;

    { // set up the output block description
    
      AliHLTComponentBlockData outBlock;
      FillBlockData( outBlock );
      outBlock.fOffset = size;
      outBlock.fSize = flatFriend->GetSize();
      outBlock.fDataType = kAliHLTDataTypeFlatESDFriend|kAliHLTDataOriginOut;
      outputBlocks.push_back( outBlock );
      fBenchmark.AddOutput(outBlock.fSize);      
      size += outBlock.fSize;
    }
    
    break;
  }

  fBenchmark.Stop(0);  
  //HLTWarning( fBenchmark.GetStatistics() );
  
  if( err ){
    HLTWarning( "Output buffer size %d exceeded, flat ESD friend event is not stored", maxOutputSize );
    return -ENOSPC;
  } 
  

  if(benchmark){		
    Double_t statistics[10]; 
    TString names[10];
    fBenchmark.GetStatisticsData(statistics, names);
    fBenchmark.Reset();
    AliSysInfo::AddStamp("AliHLTGlobalFlatEsdConverterComponent::DoEvent.Stop", (int)(statistics[1]), (int)(statistics[2]),flatEsd->GetNumberOfTracks(),flatEsd->GetNumberOfV0s() );
  }

  return 0;
}
