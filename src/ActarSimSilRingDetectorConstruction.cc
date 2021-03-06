// - AUTHOR: Hector Alvarez-Pol 04/2008
/******************************************************************
 * Copyright (C) 2005-2016, Hector Alvarez-Pol                     *
 * All rights reserved.                                            *
 *                                                                 *
 * License according to GNU LESSER GPL (see lgpl-3.0.txt).         *
 * For the list of contributors see CREDITS.                       *
 ******************************************************************/
//////////////////////////////////////////////////////////////////
/// \class ActarSimSilRingDetectorConstruction
/// Silicon ring detector description
/////////////////////////////////////////////////////////////////

#include "ActarSimSilRingDetectorConstruction.hh"
#include "ActarSimDetectorConstruction.hh"
#include "ActarSimGasDetectorConstruction.hh"
//#include "ActarSimSilDetectorMessenger.hh"
#include "ActarSimROOTAnalysis.hh"
#include "ActarSimSilRingSD.hh"

#include "G4Material.hh"
#include "G4Box.hh"
#include "G4Cons.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4PVReplica.hh"
#include "G4RotationMatrix.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4RunManager.hh"
#include "G4Transform3D.hh"

#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"

#include "globals.hh"

//////////////////////////////////////////////////////////////////
/// Constructor. Sets the material and the pointer to the Messenger
ActarSimSilRingDetectorConstruction::
ActarSimSilRingDetectorConstruction(ActarSimDetectorConstruction* det)
  :	silBulkMaterial(0),detConstruction(det) {
  SetSilBulkMaterial("Silicon");

  //Options for Silicon and scintillator coverage:
  // 6 bits to indicate which sci wall is present (1) or absent (0)
  // order is:
  // bit1 (lsb) beam output wall
  // bit2 lower (gravity based) wall
  // bit3 upper (gravity based) wall
  // bit4 left (from beam point of view) wall
  // bit5 right (from beam point of view) wall
  // bit6 (msb) beam entrance wall
  SetSideCoverage(1);

  SetXBoxSilHalfLength(100*cm);
  SetYBoxSilHalfLength(100*cm);
  SetZBoxSilHalfLength(100*cm);

  // create commands for interactive definition of the calorimeter
  //silMessenger = new ActarSimSilRingDetectorMessenger(this);
}

//////////////////////////////////////////////////////////////////
/// Destructor
ActarSimSilRingDetectorConstruction::~ActarSimSilRingDetectorConstruction(){
  //delete silMessenger;
}


//////////////////////////////////////////////////////////////////
/// Wrap for the construction functions
G4VPhysicalVolume* ActarSimSilRingDetectorConstruction::Construct(G4LogicalVolume* worldLog) {
  //Introduce here other constructors for materials around the TOF (windows, frames...)
  //which can be controlled by the calMessenger
  //ConstructTOFWorld(worldLog);
  return ConstructSil(worldLog);
}

//////////////////////////////////////////////////////////////////
/// Real construction work is performed here.
G4VPhysicalVolume* ActarSimSilRingDetectorConstruction::ConstructSil(G4LogicalVolume* worldLog) {
  //Chamber Y,Z length
  //G4double chamberSizeY=detConstruction->GetChamberYLength();
  G4double chamberSizeZ=detConstruction->GetChamberSizeZ();

  //Gas chamber position inside the chamber
  ActarSimGasDetectorConstruction* gasDet = detConstruction->GetGasDetector();
  G4double zGasBoxPosition=gasDet->GetGasBoxCenterZ();

  //----------------------------- the Silicon and CsI disks
  G4double Rmax=48*mm;
  G4double Rmin=24*mm;
  G4double Phi_0=0*deg;
  G4double Phi_f=360*deg;
  G4double Zlength=0.25*mm; //Half length 500 um

  G4Tubs *Silicon=new G4Tubs("Silicon",Rmin,Rmax,Zlength,Phi_0,Phi_f);

  G4VisAttributes* SilVisAtt = new G4VisAttributes(G4Colour(1.0,1.0,0.0));
  SilVisAtt->SetVisibility(false);

  G4LogicalVolume* SiliconDisk_log= new G4LogicalVolume(Silicon,silBulkMaterial,"SiliconDisk_log",0,0,0);

  SiliconDisk_log->SetVisAttributes(SilVisAtt);

  G4double sectorPhi=Phi_f/16.;

  G4Tubs *Sector=new G4Tubs("Sector",Rmin,Rmax,Zlength,0.,sectorPhi);

  G4LogicalVolume *Sector_log=new G4LogicalVolume(Sector,silBulkMaterial,"Sector_log",0,0,0);
  G4VisAttributes* SectorVisAtt = new G4VisAttributes(G4Colour(1.0,1.0,0.0));
  SectorVisAtt->SetVisibility(true);

  G4double silPos_x=0;
  //G4double silPos_y=chamberSizeY/2-10*mm;
  G4double silPos_y=0;
  G4double silPos_z=0*mm;

  G4double distance[3]={300*mm,530*mm,1070*mm}; //For 10Li experiment

  //G4double distance[3]={550*mm,0*mm,0*mm}; //For 16C experiment Only one ring detector

  for(G4int k=0;k<3;k++){
    silPos_z=distance[k]+chamberSizeZ-zGasBoxPosition;

    G4VPhysicalVolume *SiliconDisk_phys=new G4PVPlacement(0,
							  G4ThreeVector(silPos_x,silPos_y,silPos_z),
							  SiliconDisk_log,"SiliconDisk",worldLog,false,k);

    if(SiliconDisk_phys){;}
  }
  Sector_log->SetVisAttributes(SectorVisAtt);

  G4VPhysicalVolume *Sector_phys=new G4PVReplica("Sectors",Sector_log,SiliconDisk_log,kPhi,16,sectorPhi);

  //------------------------------------------------
  // Sensitive detectors
  //------------------------------------------------
  Sector_log->SetSensitiveDetector( detConstruction->GetSilRingSD() );

  //------------------------------------------------------------------
  // Visualization attributes
  //------------------------------------------------------------------
  G4VisAttributes* silVisAtt1 = new G4VisAttributes(G4Colour(0,1,0));
  silVisAtt1->SetVisibility(true);
  Sector_log->SetVisAttributes(silVisAtt1);

  return Sector_phys;
}

//////////////////////////////////////////////////////////////////
/// Set the material the scintillator bulk is made of
void ActarSimSilRingDetectorConstruction::SetSilBulkMaterial (G4String mat) {
  G4Material* pttoMaterial = G4Material::GetMaterial(mat);
  if (pttoMaterial) silBulkMaterial = pttoMaterial;
}

//////////////////////////////////////////////////////////////////
/// Updates Scintillator detector
void ActarSimSilRingDetectorConstruction::UpdateGeometry() {
  Construct(detConstruction->GetWorldLogicalVolume());
  G4RunManager::GetRunManager()->
    DefineWorldVolume(detConstruction->GetWorldPhysicalVolume());
}

//////////////////////////////////////////////////////////////////
/// Prints Scintillator detector parameters. To be filled
void ActarSimSilRingDetectorConstruction::PrintDetectorParameters() {
  G4cout << "##################################################################"
	 << G4endl
	 << "####  ActarSimSilRingDetectorConstruction::PrintDetectorParameters() ####"
	 << G4endl;
  G4cout << "##################################################################"
	 << G4endl;
}
