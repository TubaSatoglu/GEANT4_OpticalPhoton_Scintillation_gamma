#include "PMSensitiveDetector.hh"
#include "G4AnalysisManager.hh"
#include "G4Event.hh"
#include "G4EventManager.hh"
#include "G4OpticalPhoton.hh"
#include "G4ParticleDefinition.hh"
#include "G4SDManager.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4SystemOfUnits.hh"
#include "G4VPhysicalVolume.hh"

// 0: SD disabled (only SteppingAction counts)
// 1: SD in debug mode (additional counts and short summary)
#ifndef PM_USE_SD
#define PM_USE_SD 0
#endif

PMSensitiveDetector::PMSensitiveDetector(const G4String& name)
: G4VSensitiveDetector(name),
  fTotalOpticalPhotons(0),
  fPhotonsAtScintillator(0),
  fPhotonsAtAluminum(0),
  fPrimaryOpticalPhotons(0),
  fTotalEnergyDeposited(0.0)
{}

PMSensitiveDetector::~PMSensitiveDetector() = default;

void PMSensitiveDetector::Initialize(G4HCofThisEvent*)
{
  fTotalOpticalPhotons   = 0;
  fPhotonsAtScintillator = 0;
  fPrimaryOpticalPhotons = 0;
  fPhotonsAtAluminum     = 0;
  fTotalEnergyDeposited  = 0.0;
}

G4bool PMSensitiveDetector::ProcessHits(G4Step* step, G4TouchableHistory*)
{

  #if PM_USE_SD
  G4Track* track = step->GetTrack();
  if (!track || track->GetDefinition() != G4OpticalPhoton::OpticalPhotonDefinition())
    return false;

  // (1) Unique counting at the moment of production
  if (track->GetCurrentStepNumber() == 1) {
    ++fTotalOpticalPhotons;

    const G4StepPoint* pre = step->GetPreStepPoint();
    const G4VPhysicalVolume* preVol = pre ? pre->GetPhysicalVolume() : nullptr;
    if (preVol && preVol->GetName() == "physDetector") {
      ++fPhotonsAtScintillator;
    }

    const G4VProcess* cr = track->GetCreatorProcess();
    if (cr && cr->GetProcessName() == "Scintillation") {
      ++fPrimaryOpticalPhotons;
    }
  }

  // (2) Entering the Al volume (boundary) — counting for debug purposes only
  const G4StepPoint* post = step->GetPostStepPoint();
  const G4VPhysicalVolume* postVol = post ? post->GetPhysicalVolume() : nullptr;
  if (post && post->GetStepStatus() == fGeomBoundary && postVol) {
    if (postVol->GetName() == "physAlDetector") {
      ++fPhotonsAtAluminum;
    }
  }
  return true;
#else
  (void)step;
  return false;
#endif
}

void PMSensitiveDetector::EndOfEvent(G4HCofThisEvent*)
{
#if PM_USE_SD
  const G4Event* evt = G4EventManager::GetEventManager()->GetConstCurrentEvent();
  const G4int eventID = evt ? evt->GetEventID() : -1;

  auto* analysisManager = G4AnalysisManager::Instance();
  if (analysisManager) {
    analysisManager->FillH1(0, fTotalOpticalPhotons);
    analysisManager->FillH1(1, fPrimaryOpticalPhotons);
    analysisManager->FillH1(2, fPhotonsAtScintillator);
  }

  (void)eventID;
#endif
}
