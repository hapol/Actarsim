// - AUTHOR: Pablo Cabanelas 12/2016
/******************************************************************
 * Copyright (C) 2005-2016, Hector Alvarez-Pol                     *
 * All rights reserved.                                            *
 *                                                                 *
 * License according to GNU LESSER GPL (see lgpl-3.0.txt).         *
 * For the list of contributors see CREDITS.                       *
 ******************************************************************/
//////////////////////////////////////////////////////////////////
/// \class ActarSimROOTAnalExogam
/// The Exogam Germanium array detector part of the ROOT Analysis
/////////////////////////////////////////////////////////////////

#include "ActarSimROOTAnalExogam.hh"
#include "ActarSimExogamHit.hh"
#include "ActarSimExogamGeantHit.hh"

#include "G4ios.hh"
#include "G4RunManager.hh"
#include "G4SDManager.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include "G4Track.hh"
#include "G4ClassificationOfNewTrack.hh"
#include "G4TrackStatus.hh"
#include "G4Step.hh"
#include "G4Types.hh"

#include "TROOT.h"
#include "TApplication.h"
#include "TSystem.h"
#include "TH1.h"
#include "TH2.h"
#include "TPad.h"
#include "TCanvas.h"
#include "TTree.h"
#include "TFile.h"
#include "TClonesArray.h"

#include "CLHEP/Units/PhysicalConstants.h"
#include "CLHEP/Units/SystemOfUnits.h"

//////////////////////////////////////////////////////////////////
/// Constructor
ActarSimROOTAnalExogam::ActarSimROOTAnalExogam() {
  //The simulation file
  simFile = ((ActarSimROOTAnalysis*)gActarSimROOTAnalysis)->GetSimFile();
  simFile->cd();

  //The tree
  eventTree = ((ActarSimROOTAnalysis*)gActarSimROOTAnalysis)->GetEventTree();

  //TODO->Implement SimHits/Hits duality as R3B...

  //The clones array of Exogam Hits
  exogamHitCA = new TClonesArray("ActarSimExogamHit",2);

  exogamHitsBranch = eventTree->Branch("exogamHits",&exogamHitCA);
  exogamHitsBranch->SetAutoDelete(kTRUE);
}

//////////////////////////////////////////////////////////////////
/// Destructor. Makes nothing
ActarSimROOTAnalExogam::~ActarSimROOTAnalExogam() {
}

//////////////////////////////////////////////////////////////////
/// Actions to perform in the detector anal when generating the primaries
void ActarSimROOTAnalExogam::GeneratePrimaries(const G4Event *anEvent){
  if(anEvent){;} // to quiet the compiler
}

//////////////////////////////////////////////////////////////////
/// Actions to perform in the detector anal at the begining of the run
void ActarSimROOTAnalExogam::BeginOfRunAction(const G4Run *aRun) {

  if (aRun){;} /* keep the compiler "quiet" */

  //Storing the runID
  SetTheRunID(aRun->GetRunID());

  char newDirName[255];
  sprintf(newDirName,"%s%i","Histos",aRun->GetRunID());
  simFile->cd(newDirName);

  dirName = new char[255];

  sprintf(dirName,"%s","exogam");
  gDirectory->mkdir(dirName,dirName);
  gDirectory->cd(dirName);

  simFile->cd();
}

//////////////////////////////////////////////////////////////////
/// Actions to perform in the detector anal at the begining of the event
void ActarSimROOTAnalExogam::BeginOfEventAction(const G4Event *anEvent) {
  SetTheEventID(anEvent->GetEventID());
}

//////////////////////////////////////////////////////////////////
/// Actions to perform in the detector anal at the beginning of the run
void ActarSimROOTAnalExogam::EndOfEventAction(const G4Event *anEvent) {
  FillingHits(anEvent);
}

//////////////////////////////////////////////////////////////////
/// Actions to perform in the detector detector analysis after each step
void ActarSimROOTAnalExogam::UserSteppingAction(const G4Step *aStep){
  if(aStep){;} // to quiet the compiler
}

//////////////////////////////////////////////////////////////////
/// Defining the ActarSimExogamHits from the ActarSimExogamGeantHits
void ActarSimROOTAnalExogam::FillingHits(const G4Event *anEvent) {
  //Hit Container ID for ActarSimExogamGeantHit
  G4int hitsCollectionID =
    G4SDManager::GetSDMpointer()->GetCollectionID("ExogamCollection");

  G4HCofThisEvent* HCofEvent = anEvent->GetHCofThisEvent();

  ActarSimExogamGeantHitsCollection* hitsCollection =
    (ActarSimExogamGeantHitsCollection*) HCofEvent->GetHC(hitsCollectionID);

  //Number of Hits (or steps) in the hitsCollection
  G4int NbHits = hitsCollection->entries();
  //G4int NbHitsWithSomeEnergy = NbHits;
  //G4cout << " NbHits: " << NbHits << G4endl;

  G4int indepHits = 0; //number of Hits
  G4int* trackIDtable; //stores the trackID of primary particles for each (valid) GeantHit
  trackIDtable = new G4int[NbHits];
  G4int* detIDtable;   //stores the detIDs for each (valid) GeantHit
  detIDtable = new G4int[NbHits];
  G4int* IDtable;         //stores the order in previous array for each (valid) GeantHit
  IDtable = new G4int[NbHits];


  for (G4int i=0;i<NbHits;i++) {
    if((*hitsCollection)[i]->GetParentID()==0) { //step from primary
      if(indepHits==0) { //only for the first Hit
	trackIDtable[indepHits] = (*hitsCollection)[i]->GetTrackID();
	detIDtable[indepHits] = (*hitsCollection)[i]->GetDetID();
	IDtable[i] = indepHits;
	indepHits++;
      }
      else { // this part is never reached. Maybe because there is always only one indepHits that has parentID equals to 0.
	for(G4int j=0; j<indepHits;j++) {
	  if( (*hitsCollection)[i]->GetTrackID() == trackIDtable[j] &&
	      (*hitsCollection)[i]->GetDetID() == detIDtable[j]) { //checking trackID and detID
	    IDtable[i] = j;
	    break; //not a new Hit
	  }
	  if(j==indepHits-1){ //we got the last hit and there was no match!
	    trackIDtable[indepHits] = (*hitsCollection)[i]->GetTrackID();
	    detIDtable[indepHits] = (*hitsCollection)[i]->GetDetID();
	    IDtable[i] = indepHits;
	    indepHits++;
	  }
	}
      }
    }
  }

  //Let us create as many ActarSimExogamHit as independent primary particles
  theExogamHit = new ActarSimExogamHit*[indepHits];
  for (G4int i=0;i<indepHits;i++)
    theExogamHit[i] = new ActarSimExogamHit();

  //Clear the ClonesArray before filling it
  exogamHitCA->Clear();

  //a variable to check if the Hit was already created
  G4int* existing;
  existing = new G4int[indepHits];
  for(G4int i=0;i<indepHits;i++) existing[i] = 0;

  for(G4int i=0;i<NbHits;i++) {
    if( (*hitsCollection)[i]->GetParentID()==0 ) { //step from primary
      //the IDtable[i] contains the order in the indepHits list
      if( existing[IDtable[i]]==0) { //if the indepHits does not exist
	AddCalExogamHit(theExogamHit[IDtable[i]],(*hitsCollection)[i],0);
	existing[IDtable[i]] = 1;
      }
      else
	AddCalExogamHit(theExogamHit[IDtable[i]],(*hitsCollection)[i],1);
    }
  }

  //at the end, fill the ClonesArray
  for (G4int i=0;i<indepHits;i++)
    new((*exogamHitCA)[i])ActarSimExogamHit(*theExogamHit[i]);

  delete [] trackIDtable;
  delete [] IDtable;
  delete [] existing;
  delete [] detIDtable;
  for (G4int i=0;i<indepHits;i++) delete theExogamHit[i];
  delete [] theExogamHit;
}


//////////////////////////////////////////////////////////////////////////////////////
///  Function to move the information from the ActarSimExogamGeantHit
/// (a step hit) to ActarSimExogamHit (an event hit)
/// Two modes are possible:
/// - mode == 0 : creation; the ActarSimExogamHit is void and is
///             filled by the data from the ActarSimExogamGeantHit
/// - mode == 1 : addition; the ActarSimExogamHit was already created
///             by other ActarSimExogamGeantHit and some data members are updated
//////////////////////////////////////////////////////////////////////////////////////
void ActarSimROOTAnalExogam::AddCalExogamHit(ActarSimExogamHit* cHit,
				       ActarSimExogamGeantHit* gHit,
				       G4int mode) {

  if(mode == 0) { //creation

    cHit->SetDetectorID(gHit->GetDetID());

    cHit->SetXPos(gHit->GetLocalPrePos().x()/CLHEP::mm);
    cHit->SetYPos(gHit->GetLocalPrePos().y()/CLHEP::mm);
    cHit->SetZPos(gHit->GetLocalPrePos().z()/CLHEP::mm);

    cHit->SetTime(gHit->GetToF()/CLHEP::ns);
    cHit->SetEnergy(gHit->GetEdep()/CLHEP::MeV);
    cHit->SetEBeforeExogam(gHit->GetEBeforeExogam()/CLHEP::MeV);
    cHit->SetEAfterExogam(gHit->GetEAfterExogam()/CLHEP::MeV);

    cHit->SetTrackID(gHit->GetTrackID());
    cHit->SetEventID(GetTheEventID());
    cHit->SetRunID(GetTheRunID());

    cHit->SetParticleID(gHit->GetParticleID());
    cHit->SetParticleCharge(gHit->GetParticleCharge());
    cHit->SetParticleMass(gHit->GetParticleMass());

    cHit->SetStepsContributing(1);

  } else if(mode==1) { //addition

    cHit->SetEnergy(cHit->GetEnergy() + gHit->GetEdep()/ CLHEP::MeV);
    //taking the smaller outgoing energy of the geantHits
    if(cHit->GetEAfterExogam()>gHit->GetEAfterExogam()) cHit->SetEAfterExogam(gHit->GetEAfterExogam()/CLHEP::MeV);
    //taking the larger incoming energy of the geantHits
    if(cHit->GetEBeforeExogam()<gHit->GetEBeforeExogam()) cHit->SetEBeforeExogam(gHit->GetEBeforeExogam()/CLHEP::MeV);

    cHit->SetStepsContributing(cHit->GetStepsContributing()+1);
    // The mean value of a distribution {x_i} can also be computed iteratively
    // if the values x_i are drawn one-by-one. After a new value x, the new mean is:
    // mean(t) = mean(t-1) + (1/t)(x-mean(t-1))
    cHit->SetXPos(cHit->GetXPos() +
		  (gHit->GetLocalPrePos().x()-cHit->GetXPos())/((G4double)cHit->GetStepsContributing()));
    cHit->SetYPos(cHit->GetYPos() +
		  (gHit->GetLocalPrePos().y()-cHit->GetYPos())/((G4double)cHit->GetStepsContributing()));
    cHit->SetZPos(cHit->GetZPos() +
		  (gHit->GetLocalPrePos().z()-cHit->GetZPos())/((G4double)cHit->GetStepsContributing()));

    //taking the shorter time of the geantHits
    if(gHit->GetToF()<cHit->GetTime()) cHit->SetTime(gHit->GetToF()/CLHEP::ns);

  }
}
