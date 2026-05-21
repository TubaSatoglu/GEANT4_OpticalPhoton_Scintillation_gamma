#ifndef PMDETECTORCONSTRUCTION_HH
#define PMDETECTORCONSTRUCTION_HH 1

#include "G4VUserDetectorConstruction.hh"
#include "PMSensitiveDetector.hh"
#include "G4SDManager.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4SubtractionSolid.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "G4VisAttributes.hh"
#include "G4Color.hh"
#include "G4OpticalSurface.hh"
#include "G4LogicalBorderSurface.hh"
#include "G4LogicalSkinSurface.hh"

class G4LogicalVolume;
class G4VPhysicalVolume;
class G4Material;
class G4OpticalSurface;

class PMDetectorConstruction : public G4VUserDetectorConstruction {
public:
    PMDetectorConstruction();
    virtual ~PMDetectorConstruction();

    virtual G4VPhysicalVolume* Construct();
    void ConstructSDandField();

private:
    G4LogicalVolume* logicDetector = nullptr;
    G4LogicalVolume* logicAlDetector = nullptr;
    G4LogicalVolume* scintillatorLogical = nullptr;

    // Materyal oluşturucu fonksiyonlar
    G4Material* CreateAirMaterial();
    G4Material* CreateScintillatorMaterial();
    G4Material* CreateTeflonMaterial();
    G4OpticalSurface* CreateReflectiveSurface(); 

    // Optik yüzey tanımı
    void DefineOpticalSurfaces(
        G4VPhysicalVolume* scintillatorPhys,
        G4VPhysicalVolume* physTeflonXplus,
        G4VPhysicalVolume* physTeflonXminus,
        G4VPhysicalVolume* physTeflonYplus,
        G4VPhysicalVolume* physTeflonYminus,
        G4VPhysicalVolume* physTeflonZplus,
        G4VPhysicalVolume* physTeflonZminus
    );
};

#endif // PMDETECTORCONSTRUCTION_HH

