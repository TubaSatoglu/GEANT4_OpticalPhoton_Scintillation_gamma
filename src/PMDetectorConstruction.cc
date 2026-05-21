#include "PMDetectorConstruction.hh"
#include "G4SDManager.hh"
#include "G4LogicalVolume.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4SubtractionSolid.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4OpticalSurface.hh"
#include "G4LogicalBorderSurface.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4LogicalSkinSurface.hh"


PMDetectorConstruction::PMDetectorConstruction()
{}

PMDetectorConstruction::~PMDetectorConstruction()
{}

// ============================
// Optical Surface Definition
// ============================
G4Material* PMDetectorConstruction::CreateAirMaterial() {
    G4NistManager* nist = G4NistManager::Instance();
    G4Material* air = nist->FindOrBuildMaterial("G4_AIR");

    G4MaterialPropertiesTable* airMPT = new G4MaterialPropertiesTable();
    G4double airPhotonEnergy[] = {2.0 * eV, 3.5 * eV};
    G4double airRindex[] = {1.0, 1.0};
    airMPT->AddProperty("RINDEX", airPhotonEnergy, airRindex, 2);

    air->SetMaterialPropertiesTable(airMPT);
    return air;
}

G4Material* PMDetectorConstruction::CreateScintillatorMaterial() {
    auto* plasticScint =
        G4NistManager::Instance()->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE");

    // Optical range
    const G4int nOpticalPoints = 2;
    G4double photonEnergy[nOpticalPoints]     = { 2.0*eV, 3.5*eV };
    G4double rindex[nOpticalPoints]           = { 1.58, 1.58 };
    G4double absorptionLength[nOpticalPoints] = { 2.0*m, 2.0*m };
    G4double emission[nOpticalPoints]         = { 1.0, 1.0 };

    // EJ-276 decay constants (Holroyd 2025)
    const G4double tau1 = 13.0*ns;    // fast
    const G4double tau2 = 35.0*ns;    // medium
    const G4double tau3 = 270.0*ns;   // slow

    // Relative weights for gamma
    const G4double w1_gamma = 0.76;
    const G4double w2_gamma = 0.18;
    const G4double w3_gamma = 0.06;

    auto* MPT = new G4MaterialPropertiesTable();

    // Basic optical properties
    MPT->AddProperty("RINDEX",    photonEnergy, rindex, nOpticalPoints);
    MPT->AddProperty("ABSLENGTH", photonEnergy, absorptionLength, nOpticalPoints);

    // 3-component PSD scintillation model
    MPT->AddProperty("SCINTILLATIONCOMPONENT1", photonEnergy, emission, nOpticalPoints);
    MPT->AddProperty("SCINTILLATIONCOMPONENT2", photonEnergy, emission, nOpticalPoints);
    MPT->AddProperty("SCINTILLATIONCOMPONENT3", photonEnergy, emission, nOpticalPoints);

    MPT->AddConstProperty("SCINTILLATIONTIMECONSTANT1", tau1);
    MPT->AddConstProperty("SCINTILLATIONTIMECONSTANT2", tau2);
    MPT->AddConstProperty("SCINTILLATIONTIMECONSTANT3", tau3);

    MPT->AddConstProperty("SCINTILLATIONYIELD1", w1_gamma);
    MPT->AddConstProperty("SCINTILLATIONYIELD2", w2_gamma);
    MPT->AddConstProperty("SCINTILLATIONYIELD3", w3_gamma);

    // Global scintillation params
    MPT->AddConstProperty("SCINTILLATIONYIELD", 8600./MeV);
    MPT->AddConstProperty("RESOLUTIONSCALE", 1.0);
    MPT->AddConstProperty("SCINTILLATIONRISETIME1", 0.5 * ns);

    // ⚠ No FASTCOMPONENT, No SLOWCOMPONENT
    // ⚠ No YIELDRATIO – must be removed in 3-component mode

    // Birks constant
    plasticScint->GetIonisation()->SetBirksConstant(0.0125 * cm/MeV);

    plasticScint->SetMaterialPropertiesTable(MPT);
    return plasticScint;
}


G4Material* PMDetectorConstruction::CreateTeflonMaterial() {
    G4NistManager* nist = G4NistManager::Instance();
    G4Material* teflon = nist->FindOrBuildMaterial("G4_TEFLON");

    const G4int nEntries = 2;
    G4double photonEnergy[nEntries] = {2.0*eV, 3.5*eV};
    G4double rindex[nEntries] = {1.35, 1.35}; 

    G4MaterialPropertiesTable* teflonMPT = new G4MaterialPropertiesTable();
    teflonMPT->AddProperty("RINDEX", photonEnergy, rindex, nEntries);

    teflon->SetMaterialPropertiesTable(teflonMPT);
    return teflon;
}


G4VPhysicalVolume *PMDetectorConstruction::Construct() {
    G4bool checkOverlaps = true;

    G4Material* airMat = CreateAirMaterial();
    G4Material* plasticScint = CreateScintillatorMaterial();
    G4Material* teflonMat = CreateTeflonMaterial();
    
    // Create reflective optical surface
    G4OpticalSurface* reflectiveSurface = new G4OpticalSurface("ReflectiveSurface");
    reflectiveSurface->SetType(dielectric_metal);
    reflectiveSurface->SetFinish(polished);
    reflectiveSurface->SetModel(unified);

    const G4int nEntries = 2;
    G4double photonEnergy[nEntries] = {2.0*eV, 3.5*eV};
    G4double reflectivity[nEntries] = {0.99, 0.99}; // 99% reflectivity
    G4double efficiency[nEntries] = {0.0, 0.0};   // No absorption

    G4MaterialPropertiesTable* surfaceMPT = new G4MaterialPropertiesTable();
    surfaceMPT->AddProperty("REFLECTIVITY", photonEnergy, reflectivity, nEntries);
    surfaceMPT->AddProperty("EFFICIENCY", photonEnergy, efficiency, nEntries);

    reflectiveSurface->SetMaterialPropertiesTable(surfaceMPT);
    
    G4double xWorld = 50.0 * cm;
    G4double yWorld = 50.0 * cm;
    G4double zWorld = 50.0 * cm;
    
    G4Box* solidWorld = new G4Box("solidWorld", 0.5*xWorld, 0.5*yWorld, 0.5*zWorld);
    G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, airMat, "logicWorld");
    G4VPhysicalVolume *physWorld = new G4PVPlacement(0, G4ThreeVector(0., 0., 0.), logicWorld, "physWorld", 0, false, 0, checkOverlaps);

    // ================================
    // Scintillator
    // ================================
    G4double detectorSize = 10.0 * cm;
    G4Box* solidDetector = new G4Box("solidDetector", 0.5 * detectorSize, 0.5 * detectorSize, 0.5 * detectorSize);
    logicDetector = new G4LogicalVolume(solidDetector, plasticScint, "logicDetector");
    G4VPhysicalVolume *physDetector = new G4PVPlacement(0, G4ThreeVector(0., 0. , 10.5 * cm), 
                                                        logicDetector, "physDetector", logicWorld, 
                                                        false, 0, checkOverlaps);

    auto detVisAtt = new G4VisAttributes(G4Color(1.0, 1.0, 0.0, 1.0));
    detVisAtt->SetForceSolid(true);
    logicDetector->SetVisAttributes(detVisAtt);

    // ================================
    // Teflon Plates (for 6 faces)
    // ================================
    G4double teflonThickness = 0.001 * cm;
    G4double offset = 0.5 * detectorSize + 0.5 * teflonThickness;
    
    G4VisAttributes *teflonVisAtt = new G4VisAttributes(G4Color(1.0, 0.0, 0.0, 0.5));
    teflonVisAtt->SetForceSolid(true);  

    // ================
    // +X Direction (No hole - fully reflective)
    // ================
    G4Box *solidTeflonX = new G4Box("solidTeflonX", 
                                0.5 * teflonThickness, 
                                0.5 * detectorSize, 
                                0.5 * detectorSize);
    G4LogicalVolume *logicTeflonX = new G4LogicalVolume(solidTeflonX, teflonMat, "logicTeflonX");
    logicTeflonX->SetVisAttributes(teflonVisAtt);
    
    auto physTeflonXplus = new G4PVPlacement(0, G4ThreeVector(+offset,0.,10.5*cm), 
                            logicTeflonX, "physTeflonXplus", 
                            logicWorld, false, 0, checkOverlaps);
    
    // Define optical surface on the X+ face (the face looking towards the scintillator)
    new G4LogicalBorderSurface("TeflonXplus_Surface", physDetector, physTeflonXplus, reflectiveSurface);


    // ================
    // X− Plate (With Hole)
    // ================
    G4Box *solidTeflonXminus = new G4Box("solidTeflonXminus", 
                                        0.5 * teflonThickness, 
                                        0.5 * detectorSize, 
                                        0.5 * detectorSize);

    // Hole: 1 cm diameter, length matching the plate thickness
    G4Tubs* holeXminus = new G4Tubs("holeXminus", 
                                        0.*cm, 
                                        1.0*cm, 
                                        0.5*teflonThickness + 0.05*cm, 
                                        0.*deg, 
                                        360.*deg);

    // Rotation: 90° around the Y axis to rotate the cylinder towards the X direction
    G4RotationMatrix* rotX = new G4RotationMatrix();
    rotX->rotateY(90.*deg);

    // Subtraction process
    G4SubtractionSolid* solidTeflonXminusHole = new G4SubtractionSolid("solidTeflonXminusHole",
                                                solidTeflonXminus,
                                                holeXminus,
                                                rotX,
                                                G4ThreeVector(0.,0.,0.001*cm)); 

    // Logical volume
    G4LogicalVolume *logicTeflonXminus = new G4LogicalVolume(solidTeflonXminusHole, teflonMat, "logicTeflonXminus");
    logicTeflonXminus->SetVisAttributes(teflonVisAtt);

    // Placement
    auto physTeflonXminus = new G4PVPlacement(0, G4ThreeVector(-offset,0.,10.5*cm), 
                                                logicTeflonXminus, 
                                                "physTeflonXminus", 
                                                logicWorld, 
                                                false, 0, 
                                                checkOverlaps);
    
    // Define optical surface on the X- face (scintillator-facing surface - non-hole part only)
    new G4LogicalBorderSurface("TeflonXminus_Surface", physDetector, physTeflonXminus, reflectiveSurface);


    // ================
    // Air Hole in (X-)
    // ================
    G4Tubs* airHoleXSolid = new G4Tubs("airHoleXSolid", 
                                        0.*cm, 
                                        3.0*cm,
                                        0.5*teflonThickness + 0.05*cm, 
                                        0.*deg, 
                                        360.*deg);

    G4ThreeVector posAirHoleX = G4ThreeVector(-offset, 0., 10.5*cm); // Same position as Teflon X-

    G4RotationMatrix* rotAirX = new G4RotationMatrix();
    rotAirX->rotateY(90.*deg);

    G4LogicalVolume* logicAirHoleX = new G4LogicalVolume(airHoleXSolid, airMat, "logicAirHoleX");

    auto physAirHoleX = new G4PVPlacement(rotAirX,
                                            posAirHoleX,
                                            logicAirHoleX,
                                            "physAirHoleX",
                                            logicWorld,
                                            false,
                                            0,
                                            checkOverlaps);


    // ================            
    // Y Direction (No hole - fully reflective)
    // ================
    G4Box *solidTeflonY = new G4Box("solidTeflonY", 
                                    0.5 * detectorSize, 
                                    0.5 * teflonThickness, 
                                    0.5 * detectorSize);
    G4LogicalVolume *logicTeflonY = new G4LogicalVolume(solidTeflonY, teflonMat, "logicTeflonY");
    logicTeflonY->SetVisAttributes(teflonVisAtt);

    auto physTeflonYplus = new G4PVPlacement(0, G4ThreeVector(0.,+offset,10.5*cm), 
                        logicTeflonY, "physTeflonYplus", logicWorld, false, 0, checkOverlaps);
    auto physTeflonYminus = new G4PVPlacement(0, G4ThreeVector(0.,-offset,10.5*cm), 
                        logicTeflonY, "physTeflonYminus", logicWorld, false, 0, checkOverlaps);
    
    // Define optical surface on Y surfaces
    new G4LogicalBorderSurface("TeflonYplus_Surface", physDetector, physTeflonYplus, reflectiveSurface);
    new G4LogicalBorderSurface("TeflonYminus_Surface", physDetector, physTeflonYminus, reflectiveSurface);


    // ================================
    // Z+ Plate (No hole - fully reflective)
    // ================================
    G4Box *solidTeflonZ = new G4Box("solidTeflonZ", 
        0.5 * detectorSize, 
        0.5 * detectorSize, 
        0.5 * teflonThickness);
    G4LogicalVolume *logicTeflonZ = new G4LogicalVolume(solidTeflonZ, teflonMat, "logicTeflonZ");
    logicTeflonZ->SetVisAttributes(teflonVisAtt);
    
    auto physTeflonZplus = new G4PVPlacement(0, G4ThreeVector(0.,0.,10.5*cm + offset), 
    logicTeflonZ, "physTeflonZplus", logicWorld, false, 0, checkOverlaps);

    // Define optical surface on Z+ surface
    new G4LogicalBorderSurface("TeflonZplus_Surface", physDetector, physTeflonZplus, reflectiveSurface);


    // ==========================
    // Z− Plate (With Hole)
    // ==========================
    G4Box *solidTeflonZminus = new G4Box("solidTeflonZminus", 
                                            0.5 * detectorSize, 
                                            0.5 * detectorSize, 
                                            0.5 * teflonThickness);

    G4Tubs* holeZminus = new G4Tubs("holeZminus", 
                                    0.*cm, 
                                    0.5*cm,
                                    0.5*teflonThickness + 0.001*cm, 
                                    0.*deg, 
                                    360.*deg);

    // Subtraction process: hole is subtracted from the plate's own local center
    G4SubtractionSolid* solidTeflonZminusHole = new G4SubtractionSolid("solidTeflonZminusHole",
                                                solidTeflonZminus,
                                                holeZminus,
                                                0,
                                                G4ThreeVector(0.,0.,0.));

    // Logical volume
    G4LogicalVolume *logicTeflonZminus = new G4LogicalVolume(solidTeflonZminusHole, teflonMat, "logicTeflonZminus");
    logicTeflonZminus->SetVisAttributes(teflonVisAtt);

    // Placement: Z− plate facing the scintillator
    auto physTeflonZminus = new G4PVPlacement(0, G4ThreeVector(0.,0.,10.5*cm - offset), 
                        logicTeflonZminus, 
                        "physTeflonZminus", 
                        logicWorld, 
                        false, 0,
                        checkOverlaps);
    
    // Define optical surface on Z- face (scintillator-facing surface - non-hole part only)
    new G4LogicalBorderSurface("TeflonZminus_Surface", physDetector, physTeflonZminus, reflectiveSurface);
    
    // ========================
    // Air Hole (Z−)
    // ========================
    G4Tubs* airHoleZSolid = new G4Tubs("airHoleZSolid", 
                                        0.*cm, 
                                        0.5*cm,
                                        0.5*teflonThickness + 0.001*cm, 
                                        0.*deg, 
                                        360.*deg);

    G4ThreeVector posAirHoleZ = G4ThreeVector(0., 0., 10.5*cm - offset);  // Same position as Z− plate

    G4LogicalVolume* logicAirHoleZ = new G4LogicalVolume(airHoleZSolid, airMat, "logicAirHoleZ");

    new G4PVPlacement(0,    // No rotation
                    posAirHoleZ, 
                    logicAirHoleZ, 
                    "physAirHoleZ", 
                    logicWorld, 
                    false, 
                    0, 
                    checkOverlaps);
    
        
    // ================================
    // Aluminum Detector (Placed in front of the -X hole)
    // ================================
    G4NistManager* nist = G4NistManager::Instance();
    G4Material* aluminumMat = nist->FindOrBuildMaterial("G4_Al");

    G4double alThickness = 0.01 * cm; // 0.1 mm
    G4double alSize = 5.0 * cm;       // 3cm per side (Note: solid box dimensions use 3.5*cm below)

    G4Box* solidAlDetector = new G4Box("solidAlDetector",
                                    0.5 * alThickness,
                                    3.5 * cm,    // Y direction: 3.5 cm
                                    3.5 * cm);   // Z direction: 3.5 cm

    // 🔹 Normal aluminum MPT (non-reflective)
    G4MaterialPropertiesTable* aluminumMPT = new G4MaterialPropertiesTable();

    G4double alPhotonEnergy[] = {2.034*eV, 2.068*eV, 2.103*eV, 2.139*eV, 2.755*eV, 4.136*eV};
    const G4int alNEntries = sizeof(alPhotonEnergy)/sizeof(G4double);

    // Refractive index
    G4double alRindex[] = {1.44, 1.44, 1.44, 1.44, 1.44, 1.44};
    // Absorption length
    G4double alAbsorption[] = {0.001*mm, 0.001*mm, 0.001*mm, 0.001*mm, 0.001*mm, 0.001*mm};

    aluminumMPT->AddProperty("RINDEX", alPhotonEnergy, alRindex, alNEntries);
    aluminumMPT->AddProperty("ABSLENGTH", alPhotonEnergy, alAbsorption, alNEntries);

    aluminumMat->SetMaterialPropertiesTable(aluminumMPT);

    // Logical volume
    logicAlDetector = new G4LogicalVolume(solidAlDetector, aluminumMat, "logicAlDetector");

    // Position formula
    G4double alPosition = -offset - 0.5*teflonThickness - 2*mm - 0.5*alThickness;

    // Placement
    auto physAlDetector = new G4PVPlacement(0,
                                            G4ThreeVector(alPosition, 0., 10.5*cm),
                                            logicAlDetector,
                                            "physAlDetector",
                                            logicWorld,
                                            false,
                                            0,
                                            checkOverlaps);

    // Visualization
    auto alVisAtt = new G4VisAttributes(G4Color(0.7, 0.7, 0.7, 0.8)); // Gray
    alVisAtt->SetForceSolid(true);
    logicAlDetector->SetVisAttributes(alVisAtt);

    // 🔹 Optical Surface: detection efficiency
    G4double alReflectivity[alNEntries];
    G4double alEfficiency[alNEntries];
    for (int i = 0; i < alNEntries; i++) {
        alReflectivity[i] = 0.0; // No reflection
        alEfficiency[i]   = 1.0; // 100% detection
    }

    G4MaterialPropertiesTable* alSurfaceMPT = new G4MaterialPropertiesTable();
    alSurfaceMPT->AddProperty("REFLECTIVITY", alPhotonEnergy, alReflectivity, alNEntries);
    alSurfaceMPT->AddProperty("EFFICIENCY",   alPhotonEnergy, alEfficiency,   alNEntries);

    auto alSurface = new G4OpticalSurface("AluminumDetectorSurface");
    alSurface->SetType(dielectric_metal);
    alSurface->SetFinish(polished);
    alSurface->SetModel(unified);
    alSurface->SetMaterialPropertiesTable(alSurfaceMPT);

    // Apply to World → Aluminum boundary
    new G4LogicalBorderSurface("AlSurface",
                                physAirHoleX,  // The air volume where the hole is located
                                physAlDetector,
                                alSurface);

    G4cout << "🔧 Aluminum detector (NON-REFLECTIVE, DETECTS OPTICAL PHOTONS) created at X position: "
        << alPosition/mm << " mm" << G4endl;

    return physWorld;
}
        

void PMDetectorConstruction::ConstructSDandField()
{
    if (logicDetector) {
        auto* scintSD = new PMSensitiveDetector("ScintillatorSD");
        logicDetector->SetSensitiveDetector(scintSD);
        G4SDManager::GetSDMpointer()->AddNewDetector(scintSD);
    }

    if (logicAlDetector) {
        auto* alSD = new PMSensitiveDetector("AluminumSD");
        logicAlDetector->SetSensitiveDetector(alSD);
        G4SDManager::GetSDMpointer()->AddNewDetector(alSD);
    }
}
